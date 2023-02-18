#pragma once

#include <halley.hpp>

using namespace Halley;

enum class ShaderFormat {
    GLSL,
    HLSL,
	SPIRV
};

enum class ShaderStage {
	Vertex,
	Pixel
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
	ShaderConverter();
	~ShaderConverter();

    String convertShader(const String& src, ShaderStage stage, ShaderFormat inputFormat, ShaderFormat outputFormat);
    static std::unique_ptr<Shader> loadShader(const String& vertexSrc, const String& pixelSrc, VideoAPI& video);

private:
	static size_t nInstances;
};
