#include "game_canvas.h"

#include "system_bezel.h"
#include "src/ui/in_game_menu.h"
#include "src/config/screen_filter_config.h"
#include "src/config/bezel_config.h"
#include "src/config/core_config.h"
#include "src/config/system_config.h"
#include "src/filter_chain/filter_chain.h"
#include "src/retrograde/retrograde_environment.h"
#include "src/retrograde/retrograde_game.h"
#include "src/libretro/libretro_core.h"
#include "src/savestate/rewind_data.h"

GameCanvas::GameCanvas(UIFactory& factory, RetrogradeEnvironment& environment, const CoreConfig& coreConfig, const SystemConfig& systemConfig, String gameId, UIWidget& parentMenu)
	: UIWidget("game_canvas")
	, factory(factory)
	, environment(environment)
	, coreConfig(coreConfig)
	, systemConfig(systemConfig)
	, gameId(std::move(gameId))
	, parentMenu(parentMenu)
{
	rewindData = std::make_unique<RewindData>(16 * 1024 * 1024);
	setModal(false);

	UIInputButtons buttons;
	buttons.cancel = 12;
	setInputButtons(buttons);
}

GameCanvas::~GameCanvas()
{
	screen = {};
	core.reset();
	environment.getGame().setTargetFPSOverride(std::nullopt);
}

void GameCanvas::onAddedToRoot(UIRoot& root)
{
	fitToRoot();
}

void GameCanvas::update(Time t, bool moved)
{
	bool needToLoadGame = false;
	if (!loaded) {
		core = environment.loadCore(coreConfig, systemConfig);
		needToLoadGame = true;
		loaded = true;
	}

	if (pendingCloseState == 2) {
		doClose();
	}

	fitToRoot();

	updateBezels();

	if (needToLoadGame && !gameId.isEmpty()) {
		core->loadGame(environment.getRomsDir(systemConfig.getId()) / gameId);
	}

	if (t > 0.00001 && pendingCloseState == 0) {
		stepGame();
	}
	
	const auto windowSize = environment.getHalleyAPI().video->getWindow().getWindowRect().getSize();
	updateFilterChain(windowSize);
}

void GameCanvas::render(RenderContext& rc) const
{
	if (pendingCloseState == 1) {
		pendingCloseState = 2;
	}

	rc.bind([&] (Painter& painter)
	{
		painter.resetState();
	});

	if (core) {
		const auto canvasRect = Rect4i(getRect());
		const auto windowRect = environment.getHalleyAPI().video->getWindow().getWindowRect();
		const float zoom = static_cast<float>(windowRect.getHeight()) / static_cast<float>(canvasRect.getHeight());

		const auto coreOutScreen = core->getVideoOut();
		if (coreOutScreen.hasMaterial() && coreOutScreen.getSize().x >= 32 && coreOutScreen.getSize().y >= 32) {
			const auto origRotatedSpriteSize = coreOutScreen.getScaledSize().rotate(coreOutScreen.getRotation()).abs();
			const auto scales = Vector2f(canvasRect.getSize()) / origRotatedSpriteSize;
			auto scale = std::min(scales.x, scales.y) * coreOutScreen.getScale(); // The scale the original sprite would need to fit the view port

			if (bezel) {
				scale = bezel->update(canvasRect, scale);
			}

			const auto spriteSize = (coreOutScreen.getSize() * scale * 0.5f).round() * 2.0f; // Absolute sprite size

			if (filterChain) {
				screen = filterChain->run(coreOutScreen, rc, Vector2i((spriteSize * zoom).round()));
			} else {
				screen = coreOutScreen;
			}

			screen
				.scaleTo(spriteSize)
				.setRotation(coreOutScreen.getRotation() + Angle1f::fromDegrees(coreOutScreen.isFlipped() ? -180.0f : 0.0f))
				.setPivot(Vector2f(0.5f, 0.5f))
				.setPosition(Vector2f(canvasRect.getCenter()));
		} else {
			screen = {};
		}
	} else {
		screen = {};
	}
}

void GameCanvas::draw(UIPainter& uiPainter) const
{
	uiPainter.draw([=] (Painter& painter)
	{
		paint(painter);
	});
}

