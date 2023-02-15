#pragma once

#include <halley.hpp>
class RewindData;
class RetrogradeEnvironment;
using namespace Halley;

class LibretroCore;

class GameCanvas : public UIWidget {
public:
    GameCanvas(RetrogradeEnvironment& environment, std::unique_ptr<LibretroCore> core);
    ~GameCanvas() override;

    void update(Time t, bool moved) override;
    void draw(UIPainter& painter) const override;

private:
    RetrogradeEnvironment& environment;
    std::unique_ptr<LibretroCore> core;
	std::unique_ptr<RewindData> rewindData;

	void paint(Painter& painter) const;
    void drawScreen(Painter& painter, Sprite screen) const;
    void stepGame();
};
