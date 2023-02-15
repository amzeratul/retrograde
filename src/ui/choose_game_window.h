#pragma once

#include <halley.hpp>
using namespace Halley;

class RetrogradeEnvironment;

class ChooseGameWindow : public UIWidget {
public:
    ChooseGameWindow(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment, String systemId);

    void onMakeUI() override;

private:
    UIFactory& factory;
    RetrogradeEnvironment& retrogradeEnvironment;
    String systemId;

    void loadGame(const String& gameId);
};
