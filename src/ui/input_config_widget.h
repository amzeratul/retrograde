#pragma once

#include <halley.hpp>
class GameCanvas;
using namespace Halley;

class GameStage;

class InputConfigWidget : public UIWidget {
public:
    InputConfigWidget(UIFactory& factory, GameCanvas& gameCanvas);

private:
    UIFactory& factory;
	GameCanvas& gameCanvas;
};
