#pragma once

#include <halley.hpp>

#include "libretro.h"
#include "src/util/dll.h"
class LibretroEnvironment;
using namespace Halley;

class ILibretroCoreCallbacks {
public:
	virtual ~ILibretroCoreCallbacks() = default;

	virtual bool onEnvironment(uint32_t cmd, void* data) = 0;
	virtual void onVideoRefresh(const void* data, uint32_t width, uint32_t height, size_t size) = 0;
	virtual void onAudioSample(int16_t left, int16_t int16) = 0;
	virtual size_t onAudioSampleBatch(const int16_t* data, size_t size) = 0;
	virtual void onInputPoll() = 0;
	virtual int16_t onInputState(uint32_t port, uint32_t device, uint32_t index, uint32_t id) = 0;
	virtual void onLog(retro_log_level level, const char* str) = 0;
};

class LibretroCore : protected ILibretroCoreCallbacks {
public:
	static std::unique_ptr<LibretroCore> load(std::string_view filename, LibretroEnvironment& environment);
	~LibretroCore() override;

	bool loadGame(std::string_view path, gsl::span<const gsl::byte> data, std::string_view meta);
	void unloadGame();
	void run();

protected:
	bool onEnvironment(uint32_t cmd, void* data) override;
	void onVideoRefresh(const void* data, uint32_t width, uint32_t height, size_t size) override;
	void onAudioSample(int16_t left, int16_t int16) override;
	size_t onAudioSampleBatch(const int16_t* data, size_t size) override;
	void onInputPoll() override;
	int16_t onInputState(uint32_t port, uint32_t device, uint32_t index, uint32_t id) override;
	void onLog(retro_log_level level, const char* str) override;

private:
	DLL dll;
	LibretroEnvironment& environment;

	bool gameLoaded = false;
	bool supportNoGame = false;
	bool supportAchievements = false;

	LibretroCore(DLL dll, LibretroEnvironment& environment);

	void init();
	void deInit();

	void onEnvSetPerformanceLevel(uint32_t level);
	void onEnvGetSystemDirectory(const char** data);
	void onEnvGetVariable(retro_variable& data);
	void onEnvSetVariables(const retro_variable* data);
	void onEnvSetSupportNoGame(bool data);
	void onEnvGetSaveDirectory(const char** data);
	void onEnvSetSubsystemInfo(const retro_subsystem_info& data);
	void onEnvSetControllerInfo(const retro_controller_info& data);
	uint32_t onEnvGetLanguage();
	void onEnvSetSupportAchievements(bool data);
	void onEnvSetCoreOptions(const retro_core_option_definition** data);
	void onEnvSetCoreOptionsIntl(const retro_core_options_intl& data);
	void onEnvSetCoreOptionsDisplay(const retro_core_option_display& data);
	void onEnvSetCoreOptionsV2(const retro_core_options_v2& data);
	void onEnvSetCoreOptionsV2Intl(const retro_core_options_v2_intl& data);
};
