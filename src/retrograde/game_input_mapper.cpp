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
	assignments.resize(8);
	gameInput.resize(8);
	for (auto& input: gameInput) {
		input = std::make_shared<InputVirtual>(17, 6);
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
		const auto basePriority = input.type == InputType::Gamepad ? 1 : input.type == InputType::Keyboard ? 0 : -1;
		input.priority = input.assignedDevice.get() == last ? 2 : basePriority;
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
	for (int i = 0; i < 8; ++i) {
		core.setInputDevice(i, gameInput.at(i));
	}
}

bool GameInputMapper::Assignment::operator<(const Assignment& other) const
{
	return priority > other.priority;
}

void GameInputMapper::Assignment::clear()
{
	assignedDevice = {};
	boundDevice = {};
	type = InputType::None;
	present = false;
	priority = 0;
}

void GameInputMapper::assignJoysticks()
{
	// Mark all assignments as missing
	for (auto& input: assignments) {
		input.present = false;
	}

	// Build assignments
	const auto& inputAPI = *retrogradeEnvironment.getHalleyAPI().input;
	const auto nJoy = inputAPI.getNumberOfJoysticks();
	const auto nKey = inputAPI.getNumberOfKeyboards();
	for (size_t i = 0; i < nJoy; ++i) {
		assignDevice(inputAPI.getJoystick(static_cast<int>(i)), InputType::Gamepad);
	}
	for (size_t i = 0; i < nKey; ++i) {
		assignDevice(inputAPI.getKeyboard(static_cast<int>(i)), InputType::Keyboard);
	}

	// Remove items marked not present
	bool assignmentsRemoved = false;
	for (int i = 0; i < 8; ++i) {
		auto& assignment = assignments[i];
		auto& input = gameInput[i];
		if (!assignment.present && assignment.assignedDevice) {
			assignment.clear();
			assignmentsChanged = true;
			input->clearBindings();
			assignmentsRemoved = true;
		}
	}

	// Shift everything down
	if (assignmentsRemoved) {
		for (auto& assignment: assignments) {
			assignment.priority = assignment.assignedDevice ? 1 : 0;
		}
		std::sort(assignments.begin(), assignments.end());
	}

	// Bind devices
	bindDevices();
}

void GameInputMapper::assignDevice(std::shared_ptr<InputDevice> device, InputType type)
{
	if (device->isEnabled()) {
		if (auto assignmentId = findAssignment(device, device->isEnabled())) {
			auto& assignment = assignments[*assignmentId];
			assignment.present = true;
			assignment.type = type;
			if (assignment.assignedDevice != device) {
				assignment.assignedDevice = device;
				assignmentsChanged = true;
			}
		}
	}
}

void GameInputMapper::bindDevices()
{
	for (int i = 0; i < 8; ++i) {
		auto& assignment = assignments[i];
		auto& input = gameInput[i];

		if (assignment.boundDevice != assignment.assignedDevice) {
			if (assignment.type == InputType::Gamepad) {
				bindInputJoystick(input, assignment.assignedDevice);
			} else if (assignment.type == InputType::Keyboard) {
				bindInputKeyboard(input, assignment.assignedDevice);
			}
			assignment.boundDevice = assignment.assignedDevice;
		}
	}
}

std::optional<int> GameInputMapper::findAssignment(std::shared_ptr<InputDevice> device, bool tryNew)
{
	for (int i = 0; i < 8; ++i) {
		if (assignments[i].assignedDevice == device) {
			return i;
		}
	}

	// Try first empty assignment
	if (tryNew) {
		for (int i = 0; i < 8; ++i) {
			if (!assignments[i].assignedDevice) {
				return i;
			}
		}
	}

	// No assignment possible
	return {};
}

