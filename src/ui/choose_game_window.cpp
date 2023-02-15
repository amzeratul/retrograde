#include "choose_game_window.h"

#include <filesystem>

#include "choose_system_window.h"
#include "game_canvas.h"
#include "src/game/retrograde_environment.h"
#include "src/libretro/libretro_core.h"

ChooseGameWindow::ChooseGameWindow(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment, String systemId)
	: UIWidget("choose_game", Vector2f(), UISizer())
	, factory(factory)
	, retrogradeEnvironment(retrogradeEnvironment)
	, systemId(std::move(systemId))
{
	factory.loadUI(*this, "choose_game");

	setAnchor(UIAnchor());
}

void ChooseGameWindow::onMakeUI()
{
	auto gameList = getWidgetAs<UIList>("gameList");

	const auto dir = retrogradeEnvironment.getRomsDir(systemId);
	for (const auto& e: std::filesystem::directory_iterator(dir.getNativeString().cppStr())) {
		if (e.is_regular_file()) {
			auto path = e.path().filename().string();
			gameList->addTextItem(path, LocalisedString::fromUserString(path));
		}
	}

	setHandle(UIEventType::ListAccept, "gameList", [=] (const UIEvent& event)
	{
		loadGame(event.getStringData());
	});

	setHandle(UIEventType::ButtonClicked, "back", [=] (const UIEvent& event)
	{
		destroy();
		getRoot()->addChild(std::make_shared<ChooseSystemWindow>(factory, retrogradeEnvironment));
	});
}

void ChooseGameWindow::loadGame(const String& gameId)
{
	destroy();
	getRoot()->addChild(std::make_shared<GameCanvas>(retrogradeEnvironment, retrogradeEnvironment.loadCore(systemId, gameId)));
}
