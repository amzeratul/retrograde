#pragma once

#include <halley.hpp>

#include "src/filter_chain/filter_chain.h"

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
	std::unique_ptr<FilterChain> makeFilterChain(const String& path);

	std::shared_ptr<InputVirtual> getUIInput();

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
	Path shadersDir;

	std::shared_ptr<InputVirtual> uiInput;

	std::shared_ptr<InputVirtual> makeUIInput();
	std::shared_ptr<InputVirtual> makeInput(int idx);
};
