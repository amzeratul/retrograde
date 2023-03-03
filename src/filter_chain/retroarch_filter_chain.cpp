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
	filterLinearOutput = params["filter_linear" + toString(idx + 1)].asBool(false);
	mipMapInput = params["mipmap_input" + idxStr].asBool(false);
	mipMapOutput = params["mipmap_input" + toString(idx + 1)].asBool(false);
	wrapMode = params["wrap_mode" + idxStr].asEnum(RetroarchWrapMode::ClampToEdge);
	wrapModeOutput = params["wrap_mode" + toString(idx + 1)].asEnum(RetroarchWrapMode::ClampToEdge);
	floatFramebuffer = params["float_framebuffer" + idxStr].asBool(false);
	srgbFramebuffer = params["srgb_framebuffer" + idxStr].asBool(false);
	scaleTypeX = params["scale_type_x" + idxStr].asEnum(RetroarchScaleType::Source);
	scaleTypeY = params["scale_type_y" + idxStr].asEnum(RetroarchScaleType::Source);
	scale.x = params["scale_x" + idxStr].asFloat(1.0f);
	scale.y = params["scale_y" + idxStr].asFloat(1.0f);

	index = idx;
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

	RenderSurfaceOptions options;
	options.name = name;
	options.createDepthStencil = false;
	options.useFiltering = filterLinearOutput;
	options.powerOfTwo = false;
	options.mipMap = mipMapOutput;
	options.addressMode = getAddressMode(wrapModeOutput);
	options.colourBufferFormat = floatFramebuffer ? TextureFormat::RGBAFloat16 : (srgbFramebuffer ? TextureFormat::SRGBA : TextureFormat::RGBA);
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

std::shared_ptr<Texture> RetroarchFilterChain::Stage::getTexture(int framesBack)
{
	if (framesBack == 0) {
		return renderSurface->getRenderTarget().getTexture(0);
	} else if (framesBack == 1) {
		assert(needsHistory);
		return prevTexture;
	} else {
		throw Exception("Further texture history not available", 0);
	}
}

void RetroarchFilterChain::Stage::swapTextures()
{
	if (needsHistory) {
		if (!renderSurface->isReady()) {
			renderSurface->createNewColourTarget();
		}
		auto cur = renderSurface->getRenderTarget().getTexture(0);
		std::swap(cur, prevTexture);
		if (cur) {
			renderSurface->setColourTarget(cur);
		} else {
			renderSurface->createNewColourTarget();
		}
	}
}


RetroarchFilterChain::RetroarchFilterChain(String id, Path _path, VideoAPI& video)
	: id(std::move(id))
	, path(std::move(_path))
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

Sprite RetroarchFilterChain::run(const Sprite& src, RenderContext& rc, Vector2i viewportSize)
{
	if (stages.empty()) {
		return src;
	}

	// Frame data
	originalTexture = src.getMaterial().getTexture(0);
	const Vector2i originalSize = originalTexture->getSize();
	FrameParams frameParams;
	frameParams.frameCount = frameNumber++;
	frameParams.mvp = Matrix4f::makeIdentity(); // TODO
	frameParams.finalViewportSize = texSizeToVec4(viewportSize);

	// Set stage sizes
	{
		Vector2i sourceSize = originalSize;
		for (auto& stage: stages) {
			sourceSize = stage.updateSize(sourceSize, viewportSize);
			stage.swapTextures();
		}
	}

	// Draw stages
	for (auto& stage: stages) {
		updateStageMaterial(stage, frameParams);

		rc.with(stage.renderSurface->getRenderTarget()).bind([&] (Painter& painter)
		{
			drawStage(stage, painter);
		});
	}

	return stages.back().renderSurface->getSurfaceSprite(src.getMaterialPtr()->clone());
}

const String& RetroarchFilterChain::getId() const
{
	return id;
}


void RetroarchFilterChain::updateStageMaterial(Stage& stage, const FrameParams& frameParams)
{
	if (stage.mipMapInput && stage.index > 0) {
		stages[stage.index - 1].getTexture(0)->generateMipMaps();
	}

	for (const auto& ub: stage.materialDefinition->getUniformBlocks()) {
		for (const auto& u: ub.uniforms) {
			updateParameter(stage, u.name, *stage.material, frameParams);
		}
	}
	
	for (const auto& tex: stage.materialDefinition->getTextures()) {
		updateTexture(stage, tex.name, *stage.material);
	}
}

