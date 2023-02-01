#include "libretro_core.h"

#define RETRO_IMPORT_SYMBOLS
#include "libretro.h"


namespace {
	thread_local ILibretroCoreCallbacks* curInstance = nullptr;

	bool RETRO_CALLCONV retroEnvironmentCallback(unsigned cmd, void *data)
	{
		return curInstance->onEnvironment(cmd, data);
	}
	
	void RETRO_CALLCONV retroVideoRefreshCallback(const void *data, unsigned width, unsigned height, size_t pitch)
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
	
	int16_t RETRO_CALLCONV retroInputStateCallback(unsigned port, unsigned device, unsigned index, unsigned id)
	{
		return curInstance->onInputState(port, device, index, id);
	}
}

#define DLL_FUNC(dll, FUNC_NAME) static_cast<decltype(&(FUNC_NAME))>((dll).getFunction(#FUNC_NAME))


std::unique_ptr<LibretroCore> LibretroCore::load(std::string_view filename)
{
	DLL dll;
	
	if (dll.load(filename)) {
		const auto retroAPIVersion = static_cast<decltype(&retro_api_version)>(dll.getFunction("retro_api_version"));
		const auto dllVersion = retroAPIVersion();
		if (dllVersion == RETRO_API_VERSION) {
			return std::unique_ptr<LibretroCore>(new LibretroCore(std::move(dll)));
		}
	}
	return {};
}

LibretroCore::LibretroCore(DLL dll)
	: dll(std::move(dll))
{
	init();
}

LibretroCore::~LibretroCore()
{
	deInit();
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

bool LibretroCore::onEnvironment(unsigned cmd, void* data)
{
	switch (cmd) {
	case RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL:
		onEnvSetPerformanceLevel(*static_cast<const uint32_t*>(data));
		return false;

	case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
		onEnvGetSystemDirectory(static_cast<const char**>(data));
		return false;

	case RETRO_ENVIRONMENT_SET_VARIABLES:
		onEnvSetVariables(*static_cast<const retro_variable*>(data));
		return false;

	case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
		onEnvSetSupportNoGame(*static_cast<const bool*>(data));
		return false;

	case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
		onEnvGetLogInterface(*static_cast<retro_log_callback*>(data));
		return false;

	case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
		onEnvGetSaveDirectory(static_cast<const char**>(data));
		return false;

	case RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO:
		onEnvSetSubsystemInfo(*static_cast<const retro_subsystem_info*>(data));
		return false;

	case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
		onEnvSetControllerInfo(*static_cast<const retro_controller_info*>(data));
		return false;

	case RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS:
		onEnvSetSupportAchievements(*static_cast<const bool*>(data));
		return false;

	case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS:
		onEnvGetInputBitmasks(*static_cast<bool*>(data));
		return false;

	case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION:
		onEnvGetCoreOptionsVersion(*static_cast<uint32_t*>(data));
		return false;

	default:
		if (cmd & RETRO_ENVIRONMENT_EXPERIMENTAL) {
			Logger::logWarning("Unimplemented experimental Libretro env cmd: " + toString(static_cast<int>(cmd & 0xFFFF)));
		} else {
			Logger::logError("Unimplemented Libretro env cmd: " + toString(static_cast<int>(cmd)));
		}
		return false;
	}
}

void LibretroCore::onVideoRefresh(const void* data, unsigned width, unsigned height, size_t size)
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

int16_t LibretroCore::onInputState(unsigned port, unsigned device, unsigned index, unsigned id)
{
	// TODO
	return 0;
}

void LibretroCore::onEnvSetPerformanceLevel(uint32_t level)
{
	// TODO
}

void LibretroCore::onEnvGetSystemDirectory(const char** data)
{
	// TODO
}

void LibretroCore::onEnvSetVariables(const retro_variable& data)
{
	// TODO
}

void LibretroCore::onEnvSetSupportNoGame(bool data)
{
	// TODO
}

void LibretroCore::onEnvGetLogInterface(const retro_log_callback& data)
{
	// TODO
}

void LibretroCore::onEnvGetSaveDirectory(const char** data)
{
	// TODO
}

void LibretroCore::onEnvSetSubsystemInfo(const retro_subsystem_info& data)
{
	// TODO
}

void LibretroCore::onEnvSetControllerInfo(const retro_controller_info& data)
{
	// TODO
}

void LibretroCore::onEnvSetSupportAchievements(bool data)
{
	// TODO
}

void LibretroCore::onEnvGetInputBitmasks(bool& data)
{
	// TODO
}

void LibretroCore::onEnvGetCoreOptionsVersion(uint32_t data)
{
	// TODO
}
