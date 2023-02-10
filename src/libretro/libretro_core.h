#pragma once

#include <halley.hpp>

#include "libretro.h"
#include "src/util/dll.h"
class ZipFile;
class LibretroVFS;
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
	virtual void onSetLEDState(int led, int state) = 0;
	virtual uintptr_t onHWGetCurrentFrameBuffer() = 0;
	virtual bool onSetRumbleState(uint32_t port, retro_rumble_effect effect, uint16_t strength) = 0;

	virtual LibretroVFS& getVFS() = 0;

	static thread_local ILibretroCoreCallbacks* curInstance;
};

class LibretroCore : protected ILibretroCoreCallbacks {
public:
	struct Option {
		struct Value {
			String value;
			String display;
		};
		String value;
		String defaultValue;
		String description;
		String info;
		String category;
		Vector<Value> values;
		bool visible = true;
	};

	struct SystemInfo {
		String coreName;
		String coreVersion;
		bool supportNoGame = false;
		bool supportAchievements = false;
		bool blockExtract = false;
	};

	struct ContentInfo {
		Vector<String> validExtensions;
		bool needFullpath = false;
		bool persistData = false;

		bool isValidExtension(std::string_view filePath) const;
		bool isValidExtension(const Path& filePath) const;
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

	enum class MemoryType {
		SaveRAM,
		RTC,
		SystemRAM,
		VideoRAM
	};

	enum class SaveStateType {
		Normal,
		RunaheadSameInstance,
		RunaheadSameBinary,
		RollbackNetplay
	};

	static std::unique_ptr<LibretroCore> load(std::string_view filename, LibretroEnvironment& environment);
	~LibretroCore() override;

	bool loadGame(const Path& path);
	void unloadGame();
	bool hasGameLoaded() const;

	Bytes saveState(SaveStateType type) const;
	void loadState(SaveStateType type, gsl::span<const gsl::byte> bytes);
	void loadState(SaveStateType type, const Bytes& bytes);

	gsl::span<Byte> getMemory(MemoryType type);

	void runFrame();
	const Sprite& getVideoOut() const;
	const std::shared_ptr<AudioClipStreaming>& getAudioOut() const;

	const SystemInfo& getSystemInfo() const;
	const SystemAVInfo& getSystemAVInfo() const;

	LibretroVFS& getVFS() override;

	void setInputDevice(int idx, std::shared_ptr<InputVirtual> input);

	const HashMap<String, Option>& getOptions() const;
	void setOption(const String& key, const String& value);

protected:
	bool onEnvironment(uint32_t cmd, void* data) override;
	void onVideoRefresh(const void* data, uint32_t width, uint32_t height, size_t pitch) override;
	void onAudioSample(int16_t left, int16_t right) override;
	size_t onAudioSampleBatch(const int16_t* data, size_t size) override;
	void onInputPoll() override;
	int16_t onInputState(uint32_t port, uint32_t device, uint32_t index, uint32_t id) override;
	void onSetLEDState(int led, int state) override;
	void onLog(retro_log_level level, const char* str) override;
	uintptr_t onHWGetCurrentFrameBuffer() override;
	bool onSetRumbleState(uint32_t port, retro_rumble_effect effect, uint16_t strength) override;

private:
	DLL dll;
	LibretroEnvironment& environment;

	bool gameLoaded = false;
	bool optionsModified = false;
	bool renderCallbackNeedsReset = false;
	bool coreHandlesSaveData = false;
	bool hasAnalogStick = false;

	String gameName;
	Bytes gameBytes;
	Vector<retro_game_info_ext> gameInfos;

	uint64_t lastSaveHash = 0;
	mutable SaveStateType saveStateType = SaveStateType::Normal;

	SystemInfo systemInfo;
	SystemAVInfo systemAVInfo;
	Vector<ContentInfo> contentInfos;

	HashMap<String, Option> options;

	Sprite videoOut;
	std::unique_ptr<CPUUpdateTexture> cpuUpdateTexture;

