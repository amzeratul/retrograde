#pragma once

#include <halley.hpp>
using namespace Halley;

class RetrogradeEnvironment;

class ChooseGameWindow : public UIWidget {
public:
    ChooseGameWindow(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment, String systemId, UIWidget& parentMenu);

    void onMakeUI() override;

    void close();

private:
    UIFactory& factory;
    RetrogradeEnvironment& retrogradeEnvironment;
    String systemId;
    UIWidget& parentMenu;

    void onGamepadInput(const UIInputResults& input, Time time) override;
    void loadGame(const String& gameId);
};
