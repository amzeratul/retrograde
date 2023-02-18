#pragma once

#include <halley.hpp>
using namespace Halley;

class RetrogradeEnvironment;

class ChooseGameWindow : public UIWidget {
public:
    ChooseGameWindow(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment, String systemId, std::optional<String> gameId, UIWidget& parentMenu);

    void onMakeUI() override;
    void onAddedToRoot(UIRoot& root) override;

    void close();

private:
    UIFactory& factory;
    RetrogradeEnvironment& retrogradeEnvironment;
    String systemId;
    std::optional<String> pendingGameId;
    UIWidget& parentMenu;

    void onGamepadInput(const UIInputResults& input, Time time) override;
    void loadGame(const String& gameId);
};
