#include "shader_converter.h"

String ShaderConverter::convertShader(const String& src, ShaderFormat inputFormat, ShaderFormat outputFormat)
{
	if (inputFormat == outputFormat) {
		return src;
	}

	// Strategy: convert shader to SPIR-V via glslc, then SPIR-V to HLSL via spirv-cross
	// TODO
	return {};
}

std::unique_ptr<Shader> ShaderConverter::loadShader(const String& vertexSrc, const String& pixelSrc, VideoAPI& video)
{
	// Maybe move this to video api directly?
	// TODO
	return {};
}
