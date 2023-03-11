#include "retrograde_environment.h"
#include <filesystem>

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
	coresDir = rootDir / "cores";
	saveDir = rootDir / "save";
	romsDir = rootDir / "roms";
	shadersDir = rootDir / "shaders";
	imagesDir = rootDir / "images";
	coreAssetsDir = rootDir / "coreAssets";

	std::error_code ec;
	std::filesystem::create_directories(saveDir.getNativeString().cppStr(), ec);
	std::filesystem::create_directories(coreAssetsDir.getNativeString().cppStr(), ec);

	configDatabase.init<BezelConfig>("bezels");
	configDatabase.init<CoreConfig>("cores");
	configDatabase.init<ScreenFilterConfig>("screenFilters");
	configDatabase.init<SystemConfig>("systems");
	configDatabase.load(resources, "db/");

	imageCache = std::make_shared<ImageCache>(*halleyAPI.video, resources, imagesDir);
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
	for (int i = 0; i < 4; ++i) {
		core->setInputDevice(i, makeInput(i));
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

std::shared_ptr<InputVirtual> RetrogradeEnvironment::getUIInput()
{
	if (!uiInput) {
		uiInput = makeUIInput();
	}
	return uiInput;
}

ImageCache& RetrogradeEnvironment::getImageCache() const
{
	return *imageCache;
}

std::shared_ptr<InputVirtual> RetrogradeEnvironment::makeUIInput()
{
	auto input = std::make_shared<InputVirtual>(13, 6);

	auto joy = halleyAPI.input->getJoystick(0);
	if (joy) {
		input->bindButton(0, joy, joy->getButtonAtPosition(JoystickButtonPosition::PlatformAcceptButton));
		input->bindButton(1, joy, joy->getButtonAtPosition(JoystickButtonPosition::PlatformCancelButton));
		input->bindButton(2, joy, joy->getButtonAtPosition(JoystickButtonPosition::FaceLeft));
		input->bindButton(3, joy, joy->getButtonAtPosition(JoystickButtonPosition::FaceTop));

		input->bindButton(4, joy, joy->getButtonAtPosition(JoystickButtonPosition::Select));
		input->bindButton(5, joy, joy->getButtonAtPosition(JoystickButtonPosition::Start));

		input->bindButton(6, joy, joy->getButtonAtPosition(JoystickButtonPosition::BumperLeft));
		input->bindButton(7, joy, joy->getButtonAtPosition(JoystickButtonPosition::BumperRight));

		input->bindButton(8, joy, joy->getButtonAtPosition(JoystickButtonPosition::TriggerLeft));
		input->bindButton(9, joy, joy->getButtonAtPosition(JoystickButtonPosition::TriggerRight));
		input->bindButton(10, joy, joy->getButtonAtPosition(JoystickButtonPosition::LeftStick));
		input->bindButton(11, joy, joy->getButtonAtPosition(JoystickButtonPosition::RightStick));
		input->bindButton(12, joy, joy->getButtonAtPosition(JoystickButtonPosition::System));

		input->bindAxis(0, joy, 0);
		input->bindAxis(1, joy, 1);
		input->bindAxisButton(0, joy, joy->getButtonAtPosition(JoystickButtonPosition::DPadLeft), joy->getButtonAtPosition(JoystickButtonPosition::DPadRight));
		input->bindAxisButton(1, joy, joy->getButtonAtPosition(JoystickButtonPosition::DPadUp), joy->getButtonAtPosition(JoystickButtonPosition::DPadDown));
		input->bindAxis(2, joy, 2);
		input->bindAxis(3, joy, 3);
	}

	auto kb = halleyAPI.input->getKeyboard();
	if (kb) {
		input->bindButton(0, kb, KeyCode::Enter, KeyMods::None);
		input->bindButton(1, kb, KeyCode::Esc, KeyMods::None);
		input->bindButton(12, kb, KeyCode::F1, KeyMods::None);

		input->bindAxisButton(0, kb, KeyCode::Left, KeyCode::Right);
		input->bindAxisButton(1, kb, KeyCode::Up, KeyCode::Down);
	}

	return input;
}

std::shared_ptr<InputVirtual> RetrogradeEnvironment::makeInput(int idx)
{
	auto input = std::make_shared<InputVirtual>(17, 6);

	auto joy = halleyAPI.input->getJoystick(idx);
	if (joy) {
		input->bindButton(0, joy, joy->getButtonAtPosition(JoystickButtonPosition::DPadUp));
		input->bindButton(1, joy, joy->getButtonAtPosition(JoystickButtonPosition::DPadDown));
		input->bindButton(2, joy, joy->getButtonAtPosition(JoystickButtonPosition::DPadLeft));
		input->bindButton(3, joy, joy->getButtonAtPosition(JoystickButtonPosition::DPadRight));

		input->bindButton(4, joy, joy->getButtonAtPosition(JoystickButtonPosition::FaceRight));
		input->bindButton(5, joy, joy->getButtonAtPosition(JoystickButtonPosition::FaceBottom));
		input->bindButton(6, joy, joy->getButtonAtPosition(JoystickButtonPosition::FaceTop));
		input->bindButton(7, joy, joy->getButtonAtPosition(JoystickButtonPosition::FaceLeft));

		input->bindButton(8, joy, joy->getButtonAtPosition(JoystickButtonPosition::Select));
		input->bindButton(9, joy, joy->getButtonAtPosition(JoystickButtonPosition::Start));

		input->bindButton(10, joy, joy->getButtonAtPosition(JoystickButtonPosition::BumperLeft));
		input->bindButton(11, joy, joy->getButtonAtPosition(JoystickButtonPosition::BumperRight));
		input->bindButton(12, joy, joy->getButtonAtPosition(JoystickButtonPosition::TriggerLeft));
		input->bindButton(13, joy, joy->getButtonAtPosition(JoystickButtonPosition::TriggerRight));
		input->bindButton(14, joy, joy->getButtonAtPosition(JoystickButtonPosition::LeftStick));
		input->bindButton(15, joy, joy->getButtonAtPosition(JoystickButtonPosition::RightStick));
		input->bindButton(16, joy, joy->getButtonAtPosition(JoystickButtonPosition::System));

		input->bindAxis(0, joy, 0);
		input->bindAxis(1, joy, 1);
		input->bindAxis(2, joy, 2);
		input->bindAxis(3, joy, 3);
		input->bindAxis(4, joy, 4);
		input->bindAxis(5, joy, 5);
	}
	
	return input;
}
