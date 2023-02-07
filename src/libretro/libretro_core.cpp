#include "libretro_core.h"

#define RETRO_IMPORT_SYMBOLS
#include <cstdarg>

#include "libretro.h"
#include "libretro_environment.h"
#include "libretro_vfs.h"
#include "src/util/cpu_update_texture.h"
#include "src/util/c_string_cache.h"
#include "src/zip/zip_file.h"

#ifdef _WIN32
#define UUID_DEFINED
#define HAVE_D3D11
#include "halley/src/plugins/dx11/src/dx11_video.h"
#include "halley/src/plugins/dx11/src/dx11_texture.h"
#include "libretro_d3d.h"
#pragma comment(lib, "D3DCompiler.lib")
#endif

#ifdef WITH_VULKAN
#include "libretro_vulkan.h"
#endif

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

	void RETRO_CALLCONV retroSetLEDState(int led, int state)
	{
		return ILibretroCoreCallbacks::curInstance->onSetLEDState(led, state);
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

	uintptr_t RETRO_CALLCONV retro_hw_get_current_framebuffer()
	{
		return ILibretroCoreCallbacks::curInstance->onHWGetCurrentFrameBuffer();
	}

#ifdef WITH_DX11
	HRESULT WINAPI retro_d3d_compile(LPCVOID pSrcData, SIZE_T srcDataSize, LPCSTR pFileName, CONST D3D_SHADER_MACRO* pDefines, ID3DInclude* pInclude,
		LPCSTR pEntrypoin, LPCSTR pTarget, UINT flags1, UINT flags2, ID3DBlob** ppCode, ID3DBlob** ppErrorMsgs)
	{
		auto result = D3DCompile(pSrcData, srcDataSize, pFileName, pDefines, pInclude, pEntrypoin, pTarget, flags1, flags2, ppCode, ppErrorMsgs);
		return result;
	}
#endif
}


bool LibretroCore::ContentInfo::isValidExtension(std::string_view filePath) const
{
	return isValidExtension(Path(filePath));
}

bool LibretroCore::ContentInfo::isValidExtension(const Path& filePath) const
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
	contentInfos.clear();
	auto& cInfo = contentInfos.emplace_back();
	cInfo.validExtensions = String(retroSystemInfo.valid_extensions).split('|');
	cInfo.needFullpath = retroSystemInfo.need_fullpath;
	cInfo.persistData = false;

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

	auto guard = ScopedGuard([=]() { curInstance = nullptr; });
	curInstance = this;

	DLL_FUNC(dll, retro_deinit)();

	audioBufferStatusCallback = nullptr;
	diskControlCallbacks.reset();
	hwRenderCallback.reset();
	hwRenderInterface.reset();
	vfs.reset();
}

std::pair<const LibretroCore::ContentInfo*, size_t> LibretroCore::getContentInfo(const ZipFile& zip)
{
	const size_t n = zip.getNumFiles();
	for (size_t i = 0; i < n; ++i) {
		for (auto& cInfo: contentInfos) {
			if (cInfo.isValidExtension(std::string_view(zip.getFileName(i)))) {
				return { &cInfo, i };
			}
		}
	}
	return { &contentInfos.front(), 0 };
}

const LibretroCore::ContentInfo* LibretroCore::getContentInfo(const Path& path)
{
	for (auto& cInfo : contentInfos) {
		if (cInfo.isValidExtension(path)) {
			return &cInfo;
		}
	}
	return &contentInfos.front();
}

