#include "libretro_core.h"

#define RETRO_IMPORT_SYMBOLS
#include <cstdarg>

#include "libretro.h"
#include "libretro_environment.h"


namespace {
	thread_local ILibretroCoreCallbacks* curInstance = nullptr;

	bool RETRO_CALLCONV retroEnvironmentCallback(uint32_t cmd, void *data)
	{
		return curInstance->onEnvironment(cmd, data);
	}
	
	void RETRO_CALLCONV retroVideoRefreshCallback(const void *data, uint32_t width, uint32_t height, size_t pitch)
	{
		curInstance->onVideoRefresh(data, width, height, pitch);
	}
	
	void RETRO_CALLCONV retroAudioSampleCallback(int16_t left, int16_t right)
	{
		curInstance->onAudioSample(left, right);
	}
	
	size_t RETRO_CALLCONV retroAudioSampleBatchCallback(const int16_t *data, size_t frames)
	{
		return curInstance->onAudioSampleBatch(data, frames);
	}
	
	void RETRO_CALLCONV retroInputPollCallback()
	{
		return curInstance->onInputPoll();
	}
	
	int16_t RETRO_CALLCONV retroInputStateCallback(uint32_t port, uint32_t device, uint32_t index, uint32_t id)
	{
		return curInstance->onInputState(port, device, index, id);
	}

	void RETRO_CALLCONV retroLogPrintf(retro_log_level level, const char *fmt, ...)
	{
		char buffer[4096];

		va_list args;
		va_start(args, fmt);
		int n = vsprintf_s(buffer, sizeof(buffer), fmt, args);
		if (n > 0) {
			curInstance->onLog(level, buffer);
		}
		va_end(args);
	}
}

#define DLL_FUNC(dll, FUNC_NAME) static_cast<decltype(&(FUNC_NAME))>((dll).getFunction(#FUNC_NAME))


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

bool LibretroCore::loadGame(std::string_view path)
{
	if (needFullpath) {
		return loadGame(path, {}, {});
	} else {
		auto bytes = Path::readFile(Path(path));
		if (bytes.empty()) {
			return false;
		}
		return loadGame(path, gsl::as_bytes(gsl::span<const Byte>(bytes.data(), bytes.size())), {});
	}
}

bool LibretroCore::loadGame(std::string_view path, gsl::span<const gsl::byte> data, std::string_view meta)
{
	if (gameLoaded) {
		unloadGame();
	}

	retro_game_info gameInfo;
	gameInfo.path = path.data();
	gameInfo.meta = meta.data();
	gameInfo.size = data.size();
	gameInfo.data = data.data();

	auto guard = ScopedGuard([=]() { curInstance = nullptr; });
	curInstance = this;

	gameLoaded = DLL_FUNC(dll, retro_load_game)(&gameInfo);
	return gameLoaded;
}

void LibretroCore::unloadGame()
{
	if (gameLoaded) {
		auto guard = ScopedGuard([=]() { curInstance = nullptr; });
		curInstance = this;

		DLL_FUNC(dll, retro_unload_game)();

		gameLoaded = false;
	}
}

void LibretroCore::init()
{
	retro_system_info systemInfo = {};
	DLL_FUNC(dll, retro_get_system_info)(&systemInfo);
	Logger::logDev("Loaded core " + String(systemInfo.library_name) + " " + String(systemInfo.library_version));
	coreName = systemInfo.library_name;
	coreVersion = systemInfo.library_version;
	blockExtract = systemInfo.block_extract;
	needFullpath = systemInfo.need_fullpath;

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

void LibretroCore::deInit()
{
	if (gameLoaded) {
		unloadGame();
	}

	auto guard = ScopedGuard([=]() { curInstance = nullptr; });
	curInstance = this;

	DLL_FUNC(dll, retro_deinit)();
}

void LibretroCore::run()
{
	auto guard = ScopedGuard([=]() { curInstance = nullptr; });
	curInstance = this;

	DLL_FUNC(dll, retro_run)();
}

bool LibretroCore::hasGameLoaded() const
{
	return gameLoaded;
}

bool LibretroCore::onEnvironment(uint32_t cmd, void* data)
{
	switch (cmd) {
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

	case RETRO_ENVIRONMENT_SET_GEOMETRY:
		onEnvSetGeometry(*static_cast<const retro_game_geometry*>(data));
		return true;

	case RETRO_ENVIRONMENT_GET_LANGUAGE:
		*static_cast<uint32_t*>(data) = onEnvGetLanguage();
		return true;

	case RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS:
		onEnvSetSupportAchievements(*static_cast<const bool*>(data));
		return true;

	case RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE:
		*static_cast<int*>(data) = onEnvGetAudioVideoEnable();
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

	case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2:
		onEnvSetCoreOptionsV2(*static_cast<const retro_core_options_v2*>(data));
		return true;

	case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL:
		onEnvSetCoreOptionsV2Intl(*static_cast<const retro_core_options_v2_intl*>(data));
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

void LibretroCore::onVideoRefresh(const void* data, uint32_t width, uint32_t height, size_t size)
{
	// TODO
}

void LibretroCore::onAudioSample(int16_t left, int16_t int16)
{
	// TODO
}

size_t LibretroCore::onAudioSampleBatch(const int16_t* data, size_t size)
{
	// TODO
	return 0;
}

void LibretroCore::onInputPoll()
{
	// TODO
}

int16_t LibretroCore::onInputState(uint32_t port, uint32_t device, uint32_t index, uint32_t id)
{
	// TODO
	return 0;
}

void LibretroCore::onLog(retro_log_level level, const char* str)
{
	if (level != RETRO_LOG_DUMMY) {
		const auto halleyLevel = static_cast<LoggerLevel>(level); // By sheer coincidence, the levels match
		Logger::log(halleyLevel, "[" + coreName + "] " + str);
	}
}

void LibretroCore::onEnvSetPerformanceLevel(uint32_t level)
{
	// Don't care?
}

bool LibretroCore::onEnvSetPixelFormat(retro_pixel_format data)
{
	pixelFormat = data;
	return true;
}

void LibretroCore::onEnvSetGeometry(const retro_game_geometry& data)
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

void LibretroCore::onEnvSetInputDescriptors(const retro_input_descriptor* data)
{
	// TODO
}

void LibretroCore::onEnvGetSaveDirectory(const char** data)
{
	*data = environment.getSaveDir().c_str();
}

void LibretroCore::onEnvSetSubsystemInfo(const retro_subsystem_info& data)
{
	// TODO
}

void LibretroCore::onEnvSetControllerInfo(const retro_controller_info& data)
{
	// TODO
}

uint32_t LibretroCore::onEnvGetLanguage()
{
	return RETRO_LANGUAGE_ENGLISH;
}

void LibretroCore::onEnvSetSupportAchievements(bool data)
{
	supportAchievements = data;
}

void LibretroCore::onEnvSetSupportNoGame(bool data)
{
	supportNoGame = data;
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
	// TODO
}
