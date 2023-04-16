#pragma once

#include <halley.hpp>
class SystemConfig;
using namespace Halley;

class RetrogradeEnvironment;

class ChooseSystemWindow : public UIWidget {
public:
    enum class ViewMode {
	    Timeline,
        Generations
    };

    ChooseSystemWindow(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment, std::optional<String> systemId, std::optional<String> gameId);

    void onAddedToRoot(UIRoot& root) override;
    void onMakeUI() override;

    void update(Time t, bool moved) override;
    void draw(UIPainter& painter) const override;

    void setSelectedSystem(const SystemConfig& systemConfig);
    const String& getRegion() const;

private:
    UIFactory& factory;
    RetrogradeEnvironment& retrogradeEnvironment;
    std::optional<String> pendingSystemId;
    std::optional<String> pendingGameId;
    String region;

    void loadSystem(const String& systemId);
    void close();
    
    void populateSystems();
};

class SystemList : public UIWidget {
public:
    SystemList(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment, LocalisedString title, Vector<const SystemConfig*> systems, ChooseSystemWindow& parent, ChooseSystemWindow::ViewMode viewMode);

    void onMakeUI() override;

private:
    UIFactory& factory;
    RetrogradeEnvironment& retrogradeEnvironment;
    LocalisedString title;
    Vector<const SystemConfig*> systems;
    ChooseSystemWindow& parent;
    ChooseSystemWindow::ViewMode viewMode;

    const SystemConfig* getSystemConfig(const String& id) const;
};

class SystemCapsule : public UIWidget {
public:
    SystemCapsule(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment, const SystemConfig* systemConfig, String region);

    void onMakeUI() override;

private:
    UIFactory& factory;
    RetrogradeEnvironment& retrogradeEnvironment;
    const SystemConfig* systemConfig = nullptr;
    String region;
};
