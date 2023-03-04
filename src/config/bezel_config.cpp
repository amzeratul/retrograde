#include "bezel_config.h"

BezelImageConfig::BezelImageConfig(const ConfigNode& node)
{
	image = node["image"].asString("image");
	displayCentre = node["displayCentre"].asVector2i({});
	baseScale = node["baseScale"].asFloat(1.0f);
	layer = node["layer"].asEnum(BezelLayer::Foreground);
}

const String& BezelImageConfig::getImage() const
{
	return image;
}

Vector2i BezelImageConfig::getDisplayCentre() const
{
	return displayCentre;
}

float BezelImageConfig::getBaseScale() const
{
	return baseScale;
}

BezelLayer BezelImageConfig::getLayer() const
{
	return layer;
}


BezelConfig::BezelConfig(const ConfigNode& node)
{
	id = node["id"].asString();
	defaultZoom = node["defaultZoom"].asFloat(0.5f);
	images = node["images"].asVector<BezelImageConfig>({});
	if (node.hasKey("coreOptions")) {
		coreOptions = node["coreOptions"].asHashMap<String, HashMap<String, String>>();
	}
}

const String& BezelConfig::getId() const
{
	return id;
}

float BezelConfig::getDefaultZoom() const
{
	return defaultZoom;
}

const Vector<BezelImageConfig>& BezelConfig::getImages() const
{
	return images;
}

HashMap<String, String> BezelConfig::getCoreOptions(const String& coreId) const
{
	const auto iter = coreOptions.find(coreId);
	if (iter != coreOptions.end()) {
		return iter->second;
	}
	return {};
}
