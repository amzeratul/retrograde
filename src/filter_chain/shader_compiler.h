#pragma once

#include <halley.hpp>

using Halley::Shader;
using Halley::Bytes;
using Halley::VideoAPI;
using Halley::String;
using Halley::ShaderType;

class ShaderCompiler {
public:
	static std::shared_ptr<Shader> loadHLSLShader(VideoAPI& video, const String& name, const Bytes& vertexCode, const Bytes& pixelCode);

private:
	static Halley::HashMap<uint64_t, std::shared_ptr<Shader>> cache;

	static Bytes compileHLSL(const String& name, const Bytes& code, ShaderType type);
};
