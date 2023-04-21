#include "input_config_widget.h"

InputConfigWidget::InputConfigWidget(UIFactory& factory, GameCanvas& gameCanvas)
	: UIWidget("inputConfigWidget", {}, UISizer())
	, factory(factory)
	, gameCanvas(gameCanvas)
{
	factory.loadUI(*this, "input_config");
}

