#include "input_mapper.h"

#include "game_input_mapper.h"
#include "retrograde_environment.h"

InputMapper::InputMapper(RetrogradeEnvironment& retrogradeEnvironment)
	: retrogradeEnvironment(retrogradeEnvironment)
{
	uiInput = std::make_shared<InputVirtual>(13, 6);
}

void InputMapper::update()
{
	bindInputIfNeeded();
}

std::shared_ptr<InputVirtual> InputMapper::getUIInput()
{
	return uiInput;
}

std::shared_ptr<GameInputMapper> InputMapper::makeGameInputMapper(const SystemConfig& system)
{
	return std::make_shared<GameInputMapper>(retrogradeEnvironment, *this, system);
}

void InputMapper::bindInputIfNeeded()
{
	const auto curHash = getInputHash();
	if (curHash != lastInputHash) {
		lastInputHash = curHash;
		bindInput();
	}
}

void InputMapper::bindInput()
{
	auto& input = uiInput;
	input->clearBindings();

	auto& inputAPI = *retrogradeEnvironment.getHalleyAPI().input;
	const auto nJoy = inputAPI.getNumberOfJoysticks();

	for (int i = 0; i < static_cast<int>(nJoy); ++i) {
		const auto joy = inputAPI.getJoystick(i);
		if (joy && joy->isEnabled()) {
			input->bindButton(UI_BUTTON_A, joy, joy->getButtonAtPosition(JoystickButtonPosition::PlatformAcceptButton));
			input->bindButton(UI_BUTTON_B, joy, joy->getButtonAtPosition(JoystickButtonPosition::PlatformCancelButton));
			input->bindButton(UI_BUTTON_X, joy, joy->getButtonAtPosition(JoystickButtonPosition::FaceLeft));
			input->bindButton(UI_BUTTON_Y, joy, joy->getButtonAtPosition(JoystickButtonPosition::FaceTop));

			input->bindButton(UI_BUTTON_BACK, joy, joy->getButtonAtPosition(JoystickButtonPosition::Select));
			input->bindButton(UI_BUTTON_START, joy, joy->getButtonAtPosition(JoystickButtonPosition::Start));

			input->bindButton(UI_BUTTON_BUMPER_LEFT, joy, joy->getButtonAtPosition(JoystickButtonPosition::BumperLeft));
			input->bindButton(UI_BUTTON_BUMPER_RIGHT, joy, joy->getButtonAtPosition(JoystickButtonPosition::BumperRight));

			input->bindButton(UI_BUTTON_TRIGGER_LEFT, joy, joy->getButtonAtPosition(JoystickButtonPosition::TriggerLeft));
			input->bindButton(UI_BUTTON_TRIGGER_RIGHT, joy, joy->getButtonAtPosition(JoystickButtonPosition::TriggerRight));
			input->bindButton(UI_BUTTON_STICK_LEFT, joy, joy->getButtonAtPosition(JoystickButtonPosition::LeftStick));
			input->bindButton(UI_BUTTON_STICK_RIGHT, joy, joy->getButtonAtPosition(JoystickButtonPosition::RightStick));
			input->bindButton(UI_BUTTON_SYSTEM, joy, joy->getButtonAtPosition(JoystickButtonPosition::System));

			for (int j = 0; j < 6; ++j) {
				input->bindAxis(j, joy, j);
			}

			input->bindAxisButton(UI_AXIS_LEFT_X, joy, joy->getButtonAtPosition(JoystickButtonPosition::DPadLeft), joy->getButtonAtPosition(JoystickButtonPosition::DPadRight));
			input->bindAxisButton(UI_AXIS_LEFT_Y, joy, joy->getButtonAtPosition(JoystickButtonPosition::DPadUp), joy->getButtonAtPosition(JoystickButtonPosition::DPadDown));
		}
	}

	auto kb = retrogradeEnvironment.getHalleyAPI().input->getKeyboard();
	if (kb) {
		input->bindButton(UI_BUTTON_A, kb, KeyCode::Enter, KeyMods::None);
		input->bindButton(UI_BUTTON_B, kb, KeyCode::Esc, KeyMods::None);
		input->bindButton(UI_BUTTON_SYSTEM, kb, KeyCode::F1, KeyMods::None);

		input->bindAxisButton(UI_AXIS_LEFT_X, kb, KeyCode::Left, KeyCode::Right);
		input->bindAxisButton(UI_AXIS_LEFT_Y, kb, KeyCode::Up, KeyCode::Down);
	}
}

uint64_t InputMapper::getInputHash() const
{
	const auto& inputAPI = *retrogradeEnvironment.getHalleyAPI().input;

	Hash::Hasher hasher;

	const auto nJoy = inputAPI.getNumberOfJoysticks();
	for (int i = 0; i < static_cast<int>(nJoy); ++i) {
		const auto joy = inputAPI.getJoystick(i);
		if (joy && joy->isEnabled()) {
			hasher.feed(String(joy->getName())); // Feed name
			hasher.feed(joy.get()); // Feed ptr
		}
	}

	const auto kb = retrogradeEnvironment.getHalleyAPI().input->getKeyboard();
	if (kb) {
		hasher.feed(kb.get()); // Feed ptr
	}

	return hasher.digest();
}
