#include "in_game_menu.h"

#include "game_canvas.h"

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
	setHandle(UIEventType::ListAccept, "options", [=] (const UIEvent& event)
	{
		onChooseOption(event.getStringData());
	});
}

void InGameMenu::onChooseOption(const String& optionId)
{
	if (optionId == "resume") {
		destroy();
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
