#include "retrograde_game.h"
#include "game_stage.h"

void initOpenGLPlugin(IPluginRegistry &registry);
void initSDLSystemPlugin(IPluginRegistry &registry, std::optional<String> cryptKey);
void initSDLAudioPlugin(IPluginRegistry &registry);
void initSDLInputPlugin(IPluginRegistry &registry);
void initDX11Plugin(IPluginRegistry &registry);
void initAsioPlugin(IPluginRegistry &registry);
void initXAudio2Plugin(IPluginRegistry &registry);

void RetrogradeGame::init(const Environment& env, const Vector<String>& args)
{
	this->args = args;
}

int RetrogradeGame::initPlugins(IPluginRegistry& registry)
{
	initOpenGLPlugin(registry);
	initSDLSystemPlugin(registry, {});
	initSDLAudioPlugin(registry);
	//initXAudio2Plugin(registry);
	initSDLInputPlugin(registry);

#ifdef WITH_DX11
	initDX11Plugin(registry);
#endif
#ifdef WITH_ASIO
	initAsioPlugin(registry);
#endif

	return HalleyAPIFlags::Video | HalleyAPIFlags::Audio | HalleyAPIFlags::Input | HalleyAPIFlags::Network | HalleyAPIFlags::Platform;
}

ResourceOptions RetrogradeGame::initResourceLocator(const Path& gamePath, const Path& assetsPath, const Path& unpackedAssetsPath, ResourceLocator& locator) {
	constexpr bool localAssets = true;
	if (localAssets) {
		locator.addFileSystem(unpackedAssetsPath);
	} else {
		const String packs[] = { "images.dat", "shaders.dat", "config.dat", "music.dat", "sfx.dat" };
		for (auto& pack: packs) {
			locator.addPack(Path(assetsPath) / pack);
		}
	}
	return {};
}

String RetrogradeGame::getName() const
{
	return "Retrograde";
}

String RetrogradeGame::getDataPath() const
{
	return "Retrograde";
}

bool RetrogradeGame::isDevMode() const
{
	return true;
}

std::unique_ptr<Stage> RetrogradeGame::startGame()
{
	bool vsync = true;

	getAPI().video->setWindow(WindowDefinition(WindowType::ResizableWindow, Vector2i(1280, 720), getName()));
	getAPI().video->setVsync(vsync);
	getAPI().audio->startPlayback();
	getAPI().audio->setListener(AudioListenerData(Vector3f()));
	return std::make_unique<GameStage>();
}

const Vector<String>& RetrogradeGame::getArgs() const
{
	return args;
}

int RetrogradeGame::getTargetBackgroundFPS() const
{
	return 0;
}

int RetrogradeGame::getTargetFPS() const
{
	return 0;
}

HalleyGame(RetrogradeGame);
