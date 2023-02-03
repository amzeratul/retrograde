#include "game_stage.h"

#include "retrograde_game.h"
#include "src/libretro/libretro_core.h"
#include "src/libretro/libretro_environment.h"

GameStage::GameStage() = default;

GameStage::~GameStage()
{
}

void GameStage::init()
{
	auto& game = static_cast<RetrogradeGame&>(getGame());
	String corePath;
	String gamePath;
	if (game.getArgs().size() >= 2) {
		corePath = game.getArgs()[0];
		gamePath = String::concatList(gsl::span<const String>(game.getArgs()).subspan(1), " ");
	}

	libretroEnvironment = std::make_unique<LibretroEnvironment>("..", getResources(), getAPI());
	
	libretroCore = LibretroCore::load(libretroEnvironment->getCoresDir() + "/" + corePath, *libretroEnvironment);
	if (libretroCore) {
		const bool ok = libretroCore->loadGame(libretroEnvironment->getRomsDir() + "/" + gamePath);
		if (!ok) {
			Logger::logError("Failed to load game " + String(gamePath));
		}
	} else {
		Logger::logError("Failed to load core " + String(corePath));
	}
}

void GameStage::onVariableUpdate(Time t)
{
	if (input) {
		input->update(t);
	}

	if (libretroCore && libretroCore->hasGameLoaded()) {
		libretroCore->runFrame();
	}
}

void GameStage::onFixedUpdate(Time t)
{
}

void GameStage::onRender(RenderContext& rc) const
{
	rc.bind([&] (Painter& painter)
	{
		painter.clear(Colour4f(0.0f, 0.0f, 0.0f));

		if (libretroCore && libretroCore->hasGameLoaded()) {
			drawScreen(painter, libretroCore->getVideoOut());
		}
	});
}

void GameStage::drawScreen(Painter& painter, Sprite screen) const
{
	if (screen.hasMaterial()) {
		const auto spriteSize = screen.getScaledSize();
		const auto windowSize = Vector2f(getVideoAPI().getWindow().getDefinition().getSize());
		const auto scales = windowSize / spriteSize;
		const auto scale = std::min(scales.x, scales.y);

		screen
			.setScale(scale)
			.setPivot(Vector2f(0.5f, 0.5f))
			.setPosition(windowSize * 0.5f)
			.draw(painter);
	}
}
