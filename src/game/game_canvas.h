#pragma once

#include <halley.hpp>
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

private:
    UIFactory& factory;
    RetrogradeEnvironment& environment;
    const CoreConfig& coreConfig;
    const SystemConfig& systemConfig;
    String gameId;
    UIWidget& parentMenu;

	std::unique_ptr<LibretroCore> core;
	std::unique_ptr<RewindData> rewindData;

    mutable Sprite screen;
    std::unique_ptr<FilterChain> filterChain;
    std::unique_ptr<SystemBezel> bezel;

    int frames = 0;
    int pauseFrames = 0;
    mutable int pendingCloseState = 0;
    bool loaded = false;

    void doClose();

	void paint(Painter& painter) const;
    void drawScreen(Painter& painter, Sprite screen) const;
    void stepGame();

    void onGamepadInput(const UIInputResults& input, Time time) override;

	void updateBezels();
    void updateFilterChain(Vector2i screenSize);
};
