#include "libretro_environment.h"
#include <filesystem>

LibretroEnvironment::LibretroEnvironment(String _rootDir, String _systemId, Resources& resources, const HalleyAPI& halleyAPI)
	: resources(resources)
	, halleyAPI(halleyAPI)
	, systemId(std::move(_systemId))
	, rootDir(std::move(_rootDir))
{
	systemDir = rootDir + "/system/" + systemId;
	saveDir = rootDir + "/save/" + systemId;
	coresDir = rootDir + "/cores";
	romsDir = rootDir + "/roms/" + systemId;

	std::error_code ec;
	std::filesystem::create_directories(saveDir.cppStr(), ec);
	std::filesystem::create_directories(systemDir.cppStr(), ec);
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

Resources& LibretroEnvironment::getResources() const
{
	return resources;
}

const HalleyAPI& LibretroEnvironment::getHalleyAPI() const
{
	return halleyAPI;
}
