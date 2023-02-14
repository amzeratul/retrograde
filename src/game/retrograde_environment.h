#pragma once

#include <halley.hpp>

using namespace Halley;

class RetrogradeEnvironment {
public:
	RetrogradeEnvironment(String rootDir, String systemId, Resources& resources, const HalleyAPI& halleyAPI);

	const String& getSystemDir() const;
	const String& getSaveDir() const;
	const String& getCoresDir() const;
	const String& getRomsDir() const;

	Resources& getResources() const;
	const HalleyAPI& getHalleyAPI() const;
	const ConfigDatabase& getConfigDatabase() const;
	ConfigDatabase& getConfigDatabase();

private:
	Resources& resources;
	const HalleyAPI& halleyAPI;

	ConfigDatabase configDatabase;
	
	String systemId;
	String rootDir;
	String systemDir;
	String saveDir;
	String coresDir;
	String romsDir;
};
