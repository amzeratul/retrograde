#include "game_stage.h"

GameStage::GameStage() = default;

GameStage::~GameStage()
{
}

void GameStage::init()
{
}

void GameStage::onVariableUpdate(Time t)
{
	if (input) {
		input->update(t);
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
		if (screen.hasMaterial()) {
			const auto spriteSize = screen.getSize();
			const auto windowSize = Vector2f(getVideoAPI().getWindow().getDefinition().getSize());
			const auto scales = Vector2i((windowSize / spriteSize).floor());
			const int scale = std::min(scales.x, scales.y);

			screen
				.clone()
				.setScale(static_cast<float>(scale))
				.setPivot(Vector2f(0.5f, 0.5f))
				.setPosition(windowSize * 0.5f)
				.draw(painter);
		}

		//perfView->paint(painter);
	});
}
