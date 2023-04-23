#pragma once

#include <halley.hpp>
using namespace Halley;

namespace LibretroButtons {
	enum Buttons {
		LIBRETRO_BUTTON_UP,
		LIBRETRO_BUTTON_DOWN,
		LIBRETRO_BUTTON_LEFT,
		LIBRETRO_BUTTON_RIGHT,

		LIBRETRO_BUTTON_A,
		LIBRETRO_BUTTON_B,
		LIBRETRO_BUTTON_X,
		LIBRETRO_BUTTON_Y,

		LIBRETRO_BUTTON_SELECT,
		LIBRETRO_BUTTON_START,

		LIBRETRO_BUTTON_L,
		LIBRETRO_BUTTON_R,
		LIBRETRO_BUTTON_L2,
		LIBRETRO_BUTTON_R2,

		LIBRETRO_BUTTON_L3,
		LIBRETRO_BUTTON_R3,

		LIBRETRO_BUTTON_SYSTEM,

		LIBRETRO_BUTTON_MOUSE_LEFT,
		LIBRETRO_BUTTON_MOUSE_RIGHT,
		LIBRETRO_BUTTON_MOUSE_MIDDLE,
		LIBRETRO_BUTTON_MOUSE_4,
		LIBRETRO_BUTTON_MOUSE_5,
		LIBRETRO_BUTTON_MOUSE_WHEEL_UP,
		LIBRETRO_BUTTON_MOUSE_WHEEL_DOWN,
		LIBRETRO_BUTTON_MOUSE_WHEEL_LEFT,
		LIBRETRO_BUTTON_MOUSE_WHEEL_RIGHT,

		LIBRETRO_BUTTON_LIGHTGUN_TRIGGER,
		LIBRETRO_BUTTON_LIGHTGUN_A,
		LIBRETRO_BUTTON_LIGHTGUN_B,
		LIBRETRO_BUTTON_LIGHTGUN_C,
		LIBRETRO_BUTTON_LIGHTGUN_START,
		LIBRETRO_BUTTON_LIGHTGUN_SELECT,
		LIBRETRO_BUTTON_LIGHTGUN_IS_OFFSCREEN,
		LIBRETRO_BUTTON_LIGHTGUN_RELOAD,
		LIBRETRO_BUTTON_LIGHTGUN_DPAD_UP,
		LIBRETRO_BUTTON_LIGHTGUN_DPAD_DOWN,
		LIBRETRO_BUTTON_LIGHTGUN_DPAD_LEFT,
		LIBRETRO_BUTTON_LIGHTGUN_DPAD_RIGHT,

		LIBRETRO_BUTTON_MAX
	};

	enum Axes {
		LIBRETRO_AXIS_LEFT_X,
		LIBRETRO_AXIS_LEFT_Y,
		LIBRETRO_AXIS_RIGHT_X,
		LIBRETRO_AXIS_RIGHT_Y,
		LIBRETRO_AXIS_TRIGGER_LEFT,
		LIBRETRO_AXIS_TRIGGER_RIGHT,
		LIBRETRO_AXIS_MOUSE_X,
		LIBRETRO_AXIS_MOUSE_Y,
		LIBRETRO_AXIS_LIGHTGUN_X,
		LIBRETRO_AXIS_LIGHTGUN_Y,

		LIBRETRO_AXIS_MAX
	};
}


class MappingConfig {
public:
	enum class AxisSign : uint8_t {
		None,
		Positive,
		Negative
	};
	
	struct InputMapping {
		bool axis = false;
		AxisSign sign = AxisSign::None;
		int a = -1;
		int b = -1;
	};

    struct Mapping {
		HashMap<LibretroButtons::Buttons, InputMapping> buttons;
		HashMap<LibretroButtons::Axes, InputMapping> axes;
    };

    MappingConfig() = default;
    MappingConfig(const ConfigNode& node);

    const String& getId() const;
	bool hasMapping(const String& controllerId) const;
    const Mapping& getMapping(const String& controllerId) const;

private:
    String id;
    HashMap<String, Mapping> mappings;
    Mapping dummy;

	void addToMapping(Mapping& mapping, const String& libretro, const String& sdl);
	InputMapping parseInputMapping(String id, bool libretro) const;
};