	std::shared_ptr<AudioClipStreaming> audioOut;
	Vector<float> audioBuffer;
	retro_audio_buffer_status_callback_t audioBufferStatusCallback = nullptr;

	constexpr static int maxInputDevices = 8;
	struct Input {
		int16_t buttonMask = 0;
		std::array<Vector2f, 2> sticks;
		std::array<float, 16> analogButtons;
		std::shared_ptr<InputVirtual> device;
	};
	std::array<Input, maxInputDevices> inputs;

	std::unique_ptr<LibretroVFS> vfs;
	std::optional<retro_disk_control_ext_callback> diskControlCallbacks;

	std::shared_ptr<void> hwRenderInterface;
	std::optional<retro_hw_render_callback> hwRenderCallback;
	
	LibretroCore(DLL dll, LibretroEnvironment& environment);

	void init();
	void deInit();

	void initVideoOut();
	void initAudioOut();
	
	std::pair<const ContentInfo*, size_t> getContentInfo(const ZipFile& zip);
	const ContentInfo* getContentInfo(const Path& path);
	bool doLoadGame();

	void loadVFS();

	void addAudioSamples(gsl::span<const float> samples);

	void saveGameDataIfNeeded();
	void saveGameData(gsl::span<Byte> data);
	void loadGameData();
	String getSaveFileName() const;

	[[nodiscard]] uint32_t onEnvGetLanguage();
	void onEnvSetSupportNoGame(bool data);
	int onEnvGetAudioVideoEnable();
	void onEnvSetPerformanceLevel(uint32_t level);
	void onEnvSetSubsystemInfo(const retro_subsystem_info& data);
	void onEnvSetMessageExt(const retro_message_ext& data);

	bool onEnvSetPixelFormat(retro_pixel_format data);
	void onEnvSetGeometry(const retro_game_geometry& data);
	void onEnvSetRotation(uint32_t data);

	bool onEnvSetHWRender(retro_hw_render_callback& data);
	uint32_t onEnvGetPreferredHWRender();
	bool onEnvGetHWRenderInterface(const retro_hw_render_interface** data);
	const retro_hw_render_interface* getD3DHWRenderInterface();

	void onEnvGetSaveDirectory(const char** data);
	void onEnvGetSystemDirectory(const char** data);
	void onEnvSetSerializationQuirks(uint64_t& data);
	bool onEnvGetVFSInterface(retro_vfs_interface_info& data);
	void onEnvSetDiskControlInterface(const retro_disk_control_callback& data);
	void onEnvSetDiskControlExtInterface(const retro_disk_control_ext_callback& data);
	void onEnvSetContentInfoOverride(const retro_system_content_info_override* data);

	void onEnvSetInputDescriptors(const retro_input_descriptor* data);
	void onEnvSetControllerInfo(const retro_controller_info* data);
	void onEnvGetRumbleInterface(retro_rumble_interface& data);

	void onEnvSetSupportAchievements(bool data);
	[[nodiscard]] retro_savestate_context onEnvGetSavestateContext();

	void onEnvGetVariable(retro_variable& data);
	void onEnvSetVariables(const retro_variable* data);
	bool onEnvGetVariableUpdate();
	void onEnvSetCoreOptions(const retro_core_option_definition* data);
	void onEnvSetCoreOptionsIntl(const retro_core_options_intl& data);
	void onEnvSetCoreOptionsV2(const retro_core_options_v2& data);
	void onEnvSetCoreOptionsV2Intl(const retro_core_options_v2_intl& data);
	void onEnvSetCoreOptionsDisplay(const retro_core_option_display& data);

	void onEnvSetAudioBufferStatusCallback(const retro_audio_buffer_status_callback* data);
	void onEnvSetMinimumAudioLatency(uint32_t data);

	void dx11UpdateTextureToCurrentBound(Vector2i size, size_t pitch);
	TextureFormat getTextureFormat(retro_pixel_format retroFormat) const;
};
