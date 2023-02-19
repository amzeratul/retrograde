#include "retroarch_filter_chain.h"

#include "material_generator.h"
#include "retroarch_shader_parser.h"
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


void RetroarchFilterChain::Stage::loadMaterial(ShaderConverter& converter, VideoAPI& video)
{
	const String name = shaderPath.getFilename().replaceExtension("").getString();
	const auto parsed = RetroarchShaderParser::parse(shaderPath);

	const auto outputFormat = fromString<ShaderFormat>(video.getShaderLanguage());
	const auto vertexShader = converter.convertShader(parsed.vertexShader, ShaderStage::Vertex, ShaderFormat::GLSL, outputFormat);
	const auto pixelShader = converter.convertShader(parsed.pixelShader, ShaderStage::Pixel, ShaderFormat::GLSL, outputFormat);

	materialDefinition = MaterialGenerator::makeMaterial(video, name, vertexShader, pixelShader);
	material = std::make_shared<Material>(materialDefinition);

	if (false) {
		Path::writeFile("../tmp/" + shaderPath.getFilename().replaceExtension(".vertex." + toString(outputFormat)), vertexShader.shaderCode);
		Path::writeFile("../tmp/" + shaderPath.getFilename().replaceExtension(".pixel." + toString(outputFormat)), pixelShader.shaderCode);
	}

	// TODO: render filtering, mipmapping, float framebuffer, srgb framebuffer, wrap mode
	RenderSurfaceOptions options;
	options.name = name;
	options.createDepthStencil = false;
	options.useFiltering = false;
	renderSurface = std::make_unique<RenderSurface>(video, options);
}

void RetroarchFilterChain::Stage::applyParams(const ConfigNode& params)
{
	for (const auto& ub: materialDefinition->getUniformBlocks()) {
		for (const auto& u: ub.uniforms) {
			if (params.hasKey(u.name)) {
				material->set(u.name, params[u.name].asFloat());
			}
		}
	}
}

Vector2i RetroarchFilterChain::Stage::updateSize(Vector2i sourceSize, Vector2i viewPortSize)
{
	Vector2f sizeFloat;

	if (scaleTypeX == RetroarchScaleType::Absolute) {
		sizeFloat.x = scale.x;
	} else if (scaleTypeX == RetroarchScaleType::Source) {
		sizeFloat.x = scale.x * sourceSize.x;
	} else if (scaleTypeX == RetroarchScaleType::Viewport) {
		sizeFloat.x = scale.x * viewPortSize.x;
	}

	if (scaleTypeY == RetroarchScaleType::Absolute) {
		sizeFloat.y = scale.y;
	} else if (scaleTypeY == RetroarchScaleType::Source) {
		sizeFloat.y = scale.y * sourceSize.y;
	} else if (scaleTypeY == RetroarchScaleType::Viewport) {
		sizeFloat.y = scale.y * viewPortSize.y;
	}

	size = Vector2i(sizeFloat.round());
	renderSurface->setSize(size);
	return size;
}


RetroarchFilterChain::RetroarchFilterChain(Path _path, VideoAPI& video)
	: path(std::move(_path))
{
	const auto params = parsePreset(path);
	loadStages(params, video);
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
		stage.loadMaterial(converter, video);
		stage.applyParams(params);
	}
}

Sprite RetroarchFilterChain::run(const Sprite& src, RenderContext& rc, Vector2i viewPortSize)
{
	if (stages.empty()) {
		return src;
	}

	// Set stage sizes
	Vector2i sourceSize = src.getMaterial().getTexture(0)->getSize();
	for (auto& stage: stages) {
		sourceSize = stage.updateSize(sourceSize, viewPortSize);
	}

	// Draw stages
	for (size_t i = 0; i < stages.size(); ++i) {
		auto& stage = stages[i];

		setupStageMaterial(i, sourceSize);

		rc.with(stage.renderSurface->getRenderTarget()).bind([&] (Painter& painter)
		{
			drawStage(stage, static_cast<int>(i), painter);
		});
	}

	++frameNumber;

	return stages.back().renderSurface->getSurfaceSprite(src.getMaterialPtr()->clone());
}


void RetroarchFilterChain::setupStageMaterial(size_t stageIdx, Vector2i viewPortSize)
{
	auto& stage = stages[stageIdx];
	for (const auto& ub: stage.materialDefinition->getUniformBlocks()) {
		for (const auto& u: ub.uniforms) {
			updateParameter(u.name, *stage.material);
		}
	}
	
	for (const auto& tex: stage.materialDefinition->getTextures()) {
		updateTexture(tex.name, *stage.material);
	}
}

void RetroarchFilterChain::updateParameter(const String& name, Material& material)
{
	// TODO
}

void RetroarchFilterChain::updateTexture(const String& name, Material& material)
{
	// TODO
}

void RetroarchFilterChain::drawStage(const Stage& stage, int stageIdx, Painter& painter)
{
	// TODO
	painter.clear(Colour4f(1, 0, (stageIdx + 1) / 10.0f));
}
