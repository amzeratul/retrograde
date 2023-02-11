#include "game_stage.h"

#include "retrograde_game.h"
#include "src/libretro/libretro_core.h"
#include "src/libretro/libretro_environment.h"

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
	String corePath;
	String gamePath;
	if (game.getArgs().size() >= 3) {
		systemId = game.getArgs()[0];
		corePath = game.getArgs()[1];
		gamePath = String::concatList(gsl::span<const String>(game.getArgs()).subspan(2), " ");
	}

	libretroEnvironment = std::make_unique<LibretroEnvironment>("..", systemId, getResources(), getAPI());
	
	libretroCore = LibretroCore::load(libretroEnvironment->getCoresDir() + "/" + corePath, *libretroEnvironment);
	for (int i = 0; i < 4; ++i) {
		libretroCore->setInputDevice(i, makeInput(i));
	}
	
	if (libretroCore) {
		const bool ok = libretroCore->loadGame(Path(libretroEnvironment->getRomsDir() + "/" + gamePath).getNativeString());
		if (ok) {
			audioStreamHandle = getAudioAPI().play(libretroCore->getAudioOut(), getAudioAPI().getGlobalEmitter(), 1, true);
		} else {
			Logger::logError("Failed to load game " + String(gamePath));
		}
	} else {
		Logger::logError("Failed to load core " + String(corePath));
	}
}

void GameStage::onVariableUpdate(Time t)
{
	if (getInputAPI().getKeyboard()->isButtonPressed(KeyCode::Esc)) {
		libretroCore.reset();
		getCoreAPI().quit();
		return;
	}
	
	// TODO: move to fixed update?
	if (libretroCore && libretroCore->hasGameLoaded()) {
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
