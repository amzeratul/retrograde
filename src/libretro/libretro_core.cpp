#include "libretro_core.h"

#define RETRO_IMPORT_SYMBOLS
#include <cstdarg>

#include "libretro.h"
#include "libretro_environment.h"
#include "libretro_vfs.h"
#include "src/util/cpu_update_texture.h"
#include "src/zip/zip_file.h"

thread_local ILibretroCoreCallbacks* ILibretroCoreCallbacks::curInstance;

namespace {
	bool RETRO_CALLCONV retroEnvironmentCallback(uint32_t cmd, void *data)
	{
		return ILibretroCoreCallbacks::curInstance->onEnvironment(cmd, data);
	}
	
	void RETRO_CALLCONV retroVideoRefreshCallback(const void *data, uint32_t width, uint32_t height, size_t pitch)
	{
		ILibretroCoreCallbacks::curInstance->onVideoRefresh(data, width, height, pitch);
	}
	
	void RETRO_CALLCONV retroAudioSampleCallback(int16_t left, int16_t right)
	{
		ILibretroCoreCallbacks::curInstance->onAudioSample(left, right);
	}
	
	size_t RETRO_CALLCONV retroAudioSampleBatchCallback(const int16_t *data, size_t frames)
	{
		return ILibretroCoreCallbacks::curInstance->onAudioSampleBatch(data, frames);
	}
	
	void RETRO_CALLCONV retroInputPollCallback()
	{
		return ILibretroCoreCallbacks::curInstance->onInputPoll();
	}
	
	int16_t RETRO_CALLCONV retroInputStateCallback(uint32_t port, uint32_t device, uint32_t index, uint32_t id)
	{
		return ILibretroCoreCallbacks::curInstance->onInputState(port, device, index, id);
	}

	void RETRO_CALLCONV retroLogPrintf(retro_log_level level, const char *fmt, ...)
	{
		char buffer[4096];

		va_list args;
		va_start(args, fmt);
		int n = vsprintf_s(buffer, sizeof(buffer), fmt, args);
		if (n > 0) {
			ILibretroCoreCallbacks::curInstance->onLog(level, buffer);
		}
		va_end(args);
	}
}


bool LibretroCore::SystemInfo::isValidExtension(std::string_view filePath) const
{
	return isValidExtension(Path(filePath));
}

bool LibretroCore::SystemInfo::isValidExtension(const Path& filePath) const
{
	const auto ext = filePath.getExtension();
	return std_ex::contains(validExtensions, std::string_view(ext).substr(1));
}

void LibretroCore::SystemAVInfo::loadGeometry(const retro_game_geometry& geometry)
{
	baseSize = Vector2i(geometry.base_width, geometry.base_height);
	maxSize = Vector2i(geometry.max_width, geometry.max_height);
	aspectRatio = geometry.aspect_ratio;
	if (aspectRatio <= 0) {
		aspectRatio = static_cast<float>(baseSize.x) / static_cast<float>(baseSize.y);
	}
}

std::unique_ptr<LibretroCore> LibretroCore::load(std::string_view filename, LibretroEnvironment& environment)
{
	DLL dll;
	
	if (dll.load(filename)) {
		const auto retroAPIVersion = static_cast<decltype(&retro_api_version)>(dll.getFunction("retro_api_version"));
		const auto dllVersion = retroAPIVersion();
		if (dllVersion == RETRO_API_VERSION) {
			return std::unique_ptr<LibretroCore>(new LibretroCore(std::move(dll), environment));
		}
	}
	return {};
}

LibretroCore::LibretroCore(DLL dll, LibretroEnvironment& environment)
	: dll(std::move(dll))
	, environment(environment)
{
	init();
}

LibretroCore::~LibretroCore()
{
	deInit();
}

