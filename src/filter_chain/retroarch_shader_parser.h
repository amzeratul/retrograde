#pragma once

#include <halley.hpp>

using namespace Halley;

class RetroarchShaderParser {
public:
	struct Result {
		String vertexShader;
		String pixelShader;
	};

	static Result parse(const Path& path);
};
