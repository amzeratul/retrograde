#pragma once

#include <halley.hpp>

#include "src/metadata/game_collection.h"
#include "src/retrograde/game_input_mapper.h"
#include "src/ui/in_game_menu.h"
enum class SaveStateType;
class SaveStateCollection;
class SaveState;
class CoreConfig;
class SystemBezel;
class SystemConfig;
class FilterChain;
class RewindData;
class RetrogradeEnvironment;
using namespace Halley;

class LibretroCore;

class GameCanvas : public UIWidget {
public:
    GameCanvas(UIFactory& factory, RetrogradeEnvironment& environment, const CoreConfig& coreConfig, const SystemConfig& systemConfig, String gameId, UIWidget& parentMenu);
    ~GameCanvas() override;

    void onAddedToRoot(UIRoot& root) override;

    void update(Time t, bool moved) override;
    void render(RenderContext& rc) const override;
    void draw(UIPainter& painter) const override;

    void close();

	void resetGame();
    void startGame(std::optional<std::pair<SaveStateType, size_t>> loadState = {});

    SaveStateCollection& getSaveStateCollection();
    GameInputMapper& getGameInputMapper();

    void waitForCoreLoad();
    bool isCoreLoaded() const;
    LibretroCore& getCore();
    
private:
    UIFactory& factory;
    RetrogradeEnvironment& environment;
    const CoreConfig& coreConfig;
    const SystemConfig& systemConfig;
    String gameId;
    UIWidget& parentMenu;

	std::unique_ptr<LibretroCore> core;
	std::unique_ptr<RewindData> rewindData;
	std::unique_ptr<SaveStateCollection> saveStateCollection;
    std::shared_ptr<GameInputMapper> gameInputMapper;

    mutable Sprite screen;
    std::unique_ptr<FilterChain> filterChain;
    std::unique_ptr<SystemBezel> bezel;

    int frames = 0;
    int pauseFrames = 0;
    mutable int pendingCloseState = 0;
    bool coreLoadRequested = false;
    bool gameLoaded = false;
    bool mouseCaptured = false;
    Future<void> coreLoadingFuture;

    std::optional<std::pair<SaveStateType, size_t>> pendingLoadState;
    int pendingLoadStateAttempts = 0;

    Time autoSaveTime = 0;

	std::shared_ptr<InGameMenu> menu;

    void doClose();

	void paint(Painter& painter) const;
    void drawScreen(Painter& painter, Sprite screen) const;
    void stepGame();

    void onGamepadInput(const UIInputResults& input, Time time) override;

    void loadCore();

	void updateBezels();
    Vector2i getWindowSize() const;
    void updateFilterChain(Vector2i screenSize);

	void updateAutoSave(Time t);

    void openMenu();
    const GameCollection::Entry* getGameMetadata();

    void setMouseCapture(bool enabled);
};