void LibretroCore::init()
{
	retro_system_info retroSystemInfo = {};
	DLL_FUNC(dll, retro_get_system_info)(&retroSystemInfo);
	Logger::logDev("Loaded core " + String(retroSystemInfo.library_name) + " " + String(retroSystemInfo.library_version));
	systemInfo.coreName = retroSystemInfo.library_name;
	systemInfo.coreVersion = retroSystemInfo.library_version;
	systemInfo.blockExtract = retroSystemInfo.block_extract;
	systemInfo.needFullpath = retroSystemInfo.need_fullpath;
	systemInfo.validExtensions = String(retroSystemInfo.valid_extensions).split('|');

	auto guard = ScopedGuard([=]() { curInstance = nullptr; });
	curInstance = this;

	DLL_FUNC(dll, retro_set_environment)(&retroEnvironmentCallback);
	DLL_FUNC(dll, retro_init)();
	DLL_FUNC(dll, retro_set_video_refresh)(&retroVideoRefreshCallback);
	DLL_FUNC(dll, retro_set_audio_sample)(&retroAudioSampleCallback);
	DLL_FUNC(dll, retro_set_audio_sample_batch)(&retroAudioSampleBatchCallback);
	DLL_FUNC(dll, retro_set_input_poll)(&retroInputPollCallback);
	DLL_FUNC(dll, retro_set_input_state)(&retroInputStateCallback);
}

void LibretroCore::initVideoOut()
{
	cpuUpdateTexture = std::make_unique<CPUUpdateTexture>(*environment.getHalleyAPI().video);
	auto material = std::make_shared<Material>(environment.getResources().get<MaterialDefinition>("Halley/SpriteOpaque"));

	videoOut
		.setMaterial(std::move(material))
		.setTexRect0(Rect4f(0, 0, 1, 1))
		.setColour(Colour4f(1, 1, 1, 1))
		.setPosition(Vector2f(0, 0));
}

void LibretroCore::initAudioOut()
{
	std::array<float, 64> buffer;
	buffer.fill(0);
	audioOut = std::make_shared<AudioClipStreaming>(2);
	audioOut->addInterleavedSamples(buffer);
}

void LibretroCore::addAudioSamples(gsl::span<const float> samples)
{
	constexpr float maxPitchShift = 0.005f;
	audioOut->addInterleavedSamplesWithResampleSync(samples, static_cast<float>(systemAVInfo.sampleRate), maxPitchShift);
}

void LibretroCore::deInit()
{
	unloadGame();

	vfs.reset();

	auto guard = ScopedGuard([=]() { curInstance = nullptr; });
	curInstance = this;

	DLL_FUNC(dll, retro_deinit)();
}

Path LibretroCore::extractIntoVFS(const Path& path)
{
	auto zip = ZipFile(path, false);
	String loadPath;
	bool foundExtension = false;

	const size_t n = zip.getNumFiles();
	for (size_t i = 0; i < n; ++i) {
		String entryPath = "/zip/" + zip.getFileName(i);
		vfs->setVirtualFile(entryPath, zip.extractFile(i));
		const bool validExtension = systemInfo.isValidExtension(Path(entryPath));
		if (loadPath.isEmpty() && (!foundExtension || validExtension)) {
			loadPath = entryPath;
			foundExtension = validExtension;
		}
	}

	return loadPath;
}

bool LibretroCore::loadGame(const Path& path)
{
	const bool canExtract = !systemInfo.blockExtract && ZipFile::isZipFile(path);

	if (systemInfo.needFullpath) {
		// Fullpath cores can still read zipped files if they support VFS
		// In those cases, we'll extract the zip into VFS and load that instead
		if (vfs) {
			if (canExtract) {
				return loadGame(extractIntoVFS(path), {}, {});
			}
		}

		// If the core won't allow extraction or doesn't support VFS, we'll just let it handle the original file
		return loadGame(path, {}, {});
	} else {
		// For cores that load from RAM, just read it and feed it to them directly
		auto bytes = canExtract ? ZipFile::readFile(path) : Path::readFile(path);
		if (bytes.empty()) {
			return false;
		}
		return loadGame(path, gsl::as_bytes(gsl::span<const Byte>(bytes.data(), bytes.size())), {});
	}
}

