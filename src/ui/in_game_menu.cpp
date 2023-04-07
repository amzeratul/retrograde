#include "in_game_menu.h"

#include "src/game/game_canvas.h"

InGameMenu::InGameMenu(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment, GameCanvas& gameCanvas, Mode mode)
	: UIWidget("in_game_menu", Vector2f(), UISizer())
	, factory(factory)
	, retrogradeEnvironment(retrogradeEnvironment)
	, gameCanvas(gameCanvas)
	, mode(mode)
{
	factory.loadUI(*this, "in_game_menu");

	setAnchor(UIAnchor());
	setModal(true);

	UIInputButtons buttons;
	buttons.cancel = 1;
	if (mode == Mode::InGame) {
		buttons.secondary = 12;
	}
	setInputButtons(buttons);
}

void InGameMenu::onMakeUI()
{
	setupMenu();

	if (mode == Mode::InGame) {
		getWidgetAs<UIImage>("bg")->getSprite().setAlpha(0.8f);
	}

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

	if (mode == Mode::PreStart) {
		options->addTextItem("continue", LocalisedString::fromHardcodedString("Continue"));
		options->addTextItem("new", LocalisedString::fromHardcodedString("New Game"));
		options->addTextItem("load", LocalisedString::fromHardcodedString("Load Game"));
		options->add(std::make_shared<UIWidget>("", Vector2f(0, 100)));
		options->addTextItem("media", LocalisedString::fromHardcodedString("View Media"));
		options->addTextItem("achievements", LocalisedString::fromHardcodedString("Achievements"));
	} else if (mode == Mode::InGame) {
		options->addTextItem("resume", LocalisedString::fromHardcodedString("Resume"));
		options->addTextItem("reset", LocalisedString::fromHardcodedString("Reset"));
		options->addTextItem("savestate", LocalisedString::fromHardcodedString("Save/Load"));
		options->add(std::make_shared<UIWidget>("", Vector2f(0, 100)));
		options->addTextItem("media", LocalisedString::fromHardcodedString("View Media"));
		options->addTextItem("achievements", LocalisedString::fromHardcodedString("Achievements"));
		options->add(std::make_shared<UIWidget>("", Vector2f(0, 100)), 1);
		options->addTextItem("exit", LocalisedString::fromHardcodedString("Exit Game"));
	}

	options->setItemEnabled("continue", false);
	options->setItemEnabled("load", false);
	options->setItemEnabled("savestate", false);
	options->setItemEnabled("media", false);
	options->setItemEnabled("achievements", false);

	const auto n = options->getCount();
	for (size_t i = 0; i < n; ++i) {
		if (options->getItem(static_cast<int>(i))->isEnabled()) {
			options->setSelectedOption(static_cast<int>(i));
			break;
		}
	}
}

void InGameMenu::onChooseOption(const String& optionId)
{
	if (optionId == "resume") {
		close();
	} else if (optionId == "reset") {
		gameCanvas.resetGame();
		close();
	} else if (optionId == "new") {
		gameCanvas.setReady();
		close();
	} else if (optionId == "continue") {
		// TODO: load save
		gameCanvas.setReady();
		close();
	} else if (optionId == "load") {
		showSaveStates(false);
	} else if (optionId == "savestates") {
		showSaveStates(true);
	} else if (optionId == "media") {
		showMedia();
	} else if (optionId == "achievements") {
		showAchievements();
	} else if (optionId == "exit") {
		close();
		gameCanvas.close();
	}
}

void InGameMenu::showSaveStates(bool canSave)
{
	// TODO
}

void InGameMenu::showMedia()
{
	// TODO
}

void InGameMenu::showAchievements()
{
	// TODO
}

void InGameMenu::onGamepadInput(const UIInputResults& input, Time time)
{
	if (input.isButtonPressed(UIGamepadInput::Button::Cancel) || input.isButtonPressed(UIGamepadInput::Button::Secondary)) {
		if (mode == Mode::PreStart) {
			gameCanvas.close();
		}
		close();
	}
}

void InGameMenu::close()
{
	destroy();
}
