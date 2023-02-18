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
	struct Result {
		Bytes shaderCode;
	};

	ShaderConverter();
	~ShaderConverter();

    Result convertShader(const String& src, ShaderStage stage, ShaderFormat inputFormat, ShaderFormat outputFormat);
    static std::unique_ptr<Shader> loadShader(gsl::span<const gsl::byte> vertexSrc, gsl::span<const gsl::byte> pixelSrc, VideoAPI& video);

private:
	static size_t nInstances;

	Bytes convertToSpirv(const String& src, ShaderStage stage, ShaderFormat inputFormat);
	String convertSpirvToHLSL(const Bytes& spirv, ShaderStage stage);
};
