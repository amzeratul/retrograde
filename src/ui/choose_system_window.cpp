#include "choose_system_window.h"

#include "choose_game_window.h"
#include "src/config/system_config.h"
#include "src/metadata/game_collection.h"
#include "src/retrograde/retrograde_environment.h"
#include "src/util/image_cache.h"

ChooseSystemWindow::ChooseSystemWindow(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment, std::optional<String> systemId, std::optional<String> gameId)
	: UIWidget("choose_system", Vector2f(), UISizer())
	, factory(factory)
	, retrogradeEnvironment(retrogradeEnvironment)
	, pendingSystemId(std::move(systemId))
	, pendingGameId(std::move(gameId))
{
	factory.loadUI(*this, "choose_system");

	//bg = Sprite().setImage(retrogradeEnvironment.getResources(), "ui/background.jpg");

	setAnchor(UIAnchor());
}

void ChooseSystemWindow::onAddedToRoot(UIRoot& root)
{
	if (pendingSystemId) {
		loadSystem(*pendingSystemId);
		pendingSystemId = {};
	}
	fitToRoot();
}

void ChooseSystemWindow::onMakeUI()
{
	populateSystems();

	setHandle(UIEventType::ListAccept, "systemList", [=] (const UIEvent& event)
	{
		loadSystem(event.getStringData());
	});

	setHandle(UIEventType::ListCancel, "systemList", [=](const UIEvent& event)
	{
		close();
	});
}

void ChooseSystemWindow::update(Time t, bool moved)
{
	fitToRoot();
	const auto scales = getSize() / bg.getSize();
	const auto scale = std::max(scales.x, scales.y);
	//bg.setPivot(Vector2f(0.5f, 0.5f)).setPosition(getRect().getCenter()).setScale(scale);
}

void ChooseSystemWindow::draw(UIPainter& painter) const
{
	//painter.draw(bg);
}

void ChooseSystemWindow::setSelectedSystem(const SystemConfig& systemConfig)
{
	auto& regionConfig = systemConfig.getRegion(region);

	getWidgetAs<UILabel>("system_name")->setText(LocalisedString::fromUserString(regionConfig.getName()));
	getWidgetAs<UILabel>("system_description")->setText(factory.getI18N().get(systemConfig.getDescriptionKey()));

	auto loadCapsuleInfo = [&] (std::string_view capsuleName, std::string_view labelName, const String& data)
	{
		getWidget(capsuleName)->setActive(!data.isEmpty() && data != "?" && data != "0" && !data.startsWith("#MISSING:"));
		getWidgetAs<UILabel>(labelName)->setText(LocalisedString::fromUserString(data));
	};

	loadCapsuleInfo("game_capsule_date", "game_info_date", toString(systemConfig.getReleaseDate().year));
	loadCapsuleInfo("game_capsule_developer", "game_info_developer", systemConfig.getManufacturer());
	loadCapsuleInfo("game_capsule_generation", "game_info_generation", factory.getI18N().get("gen" + toString(systemConfig.getGeneration())).getString());
	loadCapsuleInfo("game_capsule_games", "game_info_games", ""); // TODO

	retrogradeEnvironment.getImageCache().loadIntoOr(getWidgetAs<UIImage>("system_image"), systemConfig.getInfoImage(), "systems/info_unknown.png");
}

const String& ChooseSystemWindow::getRegion() const
{
	return region;
}

void ChooseSystemWindow::loadSystem(const String& systemId)
{
	setActive(false);
	const auto& systemConfig = retrogradeEnvironment.getConfigDatabase().get<SystemConfig>(systemId);
	getRoot()->addChild(std::make_shared<ChooseGameWindow>(factory, retrogradeEnvironment, systemConfig, pendingGameId, *this));
	pendingGameId = {};
}

void ChooseSystemWindow::close()
{
	retrogradeEnvironment.getHalleyAPI().core->quit();
}