bool LibretroCore::loadGame(const Path& path, gsl::span<const gsl::byte> data, std::string_view meta)
{
	if (gameLoaded) {
		unloadGame();
	}

	const auto pathStr = path.getNativeString();

	retro_game_info gameInfo;
	gameInfo.path = pathStr.c_str();
	gameInfo.meta = meta.data();
	gameInfo.size = data.size();
	gameInfo.data = data.data();

	auto guard = ScopedGuard([=]() { curInstance = nullptr; });
	curInstance = this;

	gameLoaded = DLL_FUNC(dll, retro_load_game)(&gameInfo);

	if (gameLoaded) {
		retro_system_av_info retroAVInfo = {};
		DLL_FUNC(dll, retro_get_system_av_info)(&retroAVInfo);

		systemAVInfo.fps = retroAVInfo.timing.fps;
		systemAVInfo.sampleRate = retroAVInfo.timing.sample_rate;
		systemAVInfo.loadGeometry(retroAVInfo.geometry);

		gameName = Path(path).getFilename().replaceExtension("").getString();

		initVideoOut();
		initAudioOut();
		loadGameData();
	}
	
	return gameLoaded;
}

void LibretroCore::unloadGame()
{
	if (gameLoaded) {
		auto guard = ScopedGuard([=]() { curInstance = nullptr; });
		curInstance = this;

		DLL_FUNC(dll, retro_unload_game)();

		gameLoaded = false;
		lastSaveHash = 0;

		if (vfs) {
			vfs->clearVirtualFiles();
		}
	}
}

void LibretroCore::runFrame()
{
	auto guard = ScopedGuard([=]() { curInstance = nullptr; });
	curInstance = this;

	if (audioBufferStatusCallback) {
		const size_t nLeft = audioOut->getSamplesLeft();
		const size_t capacity = audioOut->getLatencyTarget() * 2;
		audioBufferStatusCallback(true, clamp(static_cast<int>(nLeft * 100 / capacity), 0, 100), nLeft < 400);
	}

	DLL_FUNC(dll, retro_run)();

	saveGameDataIfNeeded();
}

bool LibretroCore::hasGameLoaded() const
{
	return gameLoaded;
}

Bytes LibretroCore::saveState(SaveStateType type) const
{
	saveStateType = type;
	size_t size = DLL_FUNC(dll, retro_serialize_size)();
	Bytes bytes;
	bytes.resize(size);
	DLL_FUNC(dll, retro_serialize)(bytes.data(), bytes.size());
	return bytes;
}

void LibretroCore::loadState(SaveStateType type, gsl::span<const gsl::byte> bytes)
{
	saveStateType = type;
	DLL_FUNC(dll, retro_unserialize)(bytes.data(), bytes.size());
}

void LibretroCore::loadState(SaveStateType type, const Bytes& bytes)
{
	loadState(type, gsl::as_bytes(gsl::span<const Byte>(bytes)));
}

gsl::span<Byte> LibretroCore::getMemory(MemoryType type)
{
	int id = 0;
	switch (type) {
	case MemoryType::SaveRAM:
		id = RETRO_MEMORY_SAVE_RAM;
		break;
	case MemoryType::RTC:
		id = RETRO_MEMORY_RTC;
		break;
	case MemoryType::SystemRAM:
		id = RETRO_MEMORY_SYSTEM_RAM;
		break;
	case MemoryType::VideoRAM:
		id = RETRO_MEMORY_VIDEO_RAM;
		break;
	}

	auto guard = ScopedGuard([=]() { curInstance = nullptr; });
	curInstance = this;

	const auto size = DLL_FUNC(dll, retro_get_memory_size)(id);
	auto* data = DLL_FUNC(dll, retro_get_memory_data)(id);
	return gsl::span<Byte>(static_cast<Byte*>(data), size);
}

