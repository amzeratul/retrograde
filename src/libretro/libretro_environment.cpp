#include "libretro_environment.h"

LibretroEnvironment::LibretroEnvironment(String _rootDir)
	: rootDir(std::move(_rootDir))
{
	systemDir = rootDir + "/system";
	saveDir = rootDir + "/save";
	coreDir = rootDir + "/core";
}

const String& LibretroEnvironment::getSystemDir() const
{
	return systemDir;
}

const String& LibretroEnvironment::getSaveDir() const
{
	return saveDir;
}

const String& LibretroEnvironment::getCoreDir() const
{
	return coreDir;
}
