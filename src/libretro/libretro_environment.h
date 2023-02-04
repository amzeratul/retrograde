#pragma once

#include <halley.hpp>
using namespace Halley;

class LibretroEnvironment {
public:
	LibretroEnvironment(String rootDir, String systemId, Resources& resources, const HalleyAPI& halleyAPI);

	const String& getSystemDir() const;
	const String& getSaveDir() const;
	const String& getCoresDir() const;
	const String& getRomsDir() const;

	Resources& getResources() const;
	const HalleyAPI& getHalleyAPI() const;

private:
	Resources& resources;
	const HalleyAPI& halleyAPI;
	
	String systemId;
	String rootDir;
	String systemDir;
	String saveDir;
	String coresDir;
	String romsDir;
};
