#include "choose_system_window.h"

#include "choose_game_window.h"
#include "src/config/system_config.h"
#include "src/game/retrograde_environment.h"

ChooseSystemWindow::ChooseSystemWindow(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment)
	: UIWidget("choose_system", Vector2f(), UISizer())
	, factory(factory)
	, retrogradeEnvironment(retrogradeEnvironment)
{
	factory.loadUI(*this, "choose_system");

	setAnchor(UIAnchor());
}

void ChooseSystemWindow::onMakeUI()
{
	auto systemList = getWidgetAs<UIList>("systemList");
	for (const auto& [k, v] : retrogradeEnvironment.getConfigDatabase().getEntries<SystemConfig>()) {
		systemList->addTextItem(k, LocalisedString::fromUserString(v.getRegion("world").getName()));
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
	getRoot()->addChild(std::make_shared<ChooseGameWindow>(factory, retrogradeEnvironment, systemId, *this));
}
