#pragma once

#include <halley.hpp>
using namespace Halley;

class ImageCache {
public:
    ImageCache(VideoAPI& video, Resources& resources, Path root);

    std::shared_ptr<const Texture> getTexture(std::string_view name);
    Sprite getSprite(std::string_view name, std::string_view materialName = "Halley/Sprite");
    void loadInto(std::shared_ptr<UIImage> uiImage, std::string_view name, std::string_view materialName = "Halley/Sprite");
    void loadIntoOr(std::shared_ptr<UIImage> uiImage, std::string_view name, std::string_view fallbackName, std::string_view materialName = "Halley/Sprite");

private:
    VideoAPI& video;
    Resources& resources;
    Path root;

    HashMap<String, std::shared_ptr<Texture>> textures;

    std::shared_ptr<Texture> loadTexture(std::string_view name);
    Sprite toSprite(std::shared_ptr<const Texture> tex, std::string_view materialName);
};
