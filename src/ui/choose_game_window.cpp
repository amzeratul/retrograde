#include "choose_game_window.h"

#include <filesystem>

#include "choose_system_window.h"
#include "game_canvas.h"
#include "src/game/retrograde_environment.h"
#include "src/libretro/libretro_core.h"

ChooseGameWindow::ChooseGameWindow(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment, String systemId, std::optional<String> gameId, UIWidget& parentMenu)
	: UIWidget("choose_game", Vector2f(), UISizer())
	, factory(factory)
	, retrogradeEnvironment(retrogradeEnvironment)
	, systemId(std::move(systemId))
	, pendingGameId(std::move(gameId))
	, parentMenu(parentMenu)
{
	factory.loadUI(*this, "choose_game");

	setAnchor(UIAnchor());

	UIInputButtons buttons;
	buttons.accept = 0;
	buttons.cancel = 1;
	setInputButtons(buttons);
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

	setHandle(UIEventType::ListCancel, "gameList", [=](const UIEvent& event)
	{
		close();
	});

	setHandle(UIEventType::ButtonClicked, "back", [=] (const UIEvent& event)
	{
		close();
	});
}

void ChooseGameWindow::onAddedToRoot(UIRoot& root)
{
	if (pendingGameId) {
		loadGame(*pendingGameId);
		pendingGameId = {};
	}
}

void ChooseGameWindow::close()
{
	parentMenu.setActive(true);
	parentMenu.layout();
	destroy();
}

void ChooseGameWindow::onGamepadInput(const UIInputResults& input, Time time)
{
	if (input.isButtonPressed(UIGamepadInput::Button::Accept)) {
		loadGame(getWidgetAs<UIList>("gameList")->getSelectedOptionId());
	}

	if (input.isButtonPressed(UIGamepadInput::Button::Cancel)) {
		close();
	}
}

void ChooseGameWindow::loadGame(const String& gameId)
{
	setActive(false);
	getRoot()->addChild(std::make_shared<GameCanvas>(factory, retrogradeEnvironment, systemId, gameId, *this));
}