bool LibretroCore::loadGame(const Path& path)
{
	if (gameLoaded) {
		unloadGame();
	}

	CStringCache cache;

	gameInfos.clear();
	auto& gameInfoEx = gameInfos.emplace_back();
	gameInfoEx.full_path = nullptr;
	gameInfoEx.archive_path = nullptr;
	gameInfoEx.archive_file = nullptr;
	gameInfoEx.dir = cache(path.parentPath().getNativeString());
	gameInfoEx.name = cache(path.getFilename().replaceExtension("").getNativeString());
	gameInfoEx.ext = nullptr;
	gameInfoEx.meta = nullptr;
	gameInfoEx.data = nullptr;
	gameInfoEx.size = 0;
	gameInfoEx.file_in_archive = false;
	gameInfoEx.persistent_data = false;

	const bool canExtract = !systemInfo.blockExtract && ZipFile::isZipFile(path);
	ZipFile zip;

	const ContentInfo* targetContentInfo = nullptr;
	Path targetPath;
	size_t archiveIdx = 0;
	if (canExtract) {
		zip.open(path, false);
		std::tie(targetContentInfo, archiveIdx) = getContentInfo(zip);
		const auto archiveFileName = zip.getFileName(archiveIdx);
		targetPath = Path("!zip") / archiveFileName;
		gameInfoEx.archive_path = cache(path.getNativeString());
		gameInfoEx.archive_file = cache(archiveFileName);
		gameInfoEx.file_in_archive = true;
	} else {
		targetContentInfo = getContentInfo(path);
		targetPath = path;
	}

	gameInfoEx.full_path = cache(targetPath.getNativeString());
	gameInfoEx.persistent_data = targetContentInfo->persistData;
	gameInfoEx.ext = cache(Path(targetPath).getExtension().substr(1).asciiLower());

	if (targetContentInfo->needFullpath) {
		// Fullpath cores can still read zipped files if they support VFS
		// In those cases, we'll extract the zip into VFS and load that instead
		if (vfs && canExtract) {
			zip.extractAll("!zip", *vfs);
			gameInfoEx.full_path = cache(targetPath.getString()); // VFS requires unix style path
		}
	} else {
		if (canExtract) {
			gameBytes = zip.extractFile(archiveIdx);
		} else {
			gameBytes = Path::readFile(targetPath);
		}
		if (gameBytes.empty()) {
			return false;
		}
		gameInfoEx.data = gameBytes.data();
		gameInfoEx.size = gameBytes.size();
	}

	doLoadGame();

	if (!targetContentInfo->persistData) {
		gameBytes.clear();
	}

	if (gameLoaded) {
		gameName = Path(path).getFilename().replaceExtension("").getString();
	}

	return gameLoaded;
}

