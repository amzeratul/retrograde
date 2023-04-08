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

	retrogradeEnvironment.getImageCache().loadIntoOr(getWidgetAs<UIImage>("system_logo"), systemConfig.getRegion("world").getLogoImage(), "", "Halley/Sprite", Vector2f(780.0f, 400.0f));
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

	auto loadCapsuleInfo = [&] (std::string_view capsuleName, std::string_view labelName, const String& data)
	{
		getWidget(capsuleName)->setActive(!data.isEmpty() && data != "?" && data != "0");
		getWidgetAs<UILabel>(labelName)->setText(LocalisedString::fromUserString(data));
	};

	loadCapsuleInfo("game_capsule_date", "game_info_date", entry.date.toString());
	loadCapsuleInfo("game_capsule_developer", "game_info_developer", entry.developer);
	loadCapsuleInfo("game_capsule_genre", "game_info_genre", entry.genre);
	loadCapsuleInfo("game_capsule_nPlayers", "game_info_nPlayers", toString(entry.nPlayers.end));

	getWidgetAs<UILabel>("game_description")->setText(LocalisedString::fromUserString(entry.description));

	retrogradeEnvironment.getImageCache().loadIntoOr(getWidgetAs<UIImage>("game_image"), entry.getMedia(GameCollection::MediaType::BoxFront).toString(), "systems/info_unknown.png", "Halley/Sprite", Vector2f(550.0f, 550.0f));
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
	const auto maxSize = Vector2f(598.0f, 448.0f);

	const auto capsule = getWidgetAs<UIImage>("capsule");
	retrogradeEnvironment.getImageCache().loadIntoOr(capsule, entry.getMedia(GameCollection::MediaType::Screenshot).toString(), "games/game_unknown.png", "Halley/Sprite", maxSize);

	const auto name = getWidgetAs<UILabel>("name");
	name->setText(LocalisedString::fromUserString(entry.displayName));

	const auto selBorder = getWidgetAs<UIImage>("selBorder");
	const auto col = selBorder->getSprite().getColour();
	selBorder->setSelectable(col.withAlpha(0.0f), col.withAlpha(1.0f));

	setHandle(UIEventType::ImageUpdated, "capsule", [=] (const UIEvent& event)
	{
		const auto size = event.getConfigData().asVector2f({});
		const auto ar = 4.0f / 3.0f;
		const auto arSize = Vector2f(size.y * ar, size.y);
		const auto scale = arSize / maxSize;
		const auto finalSize = arSize / std::max(scale.x, scale.y);
		getWidgetAs<UIImage>("capsule")->setMinSize(finalSize);
	});
}
