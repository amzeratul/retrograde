#pragma once

#include <halley.hpp>

#include "libretro.h"
#include "src/util/dll.h"
class CPUUpdateTexture;
class LibretroEnvironment;
using namespace Halley;

class ILibretroCoreCallbacks {
public:
	virtual ~ILibretroCoreCallbacks() = default;

	virtual bool onEnvironment(uint32_t cmd, void* data) = 0;
	virtual void onVideoRefresh(const void* data, uint32_t width, uint32_t height, size_t pitch) = 0;
	virtual void onAudioSample(int16_t left, int16_t right) = 0;
	virtual size_t onAudioSampleBatch(const int16_t* data, size_t size) = 0;
	virtual void onInputPoll() = 0;
	virtual int16_t onInputState(uint32_t port, uint32_t device, uint32_t index, uint32_t id) = 0;
	virtual void onLog(retro_log_level level, const char* str) = 0;
};

class LibretroCore : protected ILibretroCoreCallbacks {
public:
	struct Option {
		String value;
		String defaultValue;
		String description;
		String info;
		String category;
	};

	struct SystemInfo {
		String coreName;
		String coreVersion;
		bool supportNoGame = false;
		bool supportAchievements = false;
		bool blockExtract = false;
		bool needFullpath = false;
	};

	struct SystemAVInfo {
		retro_pixel_format pixelFormat = RETRO_PIXEL_FORMAT_0RGB1555;
		double fps = 60.0;
		double sampleRate = 48000.0;
		float aspectRatio = 4.0f / 3.0f;
		Vector2i baseSize;
		Vector2i maxSize;
		
		void loadGeometry(const retro_game_geometry& geometry);
	};

	static std::unique_ptr<LibretroCore> load(std::string_view filename, LibretroEnvironment& environment);
	~LibretroCore() override;

	bool loadGame(std::string_view path);
	bool loadGame(std::string_view path, gsl::span<const gsl::byte> data, std::string_view meta);
	void unloadGame();
	void runFrame();

	bool hasGameLoaded() const;
	const Sprite& getVideoOut() const;
	const std::shared_ptr<AudioClipStreaming>& getAudioOut() const;

	const SystemInfo& getSystemInfo() const;
	const SystemAVInfo& getSystemAVInfo() const;

	void setInputDevice(int idx, std::shared_ptr<InputVirtual> input);

protected:
	bool onEnvironment(uint32_t cmd, void* data) override;
	void onVideoRefresh(const void* data, uint32_t width, uint32_t height, size_t pitch) override;
	void onAudioSample(int16_t left, int16_t right) override;
	size_t onAudioSampleBatch(const int16_t* data, size_t size) override;
	void onInputPoll() override;
	int16_t onInputState(uint32_t port, uint32_t device, uint32_t index, uint32_t id) override;
	void onLog(retro_log_level level, const char* str) override;

private:
	DLL dll;
	LibretroEnvironment& environment;

	bool gameLoaded = false;

	SystemInfo systemInfo;
	SystemAVInfo systemAVInfo;

	HashMap<String, Option> options;

	Sprite videoOut;
	std::unique_ptr<CPUUpdateTexture> cpuUpdateTexture;

	std::shared_ptr<AudioClipStreaming> audioOut;
	Vector<float> audioBuffer;
	retro_audio_buffer_status_callback_t audioBufferStatusCallback = nullptr;

	constexpr static int maxInputDevices = 8;
	std::array<uint16_t, maxInputDevices> inputJoypads;
	std::array<std::shared_ptr<InputVirtual>, maxInputDevices> inputDevices;

	LibretroCore(DLL dll, LibretroEnvironment& environment);

	void init();
	void deInit();

	void initVideoOut();
	void initAudioOut();

	void addAudioSamples(gsl::span<const float> samples);

	void onEnvSetPerformanceLevel(uint32_t level);
	bool onEnvSetPixelFormat(retro_pixel_format data);
	void onEnvSetGeometry(const retro_game_geometry& data);
	int onEnvGetAudioVideoEnable();
	void onEnvGetSystemDirectory(const char** data);
	void onEnvSetInputDescriptors(const retro_input_descriptor* data);
	void onEnvGetVariable(retro_variable& data);
	void onEnvSetVariables(const retro_variable* data);
	bool onEnvGetVariableUpdate();
	void onEnvSetSupportNoGame(bool data);
	void onEnvGetSaveDirectory(const char** data);
	void onEnvSetSubsystemInfo(const retro_subsystem_info& data);
	void onEnvSetControllerInfo(const retro_controller_info& data);
	uint32_t onEnvGetLanguage();
	void onEnvSetSupportAchievements(bool data);
	void onEnvSetCoreOptions(const retro_core_option_definition* data);
	void onEnvSetCoreOptionsIntl(const retro_core_options_intl& data);
	void onEnvSetCoreOptionsV2(const retro_core_options_v2& data);
	void onEnvSetCoreOptionsV2Intl(const retro_core_options_v2_intl& data);
	void onEnvSetCoreOptionsDisplay(const retro_core_option_display& data);

	void onEnvSetAudioBufferStatusCallback(const retro_audio_buffer_status_callback* data);
	void onEnvSetMinimumAudioLatency(uint32_t data);
};
