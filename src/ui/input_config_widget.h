#pragma once

#include <halley.hpp>

#include "src/libretro/libretro_core.h"
class InputSlotWidget;
class GameCanvas;
using namespace Halley;

class GameStage;

class InputConfigWidget : public UIWidget {
public:
    InputConfigWidget(UIFactory& factory, GameCanvas& gameCanvas);

    void onMakeUI() override;
    void update(Time t, bool moved) override;

private:
    UIFactory& factory;
	GameCanvas& gameCanvas;

    Vector<std::shared_ptr<InputSlotWidget>> slots;

    void initialInputSetup();
    void setSlots(int n);
};

class InputSlotWidget : public UIWidget {
public:
    InputSlotWidget(UIFactory& factory);

    void onMakeUI() override;
    void setSlotActive(bool active);
    void setSlotName(const String& name);
    void setDeviceTypes(Vector<LibretroCore::ControllerType> deviceTypes);

private:
    Vector<LibretroCore::ControllerType> deviceTypes;
};
