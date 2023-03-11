#pragma once

#include <halley.hpp>

#include "src/metadata/game_collection.h"
class CoreConfig;
class GameCollection;
class SystemConfig;
using namespace Halley;

class RetrogradeEnvironment;

class ChooseGameWindow : public UIWidget {
public:
    ChooseGameWindow(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment, const SystemConfig& systemConfig, std::optional<String> gameId, UIWidget& parentMenu);

    void onMakeUI() override;
    void onAddedToRoot(UIRoot& root) override;

    void update(Time t, bool moved) override;

    void close();

private:
    UIFactory& factory;
    RetrogradeEnvironment& retrogradeEnvironment;
    const SystemConfig& systemConfig;
    const CoreConfig* coreConfig = nullptr;
    std::optional<String> pendingGameId;
    UIWidget& parentMenu;
    GameCollection& collection;
   
    void onGamepadInput(const UIInputResults& input, Time time) override;
    void loadGame(size_t gameIdx);
    void loadGame(const String& gamePath);
    void onGameSelected(size_t gameIdx);
    void onErrorDueToNoCoreAvailable();
};

class GameCapsule : public UIWidget {
public:
    GameCapsule(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment, const GameCollection::Entry& entry);

    void onMakeUI() override;

private:
    UIFactory& factory;
    RetrogradeEnvironment& retrogradeEnvironment;
    const GameCollection::Entry& entry;
};
