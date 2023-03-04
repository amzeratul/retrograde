#pragma once

#include <halley.hpp>
class BezelConfig;
class RetrogradeEnvironment;
using namespace Halley;

class SystemBezel {
public:
    SystemBezel(const RetrogradeEnvironment& env);

    void setBezel(const BezelConfig* config);

	Rect4i update(Rect4i windowSize);
    void draw(Painter& painter) const;

private:
    const RetrogradeEnvironment& env;
};
