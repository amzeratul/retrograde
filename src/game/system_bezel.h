#pragma once

#include <halley.hpp>

#include "src/config/bezel_config.h"
class BezelConfig;
class RetrogradeEnvironment;
using namespace Halley;

class SystemBezel {
public:
    SystemBezel(const RetrogradeEnvironment& env);

    void setBezel(const BezelConfig* config);

	Vector2f update(Rect4i windowSize, Vector2f maxScale);
    void draw(Painter& painter) const;

private:
    struct ImageData {
        Sprite sprite;
        float baseScale = 1;
    };

    const RetrogradeEnvironment& env;

	const BezelConfig* curConfig = nullptr;
    Vector<ImageData> images;

    ImageData makeImage(const BezelImageConfig& imgConfig);
};
