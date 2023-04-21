#pragma once

#include <halley.hpp>
class GameCanvas;
using namespace Halley;

class GameStage;

class InputConfigWidget : public UIWidget {
public:
    InputConfigWidget(UIFactory& factory, GameCanvas& gameCanvas);

    void onMakeUI() override;

private:
    UIFactory& factory;
	GameCanvas& gameCanvas;
};

class InputSlotWidget : public UIWidget {
public:
    InputSlotWidget(UIFactory& factory);

    void onMakeUI() override;
    void setSlotActive(bool active);
    void setSlotName(const String& name);
};
