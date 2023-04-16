#include "mapping_config.h"

MappingConfig::MappingConfig(const ConfigNode& node)
{
	id = node["id"].asString();

	const auto physicalToLibRetro = node["physicalToLibretro"].asHashMap<String, String>();

	for (const auto& [controllerId, controllerNode]: node["controllers"].asMap()) {
		Mapping mapping;
		for (const auto& [k, v]: controllerNode["physicalToSDL"].asMap()) {
			addToMapping(mapping, physicalToLibRetro.at(k), v.asString());
		}
		mappings[controllerId] = std::move(mapping);
	}
}

const String& MappingConfig::getId() const
{
	return id;
}

bool MappingConfig::hasMapping(const String& controllerId) const
{
	const auto iter = mappings.find(controllerId);
	return iter != mappings.end();
}

const MappingConfig::Mapping& MappingConfig::getMapping(const String& controllerId) const
{
	const auto iter = mappings.find(controllerId);
	if (iter != mappings.end()) {
		return iter->second;
	}
	return dummy;
}

void MappingConfig::addToMapping(Mapping& mapping, const String& libretro, const String& sdl)
{
	const auto dst = parseInputMapping(libretro, true);
	const auto src = parseInputMapping(sdl, false);

	if (dst.axis) {
		const auto axisId = static_cast<LibretroButtons::Axes>(dst.a);
		auto& m = mapping.axes[axisId];
		if (src.axis) {
			m.axis = true;
			m.sign = src.sign == dst.sign ? AxisSign::Positive : AxisSign::Negative;
			m.a = src.a;
			m.b = -1;
		} else {
			m.axis = false;
			m.sign = AxisSign::None;
			if (src.sign == AxisSign::Negative) {
				m.a = src.a;
			} else {
				m.b = src.a;
			}
		}
	} else {
		const auto buttonId = static_cast<LibretroButtons::Buttons>(dst.a);
		auto& m = mapping.buttons[buttonId];
		if (src.axis) {
			m.axis = true;
			m.sign = dst.sign;
			m.a = src.a;
			m.b = -1;
		} else {
			m.axis = false;
			m.sign = AxisSign::None;
			m.a = src.a;
			m.b = -1;
		}
	}
}

MappingConfig::InputMapping MappingConfig::parseInputMapping(String id, bool libretro) const
{
	if (id.isEmpty()) {
		return {};
	}

	AxisSign sign = AxisSign::None;
	if (id.startsWith("+")) {
		sign = AxisSign::Positive;
		id = id.substr(1);
	} else if (id.startsWith("-")) {
		sign = AxisSign::Negative;
		id = id.substr(1);
	}

	std::array<String, 6> axisNames = {
		"leftx",
		"lefty",
		"rightx",
		"righty",
		"lefttrigger",
		"righttrigger",
	};

	std::array<String, 16> buttonNamesLibretro = {
		"dpup",
		"dpdown",
		"dpleft",
		"dpright",
		"a",
		"b",
		"x",
		"y",
		"back",
		"start",
		"leftshoulder",
		"rightshoulder",
		"lefttrigger",
		"righttrigger",
		"leftstick",
		"rightstick"
	};

	std::array<String, 18> buttonNamesSDL = {
		"y",
		"b",
		"a",
		"x",
		"leftshoulder",
		"rightshoulder",
		"lefttrigger",
		"righttrigger",
		"leftstick",
		"rightstick",
		"back",
		"start",
		"a",
		"b",
		"dpup",
		"dpright",
		"dpdown",
		"dpleft"
	};

	const gsl::span<String> buttonNames = libretro ?
		gsl::span<String>(buttonNamesLibretro.data(), buttonNamesLibretro.size()) :
		gsl::span<String>(buttonNamesSDL.data(), buttonNamesSDL.size());
	
	// Parsing button first means that trigger counts as a button
	const auto buttonIter = std::find(buttonNames.begin(), buttonNames.end(), id);
	if (buttonIter != buttonNames.end()) {
		InputMapping result;
		result.axis = false;
		result.sign = sign;
		result.a = static_cast<int>(buttonIter - buttonNames.begin());
		return result;
	}

	const auto axisIter = std::find(axisNames.begin(), axisNames.end(), id);
	if (axisIter != axisNames.end()) {
		InputMapping result;
		result.axis = true;
		result.sign = sign;
		result.a = static_cast<int>(axisIter - axisNames.begin());
		return result;
	}

	return {};
}
