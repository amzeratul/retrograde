#include "game_input_mapper.h"

#include "input_mapper.h"
#include "retrograde_environment.h"
#include "src/config/controller_config.h"
#include "src/config/mapping_config.h"
#include "src/libretro/libretro_core.h"

GameInputMapper::GameInputMapper(RetrogradeEnvironment& retrogradeEnvironment, InputMapper& inputMapper, const SystemConfig& systemConfig)
	: retrogradeEnvironment(retrogradeEnvironment)
	, inputMapper(inputMapper)
	, systemConfig(systemConfig)
{
	assignments.resize(numAssignments);
	gameInput.resize(numAssignments);
	for (auto& input: gameInput) {
		input.gameInput = std::make_shared<InputVirtual>(LibretroButtons::Buttons::LIBRETRO_BUTTON_MAX, LibretroButtons::Axes::LIBRETRO_AXIS_MAX);
	}

	if (!systemConfig.getMapping().isEmpty()) {
		mapping = &retrogradeEnvironment.getConfigDatabase().get<MappingConfig>(systemConfig.getMapping());
	}

	assignJoysticks();
}

void GameInputMapper::update()
{
	assignJoysticks();
}

void GameInputMapper::chooseBestAssignments()
{
	if (assignmentsFixed && !assignmentsChanged) {
		return;
	}

	// Sort based on last device used on UI, and device types
	const auto* last = inputMapper.getUIInput()->getLastDevice();
	for (auto& input: assignments) {
		const auto type = input.assignedDevices.empty() ? InputType::None : input.assignedDevices.front()->getInputType();
		const auto basePriority = type == InputType::Gamepad ? 1 : type == InputType::Keyboard ? 0 : -1;
		input.priority = !input.assignedDevices.empty() && input.assignedDevices.front().get() == last ? 2 : basePriority;
	}
	std::sort(assignments.begin(), assignments.end());

	// Unset fixed because if we got here with fixed set, it's because assignments changed
	assignmentsChanged = false;
	assignmentsFixed = false;

	bindDevices();
}

void GameInputMapper::setAssignmentsFixed(bool fixed)
{
	assignmentsFixed = fixed;
}

void GameInputMapper::bindCore(LibretroCore& core)
{
	for (int i = 0; i < numAssignments; ++i) {
		core.setInputDevice(i, gameInput.at(i).gameInput);
	}
}

void GameInputMapper::setScreenRect(Rect4f screen)
{
	if (screenRect != screen) {
		screenRect = screen;
		retrogradeEnvironment.getHalleyAPI().input->setMouseRemapping([screen] (Vector2i pos) -> Vector2f
		{
			return (Vector2f(pos) - screen.getTopLeft()) / (screen.getSize() - Vector2f(1, 1));
		});
	}
}

InputMapper& GameInputMapper::getInputMapper() const
{
	return inputMapper;
}

std::shared_ptr<InputDevice> GameInputMapper::getDeviceAt(int port) const
{
	if (port < 0 || port >= numAssignments) {
		return {};
	}
	return !assignments[port].assignedDevices.empty() ? assignments[port].assignedDevices[0] : std::shared_ptr<InputDevice>();
}

const Vector<std::shared_ptr<InputDevice>>& GameInputMapper::getUnassignedDevices() const
{
	return unassignedDevices;
}

const Vector<std::shared_ptr<InputDevice>>& GameInputMapper::getAllDevices() const
{
	return allDevices;
}

void GameInputMapper::assignDevice(std::shared_ptr<InputDevice> device, std::optional<int> port)
{
	// Unassign device
	for (auto& a: assignments) {
		if (a.hasAssignedDevice(device)) {
			a.clear();
		}
	}

	if (port && *port >= 0 && *port < numAssignments && assignments[*port].assignedDevices.empty()) {
		auto& dst = assignments[*port].assignedDevices;
		dst.push_back(device);
		if (devicePair.contains(device.get())) {
			dst.push_back(devicePair.at(device.get()));
		}
	}

	bindDevices();
}

