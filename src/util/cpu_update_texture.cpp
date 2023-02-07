#include "cpu_update_texture.h"

CPUUpdateTexture::CPUUpdateTexture(VideoAPI& videoAPI)
	: videoAPI(videoAPI)
{
}

std::shared_ptr<Texture> CPUUpdateTexture::getTexture() const
{
	return texture;
}

void CPUUpdateTexture::updateSize(Vector2i size)
{
	if (!texture || texture->getSize() != size) {
		texture = videoAPI.createTexture(size);
	}
}

void CPUUpdateTexture::update(Vector2i size, std::optional<int> stride, gsl::span<const gsl::byte> data, TextureFormat textureFormat)
{
	updateSize(size);
	updateTexture(data, stride, textureFormat);
}

void CPUUpdateTexture::updateTexture(gsl::span<const gsl::byte> data, std::optional<int> stride, TextureFormat textureFormat)
{
	// TODO: implement texture updating API
	texture->startLoading();
	auto texDesc = TextureDescriptor(texture->getSize(), TextureFormat::RGBA);
	texDesc.canBeUpdated = true;
	texDesc.format = textureFormat;
	texDesc.pixelFormat = PixelDataFormat::Image;
	texDesc.pixelData = TextureDescriptorImageData(data, stride);
	texture->load(std::move(texDesc));
}
