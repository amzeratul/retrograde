#pragma once

#include <halley.hpp>

using namespace Halley;

class RetroarchShaderParser {
public:
	struct Parameter {
		String name;
		String description;
		float initial = 0;
		float minimum = 0;
		float maximum = 0;
		std::optional<float> step;

		Parameter() = default;
		Parameter(const String& str);
	};

	struct Result {
		String vertexShader;
		String pixelShader;

		String name;
		String format;
		Vector<Parameter> parameters;
	};

	static Result parse(const Path& path);

private:
	static Vector<String> readFileWithIncludes(const Path& path);
	static Vector<String> processIncludes(Vector<String> orig, const Path& origDir);
};