std::optional<int> GameInputMapper::moveDevice(const std::shared_ptr<InputDevice>& device, int dx, int maxPorts)
{
	int startPos = -1;
	for (int i = 0; i < numAssignments; ++i) {
		if (assignments[i].hasAssignedDevice(device)) {
			startPos = i;
			break;
		}
	}

	int pos = startPos;
	if (dx < 0 && pos >= 0) {
		// Move to the left
		bool found = false;
		for (int i = pos; --i >= 0;) {
			if (assignments[i].assignedDevices.empty()) {
				pos = i;
				found = true;
				break;
			}
		}
		if (!found) {
			pos = -1;
		}
	} else if (dx > 0) {
		// Move to the right
		for (int i = pos + 1; i < std::min(numAssignments, maxPorts); ++i) {
			if (assignments[i].assignedDevices.empty()) {
				pos = i;
				break;
			}
		}
	}

	const auto result = pos >= 0 ? std::optional<int>(pos) : std::nullopt;
	if (pos != startPos) {
		assignDevice(device, result);
	}
	return result;
}

Colour4f GameInputMapper::getDeviceColour(const InputDevice* device) const
{
	if (!device) {
		return {};
	}
	const char* colours[] = { "#303F9F", "#9F307A", "#9F8730", "#9F3930", "#539F30", "#508EAB", "#C4516B", "#CB9236", "#682E2E", "#268242" };
	const auto iter = std_ex::find_if(allDevices, [&](auto& d) { return d.get() == device; });
	if (iter == allDevices.end()) {
		return {};
	}
	const auto idx = iter - allDevices.begin();
	return Colour4f::fromString(colours[idx % 10]);
}

String GameInputMapper::getDeviceName(const InputDevice* device) const
{
	if (!device) {
		return {};
	}
	String name = device->getName();
	if (devicePair.contains(device)) {
		name = name + " + " + devicePair.at(device)->getName();
	}
	return name;
}

bool GameInputMapper::Assignment::operator<(const Assignment& other) const
{
	return priority > other.priority;
}

void GameInputMapper::Assignment::clear()
{
	assignedDevices.clear();
	present = false;
	priority = 0;
}

bool GameInputMapper::Assignment::hasAssignedDevice(const std::shared_ptr<InputDevice>& device) const
{
	return std_ex::contains(assignedDevices, device);
}

void GameInputMapper::Binding::clear()
{
	boundDevices.clear();
	gameInput = {};
}

void GameInputMapper::assignJoysticks()
{
	// Mark all assignments as missing
	for (auto& input: assignments) {
		input.present = false;
	}

	// Prepare all devices and unassigned devices list
	allDevicesPresence.resize(allDevices.size());
	for (auto& d: allDevicesPresence) {
		d = false;
	}
	unassignedDevices.clear();
	devicePair.clear();

	// Build assignments
	const auto& inputAPI = *retrogradeEnvironment.getHalleyAPI().input;
	const auto nJoy = inputAPI.getNumberOfJoysticks();
	const auto nKey = inputAPI.getNumberOfKeyboards();
	for (size_t i = 0; i < nJoy; ++i) {
		assignDevice(inputAPI.getJoystick(static_cast<int>(i)));
	}
	for (size_t i = 0; i < nKey; ++i) {
		auto kb = inputAPI.getKeyboard(static_cast<int>(i));
		auto mouse = inputAPI.getMouse(static_cast<int>(i));
		devicePair[kb.get()] = mouse;
		assignDevice(kb);
	}

	// Remove items marked not present
	bool assignmentsRemoved = false;
	for (int i = 0; i < numAssignments; ++i) {
		auto& assignment = assignments[i];
		auto& input = gameInput[i];
		if (!assignment.present && !assignment.assignedDevices.empty()) {
			assignment.clear();
			input.clear();
			assignmentsChanged = true;
			assignmentsRemoved = true;
		}
	}

	// Remove all devices lost
	for (size_t i = 0; i < allDevices.size(); ++i) {
		if (!allDevicesPresence[i]) {
			allDevices[i] = {};
		}
	}

	// Shift everything down
	if (assignmentsRemoved) {
		for (auto& assignment: assignments) {
			assignment.priority = assignment.assignedDevices.empty() ? 0 : 1;
		}
		std::sort(assignments.begin(), assignments.end());
	}

	// Bind devices
	bindDevices();
}

