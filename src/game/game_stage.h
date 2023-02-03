#pragma once

#include <halley.hpp>
class LibretroEnvironment;
class LibretroCore;
using namespace Halley;

class GameStage : public EntityStage {
public:
	GameStage();
	~GameStage();
	
	void init() override;

	void onVariableUpdate(Time) override;
	void onFixedUpdate(Time) override;
	void onRender(RenderContext&) const override;

private:
	std::shared_ptr<Texture> texture;

	AudioHandle audioStreamHandle;

	std::unique_ptr<LibretroEnvironment> libretroEnvironment;
	std::unique_ptr<LibretroCore> libretroCore;

	void drawScreen(Painter& painter, Sprite screen) const;

	std::shared_ptr<InputVirtual> makeInput(int idx);
};
