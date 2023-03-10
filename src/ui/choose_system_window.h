#pragma once

#include <halley.hpp>
class SystemConfig;
using namespace Halley;

class RetrogradeEnvironment;

class ChooseSystemWindow : public UIWidget {
public:
    ChooseSystemWindow(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment, std::optional<String> systemId, std::optional<String> gameId);

    void onAddedToRoot(UIRoot& root) override;
    void onMakeUI() override;

    void update(Time t, bool moved) override;

private:
    UIFactory& factory;
    RetrogradeEnvironment& retrogradeEnvironment;
    std::optional<String> pendingSystemId;
    std::optional<String> pendingGameId;

    void loadSystem(const String& systemId);
    void close();
    
    void populateSystems();
};

class SystemList : public UIWidget {
public:
    SystemList(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment, LocalisedString title, Vector<const SystemConfig*> systems);

    void onMakeUI() override;

private:
    UIFactory& factory;
    RetrogradeEnvironment& retrogradeEnvironment;
    LocalisedString title;
    Vector<const SystemConfig*> systems;
};