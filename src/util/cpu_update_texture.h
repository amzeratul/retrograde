#pragma once

#include <halley.hpp>
using namespace Halley;

class CPUUpdateTexture {
public:
    CPUUpdateTexture(VideoAPI& videoAPI);

    std::shared_ptr<Texture> getTexture() const;

    void updateSize(Vector2i size);
	void update(Vector2i size, std::optional<int> stride, gsl::span<const gsl::byte> data, TextureFormat textureFormat);
    
private:
    VideoAPI& videoAPI;

    std::shared_ptr<Texture> texture;

    void updateTexture(gsl::span<const gsl::byte> data, std::optional<int> stride, TextureFormat textureFormat);
};
