#include "retroarch_filter_chain.h"
#include "retroarch_shader_parser.h"
#include "shader_compiler.h"
#include "shader_converter.h"

// Details: https://github.com/libretro/slang-shaders


RetroarchFilterChain::Stage::Stage(int idx, const ConfigNode& params, const Path& basePath)
{
	const auto idxStr = toString(idx);
	shaderPath = basePath / params["shader" + idxStr].asString();
	alias = params["alias" + idxStr].asString("");
	filterLinear = params["filter_linear" + idxStr].asBool(false);
	mipMapInput = params["mipmap_input" + idxStr].asBool(false);
	wrapMode = params["wrap_mode" + idxStr].asEnum(RetroarchWrapMode::ClampToEdge);
	floatFramebuffer = params["float_framebuffer" + idxStr].asBool(false);
	srgbFramebuffer = params["srgb_framebuffer" + idxStr].asBool(false);
	scaleTypeX = params["scale_type_x" + idxStr].asEnum(RetroarchScaleType::Source);
	scaleTypeY = params["scale_type_y" + idxStr].asEnum(RetroarchScaleType::Source);
	scale.x = params["scale_x" + idxStr].asFloat(1.0f);
	scale.y = params["scale_y" + idxStr].asFloat(1.0f);
}


void RetroarchFilterChain::Stage::loadShader(ShaderConverter& converter, VideoAPI& video, const ConfigNode& params)
{
	const auto parsed = RetroarchShaderParser::parse(shaderPath);

	const auto outputFormat = fromString<ShaderFormat>(video.getShaderLanguage());
	const auto vertexShader = converter.convertShader(parsed.vertexShader, ShaderStage::Vertex, ShaderFormat::GLSL, outputFormat);
	const auto pixelShader = converter.convertShader(parsed.pixelShader, ShaderStage::Pixel, ShaderFormat::GLSL, outputFormat);

	const bool debugOutput = false;
	if (debugOutput) {
		Path::writeFile("../tmp/" + shaderPath.getFilename().replaceExtension(".vertex.hlsl"), vertexShader.shaderCode);
		Path::writeFile("../tmp/" + shaderPath.getFilename().replaceExtension(".pixel.hlsl"), pixelShader.shaderCode);
	}

	const String name = shaderPath.getFilename().replaceExtension("").getString();
	if (outputFormat == ShaderFormat::HLSL) {
		shader = ShaderCompiler::loadHLSLShader(video, name, vertexShader.shaderCode, pixelShader.shaderCode);
	} else {
		Logger::logError("Loading shader format not implemented: " + toString(outputFormat));
	}

	loadMaterial(video, name, params, vertexShader.reflection, pixelShader.reflection);
}

void RetroarchFilterChain::Stage::loadMaterial(VideoAPI& video, const String& name, const ConfigNode& params, const ShaderReflection& vertexReflection, const ShaderReflection& pixelReflection)
{
	// Generate vertex attribute definitions
	// TODO: attributes
	Vector<MaterialAttribute> attributes;

	// Generate uniform definitions
	Vector<MaterialUniformBlock> uniformBlocks;
	for (const auto& ub: pixelReflection.uniforms) {
		auto& block = uniformBlocks.emplace_back();
		block.name = ub.name;
		for (const auto& u: ub.params) {
			auto& uniform = block.uniforms.emplace_back();
			uniform.name = u.name;
			uniform.offset = static_cast<uint32_t>(u.offset);
			uniform.predefinedOffset = true;
		}
	}

	// Generate texture definitions
	Vector<MaterialTexture> textures;
	for (const auto& tex: pixelReflection.textures) {
		textures.push_back(MaterialTexture(tex.name, "", tex.samplerType));
	}

	// Create material definition
	materialDefinition = std::make_shared<MaterialDefinition>();
	materialDefinition->setName(name);
	materialDefinition->setAttributes(std::move(attributes));
	materialDefinition->setUniformBlocks(std::move(uniformBlocks));
	materialDefinition->setTextures(std::move(textures));
	materialDefinition->addPass(MaterialPass(shader));
	materialDefinition->initialize(video);

	// Create material instance
	material = std::make_shared<Material>(materialDefinition);

	// Set initial parameters
	for (const auto& ub: pixelReflection.uniforms) {
		for (const auto& param: ub.params) {
			if (params.hasKey(param.name)) {
				material->set(param.name, params[param.name].asFloat());
			}
		}
	}
}


RetroarchFilterChain::RetroarchFilterChain(Path _path, VideoAPI& video)
	: path(std::move(_path))
{
	const auto params = parsePreset(path);
	loadStages(params, video);
}

Sprite RetroarchFilterChain::run(const Sprite& src, RenderContext& rc)
{
	// TODO
	return src;
}

ConfigNode RetroarchFilterChain::parsePreset(const Path& path)
{
	ConfigNode::MapType result;

	for (auto& line: Path::readFileLines(path)) {
		parsePresetLine(line, result);
	}

	return result;
}

void RetroarchFilterChain::parsePresetLine(std::string_view str, ConfigNode::MapType& dst)
{
	const auto eqSign = str.find('=');
	if (eqSign != std::string_view::npos) {
		auto key = String(str.substr(0, eqSign));
		auto value = String(str.substr(eqSign + 1));
		key.trimBoth();
		value.trimBoth();

		if (value.startsWith("\"")) {
			value = value.mid(1);
		}
		if (value.endsWith("\"")) {
			value = value.left(value.size() - 1);
		}

		dst[std::move(key)] = ConfigNode(std::move(value));
	}
}

void RetroarchFilterChain::loadStages(const ConfigNode& params, VideoAPI& video)
{
	const int nShaders = params["shaders"].asInt(0);
	for (int i = 0; i < nShaders; ++i) {
		stages.push_back(Stage(i, params, path.parentPath()));
	}

	ShaderConverter converter;
	for (auto& stage: stages) {
		stage.loadShader(converter, video, params);
	}
}