void ChooseSystemWindow::populateSystems()
{
	auto getCategoryId = [](const SystemConfig& s) -> String
	{
		if (s.getCategory() == SystemCategory::Computer) {
			return "computer";
		} else {
			return "gen" + toString(s.getGeneration()) + toString(s.getCategory());
		}
	};

	bool showEmptySystems = true;

	// Sort systems by category & generation
	std::map<String, Vector<const SystemConfig*>> systemsByCategory;
	for (const auto& s: retrogradeEnvironment.getConfigDatabase().getValues<SystemConfig>()) {
		if (showEmptySystems || !retrogradeEnvironment.getGameCollection(s->getId()).getEntries().empty()) {
			systemsByCategory[getCategoryId(*s)].push_back(s);
		}
	}

	const auto systemCategoryList = getWidgetAs<UIList>("systemCategoryList");
	systemCategoryList->setFocusable(false);
	for (auto& [categoryId, systems]: systemsByCategory) {
		auto title = factory.getI18N().get(categoryId);
		systemCategoryList->addItem(categoryId, std::make_shared<SystemList>(factory, retrogradeEnvironment, std::move(title), std::move(systems), *this), 1);
	}
}


SystemList::SystemList(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment, LocalisedString title, Vector<const SystemConfig*> systems, ChooseSystemWindow& parent)
	: UIWidget("system_list", {}, UISizer())
	, factory(factory)
	, retrogradeEnvironment(retrogradeEnvironment)
	, title(std::move(title))
	, systems(std::move(systems))
	, parent(parent)
{
	factory.loadUI(*this, "system_list");
}

void SystemList::onMakeUI()
{
	const auto& region = parent.getRegion();

	std::sort(systems.begin(), systems.end(), [&](const SystemConfig* a, const SystemConfig* b)
	{
		return std::tuple(a->getCategory(), -a->getUnitsSold(), a->getReleaseDate()) < std::tuple(b->getCategory(), -b->getUnitsSold(), b->getReleaseDate());
	});

	getWidgetAs<UILabel>("title")->setText(title);

	const auto systemList = getWidgetAs<UIList>("systemList");
	for (const auto& system: systems) {
		systemList->addItem(system->getId(), std::make_shared<SystemCapsule>(factory, retrogradeEnvironment, system, region));
	}
	systemList->setShowSelection(false);
	systemList->setEnabled(false);
	systemList->setFocusable(false);

	setHandle(UIEventType::SetSelected, [=](const UIEvent& event)
	{
		const bool enabled = event.getBoolData();
		systemList->setShowSelection(enabled);
		systemList->setEnabled(enabled);
		if (enabled) {
			parent.setSelectedSystem(*getSystemConfig(systemList->getSelectedOptionId()));
		}
	});

	setHandle(UIEventType::ListSelectionChanged, "systemList", [=](const UIEvent& event)
	{
		parent.setSelectedSystem(*getSystemConfig(event.getStringData()));
	});
}

const SystemConfig* SystemList::getSystemConfig(const String& id) const
{
	const auto iter = std_ex::find_if(systems, [&](const SystemConfig* c) { return c->getId() == id; });
	if (iter == systems.end()) {
		return nullptr;
	}
	return *iter;
}

SystemCapsule::SystemCapsule(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment, const SystemConfig* systemConfig, String region)
	: UIWidget("system_capsule", {}, UISizer())
	, factory(factory)
	, retrogradeEnvironment(retrogradeEnvironment)
	, systemConfig(systemConfig)
	, region(std::move(region))
{
	factory.loadUI(*this, "system_capsule");
}

void SystemCapsule::onMakeUI()
{
	const auto& regionConfig = systemConfig->getRegion(region);

	const auto maxSize = Vector2f(500, 300);
	const auto capsule = getWidgetAs<UIImage>("capsule");
	retrogradeEnvironment.getImageCache().loadIntoOr(capsule, regionConfig.getLogoImage(), "", "Halley/Sprite", maxSize);

	const auto name = getWidgetAs<UILabel>("name");
	name->setText(LocalisedString::fromUserString(regionConfig.getName()));

	const auto selBorder = getWidgetAs<UIImage>("selBorder");
	const auto col = selBorder->getSprite().getColour();
	selBorder->setSelectable(col.withAlpha(0.0f), col.withAlpha(1.0f));
}
