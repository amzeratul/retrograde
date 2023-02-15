#pragma once

#include <halley.hpp>
using namespace Halley;

class RetrogradeEnvironment;

class ChooseSystemWindow : public UIWidget {
public:
    ChooseSystemWindow(UIFactory& factory, RetrogradeEnvironment& retrogradeEnvironment);

    void onMakeUI() override;

private:
    UIFactory& factory;
    RetrogradeEnvironment& retrogradeEnvironment;
};
