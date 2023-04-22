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
#include "src/retrograde/input_mapper.h"
#include "src/savestate/rewind_data.h"
#include "src/savestate/savestate.h"
#include "src/savestate/savestate_collection.h"

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
	saveStateCollection = std::make_unique<SaveStateCollection>(environment.getSaveDir(systemConfig.getId()), this->gameId);
	gameInputMapper = environment.getInputMapper().makeGameInputMapper(systemConfig);

	setModal(false);

	UIInputButtons buttons;
	buttons.cancel = InputMapper::UIButtons::UI_BUTTON_SYSTEM;
	setInputButtons(buttons);

	loadCore();
}

GameCanvas::~GameCanvas()
{
	screen = {};
	saveStateCollection.reset();
	core.reset();
	environment.getGame().setTargetFPSOverride(std::nullopt);
}

void GameCanvas::onAddedToRoot(UIRoot& root)
{
	fitToRoot();

	getRoot()->addChild(std::make_shared<InGameMenu>(factory, environment, *this, InGameMenu::Mode::PreStart, getGameMetadata()));
}

void GameCanvas::startGame(std::optional<std::pair<SaveStateType, size_t>> loadState)
{
	gameInputMapper->chooseBestAssignments();
	gameInputMapper->bindCore(*core);
	saveStateCollection->setCore(*core);

	if (!gameLoaded) {
		core->loadGame(environment.getRomsDir(systemConfig.getId()) / gameId);
		gameLoaded = true;
	}

	if (loadState) {
		const auto result = saveStateCollection->loadGameState(loadState->first, loadState->second);
		if (result == SaveStateCollection::LoadResult::Failed) {
			pendingLoadState = loadState;
		}
	}
}

void GameCanvas::loadCore()
{
	if (!coreLoadRequested) {
		coreLoadRequested = true;
		auto windowSize = getWindowSize();

		std::array<Future<void>, 2> futures;
		futures[0] = Concurrent::execute(Executors::getCPU(), [=] ()
		{
			core = environment.loadCore(coreConfig, systemConfig);
		});
		futures[1] = Concurrent::execute(Executors::getCPU(), [=]()
		{
			updateFilterChain(windowSize);
		});

		coreLoadingFuture = Concurrent::whenAll(futures.begin(), futures.end());
	}
}

