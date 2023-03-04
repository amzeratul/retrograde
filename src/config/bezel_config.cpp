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
	images = node["images"].asVector<BezelImageConfig>({});
}

const String& BezelConfig::getId() const
{
	return id;
}

const Vector<BezelImageConfig>& BezelConfig::getImages() const
{
	return images;
}
