#include "choose_system_window.h"

#include "choose_game_window.h"
#include "src/config/system_config.h"
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
}

void ChooseSystemWindow::onMakeUI()
{
	const String region = "world";
	auto systems = retrogradeEnvironment.getConfigDatabase().getValues<SystemConfig>();
	std::sort(systems.begin(), systems.end(), [&](const SystemConfig* a, const SystemConfig* b)
	{
		return a->getRegion(region).getName() < b->getRegion(region).getName();
	});

	auto systemList = getWidgetAs<UIList>("systemList");
	for (const auto& system : systems) {
		systemList->addTextItem(system->getId(), LocalisedString::fromUserString(system->getRegion(region).getName()));
	}

	setHandle(UIEventType::ListAccept, "systemList", [=] (const UIEvent& event)
	{
		loadSystem(event.getStringData());
	});

	setHandle(UIEventType::ButtonClicked, "exit", [=] (const UIEvent& event)
	{
		retrogradeEnvironment.getHalleyAPI().core->quit();
	});
}

void ChooseSystemWindow::loadSystem(const String& systemId)
{
	setActive(false);
	const auto& systemConfig = retrogradeEnvironment.getConfigDatabase().get<SystemConfig>(systemId);
	getRoot()->addChild(std::make_shared<ChooseGameWindow>(factory, retrogradeEnvironment, systemConfig, pendingGameId, *this));
	pendingGameId = {};
}
