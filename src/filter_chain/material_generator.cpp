#include "material_generator.h"

#include "shader_compiler.h"
#include "shader_converter.h"

std::unique_ptr<MaterialDefinition> MaterialGenerator::makeMaterial(VideoAPI& video, const String& name, const ShaderCodeWithReflection& vertex, const ShaderCodeWithReflection& pixel)
{
	assert(vertex.format == pixel.format);

	std::shared_ptr<Shader> shader;
	if (vertex.format == ShaderFormat::HLSL) {
		shader = ShaderCompiler::loadHLSLShader(video, name, vertex.shaderCode, pixel.shaderCode);
	} else {
		throw Exception("Loading shader format not implemented: " + toString(vertex.format), 0);
	}

	// Generate vertex attribute definitions
	Vector<MaterialAttribute> attributes;
	for (const auto& att: vertex.reflection.attributes) {
		auto& attribute = attributes.emplace_back();
		attribute.name = att.name;
		attribute.type = att.type;
		attribute.semantic = att.semantic;
		attribute.semanticIndex = att.semanticIndex;
		attribute.offset = 0;
	}

	// Generate uniform definitions
	Vector<MaterialUniformBlock> uniformBlocks;
	for (const auto& ub: pixel.reflection.uniforms) {
		auto& block = uniformBlocks.emplace_back();
		block.name = ub.name;
		for (const auto& u: ub.params) {
			auto& uniform = block.uniforms.emplace_back();
			uniform.name = u.name;
			uniform.offset = static_cast<uint32_t>(u.offset);
			uniform.predefinedOffset = true;
			uniform.type = u.type;
		}
	}

	// Generate texture definitions
	Vector<MaterialTexture> textures;
	for (const auto& tex: pixel.reflection.textures) {
		textures.push_back(MaterialTexture(tex.name, "", tex.samplerType));
	}

	// Create material definition
	auto materialDefinition = std::make_unique<MaterialDefinition>();
	materialDefinition->setName(name);
	materialDefinition->setAttributes(std::move(attributes));
	materialDefinition->setUniformBlocks(std::move(uniformBlocks));
	materialDefinition->setTextures(std::move(textures));
	materialDefinition->addPass(MaterialPass(std::move(shader)));
	materialDefinition->initialize(video);

	return materialDefinition;
}
