#include "libretro_environment.h"

LibretroEnvironment::LibretroEnvironment(String _rootDir)
	: rootDir(std::move(_rootDir))
{
	systemDir = rootDir + "/system";
	saveDir = rootDir + "/save";
	coresDir = rootDir + "/cores";
	romsDir = rootDir + "/roms";
}

const String& LibretroEnvironment::getSystemDir() const
{
	return systemDir;
}

const String& LibretroEnvironment::getSaveDir() const
{
	return saveDir;
}

const String& LibretroEnvironment::getCoresDir() const
{
	return coresDir;
}

const String& LibretroEnvironment::getRomsDir() const
{
	return romsDir;
}
