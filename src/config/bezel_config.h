#pragma once

#include <halley.hpp>
using namespace Halley;

enum class BezelLayer {
	Background,
    Foreground
};

namespace Halley {
	template <>
	struct EnumNames<BezelLayer> {
		constexpr std::array<const char*, 2> operator()() const {
			return{{
				"background",
                "foreground"
			}};
		}
	};
}

class BezelImageConfig {
public:
    BezelImageConfig() = default;
    BezelImageConfig(const ConfigNode& node);

    const String& getImage() const;
    Vector2i getDisplayCentre() const;
    float getBaseScale() const;
    BezelLayer getLayer() const;

private:
    String image;
    Vector2i displayCentre;
    float baseScale;
    BezelLayer layer;
};

class BezelConfig {
public:
    BezelConfig() = default;
    BezelConfig(const ConfigNode& node);

    const String& getId() const;
    float getDefaultZoom() const;
    const Vector<BezelImageConfig>& getImages() const;
    HashMap<String, String> getCoreOptions(const String& coreId) const;

private:
    String id;
    float defaultZoom;
    Vector<BezelImageConfig> images;
    HashMap<String, HashMap<String, String>> coreOptions;
};