void LibretroCore::saveGameDataIfNeeded()
{
	auto sram = getMemory(MemoryType::SaveRAM);
	if (!sram.empty()) {
		Hash::Hasher hasher;
		hasher.feedBytes(gsl::as_bytes(sram));
		const auto hash = hasher.digest();
		if (hash != lastSaveHash) {
			lastSaveHash = hash;
			saveGameData(sram);
		}
	}
}

void LibretroCore::saveGameData(gsl::span<Byte> data)
{
	Path::writeFile(getSaveFileName(), gsl::as_bytes(data));
	Logger::logDev("Saved " + getSaveFileName());
}

void LibretroCore::loadGameData()
{
	lastSaveHash = 0;
	const auto bytes = Path::readFile(getSaveFileName());
	if (!bytes.empty()) {
		auto sram = getMemory(MemoryType::SaveRAM);
		if (sram.size() == bytes.size()) {
			memcpy(sram.data(), bytes.data(), sram.size());

			Hash::Hasher hasher;
			hasher.feedBytes(gsl::as_bytes(gsl::span<const Byte>(bytes)));
			lastSaveHash = hasher.digest();
		}
	}
}

String LibretroCore::getSaveFileName() const
{
	return environment.getSaveDir() + "/" + gameName + ".srm";
}

const Sprite& LibretroCore::getVideoOut() const
{
	return videoOut;
}

const std::shared_ptr<AudioClipStreaming>& LibretroCore::getAudioOut() const
{
	return audioOut;
}

const LibretroCore::SystemInfo& LibretroCore::getSystemInfo() const
{
	return systemInfo;
}

const LibretroCore::SystemAVInfo& LibretroCore::getSystemAVInfo() const
{
	Expects(gameLoaded);
	return systemAVInfo;
}

LibretroVFS& LibretroCore::getVFS()
{
	assert(!!vfs);
	return *vfs;
}

void LibretroCore::loadVFS()
{
	vfs = std::make_unique<LibretroVFS>();
}

void LibretroCore::setInputDevice(int idx, std::shared_ptr<InputVirtual> input)
{
	Expects(idx < maxInputDevices);
	inputDevices[idx] = std::move(input);
}

