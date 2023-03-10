#include "choose_game_window.h"

#include "choose_system_window.h"
#include "src/game/game_canvas.h"
#include "src/config/system_config.h"
#include "src/config/core_config.h"
#include "src/metadata/game_collection.h"
#include "src/retrograde/retrograde_environment.h"

ChooseGameWindow::ChooseGameWindow(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment, const SystemConfig& systemConfig, std::optional<String> gameId, UIWidget& parentMenu)
	: UIWidget("choose_game", Vector2f(), UISizer())
	, factory(factory)
	, retrogradeEnvironment(retrogradeEnvironment)
	, systemConfig(systemConfig)
	, coreConfig(retrogradeEnvironment.getConfigDatabase().get<CoreConfig>(systemConfig.getCores().front()))
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

	const auto& collection = retrogradeEnvironment.getGameCollection(systemConfig.getId());
	for (const auto& entry: collection.getEntries()) {
		gameList->addTextItem(entry.getBestFileToLoad(coreConfig).string(), LocalisedString::fromUserString(entry.displayName));
	}

	setHandle(UIEventType::ListAccept, "gameList", [=] (const UIEvent& event)
	{
		loadGame(event.getStringData());
	});

	setHandle(UIEventType::ListCancel, "gameList", [=](const UIEvent& event)
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
	getRoot()->addChild(std::make_shared<GameCanvas>(factory, retrogradeEnvironment, coreConfig, systemConfig, gameId, *this));
}
