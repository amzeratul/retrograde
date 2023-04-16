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
    void clear();

private:
    VideoAPI& video;
    Resources& resources;
    Path root;
    uint64_t requestNumber = 0;

    struct Request {
        uint64_t mostRecent = 0;
        uint64_t count = 0;
    };

    HashMap<String, std::shared_ptr<Texture>> textures;
    HashMap<const UIImage*, Request> pendingRequests;

    std::shared_ptr<Texture> loadTexture(std::string_view name, bool trim);
    Sprite toSprite(std::shared_ptr<const Texture> tex, std::string_view materialName);

    uint64_t startRequest(const UIImage& dst);
    bool fulfillRequest(const UIImage& dst, uint64_t id);
};