bool LibretroCore::onEnvironment(uint32_t cmd, void* data)
{
	switch (cmd) {
	case RETRO_ENVIRONMENT_SET_ROTATION:
		onEnvSetRotation(*static_cast<const uint32_t*>(data));
		return true;

	case RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL:
		onEnvSetPerformanceLevel(*static_cast<const uint32_t*>(data));
		return true;

	case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
		onEnvGetSystemDirectory(static_cast<const char**>(data));
		return true;

	case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
		return onEnvSetPixelFormat(*static_cast<const retro_pixel_format*>(data));

	case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
		onEnvSetInputDescriptors(static_cast<const retro_input_descriptor*>(data));
		return true;

	case RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE:
		// TODO (used by genesis_plus_gx)
		Logger::logWarning("TODO: implement env cmd " + toString(cmd));
		return false;

	case RETRO_ENVIRONMENT_GET_VARIABLE:
		onEnvGetVariable(*static_cast<retro_variable*>(data));
		return true;

	case RETRO_ENVIRONMENT_SET_VARIABLES:
		onEnvSetVariables(static_cast<const retro_variable*>(data));
		return true;

	case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
		*static_cast<bool*>(data) = onEnvGetVariableUpdate();
		return true;

	case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
		onEnvSetSupportNoGame(*static_cast<const bool*>(data));
		return true;

	case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
		static_cast<retro_log_callback*>(data)->log = &retroLogPrintf;
		return true;

	case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
		onEnvGetSaveDirectory(static_cast<const char**>(data));
		return true;
		
	case RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO:
		onEnvSetSubsystemInfo(*static_cast<const retro_subsystem_info*>(data));
		return true;

	case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
		onEnvSetControllerInfo(*static_cast<const retro_controller_info*>(data));
		return true;

	case RETRO_ENVIRONMENT_SET_MEMORY_MAPS:
		// TODO (used by genesis plus gx)
		Logger::logWarning("TODO: implement env cmd " + toString(cmd & 0xFF));
		return false;

	case RETRO_ENVIRONMENT_SET_GEOMETRY:
		onEnvSetGeometry(*static_cast<const retro_game_geometry*>(data));
		return true;

	case RETRO_ENVIRONMENT_GET_LANGUAGE:
		*static_cast<uint32_t*>(data) = onEnvGetLanguage();
		return true;

	case RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS:
		onEnvSetSupportAchievements(*static_cast<const bool*>(data));
		return true;

	case RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS:
		onEnvSetSerializationQuirks(*static_cast<uint64_t*>(data));
		return true;

	case RETRO_ENVIRONMENT_GET_VFS_INTERFACE:
		return onEnvGetVFSInterface(*static_cast<retro_vfs_interface_info*>(data));

	case RETRO_ENVIRONMENT_GET_LED_INTERFACE:
		// TODO (used by genesis plus gx)
		Logger::logWarning("TODO: implement env cmd " + toString(cmd & 0xFF));
		return false;

	case RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE:
		*static_cast<int*>(data) = onEnvGetAudioVideoEnable();
		return true;

	case RETRO_ENVIRONMENT_GET_FASTFORWARDING:
		*static_cast<bool*>(data) = false;
		return true;

	case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS:
		return true;

	case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION:
		*static_cast<uint32_t*>(data) = 2;
		return true;

	case RETRO_ENVIRONMENT_SET_CORE_OPTIONS:
		onEnvSetCoreOptions(static_cast<const retro_core_option_definition*>(data));
		return true;

	case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL:
		onEnvSetCoreOptionsIntl(*static_cast<const retro_core_options_intl*>(data));
		return true;

	case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY:
		onEnvSetCoreOptionsDisplay(*static_cast<retro_core_option_display*>(data));
		return true;

	case RETRO_ENVIRONMENT_GET_MESSAGE_INTERFACE_VERSION:
		*static_cast<uint32_t*>(data) = 1;
		return true;

	case RETRO_ENVIRONMENT_SET_MESSAGE_EXT:
		onEnvSetMessageExt(*static_cast<const retro_message_ext*>(data));
		return true;

	case RETRO_ENVIRONMENT_SET_AUDIO_BUFFER_STATUS_CALLBACK:
		onEnvSetAudioBufferStatusCallback(static_cast<const retro_audio_buffer_status_callback*>(data));
		return true;

	case RETRO_ENVIRONMENT_SET_MINIMUM_AUDIO_LATENCY:
		onEnvSetMinimumAudioLatency(*static_cast<const uint32_t*>(data));
		return true;

	case RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE:
		// TODO (used by genesis plus gx)
		Logger::logWarning("TODO: implement env cmd " + toString(cmd));
		return false;

	case RETRO_ENVIRONMENT_GET_GAME_INFO_EXT:
		// TODO (used by genesis plus gx)
		Logger::logWarning("TODO: implement env cmd " + toString(cmd));
		return false;

	case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2:
		onEnvSetCoreOptionsV2(*static_cast<const retro_core_options_v2*>(data));
		return true;

	case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL:
		onEnvSetCoreOptionsV2Intl(*static_cast<const retro_core_options_v2_intl*>(data));
		return true;

	case RETRO_ENVIRONMENT_GET_SAVESTATE_CONTEXT:
		if (data) {
			*static_cast<int*>(data) = onEnvGetSavestateContext();
		}
		return true;

	default:
		if (cmd & RETRO_ENVIRONMENT_EXPERIMENTAL) {
			Logger::logWarning("Unimplemented experimental Libretro env cmd: " + toString(static_cast<int>(cmd & 0xFFFF)));
		} else {
			Logger::logError("Unimplemented Libretro env cmd: " + toString(static_cast<int>(cmd)));
		}
		return false;
	}
}

