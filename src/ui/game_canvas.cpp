#include "game_canvas.h"

#include "in_game_menu.h"
#include "src/game/retrograde_environment.h"
#include "src/game/retrograde_game.h"
#include "src/libretro/libretro_core.h"
#include "src/savestate/rewind_data.h"

GameCanvas::GameCanvas(UIFactory& factory, RetrogradeEnvironment& environment, String systemId, String gameId, UIWidget& parentMenu)
	: UIWidget("game_canvas")
	, factory(factory)
	, environment(environment)
	, systemId(std::move(systemId))
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
	core.reset();
	environment.getGame().setTargetFPSOverride(std::nullopt);
}

void GameCanvas::update(Time t, bool moved)
{
	if (!loaded) {
		core = environment.loadCore(systemId, gameId);
		loaded = true;
	}

	if (pendingCloseState == 2) {
		doClose();
	}

	auto rect = getRoot()->getRect();
	setPosition(rect.getP1());
	setMinSize(rect.getSize());
	layout();
	
	if (t > 0.00001 && pendingCloseState == 0) {
		stepGame();
	}
}

void GameCanvas::draw(UIPainter& uiPainter) const
{
	uiPainter.draw([=] (Painter& painter)
	{
		paint(painter);
	});
}

void GameCanvas::close()
{
	// All this pending close state madness is to ensure that a painter.resetState() is called after the last stepGame()
	pendingCloseState = 1;
	core.reset();
}

void GameCanvas::doClose()
{
	parentMenu.setActive(true);
	parentMenu.layout();
	destroy();
}

void GameCanvas::paint(Painter& painter) const
{
	if (pendingCloseState == 1) {
		pendingCloseState = 2;
	}

	painter.resetState();
	if (pendingCloseState == 0 && core && core->hasGameLoaded()) {
		drawScreen(painter, core->getVideoOut());
	}
}

void GameCanvas::drawScreen(Painter& painter, Sprite screen) const
{
	if (screen.hasMaterial()) {
		const auto spriteSize = screen.getScaledSize().rotate(screen.getRotation()).abs();
		const auto windowSize = getSize();
		const auto scales = windowSize / spriteSize;
		const auto scale = std::min(scales.x, scales.y) * screen.getScale();

		screen
			.setScale(scale)
			.setPivot(Vector2f(0.5f, 0.5f))
			.setPosition(windowSize * 0.5f)
			.draw(painter);
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
			auto save = rewindData->getBuffer(core->getSaveStateSize(LibretroCore::SaveStateType::RewindRecording));
			core->saveState(LibretroCore::SaveStateType::RewindRecording, gsl::as_writable_bytes(gsl::span<Byte>(save)));
			rewindData->pushFrame(std::move(save));
		}
	}

	environment.getGame().setTargetFPSOverride(core->getSystemAVInfo().fps);
}

void GameCanvas::onGamepadInput(const UIInputResults& input, Time time)
{
	if (input.isButtonPressed(UIGamepadInput::Button::Cancel)) {
		getRoot()->addChild(std::make_shared<InGameMenu>(factory, environment, *this));
	}
}
