#pragma once

#include <halley.hpp>
class GameCanvas;
using namespace Halley;

class RetrogradeEnvironment;

class InGameMenu : public UIWidget {
public:
    InGameMenu(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment, GameCanvas& gameCanvas);

    void onMakeUI() override;

private:
    UIFactory& factory;
    RetrogradeEnvironment& retrogradeEnvironment;
    GameCanvas& gameCanvas;

    void onChooseOption(const String& optionId);
    void onGamepadInput(const UIInputResults& input, Time time) override;
};
