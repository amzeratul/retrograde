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
	aliveFlag = std::make_shared<bool>(true);

	const auto coreId = systemConfig.getCores().empty() ? "" : systemConfig.getCores().front();
	coreConfig = coreId.isEmpty() ? nullptr : &retrogradeEnvironment.getConfigDatabase().get<CoreConfig>(coreId);
	collection.scanGameData();

	factory.loadUI(*this, "choose_game");

	setAnchor(UIAnchor());

	UIInputButtons buttons;
	buttons.accept = 0;
	buttons.cancel = 1;
	setInputButtons(buttons);
}

ChooseGameWindow::~ChooseGameWindow()
{
	savePosition();
	*aliveFlag = false;
}

void ChooseGameWindow::onMakeUI()
{
	onNoGameSelected();

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

	retrogradeEnvironment.getImageCache().loadIntoOr(getWidgetAs<UIImage>("system_logo"), systemConfig.getRegion("world").getLogoImage(), "", "Halley/Sprite", Vector2f(780.0f, 400.0f));

	collection.whenReady([=, aliveFlag = aliveFlag]() {
		if (!*aliveFlag) {
			return;
		}
		const auto gameList = getWidgetAs<UIList>("gameList");
		gameList->clear();
		const auto& entries = collection.getEntries();
		for (size_t i = 0; i < entries.size(); ++i) {
			const auto& entry = entries[i];
			if (!entry.hidden) {
				gameList->addItem(toString(i), std::make_shared<GameCapsule>(factory, retrogradeEnvironment, entry));
			}
		}

		layout();
		loadPosition();
	});
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
		savePosition();
		setActive(false);
		getRoot()->addChild(std::make_shared<GameCanvas>(factory, retrogradeEnvironment, *coreConfig, systemConfig, gameId, *this));
	} else {
		onErrorDueToNoCoreAvailable();
	}
}

void ChooseGameWindow::onNoGameSelected()
{
	GameCollection::Entry entry;
	onGameSelected(entry);
}

void ChooseGameWindow::onGameSelected(size_t gameIdx)
{
	onGameSelected(collection.getEntries()[gameIdx]);
}

void ChooseGameWindow::onGameSelected(const GameCollection::Entry& entry)
{
	getWidgetAs<UILabel>("game_name")->setText(LocalisedString::fromUserString(entry.displayName));

	auto loadCapsuleInfo = [&] (std::string_view capsuleName, std::string_view labelName, const String& data, bool canHide = true)
	{
		getWidget(capsuleName)->setActive(!canHide || (!data.isEmpty() && data != "?" && data != "0"));
		getWidgetAs<UILabel>(labelName)->setText(LocalisedString::fromUserString(data));
	};

	loadCapsuleInfo("game_capsule_date", "game_info_date", entry.date.year ? toString(entry.date.year) : "?", false);
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

void ChooseGameWindow::savePosition()
{
	const auto gameList = getWidgetAs<UIList>("gameList");
	const auto& entries = collection.getEntries();
	const auto curSel = gameList->getSelectedOption();

	if (curSel < 0 || curSel >= static_cast<int>(entries.size())) {
		return;
	}
	const auto& curEntry = entries[curSel];

	ConfigNode::MapType windowData;
	windowData["lastEntry"] = curEntry.files.front().getString();
	retrogradeEnvironment.getSettings().setWindowData("choose_game:" + systemConfig.getId(), std::move(windowData));
	retrogradeEnvironment.getSettings().save();
}

void ChooseGameWindow::loadPosition()
{
	auto& windowData = retrogradeEnvironment.getSettings().getWindowData("choose_game:" + systemConfig.getId());
	windowData.ensureType(ConfigNodeType::Map);
	const auto lastEntry = windowData["lastEntry"].asString("");
	if (!lastEntry.isEmpty()) {
		const auto lastEntryPath = Path(lastEntry);
		const auto& entries = collection.getEntries();
		const auto iter = std_ex::find_if(entries, [&] (const GameCollection::Entry& e)
		{
			return e.files.front() == lastEntryPath;
		});

		if (iter != entries.end()) {
			const auto idx = static_cast<int>(iter - entries.begin());
			const auto gameList = getWidgetAs<UIList>("gameList");
			gameList->setSelectedOption(idx);
		}
	}
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
