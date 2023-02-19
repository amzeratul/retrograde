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

namespace spirv_cross {
	class Compiler;
}

class ShaderConverter {
public:
	struct Param {
		String name;
		String bufferName;
		size_t offset = 0;
		size_t size = 0;
	};
	struct Result {
		Bytes shaderCode;
		Vector<Param> params;

		Result(Bytes shaderCode = {}) : shaderCode(std::move(shaderCode)) {}
	};

	ShaderConverter();
	~ShaderConverter();

    Result convertShader(const String& src, ShaderStage stage, ShaderFormat inputFormat, ShaderFormat outputFormat);

private:
	static size_t nInstances;

	Bytes convertToSpirv(const String& src, ShaderStage stage, ShaderFormat inputFormat);
	Result convertSpirvToHLSL(const Bytes& spirv);
	void getReflectionInfo(spirv_cross::Compiler& compiler, Result& output);
};
