#include "shader_compiler.h"

#ifdef _WIN32
#include <D3Dcompiler.h>
#endif

using namespace Halley;

std::unique_ptr<Shader> ShaderCompiler::loadHLSLShader(VideoAPI& video, const String& name, const Bytes& vertexCode, const Bytes& pixelCode)
{
	ShaderDefinition definition;
	
	definition.name = name;
	definition.shaders[ShaderType::Vertex] = compileHLSL(name + ".vertex.hlsl", vertexCode, ShaderType::Vertex);
	definition.shaders[ShaderType::Pixel] = compileHLSL(name + ".pixel.hlsl", pixelCode, ShaderType::Pixel);

	return video.createShader(definition);
}

Bytes ShaderCompiler::compileHLSL(const String& name, const Bytes& bytes, ShaderType type)
{
#ifdef _WIN32
	String target;

	switch (type) {
	case ShaderType::Vertex:
		target = "vs_4_0";
		break;

	case ShaderType::Pixel:
		target = "ps_4_0";
		break;

	case ShaderType::Geometry:
		target = "gs_4_0";
		break;

	default:
		throw Exception("Unsupported shader type: " + toString(type), HalleyExceptions::Tools);
	}

	ID3D10Blob *codeBlob = nullptr;
	ID3D10Blob *errorBlob = nullptr;
	const int flags = Debug::isDebug() ? D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION : 0;
	const HRESULT hResult = D3DCompile2(bytes.data(), bytes.size(), name.c_str(), nullptr, nullptr, "main", target.c_str(), flags, 0, 0, nullptr, 0, &codeBlob, &errorBlob);
	if (hResult != S_OK) {
		const auto errorMessage = String(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()), errorBlob->GetBufferSize());
		errorBlob->Release();
		throw Exception("Failed to compile shader " + name + " (" + toString(type) + "):\n" + errorMessage, HalleyExceptions::Tools);
	}

	auto result = Bytes(codeBlob->GetBufferSize());
	memcpy_s(result.data(), result.size(), codeBlob->GetBufferPointer(), codeBlob->GetBufferSize());

	if (errorBlob) {
		errorBlob->Release();
	}
	if (codeBlob) {
		codeBlob->Release();
	}

	return result;

#else

	Logger::logWarning("Compiling HLSL shaders is not supported on non-Windows platforms.");
	return Bytes();

#endif
}
