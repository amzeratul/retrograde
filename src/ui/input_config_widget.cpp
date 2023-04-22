#include "input_config_widget.h"

#include "src/game/game_canvas.h"
#include "src/libretro/libretro_core.h"
#include "src/retrograde/input_mapper.h"

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
	if (t < 0.000001) {
		return;
	}

	const auto& gameInputMapper = gameCanvas.getGameInputMapper();
	const auto& uiInputMapper = gameInputMapper.getInputMapper();

	for (auto& device: gameInputMapper.getAllDevices()) {
		if (const auto input = uiInputMapper.getUIInputForDevice(*device)) {
			const auto dx = input->getAxisRepeat(InputMapper::UIAxes::UI_AXIS_LEFT_X);
			const auto dy = input->getAxisRepeat(InputMapper::UIAxes::UI_AXIS_LEFT_Y);

			if (dx != 0) {
				moveDevice(device, dx);
			} else if (dy != 0) {
				changeDeviceMapping(device, dy);
			}
		}
	}

	for (size_t i = 0; i < slots.size(); ++i) {
		slots[i]->setDevice(gameInputMapper.getDeviceAt(static_cast<int>(i)));
	}
}

void InputConfigWidget::initialInputSetup()
{
	auto& inputMapper = gameCanvas.getGameInputMapper();
	inputMapper.chooseBestAssignments();
	inputMapper.setAssignmentsFixed(true);

	const auto& controllerTypes = gameCanvas.getCore().getControllerTypes();
	const int nSlotsAvailable = std::min(static_cast<int>(controllerTypes.size()), 5);
	setSlots(nSlotsAvailable);
}

void InputConfigWidget::setSlots(int n)
{
	const auto& inputMapper = gameCanvas.getGameInputMapper();
	const auto& controllerTypes = gameCanvas.getCore().getControllerTypes();

	const auto slotsList = getWidget("slots");
	slotsList->clear();
	slots.resize(n);
	
	for (int i = 0; i < n; ++i) {
		auto slot = std::make_shared<InputSlotWidget>(factory);
		slots[i] = slot;
		slot->setSlotName("Port #" + toString(i + 1));
		slot->setDeviceTypes(controllerTypes[i]);
		slot->setDevice(inputMapper.getDeviceAt(i));

		slotsList->add(slot);
	}
}

void InputConfigWidget::moveDevice(const std::shared_ptr<InputDevice>& device, int dx)
{
	auto& inputMapper = gameCanvas.getGameInputMapper();
	inputMapper.moveDevice(device, dx, static_cast<int>(slots.size()));
}

void InputConfigWidget::changeDeviceMapping(const std::shared_ptr<InputDevice>& device, int dy)
{
	
}

InputSlotWidget::InputSlotWidget(UIFactory& factory)
	: UIWidget("inputSlot", {}, UISizer())
{
	factory.loadUI(*this, "input_slot");
}

void InputSlotWidget::onMakeUI()
{
	setSlotActive(false);
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
	if (this->deviceTypes != deviceTypes) {
		this->deviceTypes = std::move(deviceTypes);
	}
}

void InputSlotWidget::setDevice(const std::shared_ptr<InputDevice>& device)
{
	if (this->device != device) {
		this->device = device;

		setSlotActive(!!device);

		if (device) {
			const auto type = device->getInputType();
			getWidgetAs<UILabel>("deviceName")->setText(LocalisedString::fromUserString(device->getName()));
			getWidget("iconGamepad")->setActive(type == InputType::Gamepad);
			getWidget("iconKeyboard")->setActive(type == InputType::Keyboard);
		}
	}
}

