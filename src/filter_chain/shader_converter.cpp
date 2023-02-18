#include "shader_converter.h"

String ShaderConverter::convertShader(const String& src, ShaderFormat inputFormat, ShaderFormat outputFormat)
{
	if (inputFormat == outputFormat) {
		return src;
	}

	// Strategy:
	//  GLSL --> [glslc] --> SPIR-V --> [spirv-cross] --> HLSL

	// TODO
	return {};
}

std::unique_ptr<Shader> ShaderConverter::loadShader(const String& vertexSrc, const String& pixelSrc, VideoAPI& video)
{
	// Maybe move this to video api directly?
	// TODO
	return {};
}
