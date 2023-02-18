#include "shader_converter.h"

String ShaderConverter::convertShader(const String& src, ShaderFormat inputFormat, ShaderFormat outputFormat)
{
	if (inputFormat == outputFormat) {
		return src;
	}

	// TODO
	return {};
}

std::unique_ptr<Shader> ShaderConverter::loadShader(const String& vertexSrc, const String& pixelSrc, VideoAPI& video)
{
	// TODO
	return {};
}
