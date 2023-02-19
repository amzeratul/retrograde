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

	params = ConfigNode::MapType();
	for (const auto& param: parsed.parameters) {
		params[param.name] = param.initial;
	}
	shaderRenderFormat = parsed.format;
	shaderName = parsed.name;

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
	options.powerOfTwo = false;
	renderSurface = std::make_unique<RenderSurface>(video, options);
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
	params = parsePreset(path);
	loadStages(params, video);
	loadTextures(params, video);
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
	stages.clear();
	const int nShaders = params["shaders"].asInt(0);
	for (int i = 0; i < nShaders; ++i) {
		stages.push_back(Stage(i, params, path.parentPath()));
	}

	ShaderConverter converter;
	for (auto& stage: stages) {
		stage.loadMaterial(converter, video);
		setupStageMaterial(stage);
	}
}

void RetroarchFilterChain::loadTextures(const ConfigNode& params, VideoAPI& video)
{
	textures.clear();
	const auto textureNames = params["textures"].asString("").split(';');
	for (const auto& name: textureNames) {
		const auto relPath = params[name].asString("");
		if (relPath.isEmpty()) {
			Logger::logWarning("Texture details not found for " + name);
			continue;
		}
		const auto texPath = path.parentPath() / relPath;
		const bool linear = params[name + "_linear"].asBool(false);
		const bool mipMap = params[name + "_mip_map"].asBool(false);
		const auto wrapMode = params[name + "_wrap_mode"].asEnum<RetroarchWrapMode>(RetroarchWrapMode::ClampToEdge);

		textures[name] = loadTexture(video, texPath, linear, mipMap, wrapMode);
	}
}

std::unique_ptr<Texture> RetroarchFilterChain::loadTexture(VideoAPI& video, const Path& path, bool linear, bool mipMap, RetroarchWrapMode wrapMode)
{
	const auto bytes = Path::readFile(path);
	if (bytes.empty()) {
		return {};
	}

	auto image = std::make_unique<Image>(bytes.byte_span());
	if (image->getSize().x == 0) {
		return {};
	}
	const auto size = image->getSize();

	TextureDescriptor descriptor(size);
	descriptor.useFiltering = linear;
	descriptor.useMipMap = mipMap;
	descriptor.addressMode = getAddressMode(wrapMode);
	descriptor.pixelData = std::move(image);

	auto texture = video.createTexture(size);
	texture->startLoading();
	texture->load(std::move(descriptor));
	return texture;
}

Sprite RetroarchFilterChain::run(const Sprite& src, RenderContext& rc, Vector2i outputSize)
{
	if (stages.empty()) {
		return src;
	}

	Logger::logDev("Start frame");

	// Set stage sizes
	const Vector2i originalSize = src.getMaterial().getTexture(0)->getSize();
	{
		Vector2i sourceSize = originalSize;
		for (auto& stage: stages) {
			sourceSize = stage.updateSize(sourceSize, outputSize);
		}
	}

	FrameParams frameParams;
	frameParams.frameCount = frameNumber;
	frameParams.mvp = Matrix4f::makeIdentity(); // TODO
	frameParams.originalSize = texSizeToVec4(originalSize);
	frameParams.finalViewportSize = texSizeToVec4(outputSize);

	// Draw stages
	for (size_t i = 0; i < stages.size(); ++i) {
		auto& stage = stages[i];
		
		frameParams.sourceSize = texSizeToVec4(i == 0 ? originalSize : stages[i - 1].size);
		frameParams.outputSize = texSizeToVec4(stage.size);
		updateStageMaterial(stage, frameParams);

		rc.with(stage.renderSurface->getRenderTarget()).bind([&] (Painter& painter)
		{
			drawStage(stage, static_cast<int>(i), painter);
		});
	}

	++frameNumber;

	return stages.back().renderSurface->getSurfaceSprite(src.getMaterialPtr()->clone());
}


void RetroarchFilterChain::setupStageMaterial(Stage& stage)
{
	for (const auto& ub: stage.materialDefinition->getUniformBlocks()) {
		for (const auto& u: ub.uniforms) {
			updateUserParameters(stage, u.name, *stage.material);
		}
	}
}

void RetroarchFilterChain::updateStageMaterial(Stage& stage, const FrameParams& frameParams)
{
	for (const auto& ub: stage.materialDefinition->getUniformBlocks()) {
		for (const auto& u: ub.uniforms) {
			updateFrameParameters(u.name, *stage.material, frameParams);
		}
	}
	
	for (const auto& tex: stage.materialDefinition->getTextures()) {
		updateTexture(tex.name, *stage.material);
	}
}

void RetroarchFilterChain::updateUserParameters(Stage& stage, const String& name, Material& material)
{
	if (stage.params.hasKey(name)) {
		material.set(name, stage.params[name].asFloat());
		return;
	}

	if (params.hasKey(name)) {
		material.set(name, params[name].asFloat());
		return;
	}

	if (name == "MVP" || name == "SourceSize" || name == "OriginalSize" || name == "OutputSize" || name == "FinalViewportSize" || name == "FrameCount") {
		// Built in, updated later
		return;
	}

	Logger::logWarning("Missing parameter: " + name);
}

void RetroarchFilterChain::updateFrameParameters(const String& name, Material& material, const FrameParams& frameParams)
{
	if (name == "MVP") {
		material.set(name, frameParams.mvp);
	} else if (name == "SourceSize") {
		material.set(name, frameParams.sourceSize);
	} else if (name == "OriginalSize") {
		material.set(name, frameParams.originalSize);
	} else if (name == "OutputSize") {
		material.set(name, frameParams.outputSize);
	} else if (name == "FinalViewportSize") {
		material.set(name, frameParams.finalViewportSize);
	} else if (name == "FrameCount") {
		material.set(name, frameParams.frameCount);
	}
}

void RetroarchFilterChain::updateTexture(const String& name, Material& material)
{
	const auto texIter = textures.find(name);
	if (texIter != textures.end()) {
		material.set(name, texIter->second);
		return;
	}

	// TODO: reference other stages
	Logger::logWarning("Missing texture: " + name);
}

void RetroarchFilterChain::drawStage(const Stage& stage, int stageIdx, Painter& painter)
{
	// TODO
	painter.clear(Colour4f(1, 0, (stageIdx + 1) / 10.0f));
}

TextureAddressMode RetroarchFilterChain::getAddressMode(RetroarchWrapMode mode)
{
	switch (mode) {
	case RetroarchWrapMode::ClampToBorder:
		return TextureAddressMode::Border;
	case RetroarchWrapMode::ClampToEdge:
		return TextureAddressMode::Clamp;
	case RetroarchWrapMode::Repeat:
		return TextureAddressMode::Repeat;
	case RetroarchWrapMode::Mirror:
		return TextureAddressMode::Mirror;
	}
	return TextureAddressMode::Clamp;
}

Vector4f RetroarchFilterChain::texSizeToVec4(Vector2i texSize)
{
	const auto sz = Vector2f(texSize);
	return Vector4f(sz.x, sz.y, 1.0f / sz.x, 1.0f / sz.y);
}