void RetroarchFilterChain::updateParameter(Stage& stage, const String& name, Material& material, const FrameParams& frameParams)
{
	if (params.hasKey(name)) {
		material.set(name, params[name].asFloat());
		return;
	}

	if (stage.params.hasKey(name)) {
		material.set(name, stage.params[name].asFloat());
		return;
	}

	if (name == "MVP") {
		material.set(name, frameParams.mvp);
		return;
	}
	if (name == "OutputSize") {
		material.set(name, texSizeToVec4(stage.size));
		return;
	}
	if (name == "FinalViewportSize") {
		material.set(name, frameParams.finalViewportSize);
		return;
	}
	if (name == "FrameCount") {
		material.set(name, frameParams.frameCount);
		return;
	}
	
	if (name.contains("Size")) {
		// Assume it's referencing a texture, remove "Size" from variable name to get its name
		const auto texName = name.replaceOne("Size", "");
		const auto tex = lookupTexture(stage, texName);
		if (tex) {
			material.set(name, texSizeToVec4(tex->getSize()));
			return;
		}
	}

	material.set(name, 0.0f);
	//Logger::logWarning("Missing parameter: " + name);
}

void RetroarchFilterChain::updateTexture(Stage& stage, const String& name, Material& material)
{
	auto tex = lookupTexture(stage, name);
	if (tex) {
		material.set(name, tex);
		return;
	}

	Logger::logWarning("Missing texture: " + name);
}

std::shared_ptr<const Texture> RetroarchFilterChain::lookupTexture(Stage& stage, const String& name)
{
	const auto texIter = textures.find(name);
	if (texIter != textures.end()) {
		return texIter->second;
	}

	if (name == "Original") {
		return originalTexture;
	}
	if (name.startsWith("OriginalHistory")) {
		const int n = name.substr(15).toInteger();
		if (n == 0) {
			return originalTexture;
		} else {
			// TODO
			throw Exception("Unimplemented texture parameter: OriginalHistory1+", 0);
		}
	}
	if (name == "Source") {
		if (stage.index == 0) {
			return originalTexture;
		} else {
			return stages[stage.index - 1].getTexture(0);
		}
	}
	if (name.startsWith("PassOutput")) {
		const int n = name.substr(10).toInteger();
		return stages[n].getTexture(0);
	}
	if (name.startsWith("PassFeedback")) {
		const int n = name.substr(12).toInteger();
		return stages[n].getTexture(1);
	}
	if (name.startsWith("User")) {
		// TODO
		throw Exception("Unimplemented texture: User", 0);
	}

	const bool isFeedback = name.endsWith("Feedback");
	const auto& aliasName = isFeedback ? name.substr(0, name.size() - 8) : name;
	for (auto& otherStage: stages) {
		if (otherStage.alias == aliasName || otherStage.shaderName == aliasName) {
			return otherStage.getTexture(isFeedback ? 1 : 0);
		}
	}

	return {};
}

void RetroarchFilterChain::drawStage(const Stage& stage, Painter& painter)
{
	const auto stride = stage.materialDefinition->getVertexStride();
	Vector<char> vertexData;
	vertexData.resize(stride * 4);

	struct Vertex {
		Vector4f pos;
		Vector2f tex;
	};

	const auto verts = std::array<Vector2f, 4>{ Vector2f(-1, -1), Vector2f(1, -1), Vector2f(1, 1), Vector2f(-1, 1) };
	const auto tex = std::array<Vector2f, 4>{ Vector2f(0, 1), Vector2f(1, 1), Vector2f(1, 0), Vector2f(0, 0) };

	char* dst = vertexData.data();
	for (int i = 0; i < 4; ++i) {
		Vertex v;
		v.pos = Vector4f(verts[i], 0, 1);
		v.tex = tex[i];

		memcpy(dst, &v, sizeof(v));
		dst += stage.materialDefinition->getVertexStride();
	}

	painter.drawQuads(stage.material, 4, vertexData.data());
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
