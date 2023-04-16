#include "settings.h"

Settings::Settings(Path path)
	: path(std::move(path))
{
}

void Settings::load()
{
	auto config = YAMLConvert::parseConfig(Path::readFile(path));
	load(config.getRoot());
}

void Settings::save() const
{
	YAMLConvert::EmitOptions options;
	const auto bytes = YAMLConvert::generateYAML(toConfigNode(), options);
	Path::writeFile(path, bytes);
}

void Settings::load(const ConfigNode& node)
{
	romsDir = node["romsDir"].asString("./roms");
	windowData = node["windowData"].asHashMap<String, ConfigNode>();
}

ConfigNode Settings::toConfigNode() const
{
	ConfigNode::MapType result;
	result["romsDir"] = romsDir.getString();
	result["windowData"] = windowData;
	return result;
}

const Path& Settings::getRomsDir() const
{
	return romsDir;
}

void Settings::setWindowData(const String& windowId, ConfigNode data)
{
	windowData[windowId] = std::move(data);
}

ConfigNode& Settings::getWindowData(const String& windowId)
{
	return windowData[windowId];
}
