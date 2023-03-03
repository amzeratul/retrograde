#include "screen_filter_config.h"

ScreenFilterPresetConfig::ScreenFilterPresetConfig(const ConfigNode& node)
{
	shader = node["shader"].asString();
	minResY = node["minResY"].asInt(0);
}

const String& ScreenFilterPresetConfig::getShader() const
{
	return shader;
}

int ScreenFilterPresetConfig::getMinResY() const
{
	return minResY;
}


ScreenFilterConfig::ScreenFilterConfig(const ConfigNode& node)
{
	id = node["id"].asString();
	presets = node["presets"].asVector<ScreenFilterPresetConfig>({});
}

const String& ScreenFilterConfig::getId() const
{
	return id;
}

const Vector<ScreenFilterPresetConfig>& ScreenFilterConfig::getPresets() const
{
	return presets;
}

const String& ScreenFilterConfig::getShaderFor(Vector2i resolution) const
{
	for (auto& preset: presets) {
		if (resolution.y > preset.getMinResY()) {
			return preset.getShader();
		}
	}

	static String dummy;
	return dummy;
}
