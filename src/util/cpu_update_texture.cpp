#include "cpu_update_texture.h"

CPUUpdateTexture::CPUUpdateTexture(VideoAPI& videoAPI)
	: videoAPI(videoAPI)
{
}

std::shared_ptr<const Texture> CPUUpdateTexture::getTexture() const
{
	return texture;
}

void CPUUpdateTexture::update(Vector2i size, std::optional<int> stride, gsl::span<const gsl::byte> data, TextureFormat textureFormat)
{
	if (!texture || texture->getSize() != size) {
		texture = videoAPI.createTexture(size);
	}
	updateTexture(data, stride, textureFormat);
}

void CPUUpdateTexture::updateTexture(gsl::span<const gsl::byte> data, std::optional<int> stride, TextureFormat textureFormat)
{
	texture->startLoading();
	auto texDesc = TextureDescriptor(texture->getSize(), TextureFormat::RGBA);
	texDesc.canBeUpdated = true;
	texDesc.format = textureFormat;
	texDesc.pixelFormat = PixelDataFormat::Image;
	texDesc.pixelData = TextureDescriptorImageData(data, stride);
	texture->load(std::move(texDesc));
}
