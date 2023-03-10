#include "system_bezel.h"

#include "src/config/bezel_config.h"
#include "src/retrograde/retrograde_environment.h"

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
	// Read
	const auto bytes = Path::readFile(env.getImagesDir() / imgConfig.getImage());
	if (bytes.empty()) {
		Logger::logError("Bezel image not found: " + imgConfig.getImage());
		return {};
	}

	// Load
	auto img = std::make_shared<Image>(bytes.byte_span(), Image::Format::RGBA);
	if (!img) {
		Logger::logError("Unable to load bezel image: " + imgConfig.getImage());
		return {};
	}

	// Trim
	auto rect = img->getTrimRect();
	if (rect.getSize() != img->getSize()) {
		auto img2 = std::make_shared<Image>(Image::Format::RGBA, rect.getSize());
		img2->blitFrom(Vector2i(), *img, rect);
		img = std::move(img2);
	}

	// Premultiply
	img->preMultiply();

	// Load texture
	auto tex = std::shared_ptr<Texture>(env.getHalleyAPI().video->createTexture(img->getSize()));
	TextureDescriptor desc(img->getSize(), TextureFormat::RGBA);
	desc.pixelData = std::move(img);
	desc.useFiltering = true;
	desc.useMipMap = true;
	tex->startLoading();
	tex->load(std::move(desc));

	// Setup sprite
	auto sprite = Sprite()
		.setImage(tex, env.getResources().get<MaterialDefinition>("Halley/Sprite"))
		.setImageData(*tex)
		.setAbsolutePivot(Vector2f(imgConfig.getDisplayCentre() - rect.getTopLeft()));

	return ImageData{ std::move(sprite), imgConfig.getBaseScale(), imgConfig.getLayer() };
}
