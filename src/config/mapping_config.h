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

	enum Axes {
		LIBRETRO_AXIS_LEFT_X,
		LIBRETRO_AXIS_LEFT_Y,
		LIBRETRO_AXIS_RIGHT_X,
		LIBRETRO_AXIS_RIGHT_Y,
		LIBRETRO_AXIS_TRIGGER_LEFT,
		LIBRETRO_AXIS_TRIGGER_RIGHT
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
