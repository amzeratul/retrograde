#pragma once

#include <halley.hpp>
using namespace Halley;

class ImageCache {
public:
    ImageCache(VideoAPI& video, Resources& resources, Path root);

    std::shared_ptr<const Texture> getTexture(std::string_view name, bool trim = false);
    Sprite getSprite(std::string_view name, std::string_view materialName = "Halley/Sprite", bool trim = false);
    void loadInto(std::shared_ptr<UIImage> uiImage, std::string_view name, std::string_view materialName = "Halley/Sprite", std::optional<Vector2f> maxSize = {});
    void loadIntoOr(std::shared_ptr<UIImage> uiImage, std::string_view name, std::string_view fallbackName, std::string_view materialName = "Halley/Sprite", std::optional<Vector2f> maxSize = {});

private:
    VideoAPI& video;
    Resources& resources;
    Path root;

    HashMap<String, std::shared_ptr<Texture>> textures;

    std::shared_ptr<Texture> loadTexture(std::string_view name, bool trim);
    Sprite toSprite(std::shared_ptr<const Texture> tex, std::string_view materialName);
};
