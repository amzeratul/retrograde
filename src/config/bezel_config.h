#pragma once

#include <halley.hpp>
using namespace Halley;

class BezelImageConfig {
public:
    BezelImageConfig() = default;
    BezelImageConfig(const ConfigNode& node);

    const String& getImage() const;
    Vector2i getDisplayCentre() const;
    float getBaseScale() const;

private:
    String image;
    Vector2i displayCentre;
    float baseScale;
};

class BezelConfig {
public:
    BezelConfig() = default;
    BezelConfig(const ConfigNode& node);

    const String& getId() const;
    const Vector<BezelImageConfig>& getImages() const;

private:
    String id;
    Vector<BezelImageConfig> images;
};