void GameCanvas::update(Time t, bool moved)
{
	fitToRoot();
	
	if (pendingCloseState == 2) {
		doClose();
	}

	updateBezels();

	if (t > 0.00001 && pendingCloseState == 0) {
		stepGame();
	}

	if (isCoreLoaded()) {
		auto windowSize = getWindowSize();
		if (core->isScreenRotated()) {
			std::swap(windowSize.x, windowSize.y);
		}
		updateFilterChain(windowSize);
	}

	updateAutoSave(t);
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

	const bool hasCore = coreLoadRequested && coreLoadingFuture.isReady();
	if (hasCore) {
		const auto canvasRect = Rect4i(getRect());
		const auto windowRect = environment.getHalleyAPI().video->getWindow().getWindowRect();
		const float zoom = static_cast<float>(windowRect.getHeight()) / static_cast<float>(canvasRect.getHeight());

		const auto coreOutScreen = core->getVideoOut();
		if (coreOutScreen.hasMaterial() && coreOutScreen.getSize().x >= 32 && coreOutScreen.getSize().y >= 32) {
			const auto origRotatedSpriteSize = coreOutScreen.getScaledSize().rotate(coreOutScreen.getRotation()).abs();
			const auto scales = Vector2f(canvasRect.getSize()) / origRotatedSpriteSize;
			auto scale = std::min(scales.x, scales.y) * coreOutScreen.getScale(); // The scale the original sprite would need to fit the view port

			if (bezel) {
				scale = bezel->update(windowRect, canvasRect, scale, zoom);
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
	const bool running = coreLoadRequested && coreLoadingFuture.isReady() && core->hasGameLoaded();
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

	if (pendingLoadState) {
		const auto result = saveStateCollection->loadGameState(pendingLoadState->first, pendingLoadState->second);
		pendingLoadStateAttempts++;
		if (result == SaveStateCollection::LoadResult::Success || pendingLoadStateAttempts > 10) {
			pendingLoadState = {};
			if (result != SaveStateCollection::LoadResult::Success) {
				Logger::logError("Failed to resume from save state");
			}
		}
	} else {
		if (inputAPI.getKeyboard()->isButtonPressed(KeyCode::F2)) {
			saveStateCollection->saveGameState(SaveStateType::QuickSave);
		}
		if (inputAPI.getKeyboard()->isButtonPressed(KeyCode::F4)) {
			saveStateCollection->loadGameState(SaveStateType::QuickSave, 0);
		}	
	}

	const bool canRewind = systemConfig.hasCapability(SystemCapability::Rewind);
	const bool rewind = canRewind && inputAPI.getKeyboard()->isButtonDown(KeyCode::F6);
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
		// The idea here is to try to do up to 32 frames, but stop short if we're going to exceed 12 ms
		const int n = ffwd ? 32 : 1;
		auto frameStartTime = std::chrono::steady_clock::now();
		Time totalFrameTime = 0.0;
		Time lastFrameTime = 0.0;
		const Time maxCPUTime = 0.012; // 12 ms

		for (int i = 0; i < n; ++i) {
			const bool lastFrame = i == n - 1 || totalFrameTime + 2 * lastFrameTime > maxCPUTime;
			core->setFastFowarding(!lastFrame);
			core->runFrame();

			if (canRewind) {
				auto save = rewindData->getBuffer(core->getSaveStateSize(LibretroCore::SaveStateType::RewindRecording));
				const bool ok = core->saveState(LibretroCore::SaveStateType::RewindRecording, gsl::as_writable_bytes(gsl::span<Byte>(save)));
				if (ok) {
					rewindData->pushFrame(std::move(save));
				}
			}

			if (ffwd) {
				if (lastFrame) {
					break;
				}
				const auto now = std::chrono::steady_clock::now();
				lastFrameTime = std::chrono::duration<double>(now - frameStartTime).count();
				frameStartTime = now;
				totalFrameTime += lastFrameTime;
			}
		}
	}

	environment.getGame().setTargetFPSOverride(core->getSystemAVInfo().fps);
}

void GameCanvas::close()
{
	if (gameLoaded) {
		saveStateCollection->saveGameState(SaveStateType::Suspend);
	}

	// All this pending close state madness is to ensure that a painter.resetState() is called after the last stepGame()
	pendingCloseState = 1;
	screen = {};
	core.reset();
}

void GameCanvas::resetGame()
{
	core->resetGame();
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
		openMenu();
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

Vector2i GameCanvas::getWindowSize() const
{
	return environment.getHalleyAPI().video->getWindow().getWindowRect().getSize();
}

void GameCanvas::updateFilterChain(Vector2i screenSize)
{
	const auto& filters = systemConfig.getScreenFilters();
	if (filters.empty()) {
		filterChain = {};
		return;
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

void GameCanvas::updateAutoSave(Time t)
{
	if (gameLoaded) {
		autoSaveTime += t;
		if (autoSaveTime > 30.0) {
			saveStateCollection->saveGameState(SaveStateType::Suspend);
			autoSaveTime = 0;
		}
	}
}

void GameCanvas::openMenu()
{
	if (!menu || !menu->isAlive()) {
		menu = std::make_shared<InGameMenu>(factory, environment, *this, InGameMenu::Mode::InGame, getGameMetadata());
		getRoot()->addChild(menu);
	} else {
		menu->setActive(true);
	}
}

const GameCollection::Entry* GameCanvas::getGameMetadata()
{
	const auto& collection = environment.getGameCollection(systemConfig.getId());
	return collection.findEntry(gameId);
}

SaveStateCollection& GameCanvas::getSaveStateCollection()
{
	return *saveStateCollection;
}

GameInputMapper& GameCanvas::getGameInputMapper()
{
	return *gameInputMapper;
}

void GameCanvas::waitForCoreLoad()
{
	assert(coreLoadRequested);
	coreLoadingFuture.wait();
}

bool GameCanvas::isCoreLoaded() const
{
	return coreLoadRequested && coreLoadingFuture.isReady();
}

LibretroCore& GameCanvas::getCore()
{
	waitForCoreLoad();
	return *core;
}