void GameInputMapper::assignDevice(std::shared_ptr<InputDevice> device)
{
	if (device->isEnabled()) {
		if (auto assignmentId = findAssignment(device, device->isEnabled())) {
			auto& assignment = assignments[*assignmentId];
			assignment.present = true;
			if (!assignment.hasAssignedDevice(device)) {
				assignment.assignedDevices.clear();
				assignment.assignedDevices.push_back(device);
				if (devicePair.contains(device.get())) {
					assignment.assignedDevices.push_back(devicePair.at(device.get()));
				}
				assignmentsChanged = true;
			}
		} else {
			unassignedDevices.push_back(device);
		}

		// Insert into all devices list
		if (const auto iter = std_ex::find(allDevices, device); iter != allDevices.end()) {
			const auto idx = iter - allDevices.begin();
			allDevicesPresence[idx] = true;
		} else {
			const auto emptyIter = std_ex::find(allDevices, std::shared_ptr<InputDevice>());
			if (emptyIter != allDevices.end()) {
				const auto idx = emptyIter - allDevices.begin();
				allDevices[idx] = device;
				allDevicesPresence[idx] = true;
			} else {
				allDevices.push_back(device);
				allDevicesPresence.push_back(true);
			}
		}
	}
}

void GameInputMapper::bindDevices()
{
	for (int i = 0; i < numAssignments; ++i) {
		auto& assignment = assignments[i];
		auto& input = gameInput[i];

		if (input.boundDevices != assignment.assignedDevices) {
			input.gameInput->clearBindings();
			for (auto& device: assignment.assignedDevices) {
				if (device->getInputType() == InputType::Gamepad) {
					bindInputJoystick(input.gameInput, device);
				} else if (device->getInputType() == InputType::Keyboard) {
					bindInputKeyboard(input.gameInput, device);
				} else if (device->getInputType() == InputType::Mouse) {
					bindInputMouse(input.gameInput, device);
				}
			}
			input.boundDevices = assignment.assignedDevices;
		}
	}
}

std::optional<int> GameInputMapper::findAssignment(std::shared_ptr<InputDevice> device, bool tryNew)
{
	for (int i = 0; i < numAssignments; ++i) {
		if (assignments[i].hasAssignedDevice(device)) {
			return i;
		}
	}

	// Try first empty assignment
	if (tryNew && !assignmentsFixed) {
		for (int i = 0; i < numAssignments; ++i) {
			if (assignments[i].assignedDevices.empty()) {
				return i;
			}
		}
	}

	// No assignment possible
	return {};
}