bool LibretroCore::doLoadGame()
{
	assert(!gameLoaded);
	
	auto& info = gameInfos.front();
	retro_game_info gameInfo;
	gameInfo.path = info.full_path;
	gameInfo.meta = info.meta;
	gameInfo.size = info.size;
	gameInfo.data = info.data;

	auto guard = ScopedGuard([=]() { curInstance = nullptr; });
	curInstance = this;

	gameLoaded = DLL_FUNC(dll, retro_load_game)(&gameInfo);

	if (gameLoaded) {
		retro_system_av_info retroAVInfo = {};
		DLL_FUNC(dll, retro_get_system_av_info)(&retroAVInfo);

		systemAVInfo.fps = retroAVInfo.timing.fps;
		systemAVInfo.sampleRate = retroAVInfo.timing.sample_rate;
		systemAVInfo.loadGeometry(retroAVInfo.geometry);

		initVideoOut();
		initAudioOut();
		loadGameData();
	} else {
		gameInfos.clear();
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
		gameInfos.clear();
		gameBytes.clear();

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

	if (renderCallbackNeedsReset) {
		renderCallbackNeedsReset = false;
		hwRenderCallback->context_reset();
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
	inputs[idx].device = std::move(input);
}

bool LibretroCore::onEnvironment(uint32_t cmd, void* data)
{
	switch (cmd) {
	case RETRO_ENVIRONMENT_SET_ROTATION:
		onEnvSetRotation(*static_cast<const uint32_t*>(data));
		return true;

	case RETRO_ENVIRONMENT_GET_CAN_DUPE:
		*static_cast<bool*>(data) = true;
		return true;

	case RETRO_ENVIRONMENT_SET_MESSAGE:
		// TODO
		Logger::logWarning("TODO: RETRO_ENVIRONMENT_SET_MESSAGE");
		return false;

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
		onEnvSetDiskControlInterface(*static_cast<const retro_disk_control_callback*>(data));
		return true;

	case RETRO_ENVIRONMENT_SET_HW_RENDER:
		return onEnvSetHWRender(*static_cast<retro_hw_render_callback*>(data));

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

	case RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE:
		// TODO
		Logger::logWarning("TODO: RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE");
		return false;

	case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
		static_cast<retro_log_callback*>(data)->log = &retroLogPrintf;
		return true;

	case RETRO_ENVIRONMENT_GET_PERF_INTERFACE:
		// TODO
		Logger::logWarning("TODO: RETRO_ENVIRONMENT_GET_PERF_INTERFACE");
		return false;

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
		// TODO
		Logger::logWarning("TODO: RETRO_ENVIRONMENT_SET_MEMORY_MAPS");
		return false;

	case RETRO_ENVIRONMENT_SET_GEOMETRY:
		onEnvSetGeometry(*static_cast<const retro_game_geometry*>(data));
		return true;

	case RETRO_ENVIRONMENT_GET_LANGUAGE:
		*static_cast<uint32_t*>(data) = onEnvGetLanguage();
		return true;

	case RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER:
		// TODO
		Logger::logWarning("TODO: RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER");
		return false;

	case RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE:
		return onEnvGetHWRenderInterface(static_cast<const retro_hw_render_interface**>(data));

	case RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS:
		onEnvSetSupportAchievements(*static_cast<const bool*>(data));
		return true;

	case RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS:
		onEnvSetSerializationQuirks(*static_cast<uint64_t*>(data));
		return true;

	case RETRO_ENVIRONMENT_GET_VFS_INTERFACE:
		return onEnvGetVFSInterface(*static_cast<retro_vfs_interface_info*>(data));

	case RETRO_ENVIRONMENT_GET_LED_INTERFACE:
		static_cast<retro_led_interface*>(data)->set_led_state = &retroSetLEDState;
		return true;

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

	case RETRO_ENVIRONMENT_GET_PREFERRED_HW_RENDER:
		*static_cast<uint32_t*>(data) = onEnvGetPreferredHWRender();
		return true;

	case RETRO_ENVIRONMENT_GET_DISK_CONTROL_INTERFACE_VERSION:
		*static_cast<uint32_t*>(data) = 1;
		return true;

	case RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE:
		onEnvSetDiskControlExtInterface(*static_cast<retro_disk_control_ext_callback*>(data));
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
		if (data) {
			onEnvSetContentInfoOverride(static_cast<const retro_system_content_info_override*>(data));
		}
		return true;

	case RETRO_ENVIRONMENT_GET_GAME_INFO_EXT:
		*static_cast<const retro_game_info_ext**>(data) = gameInfos.data();
		return true;

	case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2:
		onEnvSetCoreOptionsV2(*static_cast<const retro_core_options_v2*>(data));
		return true;

	case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL:
		onEnvSetCoreOptionsV2Intl(*static_cast<const retro_core_options_v2_intl*>(data));
		return true;

	case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_UPDATE_DISPLAY_CALLBACK:
		// TODO
		Logger::logWarning("TODO: RETRO_ENVIRONMENT_SET_CORE_OPTIONS_UPDATE_DISPLAY_CALLBACK");
		return false;

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
	if (pitch == 0) {
		pitch = width;
	}

	const auto size = Vector2i(static_cast<int>(width), static_cast<int>(height));
	if (size.x == 0 || size.y == 0) {
		return;
	}

	// Update texture
	gsl::span<const char> dataSpan;
	Vector<char> temp;
	if (data == nullptr || data == RETRO_HW_FRAME_BUFFER_VALID) {
		if (cpuUpdateTexture->getTexture()) {
			// Dupe frame
		} else {
			temp.resize(pitch * height, 0);
			dataSpan = temp;
		}
	} else {
		dataSpan = gsl::span<const char>(static_cast<const char*>(data), pitch * height);
	}

	if (!dataSpan.empty()) {
		cpuUpdateTexture->update(size, static_cast<int>(pitch), gsl::as_bytes(dataSpan), getTextureFormat(systemAVInfo.pixelFormat));
	}
	if (data == RETRO_HW_FRAME_BUFFER_VALID) {
		dx11UpdateTextureToCurrentBound(size, pitch);
	}

	const auto tex = cpuUpdateTexture->getTexture();
	const auto texSize = Vector2f(tex->getSize());
	const auto ar = systemAVInfo.aspectRatio;
	videoOut.getMutableMaterial().set(0, tex);
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
		inputs[i].buttonMask = 0;
		if (const auto& input = inputs[i].device; input) {
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

			// Fill analogue
			for (int j = 0; j < 16; ++j) {
				inputs[i].analogButtons[j] = (value & (1 << j)) ? 1.0f : 0.0f;
			}
			inputs[i].analogButtons[RETRO_DEVICE_ID_JOYPAD_L2] = input->getAxis(4);
			inputs[i].analogButtons[RETRO_DEVICE_ID_JOYPAD_R2] = input->getAxis(5);

			inputs[i].buttonMask = value;
		}
	}
}

int16_t LibretroCore::onInputState(uint32_t port, uint32_t device, uint32_t index, uint32_t id)
{
	auto floatToInt = [&](float value) -> int16_t
	{
		return static_cast<int16_t>(clamp(static_cast<int32_t>(value * 0x8000), -0x7FFF, 0x7FFF));
	};

	if (port >= maxInputDevices) {
		return 0;
	}
	const auto& input = inputs[port];

	if (device == RETRO_DEVICE_JOYPAD) {
		if (id == RETRO_DEVICE_ID_JOYPAD_MASK) {
			return input.buttonMask;
		} else {
			return input.buttonMask & (1 << id) ? 1 : 0;
		}
	} else if (device == RETRO_DEVICE_MOUSE) {
		// TODO
		return 0;
	} else if (device == RETRO_DEVICE_KEYBOARD) {
		// TODO
		return 0;
	} else if (device == RETRO_DEVICE_LIGHTGUN) {
		// TODO
		return 0;
	} else if (device == RETRO_DEVICE_ANALOG) {
		if (index == RETRO_DEVICE_INDEX_ANALOG_LEFT || index == RETRO_DEVICE_INDEX_ANALOG_RIGHT) {
			const int axis = static_cast<int>(id); // 0 = X, 1 = Y
			if (axis < 2) {
				return floatToInt(input.sticks[index][axis]);
			} else {
				return 0;
			}
		} else if (index == RETRO_DEVICE_INDEX_ANALOG_BUTTON) {
			return floatToInt(input.analogButtons[id]);
		}
	} else if (device == RETRO_DEVICE_POINTER) {
		// TODO
		return 0;
	}
	return 0;
}

void LibretroCore::onSetLEDState(int led, int state)
{
	// TODO?
	// Apparently:
	// 0 = Power
	// 1 = CD
}

void LibretroCore::onLog(retro_log_level level, const char* str)
{
	if (level != RETRO_LOG_DUMMY) {
		const auto halleyLevel = static_cast<LoggerLevel>(level); // By sheer coincidence, the levels match
		Logger::log(halleyLevel, "[" + systemInfo.coreName + "] " + String(str).trimBoth());
	}
}

uintptr_t LibretroCore::onHWGetCurrentFrameBuffer()
{
	// TODO
	return 0;
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

bool LibretroCore::onEnvSetHWRender(retro_hw_render_callback& data)
{
	if (data.context_type == RETRO_HW_CONTEXT_DIRECT3D) {
		hwRenderCallback = data;
		renderCallbackNeedsReset = true;
		return true;
	}
	return false;
}

uint32_t LibretroCore::onEnvGetPreferredHWRender()
{
	return RETRO_HW_CONTEXT_DIRECT3D;
}

bool LibretroCore::onEnvGetHWRenderInterface(const retro_hw_render_interface** data)
{
	*data = getD3DHWRenderInterface();
	return *data != nullptr;
}

const retro_hw_render_interface* LibretroCore::getD3DHWRenderInterface()
{
#ifdef WITH_DX11
	auto hwInterface = std::make_shared<retro_hw_render_interface_d3d11>();

	auto& dx11Video = static_cast<DX11Video&>(*environment.getHalleyAPI().video);

	hwInterface->interface_type = RETRO_HW_RENDER_INTERFACE_D3D11;
	hwInterface->interface_version = RETRO_HW_RENDER_INTERFACE_D3D11_VERSION;
	hwInterface->featureLevel = dx11Video.getFeatureLevel();
	hwInterface->handle = this;
	hwInterface->device = &dx11Video.getDevice();
	hwInterface->context = &dx11Video.getDeviceContext();
	hwInterface->D3DCompile = retro_d3d_compile;

	hwRenderInterface = std::move(hwInterface);
	return static_cast<retro_hw_render_interface*>(hwRenderInterface.get());
#else
	return nullptr;
#endif
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

void LibretroCore::onEnvSetDiskControlInterface(const retro_disk_control_callback& data)
{
	diskControlCallbacks = retro_disk_control_ext_callback{};

	diskControlCallbacks->set_eject_state = data.set_eject_state;
	diskControlCallbacks->get_eject_state = data.get_eject_state;

	diskControlCallbacks->get_image_index = data.get_image_index;
	diskControlCallbacks->set_image_index = data.set_image_index;
	diskControlCallbacks->get_num_images = data.get_num_images;

	diskControlCallbacks->replace_image_index = data.replace_image_index;
	diskControlCallbacks->add_image_index = data.add_image_index;
}

void LibretroCore::onEnvSetDiskControlExtInterface(const retro_disk_control_ext_callback& data)
{
	diskControlCallbacks = data;
}

void LibretroCore::onEnvSetContentInfoOverride(const retro_system_content_info_override* data)
{
	// Keep the original...
	auto orig = std::move(contentInfos.front());

	contentInfos.clear();
	for (auto* cur = data; cur->extensions != nullptr; ++cur) {
		auto& info = contentInfos.emplace_back();
		info.validExtensions = String(data->extensions).split('|');
		info.needFullpath = cur->need_fullpath;
		info.persistData = cur->persistent_data;
	}

	// ...and re-add it at the end, so it's the lowest priority option
	contentInfos.push_back(std::move(orig));
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

		for (int i = 0; i < RETRO_NUM_CORE_OPTION_VALUES_MAX && definition->values[i].value; ++i) {
			option.values.push_back(Option::Value{ definition->values[i].value, definition->values[i].label });
		}

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

		for (int i = 0; i < RETRO_NUM_CORE_OPTION_VALUES_MAX && definition->values[i].value; ++i) {
			option.values.push_back(Option::Value{ definition->values[i].value, definition->values[i].label });
		}

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

void LibretroCore::dx11UpdateTextureToCurrentBound(Vector2i size, size_t pitch)
{
	// The following comment is from swanstation source:
	// NOTE: libretro frontend expects the data bound to PS SRV slot 0.
	// m_context->OMSetRenderTargets(0, nullptr, nullptr);
	// m_context->PSSetShaderResources(0, 1, m_framebuffer.GetD3DSRVArray());

	auto& dx11Video = static_cast<DX11Video&>(*environment.getHalleyAPI().video);
	ID3D11ShaderResourceView* view;
	dx11Video.getDeviceContext().PSGetShaderResources(0, 1, &view);

	auto tex = std::dynamic_pointer_cast<DX11Texture>(cpuUpdateTexture->getTexture());
	tex->replaceShaderResourceView(view);
}

TextureFormat LibretroCore::getTextureFormat(retro_pixel_format retroFormat) const
{
	switch (systemAVInfo.pixelFormat) {
	case RETRO_PIXEL_FORMAT_0RGB1555:
		return TextureFormat::BGRA5551;
	case RETRO_PIXEL_FORMAT_RGB565:
		return TextureFormat::BGR565;
	case RETRO_PIXEL_FORMAT_XRGB8888:
		return TextureFormat::BGRX;
	case RETRO_PIXEL_FORMAT_UNKNOWN:
	default:
		return TextureFormat::BGRX;
	}
}
