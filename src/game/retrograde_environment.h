#pragma once

#include <halley.hpp>

class LibretroCore;
class RetrogradeGame;
using namespace Halley;

class RetrogradeEnvironment {
public:
	RetrogradeEnvironment(RetrogradeGame& game, Path rootDir, Resources& resources, const HalleyAPI& halleyAPI);

	const Path& getSystemDir() const;
	const Path& getCoresDir() const;
	Path getSaveDir(const String& system) const;
	Path getRomsDir(const String& system) const;

	Resources& getResources() const;
	const HalleyAPI& getHalleyAPI() const;
	const ConfigDatabase& getConfigDatabase() const;
	ConfigDatabase& getConfigDatabase();
	RetrogradeGame& getGame() const;

	std::unique_ptr<LibretroCore> loadCore(const String& systemId, const String& gamePath = {});

private:
	RetrogradeGame& game;
	Resources& resources;
	const HalleyAPI& halleyAPI;

	ConfigDatabase configDatabase;
	
	Path rootDir;
	Path systemDir;
	Path saveDir;
	Path coresDir;
	Path romsDir;

	std::shared_ptr<InputVirtual> makeInput(int idx);
};
