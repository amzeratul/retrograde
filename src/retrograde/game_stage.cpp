#include "game_stage.h"
#include "retrograde_game.h"
#include "src/retrograde/retrograde_environment.h"
#include "src/ui/choose_system_window.h"

GameStage::GameStage() = default;

GameStage::~GameStage()
{
}

void GameStage::init()
{
	perfStats = std::make_shared<PerformanceStatsView>(getResources(), getAPI());
	perfStats->setActive(false);

	auto& game = dynamic_cast<RetrogradeGame&>(getGame());

	std::optional<String> systemId;
	std::optional<String> gamePath;
	for (const auto& arg: game.getArgs()) {
		if (arg.startsWith("--")) {
			continue;
		}

		if (!systemId) {
			systemId = arg;
		} else if (!gamePath) {
			gamePath = arg;
		} else {
			*gamePath += " " + arg;
		}
	}

	uiFactory = game.createUIFactory(getAPI(), getResources(), game.getI18N());
	UIInputButtons buttons;
	buttons.accept = 0;
	buttons.cancel = 1;
	buttons.hold = 2;
	buttons.prev = 6;
	buttons.next = 7;
	buttons.secondary = 2;
	buttons.tertiary = 3;
	buttons.xAxis = 0;
	buttons.yAxis = 1;
	uiFactory->setInputButtons("list", buttons);

	uiRoot = std::make_unique<UIRoot>(getAPI(), Rect4f(getVideoAPI().getWindow().getWindowRect()));
	env = std::make_unique<RetrogradeEnvironment>(game, getCoreAPI().getEnvironment().getProgramPath() / "..", getResources(), getAPI());
	env->setProfileId("default");

	uiRoot->addChild(std::make_shared<ChooseSystemWindow>(*uiFactory, *env, systemId, gamePath));
}

void GameStage::onVariableUpdate(Time t)
{
	onUpdate(t);
	/*
	const auto& game = dynamic_cast<RetrogradeGame&>(getGame());
	if (!game.getTargetFPSOverride()) {
		onUpdate(t);
	}
	*/
}

void GameStage::onFixedUpdate(Time t)
{
	/*
	const auto& game = dynamic_cast<RetrogradeGame&>(getGame());
	if (game.getTargetFPSOverride()) {
		onUpdate(t);
	}
	*/
}

void GameStage::onUpdate(Time t)
{
	Executor(Executors::getMainUpdateThread()).runPending();

	env->getConfigDatabase().update();

	auto kb = getInputAPI().getKeyboard();
	if ((kb->isButtonDown(KeyCode::LCtrl) || kb->isButtonDown(KeyCode::RCtrl)) && kb->isButtonPressed(KeyCode::Enter)) {
		dynamic_cast<RetrogradeGame&>(getGame()).toggleFullscreen();
	}

	const auto windowSize = Vector2f(getVideoAPI().getWindow().getWindowRect().getSize());
	const auto windowAR = windowSize.x / windowSize.y;
	zoomLevel = windowAR >= 16.0f / 9.0f ? windowSize.y / 2160.0f : windowSize.x / 3840.0f;
	const auto uiSize = windowSize / zoomLevel;
	uiRoot->setRect(Rect4f(Vector2f(), uiSize));

	auto uiInput = env->getUIInput();
	uiInput->update(t);
	uiRoot->update(t, UIInputType::Mouse, getInputAPI().getMouse(), uiInput);

	if (kb->isButtonPressed(KeyCode::F11)) {
		perfStats->setActive(!perfStats->isActive());
	}
	perfStats->update(t);
}

void GameStage::onRender(RenderContext& rc) const
{
	uiRoot->render(rc);

	Camera camera;
	camera.setZoom(zoomLevel);
	camera.setViewPort(Rect4i(uiRoot->getRect()));
	camera.setPosition(uiRoot->getRect().getCenter());

	rc.with(camera).bind([&] (Painter& painter)
	{
		painter.clear(Colour4f(0.0f, 0.0f, 0.0f));
		
		SpritePainter spritePainter;
		spritePainter.start();
		uiRoot->draw(spritePainter, 1, 0);
		spritePainter.draw(1, painter);
	});

	rc.bind([&] (Painter& painter)
	{
		if (perfStats->isActive()) {
			perfStats->paint(painter);
		}
	});
}
