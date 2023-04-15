#pragma once

#include <halley.hpp>
class RetrogradeEnvironment;
class SystemConfig;
class Settings;
using namespace Halley;

class GameInputMapper {
public:

};

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

	enum LibretroButtons {
		LIBRETRO_BUTTON_UP,
		LIBRETRO_BUTTON_DOWN,
		LIBRETRO_BUTTON_LEFT,
		LIBRETRO_BUTTON_RIGHT,

		LIBRETRO_BUTTON_B,
		LIBRETRO_BUTTON_A,
		LIBRETRO_BUTTON_Y,
		LIBRETRO_BUTTON_X,

		LIBRETRO_BUTTON_BACK,
		LIBRETRO_BUTTON_START,

		LIBRETRO_BUTTON_BUMPER_LEFT,
		LIBRETRO_BUTTON_BUMPER_RIGHT,
		LIBRETRO_BUTTON_TRIGGER_LEFT,
		LIBRETRO_BUTTON_TRIGGER_RIGHT,

		LIBRETRO_BUTTON_STICK_LEFT,
		LIBRETRO_BUTTON_STICK_RIGHT,

		LIBRETRO_BUTTON_SYSTEM,
	};

	enum LibretroAxes {
		LIBRETRO_AXIS_LEFT_X,
		LIBRETRO_AXIS_LEFT_Y,
		LIBRETRO_AXIS_RIGHT_X,
		LIBRETRO_AXIS_RIGHT_Y,
		LIBRETRO_AXIS_TRIGGER_LEFT,
		LIBRETRO_AXIS_TRIGGER_RIGHT
	};

	InputMapper(RetrogradeEnvironment& retrogradeEnvironment);

	void update();
	void chooseBestAssignments(const SystemConfig& systemConfig);
	void setAssignmentsFixed(bool fixed);

	std::shared_ptr<InputVirtual> getInput(int idx);
	std::shared_ptr<InputVirtual> getUIInput();

private:
	struct Assignment {
		std::shared_ptr<InputDevice> srcDevice;
		bool present = false;
		int priority = 0;
		InputType type = InputType::None;

		bool operator<(const Assignment& other) const;
	};

	RetrogradeEnvironment& retrogradeEnvironment;

	std::shared_ptr<InputVirtual> uiInput;
	Vector<std::shared_ptr<InputVirtual>> gameInput;
	Vector<Assignment> assignments;

	bool assignmentsChanged = false;
	bool assignmentsFixed = false;

	void assignJoysticks();
	void assignDevice(std::shared_ptr<InputDevice> device, InputType type);
	void bindInputJoystick(std::shared_ptr<InputVirtual> dst, std::shared_ptr<InputDevice> joy, const String& systemId);
	void bindInputKeyboard(std::shared_ptr<InputVirtual> dst, std::shared_ptr<InputDevice> kb);
	void bindUIInput();
	void bindDevices(const String& systemId);
	std::optional<int> findAssignment(std::shared_ptr<InputDevice> device, bool tryNew);
};
