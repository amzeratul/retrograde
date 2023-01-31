#pragma once

#include <halley.hpp>

class GameStage : public EntityStage {
public:
	GameStage();
	~GameStage();
	
	void init() override;

	void onVariableUpdate(Time) override;
	void onFixedUpdate(Time) override;
	void onRender(RenderContext&) const override;

private:
	Sprite screen;
	std::shared_ptr<Texture> texture;
	std::shared_ptr<InputVirtual> input;
};
