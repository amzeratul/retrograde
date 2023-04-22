#include "input_config_widget.h"

#include "src/game/game_canvas.h"
#include "src/libretro/libretro_core.h"

InputConfigWidget::InputConfigWidget(UIFactory& factory, GameCanvas& gameCanvas)
	: UIWidget("inputConfigWidget", {}, UISizer())
	, factory(factory)
	, gameCanvas(gameCanvas)
{
	factory.loadUI(*this, "input_config");
}

void InputConfigWidget::onMakeUI()
{
	initialInputSetup();
}

void InputConfigWidget::update(Time t, bool moved)
{
	auto& inputMapper = gameCanvas.getGameInputMapper();

	// TODO
}

void InputConfigWidget::initialInputSetup()
{
	auto& inputMapper = gameCanvas.getGameInputMapper();
	inputMapper.chooseBestAssignments();
	inputMapper.setAssignmentsFixed(true);

	const auto& controllerTypes = gameCanvas.getCore().getControllerTypes();
	const int nSlotsAvailable = std::min(static_cast<int>(controllerTypes.size()), 5);
	setSlots(nSlotsAvailable);

	for (int i = 0; i < nSlotsAvailable; ++i) {
		slots[i]->setDeviceTypes(controllerTypes[i]);
	}
}

void InputConfigWidget::setSlots(int n)
{
	const auto slotsList = getWidget("slots");
	slotsList->clear();
	slots.resize(n);
	
	for (int i = 0; i < n; ++i) {
		auto slot = std::make_shared<InputSlotWidget>(factory);
		slots[i] = slot;
		slot->setSlotActive(false);
		slot->setSlotName("Port #" + toString(i + 1));
		slotsList->add(slot);
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

void InputSlotWidget::setDeviceTypes(Vector<LibretroCore::ControllerType> deviceTypes)
{
	this->deviceTypes = std::move(deviceTypes);
}

