#pragma once

#include <halley.hpp>

#include "src/metadata/game_collection.h"
enum class SaveStateType;
class SaveStateCollection;
class GameCanvas;
using namespace Halley;

class RetrogradeEnvironment;

class InGameMenu : public UIWidget {
public:
    enum class Mode {
	    PreStart,
        InGame
    };

    InGameMenu(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment, GameCanvas& gameCanvas, Mode mode, const GameCollection::Entry* metadata);

    void onMakeUI() override;
    void onAddedToRoot(UIRoot& root) override;

    void update(Time t, bool moved) override;

private:
    UIFactory& factory;
    RetrogradeEnvironment& retrogradeEnvironment;
    GameCanvas& gameCanvas;
    const Mode mode;
    const GameCollection::Entry* metadata;

    void setupMenu();
    void showRoot();
    void showSaveStates(bool canSave);
    void showSwapDisc();
    void showMedia();
    void showAchievements();
    void showInput();

    void refreshSaveStateList(bool canSave);

    void onChooseOption(const String& optionId);
    void onGamepadInput(const UIInputResults& input, Time time) override;
    void back();
    void close();
    void hide();

    void startGame(std::optional<std::pair<SaveStateType, size_t>> loadState);
};

class SaveStateCapsule : public UIWidget {
public:
    SaveStateCapsule(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment);
    void loadData(SaveStateCollection& ssc, SaveStateType type, size_t idx);

private:
    RetrogradeEnvironment& retrogradeEnvironment;

    String getDate(uint64_t timestamp) const;
};