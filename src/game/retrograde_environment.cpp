#include "retrograde_environment.h"
#include <filesystem>

#include "src/config/core_config.h"
#include "src/config/system_config.h"

RetrogradeEnvironment::RetrogradeEnvironment(String _rootDir, String _systemId, Resources& resources, const HalleyAPI& halleyAPI)
	: resources(resources)
	, halleyAPI(halleyAPI)
	, systemId(std::move(_systemId))
	, rootDir(std::move(_rootDir))
{
	systemDir = Path(rootDir + "/system").getNativeString();
	coresDir = Path(rootDir + "/cores").getNativeString();
	saveDir = Path(rootDir + "/save/" + systemId).getNativeString();
	romsDir = Path(rootDir + "/roms/" + systemId).getNativeString();

	std::error_code ec;
	std::filesystem::create_directories(saveDir.cppStr(), ec);

	configDatabase.init<CoreConfig>("cores");
	configDatabase.init<SystemConfig>("systems");
	configDatabase.load(resources, "db/");
}

const String& RetrogradeEnvironment::getSystemDir() const
{
	return systemDir;
}

const String& RetrogradeEnvironment::getSaveDir() const
{
	return saveDir;
}

const String& RetrogradeEnvironment::getCoresDir() const
{
	return coresDir;
}

const String& RetrogradeEnvironment::getRomsDir() const
{
	return romsDir;
}

Resources& RetrogradeEnvironment::getResources() const
{
	return resources;
}

const HalleyAPI& RetrogradeEnvironment::getHalleyAPI() const
{
	return halleyAPI;
}

const ConfigDatabase& RetrogradeEnvironment::getConfigDatabase() const
{
	return configDatabase;
}