void GameInputMapper::bindInputJoystick(std::shared_ptr<InputVirtual> input, std::shared_ptr<InputDevice> joy)
{
	input->clearBindings();

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

		input->bindButton(LIBRETRO_BUTTON_B, joy, joy->getButtonAtPosition(JoystickButtonPosition::FaceRight));
		input->bindButton(LIBRETRO_BUTTON_A, joy, joy->getButtonAtPosition(JoystickButtonPosition::FaceBottom));
		input->bindButton(LIBRETRO_BUTTON_Y, joy, joy->getButtonAtPosition(JoystickButtonPosition::FaceTop));
		input->bindButton(LIBRETRO_BUTTON_X, joy, joy->getButtonAtPosition(JoystickButtonPosition::FaceLeft));

		input->bindButton(LIBRETRO_BUTTON_BACK, joy, joy->getButtonAtPosition(JoystickButtonPosition::Select));
		input->bindButton(LIBRETRO_BUTTON_START, joy, joy->getButtonAtPosition(JoystickButtonPosition::Start));

		input->bindButton(LIBRETRO_BUTTON_BUMPER_LEFT, joy, joy->getButtonAtPosition(JoystickButtonPosition::BumperLeft));
		input->bindButton(LIBRETRO_BUTTON_BUMPER_RIGHT, joy, joy->getButtonAtPosition(JoystickButtonPosition::BumperRight));
		input->bindButton(LIBRETRO_BUTTON_TRIGGER_LEFT, joy, joy->getButtonAtPosition(JoystickButtonPosition::TriggerLeft));
		input->bindButton(LIBRETRO_BUTTON_TRIGGER_RIGHT, joy, joy->getButtonAtPosition(JoystickButtonPosition::TriggerRight));

		input->bindButton(LIBRETRO_BUTTON_STICK_LEFT, joy, joy->getButtonAtPosition(JoystickButtonPosition::LeftStick));
		input->bindButton(LIBRETRO_BUTTON_STICK_RIGHT, joy, joy->getButtonAtPosition(JoystickButtonPosition::RightStick));

		input->bindAxis(LIBRETRO_AXIS_LEFT_X, joy, 0);
		input->bindAxis(LIBRETRO_AXIS_LEFT_Y, joy, 1);
		input->bindAxis(LIBRETRO_AXIS_RIGHT_X, joy, 2);
		input->bindAxis(LIBRETRO_AXIS_RIGHT_Y, joy, 3);
		input->bindAxis(LIBRETRO_AXIS_TRIGGER_LEFT, joy, 4);
		input->bindAxis(LIBRETRO_AXIS_TRIGGER_RIGHT, joy, 5);
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
	input->clearBindings();

	if (kb) {
		using namespace LibretroButtons;

		input->bindButton(LIBRETRO_BUTTON_UP, kb, KeyCode::Up);
		input->bindButton(LIBRETRO_BUTTON_DOWN, kb, KeyCode::Down);
		input->bindButton(LIBRETRO_BUTTON_LEFT, kb, KeyCode::Left);
		input->bindButton(LIBRETRO_BUTTON_RIGHT, kb, KeyCode::Right);

		input->bindButton(LIBRETRO_BUTTON_B, kb, KeyCode::A);
		input->bindButton(LIBRETRO_BUTTON_A, kb, KeyCode::S);
		input->bindButton(LIBRETRO_BUTTON_Y, kb, KeyCode::F);
		input->bindButton(LIBRETRO_BUTTON_X, kb, KeyCode::D);

		input->bindButton(LIBRETRO_BUTTON_BACK, kb, KeyCode::Space);
		input->bindButton(LIBRETRO_BUTTON_START, kb, KeyCode::Enter);

		input->bindButton(LIBRETRO_BUTTON_BUMPER_LEFT, kb, KeyCode::Q);
		input->bindButton(LIBRETRO_BUTTON_BUMPER_RIGHT, kb, KeyCode::W);

		input->bindButton(LIBRETRO_BUTTON_SYSTEM, kb, KeyCode::F1);
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
