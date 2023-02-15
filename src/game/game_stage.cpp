#include "game_stage.h"
#include "retrograde_game.h"
#include "src/game/retrograde_environment.h"
#include "src/ui/choose_system_window.h"
#include "src/ui/game_canvas.h"

GameStage::GameStage() = default;

GameStage::~GameStage()
{
}

void GameStage::init()
{
	auto& game = static_cast<RetrogradeGame&>(getGame());
	String systemId;
	String gamePath;
	if (game.getArgs().size() >= 2) {
		systemId = game.getArgs()[0];
		gamePath = String::concatList(gsl::span<const String>(game.getArgs()).subspan(1), " ");
	}

	perfStats = std::make_shared<PerformanceStatsView>(getResources(), getAPI());
	perfStats->setActive(false);

	uiFactory = game.createUIFactory(getAPI(), getResources(), game.getI18N());
	uiRoot = std::make_unique<UIRoot>(getAPI(), Rect4f(getVideoAPI().getWindow().getWindowRect()));
	env = std::make_unique<RetrogradeEnvironment>(game, "..", getResources(), getAPI());

	//uiRoot->addChild(std::make_shared<GameCanvas>(*env, env->loadCore(systemId, gamePath)));
	uiRoot->addChild(std::make_shared<ChooseSystemWindow>(*uiFactory, *env));
}

void GameStage::onVariableUpdate(Time t)
{
	env->getConfigDatabase().update();

	if (getInputAPI().getKeyboard()->isButtonPressed(KeyCode::Esc)) {
		uiRoot.reset();
		getCoreAPI().quit();
		return;
	}

	uiRoot->setRect(Rect4f(Vector2f(), Vector2f(getVideoAPI().getWindow().getWindowRect().getSize())));
	uiRoot->update(t, UIInputType::Mouse, getInputAPI().getMouse(), {});

	if (getInputAPI().getKeyboard()->isButtonPressed(KeyCode::F11)) {
		perfStats->setActive(!perfStats->isActive());
	}
	perfStats->update(t);
}

void GameStage::onFixedUpdate(Time t)
{

}

void GameStage::onRender(RenderContext& rc) const
{
	uiRoot->render(rc);

	rc.bind([&] (Painter& painter)
	{
		painter.clear(Colour4f(0.0f, 0.0f, 0.0f));
		
		SpritePainter spritePainter;
		spritePainter.start();
		uiRoot->draw(spritePainter, 1, 0);
		spritePainter.draw(1, painter);

		perfStats->paint(painter);
	});
}
