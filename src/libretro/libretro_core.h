#pragma once

#include <halley.hpp>

#include "libretro.h"
#include "src/util/dll.h"
using namespace Halley;

class ILibretroCoreCallbacks {
public:
	virtual ~ILibretroCoreCallbacks() = default;

	virtual bool onEnvironment(unsigned cmd, void* data) = 0;
	virtual void onVideoRefresh(const void* data, unsigned width, unsigned height, size_t size) = 0;
	virtual void onAudioSample(int16_t left, int16_t int16) = 0;
	virtual size_t onAudioSampleBatch(const int16_t* data, size_t size) = 0;
	virtual void onInputPoll() = 0;
	virtual int16_t onInputState(unsigned port, unsigned device, unsigned index, unsigned id) = 0;
};

class LibretroCore : protected ILibretroCoreCallbacks {
public:
	static std::unique_ptr<LibretroCore> load(std::string_view filename);
	~LibretroCore() override;

	bool loadGame(std::string_view path, gsl::span<const gsl::byte> data, std::string_view meta);
	void unloadGame();
	void run();

protected:
	bool onEnvironment(unsigned cmd, void* data) override;
	void onVideoRefresh(const void* data, unsigned width, unsigned height, size_t size) override;
	void onAudioSample(int16_t left, int16_t int16) override;
	size_t onAudioSampleBatch(const int16_t* data, size_t size) override;
	void onInputPoll() override;
	int16_t onInputState(unsigned port, unsigned device, unsigned index, unsigned id) override;

private:
	DLL dll;
	bool gameLoaded = false;

	explicit LibretroCore(DLL dll);

	void init();
	void deInit();

	void onEnvSetPerformanceLevel(uint32_t level);
	void onEnvGetSystemDirectory(const char** data);
	void onEnvSetVariables(const retro_variable& data);
	void onEnvSetSupportNoGame(bool data);
	void onEnvGetLogInterface(const retro_log_callback& data);
	void onEnvGetSaveDirectory(const char** data);
	void onEnvSetSubsystemInfo(const retro_subsystem_info& data);
	void onEnvSetControllerInfo(const retro_controller_info& data);
	void onEnvSetSupportAchievements(bool data);
	void onEnvGetInputBitmasks(bool& data);
	void onEnvGetCoreOptionsVersion(uint32_t data);
};
