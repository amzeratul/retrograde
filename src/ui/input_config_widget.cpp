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
		auto device = gameInputMapper.getDeviceAt(static_cast<int>(i));
		slots[i]->setDevice(device, gameInputMapper.getDeviceColour(*device));
	}

	setUnassignedDevices(gameInputMapper.getUnassignedDevices());
}

void InputConfigWidget::initialInputSetup()
{
	auto& inputMapper = gameCanvas.getGameInputMapper();
	inputMapper.chooseBestAssignments();
	inputMapper.setAssignmentsFixed(true);

	const auto& controllerTypes = gameCanvas.getCore().getControllerTypes();
	const int nSlotsAvailable = std::min(static_cast<int>(controllerTypes.size()), 5);
	setSlots(nSlotsAvailable);
	setUnassignedDevices(inputMapper.getUnassignedDevices());
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
		slot->setDeviceTypes(controllerTypes[i].types, controllerTypes[i].curType);
		auto device = inputMapper.getDeviceAt(i);
		slot->setDevice(device, inputMapper.getDeviceColour(*device));

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
	for (size_t i = 0; i < slots.size(); ++i) {
		if (slots[i]->getDevice() == device) {
			slots[i]->changeDeviceMapping(dy);
		}
	}
}

void InputConfigWidget::setUnassignedDevices(const Vector<std::shared_ptr<InputDevice>>& devices)
{
	auto& inputMapper = gameCanvas.getGameInputMapper();

	if (devices != unassignedDevices) {
		unassignedDevices = devices;

		const auto parkingSpace = getWidget("parkingSpace");
		parkingSpace->clear();

		for (auto& device: unassignedDevices) {
			parkingSpace->add(std::make_shared<InputParkedDeviceWidget>(factory, device, inputMapper.getDeviceColour(*device)));
		}
	}
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

void InputSlotWidget::setDeviceTypes(Vector<LibretroCore::ControllerType> deviceTypes, int current)
{
	bool changed = false;
	if (this->deviceTypes != deviceTypes) {
		this->deviceTypes = std::move(deviceTypes);
		changed = true;
	}
	if (current != curDeviceType) {
		curDeviceType = current;
		changed = true;
	}
	if (changed) {
		updateDeviceType();
	}
}

void InputSlotWidget::setDevice(const std::shared_ptr<InputDevice>& device, Colour4f colour)
{
	if (this->device != device) {
		this->device = device;

		setSlotActive(!!device);

		if (device) {
			const auto type = device->getInputType();
			getWidgetAs<UILabel>("deviceName")->setText(LocalisedString::fromUserString(device->getName()));
			getWidget("iconGamepad")->setActive(type == InputType::Gamepad);
			getWidget("iconKeyboard")->setActive(type == InputType::Keyboard);

			getWidgetAs<UIImage>("leftArrow")->getSprite().setColour(colour);
			getWidgetAs<UIImage>("rightArrow")->getSprite().setColour(colour);
			getWidgetAs<UIImage>("upArrow")->getSprite().setColour(colour);
			getWidgetAs<UIImage>("downArrow")->getSprite().setColour(colour);
			getWidgetAs<UIImage>("deviceNameBg")->getSprite().setColour(colour);
			getWidgetAs<UIImage>("virtualDeviceBg")->getSprite().setColour(colour);
		}
	}
}

std::shared_ptr<InputDevice> InputSlotWidget::getDevice() const
{
	return device;
}

void InputSlotWidget::changeDeviceMapping(int dy)
{
	const auto type = clamp(curDeviceType + dy, 0, static_cast<int>(deviceTypes.size()) - 1);
	if (type != curDeviceType) {
		curDeviceType = type;
		updateDeviceType();
	}
}

void InputSlotWidget::updateDeviceType()
{
	getWidgetAs<UILabel>("virtualDeviceName")->setText(LocalisedString::fromUserString(deviceTypes.at(curDeviceType).desc));
}

InputParkedDeviceWidget::InputParkedDeviceWidget(UIFactory& factory, std::shared_ptr<InputDevice> device, Colour4f colour)
	: UIWidget("parkedDevice", {}, UISizer())
	, device(std::move(device))
	, colour(colour)
{
	factory.loadUI(*this, "input_idle");
}

void InputParkedDeviceWidget::onMakeUI()
{
	const auto type = device->getInputType();
	//getWidgetAs<UILabel>("deviceName")->setText(LocalisedString::fromUserString(device->getName()));
	getWidget("iconGamepad")->setActive(type == InputType::Gamepad);
	getWidget("iconKeyboard")->setActive(type == InputType::Keyboard);
	getWidgetAs<UIImage>("bg")->getSprite().setColour(colour);
	getWidgetAs<UIImage>("arrow")->getSprite().setColour(colour);
}

