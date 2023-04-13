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

	const size_t nGames = retrogradeEnvironment.getGameCollection(systemConfig.getId()).getEntries().size();
	loadCapsuleInfo("game_capsule_games", "game_info_games", nGames == 0 ? "" : (nGames > 1 ? toString(nGames) + " Games" : "1 Game"));

	getWidgetAs<UIImage>("system_image")->setSprite({});
	retrogradeEnvironment.getImageCache().loadIntoOr(getWidgetAs<UIImage>("system_image"), regionConfig.getMachineImage(), "systems/info_unknown.png", "Halley/Sprite", Vector2f(1000.0f, 500.0f));
}

const String& ChooseSystemWindow::getRegion() const
{
	return region;
}

void ChooseSystemWindow::loadSystem(const String& systemId)
{
	setActive(false);
	retrogradeEnvironment.getImageCache().clear();
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
	const auto viewMode = ViewMode::Timeline;
	const bool showEmptySystems = true;

	using CategoryType = std::pair<SystemCategory, std::optional<int>>;
	auto getCategoryId = [&](const SystemConfig& s) -> CategoryType
	{
		return { s.getCategory(), viewMode == ViewMode::Generations && s.getGeneration() != 0 ? std::optional<int>(s.getGeneration()) : std::nullopt };
	};

	// Sort systems by category & generation
	Vector<CategoryType> categories;
	HashMap<CategoryType, Vector<const SystemConfig*>> systemsByCategory;
	for (const auto& s: retrogradeEnvironment.getConfigDatabase().getValues<SystemConfig>()) {
		if (showEmptySystems || !retrogradeEnvironment.getGameCollection(s->getId()).getEntries().empty()) {
			const auto cat = getCategoryId(*s);
			systemsByCategory[cat].push_back(s);
			if (!std_ex::contains(categories, cat)) {
				categories.push_back(cat);
			}
		}
	}

	// Sort categories
	std::sort(categories.begin(), categories.end());

	// Generate UI
	const auto systemCategoryList = getWidgetAs<UIList>("systemCategoryList");
	systemCategoryList->setFocusable(false);
	for (auto& categoryId: categories) {
		const auto sysIter = systemsByCategory.find(categoryId);
		if (sysIter == systemsByCategory.end()) {
			continue;
		}
		auto systems = sysIter->second;

		const auto id = (categoryId.second ? "gen" + toString(categoryId.second.value()) : "") + toString(categoryId.first);
		auto title = factory.getI18N().get(id);

		if (viewMode == ViewMode::Generations) {
			std::sort(systems.begin(), systems.end(), [&](const SystemConfig* a, const SystemConfig* b)
			{
				return std::tuple(a->getCategory(), -a->getUnitsSold(), a->getReleaseDate()) < std::tuple(b->getCategory(), -b->getUnitsSold(), b->getReleaseDate());
			});
		} else {
			std::sort(systems.begin(), systems.end(), [&](const SystemConfig* a, const SystemConfig* b)
			{
				return a->getReleaseDate() < b->getReleaseDate();
			});
		}

		systemCategoryList->addItem(id, std::make_shared<SystemList>(factory, retrogradeEnvironment, std::move(title), std::move(systems), *this, viewMode), 1);
	}
}


SystemList::SystemList(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment, LocalisedString title, Vector<const SystemConfig*> systems, ChooseSystemWindow& parent, ChooseSystemWindow::ViewMode viewMode)
	: UIWidget("system_list", {}, UISizer())
	, factory(factory)
	, retrogradeEnvironment(retrogradeEnvironment)
	, title(std::move(title))
	, systems(std::move(systems))
	, parent(parent)
	, viewMode(viewMode)
{
	factory.loadUI(*this, "system_list");
}

void SystemList::onMakeUI()
{
	const auto& region = parent.getRegion();
	getWidgetAs<UILabel>("title")->setText(title);

	// Populate systems
	const auto systemList = getWidgetAs<UIList>("systemList");
	int lastGen = 0;
	int lastYear = 0;
	for (const auto& system: systems) {
		const auto gen = system->getGeneration();
		const auto year = system->getReleaseDate().year;

		if (viewMode == ChooseSystemWindow::ViewMode::Timeline && (gen != lastGen || year != lastYear)) {
			auto timelinePoint = factory.makeUI("timeline_point");
			timelinePoint->getWidgetAs<UILabel>("year")->setText(LocalisedString::fromUserString(year != lastYear ? toString(year) : ""));
			timelinePoint->getWidgetAs<UILabel>("gen")->setText(gen != lastGen ? factory.getI18N().get("gen" + toString(gen)) : LocalisedString());
			systemList->add(timelinePoint, 0, Vector4f(-25, 0, -25, 0), UISizerAlignFlags::Centre);
			lastGen = gen;
			lastYear = year;
		}

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
