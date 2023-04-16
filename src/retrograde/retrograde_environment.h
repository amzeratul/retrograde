#pragma once

#include <halley.hpp>

#include "settings.h"
#include "src/filter_chain/filter_chain.h"
#include "src/ui/choose_game_window.h"

class InputMapper;
class ImageCache;
class CoreConfig;
class LibretroCore;
class RetrogradeGame;
using namespace Halley;

class RetrogradeEnvironment {
public:
	RetrogradeEnvironment(RetrogradeGame& game, Path rootDir, Resources& resources, const HalleyAPI& halleyAPI);

	const Path& getSystemDir() const;
	const Path& getCoresDir() const;
	const Path& getImagesDir() const;
	Path getSaveDir(const String& system) const;
	Path getRomsDir(const String& system) const;
	Path getCoreAssetsDir(const String& core) const;

	Resources& getResources() const;
	const HalleyAPI& getHalleyAPI() const;
	const ConfigDatabase& getConfigDatabase() const;
	ConfigDatabase& getConfigDatabase();
	RetrogradeGame& getGame() const;

	std::unique_ptr<LibretroCore> loadCore(const CoreConfig& coreConfig, const SystemConfig& systemConfig);
	std::unique_ptr<FilterChain> makeFilterChain(const String& path);
	GameCollection& getGameCollection(const String& systemId);

	InputMapper& getInputMapper();
	ImageCache& getImageCache() const;

	void setProfileId(String id);
	const String& getProfileId();

	Settings& getSettings();

private:
	RetrogradeGame& game;
	Resources& resources;
	const HalleyAPI& halleyAPI;

	Path rootDir;
	Path profilesDir;
	Path curProfileDir;
	Path systemDir;
	Path saveDir;
	Path coresDir;
	Path romsDir;
	Path shadersDir;
	Path imagesDir;
	Path coreAssetsDir;

	String profileId;

	ConfigDatabase configDatabase;
	Settings settings;

	HashMap<String, std::shared_ptr<GameCollection>> gameCollections;

	std::shared_ptr<ImageCache> imageCache;
	std::shared_ptr<InputMapper> inputMapper;
};
