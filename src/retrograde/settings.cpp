#include "settings.h"

void Settings::load(const Path& path)
{
	auto config = YAMLConvert::parseConfig(Path::readFile(path));
	load(config.getRoot());
}

void Settings::load(const ConfigNode& node)
{
	romsDir = node["romsDir"].asString("./roms");
}

const Path& Settings::getRomsDir() const
{
	return romsDir;
}
