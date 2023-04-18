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

	for (size_t i = 0; i < args.size(); ++i) {
		const auto& arg = args[i];

		if (arg.startsWith("--devcon=")) {
			devConAddress = arg.mid(9);
		}
	}
}

int RetrogradeGame::initPlugins(IPluginRegistry& registry)
{
	initOpenGLPlugin(registry);
	initSDLSystemPlugin(registry, {});
	//initSDLAudioPlugin(registry);
	initXAudio2Plugin(registry);
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
	constexpr bool localAssets = false;
	if (localAssets) {
		locator.addFileSystem(unpackedAssetsPath);
	} else {
		const String packs[] = { "images.dat", "shaders.dat", "config.dat" };
		for (auto& pack: packs) {
			locator.addPack(Path(assetsPath) / pack, "", true);
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
#if defined(DEV_BUILD) && !defined(RELEASE_MODE)
	return true;
#else
	return false;
#endif
}

std::unique_ptr<Stage> RetrogradeGame::startGame()
{
	const bool vsync = true;

	const auto screenSize = getAPI().system->getScreenSize(0);
	WindowGLVersion glVersion = { 4, 2 };
	windowDefinition = WindowDefinition(WindowType::ResizableWindow, Vector2i(1280, 960), getName(), true, 0, glVersion);
	fullscreenDefinition = WindowDefinition(WindowType::BorderlessWindow, screenSize, getName(), true, 0, glVersion);
	getAPI().video->setWindow(WindowDefinition(windowDefinition));
	getAPI().video->getWindow().setTitleColour(Colour4f(), Colour4f(1, 1, 1, 1));
	getAPI().video->setVsync(vsync);
	getAPI().audio->startPlayback();
	getAPI().audio->setListener(AudioListenerData(Vector3f()));

	i18n = std::make_unique<I18N>(getResources());

	return std::make_unique<GameStage>();
}

const Vector<String>& RetrogradeGame::getArgs() const
{
	return args;
}

double RetrogradeGame::getTargetFPS() const
{
	return 0;
	//return targetFps.value_or(0);
}

double RetrogradeGame::getFixedUpdateFPS() const
{
	return targetFps.value_or(60);
}

void RetrogradeGame::setTargetFPSOverride(std::optional<double> fps)
{
	targetFps = fps;
}

std::optional<double> RetrogradeGame::getTargetFPSOverride() const
{
	return targetFps;
}

I18N& RetrogradeGame::getI18N()
{
	return *i18n;
}

void RetrogradeGame::toggleFullscreen()
{
	auto& window = getAPI().video->getWindow();
	const bool isCurrentlyFullscreen = window.getDefinition().getWindowType() != WindowType::ResizableWindow;
	window.update(WindowDefinition(isCurrentlyFullscreen ? windowDefinition : fullscreenDefinition));
}

String RetrogradeGame::getDevConAddress() const
{
	return devConAddress;
}

HalleyGame(RetrogradeGame);
