#include "in_game_menu.h"

#include "src/game/game_canvas.h"

InGameMenu::InGameMenu(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment, GameCanvas& gameCanvas)
	: UIWidget("in_game_menu", Vector2f(), UISizer())
	, factory(factory)
	, retrogradeEnvironment(retrogradeEnvironment)
	, gameCanvas(gameCanvas)
{
	factory.loadUI(*this, "in_game_menu");

	setAnchor(UIAnchor());
	setModal(true);

	UIInputButtons buttons;
	buttons.cancel = 1;
	setInputButtons(buttons);
}

void InGameMenu::onMakeUI()
{
	setupMenu();

	setHandle(UIEventType::ListAccept, "options", [=] (const UIEvent& event)
	{
		onChooseOption(event.getStringData());
	});
}

void InGameMenu::onAddedToRoot(UIRoot& root)
{
	fitToRoot();
}

void InGameMenu::update(Time t, bool moved)
{
	fitToRoot();
}

void InGameMenu::setupMenu()
{
	const auto options = getWidgetAs<UIList>("options");
	options->clear();

	options->addTextItem("resume", LocalisedString::fromHardcodedString("Resume"));
	options->addTextItem("reset", LocalisedString::fromHardcodedString("Reset"));
	options->addTextItem("savestate", LocalisedString::fromHardcodedString("Save/Load"));
	options->add(std::make_shared<UIWidget>("", Vector2f(0, 100)));
	options->addTextItem("media", LocalisedString::fromHardcodedString("View Media"));
	options->addTextItem("achievements", LocalisedString::fromHardcodedString("Achievements"));
	options->add(std::make_shared<UIWidget>("", Vector2f(0, 100)));
	options->addTextItem("exit", LocalisedString::fromHardcodedString("Exit Game"));

	options->setItemEnabled("savestate", false);
	options->setItemEnabled("media", false);
	options->setItemEnabled("achievements", false);
}

void InGameMenu::onChooseOption(const String& optionId)
{
	if (optionId == "resume") {
		destroy();
	} else if (optionId == "reset") {
		destroy();
		gameCanvas.resetGame();
	} else if (optionId == "savestates") {
		// TODO
	} else if (optionId == "media") {
		// TODO
	} else if (optionId == "achievements") {
		// TODO
	} else if (optionId == "exit") {
		destroy();
		gameCanvas.close();
	}
}

void InGameMenu::onGamepadInput(const UIInputResults& input, Time time)
{
	if (input.isButtonPressed(UIGamepadInput::Button::Cancel)) {
		destroy();
	}
}
