#pragma once

#include <halley.hpp>

using namespace Halley;

enum class ShaderFormat {
    GLSL,
    HLSL,
	SPIRV,
	MSL
};

enum class ShaderStage {
	Vertex,
	Pixel
};

namespace Halley {
	template <>
	struct EnumNames<ShaderFormat> {
		constexpr std::array<const char*, 4> operator()() const {
			return{ {
				"glsl",
				"hlsl",
				"spirv",
				"msl"
			} };
		}
	};
}

namespace spirv_cross {
	struct SPIRType;
	class Compiler;
}

class ShaderReflection {
public:
	struct Param {
		String name;
		size_t offset = 0;
		size_t size = 0;
		ShaderParameterType type = ShaderParameterType::Invalid;
	};

	struct Block {
		String name;
		std::optional<size_t> binding;
		Vector<Param> params;
	};

	struct Texture {
		String name;
		std::optional<size_t> binding;
		TextureSamplerType samplerType = TextureSamplerType::Texture2D;
	};

	Vector<Block> uniforms;
	Vector<Texture> textures;
};

class ShaderConverter {
public:
	struct Result {
		Bytes shaderCode;
		ShaderReflection reflection;

		Result(Bytes shaderCode = {}) : shaderCode(std::move(shaderCode)) {}
	};

	ShaderConverter();
	~ShaderConverter();

    Result convertShader(const String& src, ShaderStage stage, ShaderFormat inputFormat, ShaderFormat outputFormat);

private:
	static size_t nInstances;

	Bytes convertToSpirv(const String& src, ShaderStage stage, ShaderFormat inputFormat);
	Result convertSpirvToHLSL(const Bytes& spirv);
	ShaderReflection getReflectionInfo(spirv_cross::Compiler& compiler);
	ShaderParameterType getParameterType(const spirv_cross::SPIRType& type);
	TextureSamplerType getSamplerType(const spirv_cross::SPIRType& type);
};
