#pragma once

#include <halley.hpp>

#include "src/libretro/libretro_core.h"
#include "src/retrograde/game_input_mapper.h"
#include "src/retrograde/input_mapper.h"
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

    void moveDevice(const std::shared_ptr<InputDevice>& device, int dx);
    void changeDeviceMapping(const std::shared_ptr<InputDevice>& device, int dy);
};

class InputSlotWidget : public UIWidget {
public:
    InputSlotWidget(UIFactory& factory);

    void onMakeUI() override;
    void setSlotActive(bool active);
    void setSlotName(const String& name);
    void setDeviceTypes(Vector<LibretroCore::ControllerType> deviceTypes);
    void setDevice(const std::shared_ptr<InputDevice>& device);

private:
    Vector<LibretroCore::ControllerType> deviceTypes;
    std::shared_ptr<InputDevice> device;
};
