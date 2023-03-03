#pragma once

#include <halley.hpp>

using Halley::Shader;
using Halley::Bytes;
using Halley::VideoAPI;
using Halley::String;
using Halley::ShaderType;

class ShaderCompiler {
public:
	static std::unique_ptr<Shader> loadHLSLShader(VideoAPI& video, const String& name, const Bytes& vertexCode, const Bytes& pixelCode);
	static const Bytes& compileHLSL(const String& name, const Bytes& code, ShaderType type);

private:
	static Halley::HashMap<uint64_t, Bytes> cache;

	static Bytes doCompileHLSL(const String& name, const Bytes& code, ShaderType type);
};
