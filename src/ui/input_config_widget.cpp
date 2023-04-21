#include "input_config_widget.h"

InputConfigWidget::InputConfigWidget(UIFactory& factory, GameCanvas& gameCanvas)
	: UIWidget("inputConfigWidget", {}, UISizer())
	, factory(factory)
	, gameCanvas(gameCanvas)
{
	factory.loadUI(*this, "input_config");
}

void InputConfigWidget::onMakeUI()
{
	const auto slots = getWidget("slots");

	for (int i = 0; i < 4; ++i) {
		auto slot = std::make_shared<InputSlotWidget>(factory);
		slot->setSlotActive(i < 2);
		slot->setSlotName("Port #" + toString(i + 1));
		slots->add(slot);
	}
}


InputSlotWidget::InputSlotWidget(UIFactory& factory)
	: UIWidget("inputSlot", {}, UISizer())
{
	factory.loadUI(*this, "input_slot");
}

void InputSlotWidget::onMakeUI()
{
}

void InputSlotWidget::setSlotActive(bool active)
{
	getWidget("hardwareDeviceLine")->setActive(active);
	getWidget("activeSlot")->setActive(active);
	getWidget("inactiveSlot")->setActive(!active);
}

void InputSlotWidget::setSlotName(const String& name)
{
	getWidgetAs<UILabel>("portName")->setText(LocalisedString::fromUserString(name));
}

