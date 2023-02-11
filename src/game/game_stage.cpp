#include "game_stage.h"

#include "retrograde_game.h"
#include "src/config/core_config.h"
#include "src/config/system_config.h"
#include "src/libretro/libretro_core.h"
#include "src/game/retrograde_environment.h"

GameStage::GameStage() = default;

GameStage::~GameStage()
{
	if (audioStreamHandle) {
		audioStreamHandle->stop();
	}
	libretroCore.reset();
}

void GameStage::init()
{
	auto& game = static_cast<RetrogradeGame&>(getGame());
	String systemId;
	String gamePath;
	if (game.getArgs().size() >= 2) {
		systemId = game.getArgs()[0];
		gamePath = String::concatList(gsl::span<const String>(game.getArgs()).subspan(1), " ");
	}

	env = std::make_unique<RetrogradeEnvironment>("..", systemId, getResources(), getAPI());
	loadGame(systemId, gamePath);

	perfStats = std::make_shared<PerformanceStatsView>(getResources(), getAPI());
	perfStats->setActive(false);
}

void GameStage::onVariableUpdate(Time t)
{
	env->getConfigDatabase().update();

	if (getInputAPI().getKeyboard()->isButtonPressed(KeyCode::Esc)) {
		libretroCore.reset();
		getCoreAPI().quit();
		return;
	}

	if (getInputAPI().getKeyboard()->isButtonPressed(KeyCode::F11)) {
		perfStats->setActive(!perfStats->isActive());
	}
	perfStats->update(t);

	// TODO: move to fixed update?
	if (libretroCore && libretroCore->hasGameLoaded()) {
		if (getInputAPI().getKeyboard()->isButtonPressed(KeyCode::F2)) {
			Path::writeFile(Path(env->getSaveDir()) / (libretroCore->getGameName() + ".state0"), libretroCore->saveState(LibretroCore::SaveStateType::Normal));
		}

		if (getInputAPI().getKeyboard()->isButtonPressed(KeyCode::F4)) {
			const auto saveState = Path::readFile(Path(env->getSaveDir()) / (libretroCore->getGameName() + ".state0"));
			if (!saveState.empty()) {
				libretroCore->loadState(saveState);
			}
		}

		libretroCore->runFrame();

		dynamic_cast<RetrogradeGame&>(getGame()).setTargetFPS(libretroCore->getSystemAVInfo().fps);
	} else {
		dynamic_cast<RetrogradeGame&>(getGame()).setTargetFPS(std::nullopt);
	}
}

void GameStage::onFixedUpdate(Time t)
{

}

void GameStage::onRender(RenderContext& rc) const
{
	rc.bind([&] (Painter& painter)
	{
		painter.clear(Colour4f(0.0f, 0.0f, 0.0f));

		if (libretroCore && libretroCore->hasGameLoaded()) {
			drawScreen(painter, libretroCore->getVideoOut());
		}

		perfStats->paint(painter);
	});
}

void GameStage::drawScreen(Painter& painter, Sprite screen) const
{
	if (screen.hasMaterial()) {
		const auto spriteSize = screen.getScaledSize().rotate(screen.getRotation()).abs();
		const auto windowSize = Vector2f(getVideoAPI().getWindow().getDefinition().getSize());
		const auto scales = windowSize / spriteSize;
		const auto scale = std::min(scales.x, scales.y) * screen.getScale();

		screen
			.setScale(scale)
			.setPivot(Vector2f(0.5f, 0.5f))
			.setPosition(windowSize * 0.5f)
			.draw(painter);
	}
}

void GameStage::loadGame(const String& systemId, const String& gamePath)
{
	const auto& systemConfig = env->getConfigDatabase().get<SystemConfig>(systemId);
	const String coreId = systemConfig.getCores().at(0);
	const auto& coreConfig = env->getConfigDatabase().get<CoreConfig>(coreId);
	const String corePath = coreId + "_libretro.dll";

	libretroCore = LibretroCore::load(env->getCoresDir() + "/" + corePath, *env);
	for (int i = 0; i < 4; ++i) {
		libretroCore->setInputDevice(i, makeInput(i));
	}
	
	if (libretroCore) {
		for (const auto& [k, v]: coreConfig.getOptions()) {
			libretroCore->setOption(k, v);
		}

		const bool ok = libretroCore->loadGame(Path(env->getRomsDir() + "/" + gamePath).getNativeString());
		if (ok) {
			audioStreamHandle = getAudioAPI().play(libretroCore->getAudioOut(), getAudioAPI().getGlobalEmitter(), 1, true);
		} else {
			Logger::logError("Failed to load game " + String(gamePath));
		}
	} else {
		Logger::logError("Failed to load core " + String(corePath));
	}
}


std::shared_ptr<InputVirtual> GameStage::makeInput(int idx)
{
	auto input = std::make_shared<InputVirtual>(16, 6);

	auto joy = getInputAPI().getJoystick(idx);

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
