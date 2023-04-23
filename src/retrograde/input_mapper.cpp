#include "input_mapper.h"

#include "game_input_mapper.h"
#include "retrograde_environment.h"

InputMapper::InputMapper(RetrogradeEnvironment& retrogradeEnvironment)
	: retrogradeEnvironment(retrogradeEnvironment)
{
	uiInput = std::make_shared<InputVirtual>(13, 6);
}

void InputMapper::update(Time t)
{
	bindInputIfNeeded();
	uiInput->update(t);
	for (auto& [k, v]: uiInputForDevice) {
		v->update(t);
	}
}

std::shared_ptr<InputVirtual> InputMapper::getUIInput() const
{
	return uiInput;
}

std::shared_ptr<InputVirtual> InputMapper::getUIInputForDevice(InputDevice& device) const
{
	const auto iter = uiInputForDevice.find(&device);
	if (iter != uiInputForDevice.end()) {
		return iter->second;
	}
	return {};
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
	uiInputForDevice.clear();

	const auto& inputAPI = *retrogradeEnvironment.getHalleyAPI().input;

	const auto nJoy = inputAPI.getNumberOfJoysticks();
	for (int i = 0; i < static_cast<int>(nJoy); ++i) {
		const auto joy = inputAPI.getJoystick(i);
		if (joy && joy->isEnabled()) {
			auto forDevice = std::make_shared<InputVirtual>(13, 6);

			bindGamepad(*input, joy);
			bindGamepad(*forDevice, joy);

			uiInputForDevice[joy.get()] = std::move(forDevice);
		}
	}

	const auto nKey = inputAPI.getNumberOfKeyboards();
	for (int i = 0; i < static_cast<int>(nKey); ++i) {
		const auto kb = inputAPI.getKeyboard(i);
		if (kb) {
			auto forDevice = std::make_shared<InputVirtual>(13, 6);

			bindKeyboard(*input, kb);
			bindKeyboard(*forDevice, kb);

			uiInputForDevice[kb.get()] = std::move(forDevice);
		}
	}
}

void InputMapper::bindGamepad(InputVirtual& dst, std::shared_ptr<InputDevice> joy)
{
	dst.bindButton(UI_BUTTON_A, joy, joy->getButtonAtPosition(JoystickButtonPosition::PlatformAcceptButton));
	dst.bindButton(UI_BUTTON_B, joy, joy->getButtonAtPosition(JoystickButtonPosition::PlatformCancelButton));
	dst.bindButton(UI_BUTTON_X, joy, joy->getButtonAtPosition(JoystickButtonPosition::FaceLeft));
	dst.bindButton(UI_BUTTON_Y, joy, joy->getButtonAtPosition(JoystickButtonPosition::FaceTop));

	dst.bindButton(UI_BUTTON_BACK, joy, joy->getButtonAtPosition(JoystickButtonPosition::Select));
	dst.bindButton(UI_BUTTON_START, joy, joy->getButtonAtPosition(JoystickButtonPosition::Start));

	dst.bindButton(UI_BUTTON_BUMPER_LEFT, joy, joy->getButtonAtPosition(JoystickButtonPosition::BumperLeft));
	dst.bindButton(UI_BUTTON_BUMPER_RIGHT, joy, joy->getButtonAtPosition(JoystickButtonPosition::BumperRight));

	dst.bindButton(UI_BUTTON_TRIGGER_LEFT, joy, joy->getButtonAtPosition(JoystickButtonPosition::TriggerLeft));
	dst.bindButton(UI_BUTTON_TRIGGER_RIGHT, joy, joy->getButtonAtPosition(JoystickButtonPosition::TriggerRight));
	dst.bindButton(UI_BUTTON_STICK_LEFT, joy, joy->getButtonAtPosition(JoystickButtonPosition::LeftStick));
	dst.bindButton(UI_BUTTON_STICK_RIGHT, joy, joy->getButtonAtPosition(JoystickButtonPosition::RightStick));
	dst.bindButton(UI_BUTTON_SYSTEM, joy, joy->getButtonAtPosition(JoystickButtonPosition::System));
	dst.bindButtonChord(UI_BUTTON_SYSTEM, joy, joy->getButtonAtPosition(JoystickButtonPosition::TriggerLeft), joy->getButtonAtPosition(JoystickButtonPosition::TriggerRight));

	for (int j = 0; j < 6; ++j) {
		dst.bindAxis(j, joy, j);
	}

	dst.bindAxisButton(UI_AXIS_LEFT_X, joy, joy->getButtonAtPosition(JoystickButtonPosition::DPadLeft), joy->getButtonAtPosition(JoystickButtonPosition::DPadRight));
	dst.bindAxisButton(UI_AXIS_LEFT_Y, joy, joy->getButtonAtPosition(JoystickButtonPosition::DPadUp), joy->getButtonAtPosition(JoystickButtonPosition::DPadDown));
}

void InputMapper::bindKeyboard(InputVirtual& dst, std::shared_ptr<InputDevice> kb)
{
	dst.bindButton(UI_BUTTON_A, kb, KeyCode::Enter, KeyMods::None);
	dst.bindButton(UI_BUTTON_B, kb, KeyCode::Esc, KeyMods::None);
	dst.bindButton(UI_BUTTON_SYSTEM, kb, KeyCode::F1, KeyMods::None);

	dst.bindAxisButton(UI_AXIS_LEFT_X, kb, KeyCode::Left, KeyCode::Right);
	dst.bindAxisButton(UI_AXIS_LEFT_Y, kb, KeyCode::Up, KeyCode::Down);
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

	const auto nKey = inputAPI.getNumberOfKeyboards();
	for (int i = 0; i < static_cast<int>(nKey); ++i) {
		const auto kb = inputAPI.getKeyboard(i);
		if (kb) {
			hasher.feed(kb.get()); // Feed ptr
		}
	}

	return hasher.digest();
}
