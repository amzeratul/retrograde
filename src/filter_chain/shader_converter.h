#pragma once

#include <halley.hpp>

using namespace Halley;

enum class ShaderFormat {
    GLSL,
    HLSL
};

namespace Halley {
	template <>
	struct EnumNames<ShaderFormat> {
		constexpr std::array<const char*, 2> operator()() const {
			return{ {
				"glsl",
				"hlsl"
			} };
		}
	};
}

class ShaderConverter {
public:
    static String convertShader(const String& src, ShaderFormat inputFormat, ShaderFormat outputFormat);
    static std::unique_ptr<Shader> loadShader(const String& vertexSrc, const String& pixelSrc, VideoAPI& video);
};
