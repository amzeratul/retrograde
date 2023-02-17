#pragma once

#include <halley.hpp>
class RewindData;
class RetrogradeEnvironment;
using namespace Halley;

class LibretroCore;

class GameCanvas : public UIWidget {
public:
    GameCanvas(UIFactory& factory, RetrogradeEnvironment& environment, std::unique_ptr<LibretroCore> core, UIWidget& parentMenu);
    ~GameCanvas() override;

    void update(Time t, bool moved) override;
    void draw(UIPainter& painter) const override;

    void close();

private:
    UIFactory& factory;
    RetrogradeEnvironment& environment;
    std::unique_ptr<LibretroCore> core;
	std::unique_ptr<RewindData> rewindData;
    UIWidget& parentMenu;
    int pauseFrames = 0;

	void paint(Painter& painter) const;
    void drawScreen(Painter& painter, Sprite screen) const;
    void stepGame();

    void onGamepadInput(const UIInputResults& input, Time time) override;
};
