#include "system_bezel.h"

#include "src/config/bezel_config.h"
#include "src/retrograde/retrograde_environment.h"
#include "src/util/image_cache.h"

SystemBezel::SystemBezel(const RetrogradeEnvironment& env)
	: env(env)
{
}

void SystemBezel::setBezel(const BezelConfig* config)
{
	if (config == curConfig) {
		return;
	}
	curConfig = config;

	images.clear();
	if (config) {
		for (const auto& imgConfig: config->getImages()) {
			images.push_back(makeImage(imgConfig));
		}
	}
}

Vector2f SystemBezel::update(Rect4i windowSize, Vector2f maxScale)
{
	if (!curConfig) {
		return maxScale;
	}

	const auto pos = Vector2f(windowSize.getCenter());
	const auto scale = std::ceil(maxScale.y * curConfig->getDefaultZoom());

	for (auto& image: images) {
		image.sprite
			.setPosition(pos)
			.setScale(scale / image.baseScale);
	}
	
	return maxScale * (scale / maxScale.y);
}

void SystemBezel::draw(Painter& painter, BezelLayer layer) const
{
	for (auto& image: images) {
		if (image.sprite.hasMaterial() && image.layer == layer) {
			image.sprite.draw(painter);
		}
	}
}

SystemBezel::ImageData SystemBezel::makeImage(const BezelImageConfig& imgConfig)
{
	auto sprite = env.getImageCache().getSprite(imgConfig.getImage());
	sprite.setAbsolutePivot(Vector2f(imgConfig.getDisplayCentre()) - sprite.getAbsolutePivot());
	
	return ImageData{ std::move(sprite), imgConfig.getBaseScale(), imgConfig.getLayer() };
}
