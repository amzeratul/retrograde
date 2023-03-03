#include <halley.hpp>
using namespace Halley;

class ScreenFilterPresetConfig {
public:
    ScreenFilterPresetConfig() = default;
    ScreenFilterPresetConfig(const ConfigNode& node);

    const String& getShader() const;
    int getMinResY() const;

private:
    String shader;
    int minResY;
};

class ScreenFilterConfig {
public:
    ScreenFilterConfig() = default;
    ScreenFilterConfig(const ConfigNode& node);

    const String& getId() const;
    const Vector<ScreenFilterPresetConfig>& getPresets() const;

    const String& getShaderFor(Vector2i resolution) const;

private:
    String id;
    Vector<ScreenFilterPresetConfig> presets;
};
