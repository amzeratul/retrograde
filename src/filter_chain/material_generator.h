#pragma once

#include <halley.hpp>

using namespace Halley;

class ShaderCodeWithReflection;

class MaterialGenerator {
public:
	static std::unique_ptr<MaterialDefinition> makeMaterial(VideoAPI& video, const String& name, const ShaderCodeWithReflection& vertex, const ShaderCodeWithReflection& pixel);
};