void GameInputMapper::bindInputJoystick(std::shared_ptr<InputVirtual> input, std::shared_ptr<InputDevice> joy)
{
	if (joy) {
		using namespace LibretroButtons;

		input->bindButton(LIBRETRO_BUTTON_SYSTEM, joy, joy->getButtonAtPosition(JoystickButtonPosition::System));

		if (mapping) {
			const auto type = getControllerType(joy->getName());
			if (mapping->hasMapping(type)) {
				bindInputJoystickWithMapping(input, joy, mapping->getMapping(type));
				return;
			}
		}

		input->bindButton(LIBRETRO_BUTTON_UP, joy, joy->getButtonAtPosition(JoystickButtonPosition::DPadUp));
		input->bindButton(LIBRETRO_BUTTON_DOWN, joy, joy->getButtonAtPosition(JoystickButtonPosition::DPadDown));
		input->bindButton(LIBRETRO_BUTTON_LEFT, joy, joy->getButtonAtPosition(JoystickButtonPosition::DPadLeft));
		input->bindButton(LIBRETRO_BUTTON_RIGHT, joy, joy->getButtonAtPosition(JoystickButtonPosition::DPadRight));

		// NB: Libretro uses SNES assignments
		input->bindButton(LIBRETRO_BUTTON_A, joy, joy->getButtonAtPosition(JoystickButtonPosition::FaceRight));
		input->bindButton(LIBRETRO_BUTTON_B, joy, joy->getButtonAtPosition(JoystickButtonPosition::FaceBottom));
		input->bindButton(LIBRETRO_BUTTON_X, joy, joy->getButtonAtPosition(JoystickButtonPosition::FaceTop));
		input->bindButton(LIBRETRO_BUTTON_Y, joy, joy->getButtonAtPosition(JoystickButtonPosition::FaceLeft));

		input->bindButton(LIBRETRO_BUTTON_SELECT, joy, joy->getButtonAtPosition(JoystickButtonPosition::Select));
		input->bindButton(LIBRETRO_BUTTON_START, joy, joy->getButtonAtPosition(JoystickButtonPosition::Start));

		input->bindButton(LIBRETRO_BUTTON_L, joy, joy->getButtonAtPosition(JoystickButtonPosition::BumperLeft));
		input->bindButton(LIBRETRO_BUTTON_R, joy, joy->getButtonAtPosition(JoystickButtonPosition::BumperRight));
		input->bindButton(LIBRETRO_BUTTON_L2, joy, joy->getButtonAtPosition(JoystickButtonPosition::TriggerLeft));
		input->bindButton(LIBRETRO_BUTTON_R2, joy, joy->getButtonAtPosition(JoystickButtonPosition::TriggerRight));
		input->bindButton(LIBRETRO_BUTTON_L3, joy, joy->getButtonAtPosition(JoystickButtonPosition::LeftStick));
		input->bindButton(LIBRETRO_BUTTON_R3, joy, joy->getButtonAtPosition(JoystickButtonPosition::RightStick));

		input->bindButton(LIBRETRO_BUTTON_MOUSE_LEFT, joy, joy->getButtonAtPosition(JoystickButtonPosition::FaceBottom));
		input->bindButton(LIBRETRO_BUTTON_MOUSE_RIGHT, joy, joy->getButtonAtPosition(JoystickButtonPosition::FaceRight));
		input->bindButton(LIBRETRO_BUTTON_MOUSE_MIDDLE, joy, joy->getButtonAtPosition(JoystickButtonPosition::FaceLeft));
		input->bindButton(LIBRETRO_BUTTON_MOUSE_4, joy, joy->getButtonAtPosition(JoystickButtonPosition::FaceTop));
		input->bindButton(LIBRETRO_BUTTON_MOUSE_5, joy, joy->getButtonAtPosition(JoystickButtonPosition::Start));
		input->bindButton(LIBRETRO_BUTTON_MOUSE_WHEEL_DOWN, joy, joy->getButtonAtPosition(JoystickButtonPosition::BumperLeft));
		input->bindButton(LIBRETRO_BUTTON_MOUSE_WHEEL_UP, joy, joy->getButtonAtPosition(JoystickButtonPosition::BumperRight));
		input->bindButton(LIBRETRO_BUTTON_MOUSE_WHEEL_LEFT, joy, joy->getButtonAtPosition(JoystickButtonPosition::TriggerLeft));
		input->bindButton(LIBRETRO_BUTTON_MOUSE_WHEEL_RIGHT, joy, joy->getButtonAtPosition(JoystickButtonPosition::TriggerRight));

		input->bindAxis(LIBRETRO_AXIS_LEFT_X, joy, 0);
		input->bindAxis(LIBRETRO_AXIS_LEFT_Y, joy, 1);
		input->bindAxis(LIBRETRO_AXIS_RIGHT_X, joy, 2);
		input->bindAxis(LIBRETRO_AXIS_RIGHT_Y, joy, 3);
		input->bindAxis(LIBRETRO_AXIS_TRIGGER_LEFT, joy, 4);
		input->bindAxis(LIBRETRO_AXIS_TRIGGER_RIGHT, joy, 5);

		const float mouseSpeed = 5.0f;
		input->bindAxis(LIBRETRO_AXIS_MOUSE_X, joy, 0, mouseSpeed);
		input->bindAxis(LIBRETRO_AXIS_MOUSE_X, joy, 2, mouseSpeed);
		input->bindAxis(LIBRETRO_AXIS_MOUSE_Y, joy, 1, mouseSpeed);
		input->bindAxis(LIBRETRO_AXIS_MOUSE_Y, joy, 3, mouseSpeed);
	}
}

