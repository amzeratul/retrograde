#pragma once

#include <halley.hpp>
using namespace Halley;

class RetrogradeEnvironment;

class ChooseGameWindow : public UIWidget {
public:
    ChooseGameWindow(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment);

    void onMakeUI() override;

private:
    UIFactory& factory;
    RetrogradeEnvironment& retrogradeEnvironment;
};
