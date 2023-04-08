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
	return toSprite(getTexture(name, trim), materialName);
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
		uiImage->setMinSize(Vector2f{});
	} else {
		String mat = materialName;
		tex->onLoad().then(Executors::getMainUpdateThread(), [this, tex, uiImage, mat] ()
		{
			uiImage->setSprite(toSprite(tex, mat));
			uiImage->setMinSize(Vector2f{});
		});
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
