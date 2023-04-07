#pragma once

#include <halley.hpp>
class GameCanvas;
using namespace Halley;

class RetrogradeEnvironment;

class InGameMenu : public UIWidget {
public:
    enum class Mode {
	    PreStart,
        InGame
    };

    InGameMenu(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment, GameCanvas& gameCanvas, Mode mode);

    void onMakeUI() override;
    void onAddedToRoot(UIRoot& root) override;

    void update(Time t, bool moved) override;

private:
    UIFactory& factory;
    RetrogradeEnvironment& retrogradeEnvironment;
    GameCanvas& gameCanvas;
    const Mode mode;

    void setupMenu();
    void showSaveStates(bool canSave);
    void showMedia();
    void showAchievements();

    void onChooseOption(const String& optionId);
    void onGamepadInput(const UIInputResults& input, Time time) override;
    void close();
};
