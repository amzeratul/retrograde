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

Vector2f SystemBezel::update(Rect4i windowSize, Rect4i canvasSize, Vector2f canvasScale, float zoom)
{
	if (!curConfig) {
		return canvasScale;
	}

	const auto pos = Vector2f(canvasSize.getCenter());

	const auto scale = std::ceil(canvasScale.y * zoom * curConfig->getDefaultZoom()) / zoom;

	for (auto& image: images) {
		image.sprite
			.setPosition(pos)
			.setScale(scale / image.baseScale);
	}
	
	return canvasScale * (scale / canvasScale.y);
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
	auto sprite = env.getImageCache().getSprite(imgConfig.getImage(), "Halley/Sprite", true);
	sprite.setAbsolutePivot(Vector2f(imgConfig.getDisplayCentre()) - sprite.getAbsolutePivot());
	
	return ImageData{ std::move(sprite), imgConfig.getBaseScale(), imgConfig.getLayer() };
}
