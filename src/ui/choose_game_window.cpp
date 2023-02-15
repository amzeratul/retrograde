#include "choose_game_window.h"

ChooseGameWindow::ChooseGameWindow(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment)
	: UIWidget("choose_game", Vector2f(), UISizer())
	, factory(factory)
	, retrogradeEnvironment(retrogradeEnvironment)
{
}

void ChooseGameWindow::onMakeUI()
{
	UIWidget::onMakeUI();
}
