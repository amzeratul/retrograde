#include "game_canvas.h"

#include "src/game/retrograde_environment.h"
#include "src/game/retrograde_game.h"
#include "src/libretro/libretro_core.h"
#include "src/savestate/rewind_data.h"

GameCanvas::GameCanvas(RetrogradeEnvironment& environment, std::unique_ptr<LibretroCore> core)
	: UIWidget("game_canvas")
	, environment(environment)
	, core(std::move(core))
{
	rewindData = std::make_unique<RewindData>(16 * 1024 * 1024);
}

GameCanvas::~GameCanvas()
{
	core.reset();
}

void GameCanvas::update(Time t, bool moved)
{
	auto rect = getRoot()->getRect();
	setPosition(rect.getP1());
	setMinSize(rect.getSize());
	layout();

	if (t > 0.00001) {
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

void GameCanvas::paint(Painter& painter) const
{
	if (core && core->hasGameLoaded()) {
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
	if (core && core->hasGameLoaded()) {
		auto& inputAPI = *environment.getHalleyAPI().input;

		if (inputAPI.getKeyboard()->isButtonPressed(KeyCode::F2)) {
			Path::writeFile(Path(environment.getSaveDir()) / (core->getGameName() + ".state0"), core->saveState(LibretroCore::SaveStateType::Normal));
		}

		if (inputAPI.getKeyboard()->isButtonPressed(KeyCode::F4)) {
			const auto saveState = Path::readFile(Path(environment.getSaveDir()) / (core->getGameName() + ".state0"));
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

		environment.getGame().setTargetFPS(core->getSystemAVInfo().fps);
	} else {
		environment.getGame().setTargetFPS(std::nullopt);
	}
}
