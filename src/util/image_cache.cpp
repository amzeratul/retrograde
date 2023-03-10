#include "image_cache.h"

ImageCache::ImageCache(VideoAPI& video, Resources& resources, Path root)
	: video(video)
	, resources(resources)
	, root(std::move(root))
{
}

std::shared_ptr<const Texture> ImageCache::getTexture(std::string_view name)
{
	const auto iter = textures.find(name);
	if (iter != textures.end()) {
		return iter->second;
	}

	auto tex = loadTexture(name);
	textures[name] = tex;
	return tex;
}

Sprite ImageCache::getSprite(std::string_view name, std::string_view materialName)
{
	return toSprite(getTexture(name), materialName);
}

void ImageCache::loadInto(std::shared_ptr<UIImage> uiImage, std::string_view name, std::string_view materialName)
{
	loadIntoOr(std::move(uiImage), name, "", materialName);
}

void ImageCache::loadIntoOr(std::shared_ptr<UIImage> uiImage, std::string_view name, std::string_view fallbackName, std::string_view materialName)
{
	auto tex = getTexture(name);

	if (!tex && !fallbackName.empty()) {
		tex = getTexture(fallbackName);
	}

	if (!tex) {
		uiImage->setSprite(Sprite());
		return;
	}

	if (tex->isLoaded()) {
		uiImage->setSprite(toSprite(std::move(tex), materialName));
	} else {
		// TODO
	}
}

Sprite ImageCache::toSprite(std::shared_ptr<const Texture> tex, std::string_view materialName)
{
	if (!tex) {
		return Sprite();
	}
	return Sprite().setImage(std::move(tex), resources.get<MaterialDefinition>(materialName));
}

std::shared_ptr<Texture> ImageCache::loadTexture(std::string_view name)
{
	auto bytes = Path::readFile(root / name);
	if (bytes.empty()) {
		return {};
	}
	auto image = std::make_unique<Image>(bytes.byte_span(), Image::Format::RGBA);
	image->preMultiply();

	auto tex = std::shared_ptr<Texture>(video.createTexture(image->getSize()));
	TextureDescriptor desc(image->getSize(), TextureFormat::RGBA);
	desc.pixelData = std::move(image);
	desc.useFiltering = true;
	desc.useMipMap = true;
	tex->startLoading();
	tex->load(std::move(desc));
	return tex;
}
