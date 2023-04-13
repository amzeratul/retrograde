#include "in_game_menu.h"

#include "src/game/game_canvas.h"
#include "src/retrograde/retrograde_environment.h"
#include "src/savestate/savestate.h"
#include "src/savestate/savestate_collection.h"
#include "src/util/image_cache.h"

InGameMenu::InGameMenu(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment, GameCanvas& gameCanvas, Mode mode, const GameCollection::Entry* metadata)
	: UIWidget("in_game_menu", Vector2f(), UISizer())
	, factory(factory)
	, retrogradeEnvironment(retrogradeEnvironment)
	, gameCanvas(gameCanvas)
	, mode(mode)
	, metadata(metadata)
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

	if (metadata) {
		const auto logo = getWidgetAs<UIImage>("logo");
		retrogradeEnvironment.getImageCache().loadIntoOr(logo, metadata->getMedia(GameCollection::MediaType::Logo).toString(), "", "Halley/Sprite", Vector2f(2000.0f, 600.0f));
	} else {
		Logger::logWarning("Couldn't find metadata for game!");
	}
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
		options->addTextItem("continue", LocalisedString::fromHardcodedString("Continue"), -1, true);
		options->addTextItem("new", LocalisedString::fromHardcodedString("New Game"), -1, true);
		options->addTextItem("load", LocalisedString::fromHardcodedString("Load Game"), -1, true);
		options->add(std::make_shared<UIWidget>("", Vector2f(0, 80)));
		options->addTextItem("media", LocalisedString::fromHardcodedString("View Media"), -1, true);
		options->addTextItem("achievements", LocalisedString::fromHardcodedString("Achievements"), -1, true);
	} else if (mode == Mode::InGame) {
		options->addTextItem("resume", LocalisedString::fromHardcodedString("Resume"), -1, true);
		options->addTextItem("reset", LocalisedString::fromHardcodedString("Reset"), -1, true);
		options->addTextItem("savestates", LocalisedString::fromHardcodedString("Save/Load"), -1, true);
		options->addTextItem("swapdisc", LocalisedString::fromHardcodedString("Swap Disc"), -1, true);
		//options->add(std::make_shared<UIWidget>("", Vector2f(0, 100)));
		options->addTextItem("media", LocalisedString::fromHardcodedString("View Media"), -1, true);
		options->addTextItem("achievements", LocalisedString::fromHardcodedString("Achievements"), -1, true);
		options->add(std::make_shared<UIWidget>("", Vector2f(0, 50)), 1);
		options->addTextItem("exit", LocalisedString::fromHardcodedString("Exit Game"), -1, true);
	}

	const auto& ssc = gameCanvas.getSaveStateCollection();
	options->setItemEnabled("continue", ssc.hasSuspendSave());
	options->setItemEnabled("load", ssc.hasAnySave());
	options->setItemEnabled("swapdisc", gameCanvas.canSwapDisc());
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
		hide();
	} else if (optionId == "reset") {
		gameCanvas.resetGame();
		getWidgetAs<UIList>("options")->setSelectedOptionId("resume");
		hide();
	} else if (optionId == "new") {
		gameCanvas.startGame();
		close();
	} else if (optionId == "continue") {
		gameCanvas.startGame(std::pair(SaveStateType::Suspend, 0));
		close();
	} else if (optionId == "load") {
		showSaveStates(false);
	} else if (optionId == "savestates") {
		showSaveStates(true);
	} else if (optionId == "swapdisc") {
		showSwapDisc();
	} else if (optionId == "media") {
		showMedia();
	} else if (optionId == "achievements") {
		showAchievements();
	} else if (optionId == "exit") {
		close();
		gameCanvas.close();
	}
}

void InGameMenu::showRoot()
{
	getWidget("optionsPane")->setActive(true);
	getWidget("savestatePane")->setActive(false);
}

void InGameMenu::showSaveStates(bool canSave)
{
	getWidget("optionsPane")->setActive(false);
	getWidget("savestatePane")->setActive(true);

	refreshSaveStateList(canSave);

	setHandle(UIEventType::ListAccept, "savestateList", [=] (const UIEvent& event)
	{
		if (event.getStringData() == "save") {
			gameCanvas.getSaveStateCollection().saveGameState(SaveStateType::Permanent).then(Executors::getMainUpdateThread(), [=] (SaveState savestate)
			{
				refreshSaveStateList(canSave);
			});
		} else {
			const auto split = event.getStringData().split(':');
			const auto type = fromString<SaveStateType>(split[0]);
			const auto idx = split[1].toInteger();
			gameCanvas.startGame(std::pair(type, idx));
			close();
		}
	});
}

void InGameMenu::refreshSaveStateList(bool canSave)
{
	auto& ssc = gameCanvas.getSaveStateCollection();

	const auto savestatesList = getWidgetAs<UIList>("savestateList");
	savestatesList->clear();

	if (canSave) {
		const auto id = "save";
		auto entry = std::make_shared<SaveStateCapsule>(factory, retrogradeEnvironment);
		savestatesList->addItem(id, std::move(entry), 1);
	}

	for (const auto& e: ssc.enumerate()) {
		const auto id = toString(e.first) + ":" + toString(e.second);
		auto entry = std::make_shared<SaveStateCapsule>(factory, retrogradeEnvironment);
		entry->loadData(ssc, e.first, e.second);
		savestatesList->addItem(id, std::move(entry), 1);
	}
}

void InGameMenu::showSwapDisc()
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
	if (input.isButtonPressed(UIGamepadInput::Button::Cancel)) {
		back();
	}

	if (input.isButtonPressed(UIGamepadInput::Button::Secondary)) {
		hide();
	}
}

void InGameMenu::back()
{
	if (getWidget("optionsPane")->isActive()) {
		if (mode == Mode::PreStart) {
			gameCanvas.close();
			close();
		} else {
			hide();
		}
	} else {
		showRoot();
	}
}

void InGameMenu::close()
{
	destroy();
}

void InGameMenu::hide()
{
	setActive(false);
}

SaveStateCapsule::SaveStateCapsule(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment)
	: UIWidget("", {}, UISizer())
	, retrogradeEnvironment(retrogradeEnvironment)
{
	factory.loadUI(*this, "savestate_capsule");
}

void SaveStateCapsule::loadData(SaveStateCollection& ssc, SaveStateType type, size_t idx)
{
	if (const auto ss = ssc.getSaveState(type, idx)) {
		auto image = ss->getScreenShot();

		const float aspectRatio = ss->getScreenShotAspectRatio();
		const auto maxSize = Vector2f(aspectRatio * 672.0f, 672.0f);

		Sprite sprite = Sprite().setImage(retrogradeEnvironment.getResources(), *retrogradeEnvironment.getHalleyAPI().video, std::move(image));
		getWidgetAs<UIImage>("image")->setSprite(sprite);
		getWidgetAs<UIImage>("image")->setMinSize(maxSize);

		String label;
		switch (type) {
		case SaveStateType::Suspend:
			label = "Suspended";
			break;
		case SaveStateType::QuickSave:
			label = "Quicksave";
			break;
		case SaveStateType::Permanent:
			label = "Save " + toString(idx + 1);
			break;
		}

		label += "      " + getDate(ss->getTimeStamp());

		getWidgetAs<UILabel>("label")->setText(LocalisedString::fromUserString(label));
	}
}

String SaveStateCapsule::getDate(uint64_t timestamp) const
{
    std::time_t t = static_cast<std::time_t>(timestamp);
    std::tm tm;
    
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d  %H:%M");

    return ss.str();
}
