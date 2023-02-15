#include "retrograde_environment.h"
#include <filesystem>

#include "src/config/core_config.h"
#include "src/config/system_config.h"
#include "src/libretro/libretro_core.h"

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

	std::error_code ec;
	std::filesystem::create_directories(saveDir.getNativeString().cppStr(), ec);

	configDatabase.init<CoreConfig>("cores");
	configDatabase.init<SystemConfig>("systems");
	configDatabase.load(resources, "db/");
}

const Path& RetrogradeEnvironment::getSystemDir() const
{
	return systemDir;
}

const Path& RetrogradeEnvironment::getSaveDir() const
{
	return saveDir;
}

const Path& RetrogradeEnvironment::getCoresDir() const
{
	return coresDir;
}

const Path& RetrogradeEnvironment::getRomsDir() const
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

ConfigDatabase& RetrogradeEnvironment::getConfigDatabase()
{
	return configDatabase;
}

RetrogradeGame& RetrogradeEnvironment::getGame()
{
	return game;
}

std::unique_ptr<LibretroCore> RetrogradeEnvironment::loadCore(const String& systemId, const String& gamePath)
{
	const auto& systemConfig = getConfigDatabase().get<SystemConfig>(systemId);
	const String coreId = systemConfig.getCores().at(0);
	const auto& coreConfig = getConfigDatabase().get<CoreConfig>(coreId);
	const String corePath = coreId + "_libretro.dll";

	auto core = LibretroCore::load(getCoresDir() + "/" + corePath, *this);
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

	if (!gamePath.isEmpty()) {
		core->loadGame(getRomsDir() / systemId / gamePath);
	}

	return core;
}

std::shared_ptr<InputVirtual> RetrogradeEnvironment::makeInput(int idx)
{
	auto input = std::make_shared<InputVirtual>(16, 6);

	auto joy = halleyAPI.input->getJoystick(idx);

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

	input->bindAxis(0, joy, 0);
	input->bindAxis(1, joy, 1);
	input->bindAxis(2, joy, 2);
	input->bindAxis(3, joy, 3);
	input->bindAxis(4, joy, 4);
	input->bindAxis(5, joy, 5);
	
	return input;
}
