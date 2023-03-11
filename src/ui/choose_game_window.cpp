#include "choose_game_window.h"

#include "choose_system_window.h"
#include "src/game/game_canvas.h"
#include "src/config/system_config.h"
#include "src/config/core_config.h"
#include "src/metadata/game_collection.h"
#include "src/retrograde/retrograde_environment.h"
#include "src/util/image_cache.h"

ChooseGameWindow::ChooseGameWindow(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment, const SystemConfig& systemConfig, std::optional<String> gameId, UIWidget& parentMenu)
	: UIWidget("choose_game", Vector2f(), UISizer())
	, factory(factory)
	, retrogradeEnvironment(retrogradeEnvironment)
	, systemConfig(systemConfig)
	, pendingGameId(std::move(gameId))
	, parentMenu(parentMenu)
	, collection(retrogradeEnvironment.getGameCollection(systemConfig.getId()))
{
	const auto coreId = systemConfig.getCores().empty() ? "" : systemConfig.getCores().front();
	coreConfig = coreId.isEmpty() ? nullptr : &retrogradeEnvironment.getConfigDatabase().get<CoreConfig>(coreId);

	factory.loadUI(*this, "choose_game");

	setAnchor(UIAnchor());

	UIInputButtons buttons;
	buttons.accept = 0;
	buttons.cancel = 1;
	setInputButtons(buttons);
}

void ChooseGameWindow::onMakeUI()
{
	setHandle(UIEventType::ListAccept, "gameList", [=] (const UIEvent& event)
	{
		loadGame(event.getIntData());
	});

	setHandle(UIEventType::ListCancel, "gameList", [=](const UIEvent& event)
	{
		close();
	});

	setHandle(UIEventType::ListSelectionChanged, "gameList", [=] (const UIEvent& event)
	{
		onGameSelected(event.getIntData());
	});

	const auto gameList = getWidgetAs<UIList>("gameList");
	for (size_t i = 0; i < collection.getEntries().size(); ++i) {
		const auto& entry = collection.getEntries()[i];
		gameList->addItem(toString(i), std::make_shared<GameCapsule>(factory, retrogradeEnvironment, entry));
	}
}

void ChooseGameWindow::onAddedToRoot(UIRoot& root)
{
	if (pendingGameId) {
		loadGame(*pendingGameId);
		pendingGameId = {};
	}
	fitToRoot();
}

void ChooseGameWindow::update(Time t, bool moved)
{
	fitToRoot();
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
		loadGame(getWidgetAs<UIList>("gameList")->getSelectedOption());
	}

	if (input.isButtonPressed(UIGamepadInput::Button::Cancel)) {
		close();
	}
}

void ChooseGameWindow::loadGame(size_t gameIdx)
{
	if (coreConfig) {
		loadGame(collection.getEntries()[gameIdx].getBestFileToLoad(*coreConfig).string());
	} else {
		onErrorDueToNoCoreAvailable();
	}
}

void ChooseGameWindow::loadGame(const String& gameId)
{
	if (coreConfig) {
		setActive(false);
		getRoot()->addChild(std::make_shared<GameCanvas>(factory, retrogradeEnvironment, *coreConfig, systemConfig, gameId, *this));
	} else {
		onErrorDueToNoCoreAvailable();
	}
}

void ChooseGameWindow::onGameSelected(size_t gameIdx)
{
	const auto& entry = collection.getEntries()[gameIdx];

	getWidgetAs<UILabel>("game_name")->setText(LocalisedString::fromUserString(entry.displayName));
	getWidgetAs<UILabel>("game_info")->setText(LocalisedString::fromUserString("?"));
	getWidgetAs<UILabel>("game_description")->setText(LocalisedString::fromUserString("?"));

	//retrogradeEnvironment.getImageCache().loadIntoOr(getWidgetAs<UIImage>("game_image"), systemConfig.getInfoImage(), "systems/info_unknown.png");
}

void ChooseGameWindow::onErrorDueToNoCoreAvailable()
{
	// TODO
}


GameCapsule::GameCapsule(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment, const GameCollection::Entry& entry)
	: UIWidget("game_capsule", {}, UISizer())
	, factory(factory)
	, retrogradeEnvironment(retrogradeEnvironment)
	, entry(entry)
{
	factory.loadUI(*this, "game_capsule");
}

void GameCapsule::onMakeUI()
{
	const auto capsule = getWidgetAs<UIImage>("capsule");
	//retrogradeEnvironment.getImageCache().loadIntoOr(capsule, systemConfig->getCapsuleImage(), "systems/capsule_unknown.png");

	const auto name = getWidgetAs<UILabel>("name");
	name->setText(LocalisedString::fromUserString(entry.displayName));
}
