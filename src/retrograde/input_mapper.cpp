#include "input_mapper.h"

InputMapper::InputMapper(InputAPI& inputAPI, Settings& settings)
	: inputAPI(inputAPI)
	, settings(settings)
{
	gameInput.resize(8);
	for (auto& input: gameInput) {
		input.input = std::make_shared<InputVirtual>(17, 6);
	}
	uiInput = std::make_shared<InputVirtual>(13, 6);
	bindUIInput();

	assignJoysticks();
}

void InputMapper::update()
{
	assignJoysticks();
}

std::shared_ptr<InputVirtual> InputMapper::getInput(int idx)
{
	return gameInput.at(idx).input;
}

std::shared_ptr<InputVirtual> InputMapper::getUIInput()
{
	return uiInput;
}

void InputMapper::assignJoysticks()
{
	// Mark all assignments as missing
	for (auto& input: gameInput) {
		input.present = false;
	}

	// Build assignments
	const auto nJoy = inputAPI.getNumberOfJoysticks();
	for (size_t i = 0; i < nJoy; ++i) {
		assignDevice(inputAPI.getJoystick(static_cast<int>(i)));
	}

	// Remove items marked not present
	for (auto& input: gameInput) {
		if (!input.present && input.srcDevice) {
			input.srcDevice = {};
			input.input->clearBindings();
		}
	}
}

void InputMapper::assignDevice(std::shared_ptr<InputDevice> device)
{
	if (auto* assignment = findAssignment(device, device->isEnabled())) {
		if (device->isEnabled()) {
			assignment->present = true;
			if (assignment->srcDevice != device) {
				bindInputJoystick(assignment->input, device);
			}
		}
	}
}

void InputMapper::bindInputJoystick(std::shared_ptr<InputVirtual> input, std::shared_ptr<InputDevice> joy)
{
	input->clearBindings();

	if (joy) {
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
		input->bindButton(LIBRETRO_BUTTON_SYSTEM, joy, joy->getButtonAtPosition(JoystickButtonPosition::System));

		input->bindAxis(LIBRETRO_AXIS_LEFT_X, joy, 0);
		input->bindAxis(LIBRETRO_AXIS_LEFT_Y, joy, 1);
		input->bindAxis(LIBRETRO_AXIS_RIGHT_X, joy, 2);
		input->bindAxis(LIBRETRO_AXIS_RIGHT_Y, joy, 3);
		input->bindAxis(LIBRETRO_AXIS_TRIGGER_LEFT, joy, 4);
		input->bindAxis(LIBRETRO_AXIS_TRIGGER_RIGHT, joy, 5);
	}
}

void InputMapper::bindInputKeyboard(std::shared_ptr<InputVirtual> input, std::shared_ptr<InputDevice> kb)
{
	input->clearBindings();

	if (kb) {
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

void InputMapper::bindUIInput()
{
	auto& input = uiInput;
	input->clearBindings();

	for (int i = 0; i < 8; ++i) {
		const auto joy = gameInput[i].input;
		if (joy) {
			input->bindButton(UI_BUTTON_A, joy, LIBRETRO_BUTTON_A);
			input->bindButton(UI_BUTTON_B, joy, LIBRETRO_BUTTON_B);
			input->bindButton(UI_BUTTON_X, joy, LIBRETRO_BUTTON_X);
			input->bindButton(UI_BUTTON_Y, joy, LIBRETRO_BUTTON_Y);

			input->bindButton(UI_BUTTON_BACK, joy, LIBRETRO_BUTTON_BACK);
			input->bindButton(UI_BUTTON_START, joy, LIBRETRO_BUTTON_START);

			input->bindButton(UI_BUTTON_BUMPER_LEFT, joy, LIBRETRO_BUTTON_BUMPER_LEFT);
			input->bindButton(UI_BUTTON_BUMPER_RIGHT, joy, LIBRETRO_BUTTON_BUMPER_RIGHT);

			input->bindButton(UI_BUTTON_TRIGGER_LEFT, joy, LIBRETRO_BUTTON_TRIGGER_LEFT);
			input->bindButton(UI_BUTTON_TRIGGER_RIGHT, joy, LIBRETRO_BUTTON_TRIGGER_RIGHT);
			input->bindButton(UI_BUTTON_STICK_LEFT, joy, LIBRETRO_BUTTON_STICK_LEFT);
			input->bindButton(UI_BUTTON_STICK_RIGHT, joy, LIBRETRO_BUTTON_STICK_RIGHT);
			input->bindButton(UI_BUTTON_SYSTEM, joy, LIBRETRO_BUTTON_SYSTEM);

			for (int j = 0; j < 6; ++j) {
				input->bindAxis(j, joy, j);
			}

			input->bindAxisButton(UI_AXIS_LEFT_X, joy, LIBRETRO_BUTTON_LEFT, LIBRETRO_BUTTON_RIGHT);
			input->bindAxisButton(UI_AXIS_LEFT_Y, joy, LIBRETRO_BUTTON_UP, LIBRETRO_BUTTON_DOWN);
		}
	}

	auto kb = inputAPI.getKeyboard();
	if (kb) {
		input->bindButton(UI_BUTTON_A, kb, KeyCode::Enter, KeyMods::None);
		input->bindButton(UI_BUTTON_B, kb, KeyCode::Esc, KeyMods::None);
		input->bindButton(UI_BUTTON_SYSTEM, kb, KeyCode::F1, KeyMods::None);

		input->bindAxisButton(UI_AXIS_LEFT_X, kb, KeyCode::Left, KeyCode::Right);
		input->bindAxisButton(UI_AXIS_LEFT_Y, kb, KeyCode::Up, KeyCode::Down);
	}
}

InputMapper::Assignment* InputMapper::findAssignment(std::shared_ptr<InputDevice> device, bool tryNew)
{
	for (auto& input: gameInput) {
		if (input.srcDevice == device) {
			return &input;
		}
	}

	// Try first empty assignment
	if (tryNew) {
		for (auto& input : gameInput) {
			if (!input.srcDevice) {
				return &input;
			}
		}
	}

	// No assignment possible
	return nullptr;
}
