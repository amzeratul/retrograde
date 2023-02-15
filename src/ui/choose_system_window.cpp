#include "choose_system_window.h"

ChooseSystemWindow::ChooseSystemWindow(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment)
	: UIWidget("choose_system", Vector2f(), UISizer())
	, factory(factory)
	, retrogradeEnvironment(retrogradeEnvironment)
{
	factory.loadUI(*this, "choose_system");
}

void ChooseSystemWindow::onMakeUI()
{
}
