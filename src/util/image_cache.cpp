#include "image_cache.h"

ImageCache::ImageCache(VideoAPI& video, Resources& resources, Path root)
	: video(video)
	, resources(resources)
	, root(std::move(root))
{
}

std::shared_ptr<const Texture> ImageCache::getTexture(std::string_view name, bool trim)
{
	if (name.empty()) {
		return {};
	}

	const auto iter = textures.find(name);
	if (iter != textures.end()) {
		return iter->second;
	}

	auto tex = loadTexture(name, trim);
	textures[name] = tex;
	return tex;
}

Sprite ImageCache::getSprite(std::string_view name, std::string_view materialName, bool trim)
{
	auto tex = getTexture(name, trim);
	tex->waitForLoad();
	return toSprite(tex, materialName);
}

void ImageCache::loadInto(std::shared_ptr<UIImage> uiImage, std::string_view name, std::string_view materialName, std::optional<Vector2f> maxSize)
{
	loadIntoOr(std::move(uiImage), name, "", materialName, maxSize);
}

void ImageCache::loadIntoOr(std::shared_ptr<UIImage> uiImage, std::string_view name, std::string_view fallbackName, std::string_view materialName, std::optional<Vector2f> maxSize)
{
	auto tex = getTexture(name);

	if (!tex && !fallbackName.empty()) {
		tex = getTexture(fallbackName);
	}

	if (!tex) {
		uiImage->setSprite(Sprite());
		uiImage->sendEvent(UIEvent(UIEventType::ImageUpdated, uiImage->getId(), ConfigNode(Vector2f())));
		return;
	}

	String mat = materialName;
	auto doUpdate = [this, tex, uiImage, mat, maxSize]()
	{
		uiImage->setSprite(toSprite(tex, mat));
		if (maxSize) {
			const auto size = uiImage->getMinimumSize();
			const auto scale = size / *maxSize;
			const auto finalSize = size / std::max(scale.x, scale.y);
			uiImage->setMinSize(finalSize);
		}
		uiImage->sendEvent(UIEvent(UIEventType::ImageUpdated, uiImage->getId(), ConfigNode(uiImage->getMinimumSize())));
	};

	if (tex->isLoaded()) {
		doUpdate();
	} else {
		tex->onLoad().then(Executors::getMainUpdateThread(), doUpdate);
	}
}

Sprite ImageCache::toSprite(std::shared_ptr<const Texture> tex, std::string_view materialName)
{
	if (!tex) {
		return Sprite();
	}
	const auto trimRect = tex->getMeta().getValue("trimRect").asVector4f();
	return Sprite()
		.setImage(std::move(tex), resources.get<MaterialDefinition>(materialName))
		.setAbsolutePivot(trimRect.xy());
}

std::shared_ptr<Texture> ImageCache::loadTexture(std::string_view name, bool trim)
{
	const auto path = Path(name).isAbsolute() ? Path(name) : root / name;
	if (!Path::exists(path)) {
		return {};
	}

	auto tex = std::shared_ptr<Texture>(video.createTexture(Vector2i()));

	Concurrent::execute(Executors::getCPU(), [path, trim, tex] () -> std::unique_ptr<Image>
	{
		auto bytes = Path::readFile(path);
		if (bytes.empty()) {
			return {};
		}

		auto image = std::make_unique<Image>(bytes.byte_span(), Image::Format::RGBA);
		const auto origRect = image->getRect();
		auto trimRect = origRect;

		if (trim) {
			trimRect = image->getTrimRect();
			if (trimRect.getSize() != image->getSize()) {
				auto image2 = std::make_unique<Image>(Image::Format::RGBA, trimRect.getSize());
				image2->blitFrom(Vector2i(), *image, trimRect);
				image = std::move(image2);
			}
		}
		image->preMultiply();

		Metadata meta;
		meta.set("trimRect", ConfigNode(trimRect));
		meta.set("origRect", ConfigNode(origRect));
		tex->setMeta(meta);

		return image;
	}).then(Executors::getVideoAux(), [=] (std::unique_ptr<Image> image) {
		if (image) {
			TextureDescriptor desc(image->getSize(), TextureFormat::RGBA);
			desc.pixelData = std::move(image);
			desc.useFiltering = true;
			desc.useMipMap = true;
			tex->load(std::move(desc));
			tex->doneLoading();
		} else {
			tex->loadingFailed();
		}
	});

	return tex;
}
