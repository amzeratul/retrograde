#include "libretro_core.h"

#define RETRO_IMPORT_SYMBOLS
#include "libretro.h"

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

void LibretroCore::init()
{
	retro_system_info systemInfo;
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
	// TODO
	return false;
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
