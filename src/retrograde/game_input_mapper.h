#pragma once

#include <halley.hpp>

#include "src/libretro/libretro_core.h"
class InputMapper;
class RetrogradeEnvironment;
class SystemConfig;
class Settings;
using namespace Halley;

class GameInputMapper {
public:
	
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

	GameInputMapper(RetrogradeEnvironment& environment, InputMapper& inputMapper, const SystemConfig& systemConfig);

	void update();
	void chooseBestAssignments();
	void setAssignmentsFixed(bool fixed);
	void bindCore(LibretroCore& core);

private:
	struct Assignment {
		std::shared_ptr<InputDevice> srcDevice;
		bool present = false;
		int priority = 0;
		InputType type = InputType::None;

		bool operator<(const Assignment& other) const;
	};

	RetrogradeEnvironment& retrogradeEnvironment;
	InputMapper& inputMapper;
	const SystemConfig& systemConfig;

	Vector<std::shared_ptr<InputVirtual>> gameInput;
	Vector<Assignment> assignments;

	bool assignmentsChanged = false;
	bool assignmentsFixed = false;

	void assignJoysticks();
	void assignDevice(std::shared_ptr<InputDevice> device, InputType type);
	void bindInputJoystick(std::shared_ptr<InputVirtual> dst, std::shared_ptr<InputDevice> joy);
	void bindInputKeyboard(std::shared_ptr<InputVirtual> dst, std::shared_ptr<InputDevice> kb);
	void bindDevices();
	std::optional<int> findAssignment(std::shared_ptr<InputDevice> device, bool tryNew);

};