void LibretroCore::onVideoRefresh(const void* data, uint32_t width, uint32_t height, size_t pitch)
{
	TextureFormat textureFormat;
	switch (systemAVInfo.pixelFormat) {
	case RETRO_PIXEL_FORMAT_0RGB1555:
		textureFormat = TextureFormat::BGRA5551;
		break;
	case RETRO_PIXEL_FORMAT_RGB565:
		textureFormat = TextureFormat::BGR565;
		break;
	case RETRO_PIXEL_FORMAT_XRGB8888:
		textureFormat = TextureFormat::BGRX;
		break;
	case RETRO_PIXEL_FORMAT_UNKNOWN:
	default:
		return;
	}

	const auto size = Vector2i(static_cast<int>(width), static_cast<int>(height));
	gsl::span<const char> dataSpan;
	Vector<char> temp;
	if (data) {
		dataSpan = gsl::span<const char>(static_cast<const char*>(data), pitch * height);
	} else {
		temp.resize(pitch * height, 0);
		dataSpan = temp;
	}

	cpuUpdateTexture->update(size, static_cast<int>(pitch), gsl::as_bytes(dataSpan), textureFormat);
	const auto tex = cpuUpdateTexture->getTexture();
	videoOut.getMutableMaterial().set(0, tex);

	const auto texSize = Vector2f(tex->getSize());
	const auto ar = systemAVInfo.aspectRatio;
	videoOut.setSize(texSize);
	videoOut.scaleTo(Vector2f(texSize.y * ar, texSize.y));
}

void LibretroCore::onAudioSample(int16_t left, int16_t right)
{
	const float samples[2] = { left / 32768.0f, right / 32768.0f };
	addAudioSamples(gsl::span(samples));
}

size_t LibretroCore::onAudioSampleBatch(const int16_t* data, size_t frames)
{
	const size_t samples = frames * 2;

	if (audioBuffer.size() < samples) {
		audioBuffer.resize(nextPowerOf2(samples));
	}

	for (size_t i = 0; i < samples; ++i) {
		audioBuffer[i] = data[i] / 32768.0f;
	}

	addAudioSamples(gsl::span<const float>(audioBuffer.data(), samples));
	
	return frames;
}

void LibretroCore::onEnvSetInputDescriptors(const retro_input_descriptor* data)
{
	// TODO
}

void LibretroCore::onEnvSetControllerInfo(const retro_controller_info& data)
{
	// TODO
}

void LibretroCore::onInputPoll()
{
	for (int i = 0; i < maxInputDevices; ++i) {
		inputJoypads[i] = 0;
		if (auto input = inputDevices[i]; input) {
			input->update(0); // TODO: pass correct time?
			
			uint16_t value = 0;

			value |= input->isButtonDown(0) ? (1 << RETRO_DEVICE_ID_JOYPAD_UP) : 0;
			value |= input->isButtonDown(1) ? (1 << RETRO_DEVICE_ID_JOYPAD_DOWN) : 0;
			value |= input->isButtonDown(2) ? (1 << RETRO_DEVICE_ID_JOYPAD_LEFT) : 0;
			value |= input->isButtonDown(3) ? (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT) : 0;

			value |= input->isButtonDown(4) ? (1 << RETRO_DEVICE_ID_JOYPAD_A) : 0;
			value |= input->isButtonDown(5) ? (1 << RETRO_DEVICE_ID_JOYPAD_B) : 0;
			value |= input->isButtonDown(6) ? (1 << RETRO_DEVICE_ID_JOYPAD_X) : 0;
			value |= input->isButtonDown(7) ? (1 << RETRO_DEVICE_ID_JOYPAD_Y) : 0;

			value |= input->isButtonDown(8) ? (1 << RETRO_DEVICE_ID_JOYPAD_SELECT) : 0;
			value |= input->isButtonDown(9) ? (1 << RETRO_DEVICE_ID_JOYPAD_START) : 0;
			
			value |= input->isButtonDown(10) ? (1 << RETRO_DEVICE_ID_JOYPAD_L) : 0;
			value |= input->isButtonDown(11) ? (1 << RETRO_DEVICE_ID_JOYPAD_R) : 0;
			value |= input->isButtonDown(12) ? (1 << RETRO_DEVICE_ID_JOYPAD_L2) : 0;
			value |= input->isButtonDown(13) ? (1 << RETRO_DEVICE_ID_JOYPAD_R2) : 0;
			value |= input->isButtonDown(14) ? (1 << RETRO_DEVICE_ID_JOYPAD_L3) : 0;
			value |= input->isButtonDown(15) ? (1 << RETRO_DEVICE_ID_JOYPAD_R3) : 0;

			bool leftStickAsDPad = true;
			if (leftStickAsDPad) {
				const float threshold = 0.2f;
				value |= input->getAxis(1) < -threshold ? (1 << RETRO_DEVICE_ID_JOYPAD_UP) : 0;
				value |= input->getAxis(1) > threshold ? (1 << RETRO_DEVICE_ID_JOYPAD_DOWN) : 0;
				value |= input->getAxis(0) < -threshold ? (1 << RETRO_DEVICE_ID_JOYPAD_LEFT) : 0;
				value |= input->getAxis(0) > threshold ? (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT) : 0;
			}

			inputJoypads[i] = value;
		}
	}
}