void GameCanvas::paint(Painter& painter) const
{
	if (bezel) {
		bezel->draw(painter, BezelLayer::Background);
	}

	if (pendingCloseState == 0 && screen.hasMaterial()) {
		screen.draw(painter);
	}

	if (bezel) {
		bezel->draw(painter, BezelLayer::Foreground);
	}
}

void GameCanvas::stepGame()
{
	const bool running = core && core->hasGameLoaded();
	if (!running) {
		return;
	}

	const bool needsPause = getRoot()->hasModalUI();
	const bool paused = needsPause || pauseFrames > 0;
	if (pauseFrames > 0) {
		--pauseFrames;
	}
	core->setPaused(paused);
	if (paused) {
		if (needsPause) {
			pauseFrames = 3;
		}
		return;
	}

	auto& inputAPI = *environment.getHalleyAPI().input;

	if (inputAPI.getKeyboard()->isButtonPressed(KeyCode::F2)) {
		Path::writeFile(Path(environment.getSaveDir(core->getSystemId())) / (core->getGameName() + ".state0"), core->saveState(LibretroCore::SaveStateType::Normal));
	}

	if (inputAPI.getKeyboard()->isButtonPressed(KeyCode::F4)) {
		const auto saveState = Path::readFile(Path(environment.getSaveDir(core->getSystemId())) / (core->getGameName() + ".state0"));
		if (!saveState.empty()) {
			core->loadState(saveState);
		}
	}

	const bool rewind = inputAPI.getKeyboard()->isButtonDown(KeyCode::F6);
	const bool ffwd = !rewind && inputAPI.getKeyboard()->isButtonDown(KeyCode::F7);

	core->setRewinding(rewind);
	if (rewind) {
		const auto bytes = rewindData->popFrame();
		if (bytes) {
			core->setFastFowarding(false);
			core->loadState(*bytes);
			core->runFrame();
		}
	} else {
		const int n = ffwd ? 8 : 1;
		for (int i = 0; i < n; ++i) {
			core->setFastFowarding(i < n - 1);
			core->runFrame();
			//auto save = rewindData->getBuffer(core->getSaveStateSize(LibretroCore::SaveStateType::RewindRecording));
			//core->saveState(LibretroCore::SaveStateType::RewindRecording, gsl::as_writable_bytes(gsl::span<Byte>(save)));
			//rewindData->pushFrame(std::move(save));
		}
	}

	environment.getGame().setTargetFPSOverride(core->getSystemAVInfo().fps);
}

void GameCanvas::close()
{
	// All this pending close state madness is to ensure that a painter.resetState() is called after the last stepGame()
	pendingCloseState = 1;
	screen = {};
	core.reset();
}

void GameCanvas::doClose()
{
	parentMenu.setActive(true);
	parentMenu.layout();
	destroy();
}

void GameCanvas::onGamepadInput(const UIInputResults& input, Time time)
{
	if (input.isButtonPressed(UIGamepadInput::Button::Cancel)) {
		getRoot()->addChild(std::make_shared<InGameMenu>(factory, environment, *this));
	}
}

void GameCanvas::updateBezels()
{
	if (!bezel) {
		bezel = std::make_unique<SystemBezel>(environment);
	}

	const auto& bezels = systemConfig.getBezels();
	if (bezels.empty()) {
		bezel->setBezel(nullptr);
	} else {
		const auto& bezelConfig = environment.getConfigDatabase().get<BezelConfig>(bezels.front());
		if (core) {
			for (const auto& [k, v]: bezelConfig.getCoreOptions(core->getCoreConfig().getId())) {
				core->setOption(k, v);
			}
		}
		bezel->setBezel(&bezelConfig);
	}
}

void GameCanvas::updateFilterChain(Vector2i screenSize)
{
	if (!core) {
		return;
	}

	const auto& filters = systemConfig.getScreenFilters();
	if (filters.empty()) {
		filterChain = {};
		return;
	}

	if (core->isScreenRotated()) {
		std::swap(screenSize.x, screenSize.y);
	}

	const auto& screenFilterConfig = environment.getConfigDatabase().get<ScreenFilterConfig>(filters.front());
	const auto& shader = screenFilterConfig.getShaderFor(screenSize);
	if (shader.isEmpty()) {
		filterChain = {};
		return;
	}

	if (!filterChain || filterChain->getId() != shader) {
		filterChain = environment.makeFilterChain(shader);
	}
}
