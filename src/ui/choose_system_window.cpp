#include "choose_system_window.h"

#include "choose_game_window.h"
#include "src/config/system_config.h"
#include "src/metadata/game_collection.h"
#include "src/retrograde/retrograde_environment.h"

ChooseSystemWindow::ChooseSystemWindow(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment, std::optional<String> systemId, std::optional<String> gameId)
	: UIWidget("choose_system", Vector2f(), UISizer())
	, factory(factory)
	, retrogradeEnvironment(retrogradeEnvironment)
	, pendingSystemId(std::move(systemId))
	, pendingGameId(std::move(gameId))
{
	factory.loadUI(*this, "choose_system");

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
	auto systems = retrogradeEnvironment.getConfigDatabase().getValues<SystemConfig>();

	// Sort systems by generation
	std::map<int, Vector<const SystemConfig*>> systemsByGen;
	for (const auto& s: systems) {
		if (!retrogradeEnvironment.getGameCollection(s->getId()).getEntries().empty()) {
			systemsByGen[s->getGeneration()].push_back(s);
		}
	}

	auto systemCategoryList = getWidgetAs<UIList>("systemCategoryList");
	for (auto& [gen, systems]: systemsByGen) {
		auto title = factory.getI18N().get("gen" + toString(gen));
		systemCategoryList->addItem("gen" + toString(gen), std::make_shared<SystemList>(factory, retrogradeEnvironment, std::move(title), std::move(systems)), 1);
	}
}


SystemList::SystemList(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment, LocalisedString title, Vector<const SystemConfig*> systems)
	: UIWidget("system_list", {}, UISizer())
	, factory(factory)
	, retrogradeEnvironment(retrogradeEnvironment)
	, title(std::move(title))
	, systems(std::move(systems))
{
	factory.loadUI(*this, "system_list");
}

void SystemList::onMakeUI()
{
	const String region = "world";

	std::sort(systems.begin(), systems.end(), [&](const SystemConfig* a, const SystemConfig* b)
	{
		return a->getReleaseDate() < b->getReleaseDate();
	});

	getWidgetAs<UILabel>("title")->setText(title);

	const auto systemList = getWidgetAs<UIList>("systemList");
	for (const auto& system: systems) {
		systemList->addTextItem(system->getId(), LocalisedString::fromUserString(system->getRegion(region).getName()));
	}
	systemList->setShowSelection(false);
	systemList->setEnabled(false);

	setHandle(UIEventType::SetSelected, [=](const UIEvent& event)
	{
		systemList->setShowSelection(event.getBoolData());
		systemList->setEnabled(event.getBoolData());
	});
}