int16_t LibretroCore::onInputState(uint32_t port, uint32_t device, uint32_t index, uint32_t id)
{
	if (device == RETRO_DEVICE_JOYPAD) {
		assert(port < maxInputDevices);
		if (id == RETRO_DEVICE_ID_JOYPAD_MASK) {
			return inputJoypads[port];
		} else {
			return inputJoypads[port] & (1 << id) ? 1 : 0;
		}
	}
	return 0;
}

void LibretroCore::onLog(retro_log_level level, const char* str)
{
	if (level != RETRO_LOG_DUMMY) {
		const auto halleyLevel = static_cast<LoggerLevel>(level); // By sheer coincidence, the levels match
		Logger::log(halleyLevel, "[" + systemInfo.coreName + "] " + String(str).trimBoth());
	}
}

void LibretroCore::onEnvSetPerformanceLevel(uint32_t level)
{
	// Don't care?
}

bool LibretroCore::onEnvSetPixelFormat(retro_pixel_format data)
{
	systemAVInfo.pixelFormat = data;
	return true;
}

void LibretroCore::onEnvSetGeometry(const retro_game_geometry& data)
{
	systemAVInfo.loadGeometry(data);
}

void LibretroCore::onEnvSetRotation(uint32_t data)
{
	// TODO
}

int LibretroCore::onEnvGetAudioVideoEnable()
{
	constexpr int enableVideo = 0x1;
	constexpr int enableAudio = 0x2;
	constexpr int fastSaveSate = 0x4;
	constexpr int hardDisableAudio = 0x8;
	return enableVideo | enableAudio;
}

void LibretroCore::onEnvGetSystemDirectory(const char** data)
{
	*data = environment.getSystemDir().c_str();
}

void LibretroCore::onEnvSetSerializationQuirks(uint64_t& data)
{
	if (data & RETRO_SERIALIZATION_QUIRK_MUST_INITIALIZE) {
		Logger::logWarning("Serialization quirk not implemented: must initialize");
		data &= ~RETRO_SERIALIZATION_QUIRK_MUST_INITIALIZE;
	}
	if (data & RETRO_SERIALIZATION_QUIRK_CORE_VARIABLE_SIZE) {
		data |= RETRO_SERIALIZATION_QUIRK_FRONT_VARIABLE_SIZE;
	}
}

bool LibretroCore::onEnvGetVFSInterface(retro_vfs_interface_info& data)
{
	constexpr int versionSupported = 3;
	if (data.required_interface_version > versionSupported) {
		return false;
	}

	loadVFS();
	data.required_interface_version = versionSupported;
	data.iface = LibretroVFS::getLibretroInterface();

	return true;
}

void LibretroCore::onEnvGetSaveDirectory(const char** data)
{
	*data = environment.getSaveDir().c_str();
}

void LibretroCore::onEnvSetSubsystemInfo(const retro_subsystem_info& data)
{
	// TODO
}

