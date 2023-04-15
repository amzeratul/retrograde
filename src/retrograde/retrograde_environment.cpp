#include "retrograde_environment.h"
#include <filesystem>

#include "input_mapper.h"
#include "src/config/bezel_config.h"
#include "src/config/core_config.h"
#include "src/config/screen_filter_config.h"
#include "src/config/system_config.h"
#include "src/filter_chain/retroarch_filter_chain.h"
#include "src/libretro/libretro_core.h"
#include "src/metadata/game_collection.h"
#include "src/util/image_cache.h"

RetrogradeEnvironment::RetrogradeEnvironment(RetrogradeGame& game, Path _rootDir, Resources& resources, const HalleyAPI& halleyAPI)
	: game(game)
	, resources(resources)
	, halleyAPI(halleyAPI)
	, rootDir(std::move(_rootDir))
{
	systemDir = rootDir / "system";
	profilesDir = rootDir / "profiles";
	coresDir = rootDir / "cores";
	shadersDir = rootDir / "shaders";
	imagesDir = rootDir / "images";
	coreAssetsDir = rootDir / "coreAssets";

	std::error_code ec;
	std::filesystem::create_directories(coreAssetsDir.getNativeString().cppStr(), ec);

	configDatabase.init<BezelConfig>("bezels");
	configDatabase.init<CoreConfig>("cores");
	configDatabase.init<ScreenFilterConfig>("screenFilters");
	configDatabase.init<SystemConfig>("systems");
	configDatabase.load(resources, "db/");

	imageCache = std::make_shared<ImageCache>(*halleyAPI.video, resources, imagesDir);

	settings.load(rootDir / "config" / "settings.yaml");
	romsDir = settings.getRomsDir().isAbsolute() ? settings.getRomsDir() : (rootDir / settings.getRomsDir());

	inputMapper = std::make_shared<InputMapper>(*this);
}

const Path& RetrogradeEnvironment::getSystemDir() const
{
	return systemDir;
}

const Path& RetrogradeEnvironment::getCoresDir() const
{
	return coresDir;
}

const Path& RetrogradeEnvironment::getImagesDir() const
{
	return imagesDir;
}

Path RetrogradeEnvironment::getSaveDir(const String& system) const
{
	return saveDir / system;
}

Path RetrogradeEnvironment::getRomsDir(const String& system) const
{
	return romsDir / system;
}

Path RetrogradeEnvironment::getCoreAssetsDir(const String& core) const
{
	return coreAssetsDir / core;
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

ConfigDatabase& RetrogradeEnvironment::getConfigDatabase()
{
	return configDatabase;
}

RetrogradeGame& RetrogradeEnvironment::getGame() const
{
	return game;
}

std::unique_ptr<LibretroCore> RetrogradeEnvironment::loadCore(const CoreConfig& coreConfig, const SystemConfig& systemConfig)
{
	const String corePath = coreConfig.getId() + "_libretro.dll";

	auto core = LibretroCore::load(coreConfig, getCoresDir() + "/" + corePath, systemConfig.getId(), *this);
	for (int i = 0; i < 8; ++i) {
		core->setInputDevice(i, inputMapper->getInput(i));
	}
	
	if (!core) {
		Logger::logError("Failed to load core " + String(corePath));
		return {};
	}

	for (const auto& [k, v]: coreConfig.getOptions()) {
		core->setOption(k, v);
	}

	return core;
}

std::unique_ptr<FilterChain> RetrogradeEnvironment::makeFilterChain(const String& path)
{
	return std::make_unique<RetroarchFilterChain>(path, shadersDir / path, *halleyAPI.video);
}

GameCollection& RetrogradeEnvironment::getGameCollection(const String& systemId)
{
	const auto iter = gameCollections.find(systemId);
	if (iter != gameCollections.end()) {
		return *iter->second;
	}

	auto col = std::make_shared<GameCollection>(getRomsDir(systemId));
	col->scanGames();
	gameCollections[systemId] = col;
	return *col;
}

InputMapper& RetrogradeEnvironment::getInputMapper()
{
	return *inputMapper;
}

ImageCache& RetrogradeEnvironment::getImageCache() const
{
	return *imageCache;
}

void RetrogradeEnvironment::setProfileId(String id)
{
	profileId = std::move(id);
	curProfileDir = profilesDir / profileId;
	saveDir = curProfileDir / "save";
	std::error_code ec;
	std::filesystem::create_directories(curProfileDir.getNativeString().cppStr(), ec);
	std::filesystem::create_directories(saveDir.getNativeString().cppStr(), ec);
}

const String& RetrogradeEnvironment::getProfileId()
{
	return profileId;
}

Settings& RetrogradeEnvironment::getSettings()
{
	return settings;
}
