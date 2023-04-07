#pragma once

#include <halley.hpp>

#include "libretro.h"
#include "src/util/c_string_cache.h"
#include "src/util/dll.h"
#include "src/util/dx11_state.h"

class CoreConfig;

namespace Halley {
	class DX11Texture;
}

class OpenGLInteropObject;
class LibretroVFS;
class CPUUpdateTexture;
class RetrogradeEnvironment;
class OpenGLInterop;
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
	virtual retro_proc_address_t onHWGetProcAddress(const char* sym) = 0;
	virtual bool onSetRumbleState(uint32_t port, retro_rumble_effect effect, uint16_t strength) = 0;

	virtual LibretroVFS& getVFS() = 0;

	static ILibretroCoreCallbacks* curInstance;
	static size_t curInstanceDepth;
};

class LibretroCore : protected ILibretroCoreCallbacks {
public:
	struct Option {
		struct Value {
			String value;
			String display;

			const String& toString() const;
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
		int rotation = 0;

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
		RewindRecording,
		RunaheadSameInstance,
		RunaheadSameBinary,
		RollbackNetplay
	};

	static std::unique_ptr<LibretroCore> load(const CoreConfig& coreConfig, std::string_view filename, String systemId, const RetrogradeEnvironment& environment);
	~LibretroCore() override;

	bool loadGame(const Path& path);
	void unloadGame();
	void resetGame();

	bool hasGameLoaded() const;
	const String& getGameName() const;
	const String& getSystemId() const;

	size_t getSaveStateSize(SaveStateType type) const;
	Bytes saveState(SaveStateType type) const;
	void saveState(SaveStateType type, gsl::span<gsl::byte> bytes) const;
	void loadState(gsl::span<const gsl::byte> bytes);
	void loadState(const Bytes& bytes);

	gsl::span<Byte> getMemory(MemoryType type);

	void setRewinding(bool rewind);
	void setFastFowarding(bool ffwd);
	void setPaused(bool paused);
	void runFrame();
	const Sprite& getVideoOut() const;

	const CoreConfig& getCoreConfig() const;
	const SystemInfo& getSystemInfo() const;
	const SystemAVInfo& getSystemAVInfo() const;
	bool isScreenRotated() const;

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
	retro_proc_address_t onHWGetProcAddress(const char* sym) override;
	bool onSetRumbleState(uint32_t port, retro_rumble_effect effect, uint16_t strength) override;

private:
	const CoreConfig& coreConfig;
	DLL dll;
	const RetrogradeEnvironment& environment;

	bool gameLoaded = false;
	bool optionsModified = false;
	bool renderCallbackNeedsReset = false;
	bool coreHandlesSaveData = false;
	bool hasAnalogStick = false;
	bool needsToSaveSRAM = false;
	bool rewinding = false;
	bool fastForwarding = false;
	bool paused = false;

	String gameName;
	String systemId;
	Bytes gameBytes;
	Vector<retro_game_info_ext> gameInfos;

	uint64_t lastSaveHash = 0;
	int framesSinceSRAMModified = 0;
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
	std::shared_ptr<SingleThreadExecutor> audioThread;
	AudioHandle audioStreamHandle;

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

	std::unique_ptr<OpenGLInterop> glInterop;
	std::shared_ptr<OpenGLInteropObject> glFramebuffer;
	std::shared_ptr<DX11Texture> dx11Framebuffer;
	std::shared_ptr<DX11State> dx11State;

	CStringCache stringCache;
	
	LibretroCore(DLL dll, const CoreConfig& coreConfig, String systemId, const RetrogradeEnvironment& environment);

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
	void onEnvGetPerfInterface(retro_perf_callback& data);

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

	std::shared_ptr<Texture> getDX11HWTexture(Vector2i size);
	std::shared_ptr<Texture> getOpenGLHWTexture();
	TextureFormat getTextureFormat(retro_pixel_format retroFormat) const;

	void pushInstance() const;
	void popInstance() const;
};