void LibretroCore::onEnvSetMessageExt(const retro_message_ext& data)
{
	// TODO: missing other features
	const auto halleyLevel = static_cast<LoggerLevel>(data.level); // By sheer coincidence, the levels match
	Logger::log(halleyLevel, "[" + systemInfo.coreName + "] " + data.msg);
}

uint32_t LibretroCore::onEnvGetLanguage()
{
	return RETRO_LANGUAGE_ENGLISH;
}

void LibretroCore::onEnvSetSupportAchievements(bool data)
{
	systemInfo.supportAchievements = data;
}

retro_savestate_context LibretroCore::onEnvGetSavestateContext()
{
	switch (saveStateType) {
	case SaveStateType::Normal:
		return RETRO_SAVESTATE_CONTEXT_NORMAL;
	case SaveStateType::RunaheadSameInstance:
		return RETRO_SAVESTATE_CONTEXT_RUNAHEAD_SAME_INSTANCE;
	case SaveStateType::RunaheadSameBinary:
		return RETRO_SAVESTATE_CONTEXT_RUNAHEAD_SAME_BINARY;
	case SaveStateType::RollbackNetplay:
		return RETRO_SAVESTATE_CONTEXT_ROLLBACK_NETPLAY;
	default:
		return RETRO_SAVESTATE_CONTEXT_UNKNOWN;
	}
}

void LibretroCore::onEnvSetSupportNoGame(bool data)
{
	systemInfo.supportNoGame = data;
}

void LibretroCore::onEnvGetVariable(retro_variable& data)
{
	const auto iter = options.find(data.key);
	if (iter == options.end()) {
		data.value = nullptr;
	} else {
		data.value = iter->second.value.c_str();
	}
}

void LibretroCore::onEnvSetVariables(const retro_variable* data)
{
	for (const retro_variable* var = data; var->value || var->key; ++var) {
		Logger::logDev("Set variable: " + String(var->key) + " = " + var->value);
	}
}

bool LibretroCore::onEnvGetVariableUpdate()
{
	return false;
}

void LibretroCore::onEnvSetCoreOptions(const retro_core_option_definition* data) 
{
	retro_core_option_definition emptyDef = {};
	for (auto* definition = data; memcmp(definition, &emptyDef, sizeof(emptyDef)) != 0; ++definition) {
		Option& option = options[definition->key];
		option.category = "";
		option.defaultValue = definition->default_value;
		option.description = definition->desc;
		option.info = definition->info;

		if (option.value.isEmpty()) {
			option.value = option.defaultValue;
		}
	}
}

void LibretroCore::onEnvSetCoreOptionsIntl(const retro_core_options_intl& data) 
{
	onEnvSetCoreOptions(data.us);
}

void LibretroCore::onEnvSetCoreOptionsV2(const retro_core_options_v2& data) 
{
	retro_core_option_v2_definition emptyDef = {};
	for (auto* definition = data.definitions; memcmp(definition, &emptyDef, sizeof(emptyDef)) != 0; ++definition) {
		Option& option = options[definition->key];
		option.category = definition->category_key;
		option.defaultValue = definition->default_value;
		option.description = definition->desc;
		option.info = definition->info;

		if (option.value.isEmpty()) {
			option.value = option.defaultValue;
		}
	}
}

void LibretroCore::onEnvSetCoreOptionsV2Intl(const retro_core_options_v2_intl& data) 
{
	onEnvSetCoreOptionsV2(*data.us);
}

void LibretroCore::onEnvSetCoreOptionsDisplay(const retro_core_option_display& data)
{
	options[data.key].visible = data.visible;
}

void LibretroCore::onEnvSetAudioBufferStatusCallback(const retro_audio_buffer_status_callback* data)
{
	audioBufferStatusCallback = data ? data->callback : nullptr;
}

void LibretroCore::onEnvSetMinimumAudioLatency(uint32_t data)
{
	audioOut->setLatencyTarget(std::max(1024u, data * 48));
}