void GameInputMapper::bindInputJoystickWithMapping(std::shared_ptr<InputVirtual> input, std::shared_ptr<InputDevice> joy, const MappingConfig::Mapping& mapping)
{
	for (const auto& [axisId, entry]: mapping.axes) {
		if (entry.axis) {
			input->bindAxis(axisId, joy, entry.a);
		} else {
			input->bindAxisButton(axisId, joy, joy->getButtonAtPosition(static_cast<JoystickButtonPosition>(entry.a)), joy->getButtonAtPosition(static_cast<JoystickButtonPosition>(entry.b)));
		}
	}
	for (const auto& [buttonId, entry]: mapping.buttons) {
		if (entry.axis) {
			// ??
		} else {
			input->bindButton(buttonId, joy, joy->getButtonAtPosition(static_cast<JoystickButtonPosition>(entry.a)));
		}
	}
}

void GameInputMapper::bindInputKeyboard(std::shared_ptr<InputVirtual> input, std::shared_ptr<InputDevice> kb)
{
	if (kb) {
		using namespace LibretroButtons;

		input->bindButton(LIBRETRO_BUTTON_UP, kb, KeyCode::Up);
		input->bindButton(LIBRETRO_BUTTON_DOWN, kb, KeyCode::Down);
		input->bindButton(LIBRETRO_BUTTON_LEFT, kb, KeyCode::Left);
		input->bindButton(LIBRETRO_BUTTON_RIGHT, kb, KeyCode::Right);

		// NB: Libretro uses SNES assignments
		input->bindButton(LIBRETRO_BUTTON_A, kb, KeyCode::A);
		input->bindButton(LIBRETRO_BUTTON_B, kb, KeyCode::S);
		input->bindButton(LIBRETRO_BUTTON_X, kb, KeyCode::F);
		input->bindButton(LIBRETRO_BUTTON_Y, kb, KeyCode::D);

		input->bindButton(LIBRETRO_BUTTON_SELECT, kb, KeyCode::Space);
		input->bindButton(LIBRETRO_BUTTON_START, kb, KeyCode::Enter);

		input->bindButton(LIBRETRO_BUTTON_L, kb, KeyCode::Q);
		input->bindButton(LIBRETRO_BUTTON_R, kb, KeyCode::W);

		input->bindButton(LIBRETRO_BUTTON_SYSTEM, kb, KeyCode::F1);
	}
}

void GameInputMapper::bindInputMouse(std::shared_ptr<InputVirtual> input, std::shared_ptr<InputDevice> mouse)
{
	if (mouse) {
		using namespace LibretroButtons;

		input->bindButton(LIBRETRO_BUTTON_MOUSE_LEFT, mouse, 0);
		input->bindButton(LIBRETRO_BUTTON_MOUSE_MIDDLE, mouse, 1);
		input->bindButton(LIBRETRO_BUTTON_MOUSE_RIGHT, mouse, 2);
		input->bindButton(LIBRETRO_BUTTON_MOUSE_4, mouse, 3);
		input->bindButton(LIBRETRO_BUTTON_MOUSE_5, mouse, 4);

		input->bindButton(LIBRETRO_BUTTON_LIGHTGUN_TRIGGER, mouse, 0);
		input->bindButton(LIBRETRO_BUTTON_LIGHTGUN_B, mouse, 1);
		input->bindButton(LIBRETRO_BUTTON_LIGHTGUN_A, mouse, 2);
		input->bindButton(LIBRETRO_BUTTON_LIGHTGUN_C, mouse, 3);

		const float mouseSpeed = 1.0f;
		input->bindAxis(LIBRETRO_AXIS_MOUSE_X, mouse, 0, mouseSpeed);
		input->bindAxis(LIBRETRO_AXIS_MOUSE_Y, mouse, 1, mouseSpeed);

		input->bindAxis(LIBRETRO_AXIS_LIGHTGUN_X, mouse, 2, mouseSpeed);
		input->bindAxis(LIBRETRO_AXIS_LIGHTGUN_Y, mouse, 3, mouseSpeed);
	}
}

String GameInputMapper::getControllerType(const String& name)
{
	String bestResult;
	for (const auto& [k, v]: retrogradeEnvironment.getConfigDatabase().getEntries<ControllerConfig>()) {
		if (v.matches(name)) {
			return v.getId();
		} else if (v.isDefault()) {
			bestResult = v.getId();
		}
	}
	return bestResult;
}
