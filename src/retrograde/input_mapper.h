#pragma once

#include <halley.hpp>
class GameInputMapper;
class RetrogradeEnvironment;
class SystemConfig;
class Settings;
using namespace Halley;

class InputMapper {
public:
	enum UIButtons {
		UI_BUTTON_A,
		UI_BUTTON_B,
		UI_BUTTON_X,
		UI_BUTTON_Y,

		UI_BUTTON_BACK,
		UI_BUTTON_START,

		UI_BUTTON_BUMPER_LEFT,
		UI_BUTTON_BUMPER_RIGHT,
		UI_BUTTON_TRIGGER_LEFT,
		UI_BUTTON_TRIGGER_RIGHT,

		UI_BUTTON_STICK_LEFT,
		UI_BUTTON_STICK_RIGHT,

		UI_BUTTON_SYSTEM,
	};

	enum UIAxes {
		UI_AXIS_LEFT_X,
		UI_AXIS_LEFT_Y,
		UI_AXIS_RIGHT_X,
		UI_AXIS_RIGHT_Y,
		UI_AXIS_TRIGGER_LEFT,
		UI_AXIS_TRIGGER_RIGHT
	};

	InputMapper(RetrogradeEnvironment& retrogradeEnvironment);

	void update(Time t);

	std::shared_ptr<InputVirtual> getUIInput() const;
	std::shared_ptr<InputVirtual> getUIInputForDevice(InputDevice& device) const;
	std::shared_ptr<GameInputMapper> makeGameInputMapper(const SystemConfig& system);

private:
	RetrogradeEnvironment& retrogradeEnvironment;

	std::shared_ptr<InputVirtual> uiInput;
	HashMap<InputDevice*, std::shared_ptr<InputVirtual>> uiInputForDevice;
	uint64_t lastInputHash;

	void bindInputIfNeeded();
	void bindInput();
	void bindGamepad(InputVirtual& dst, std::shared_ptr<InputDevice> src);
	void bindKeyboard(InputVirtual& dst, std::shared_ptr<InputDevice> src);
	uint64_t getInputHash() const;
};
