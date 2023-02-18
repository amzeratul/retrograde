#include "retroarch_filter_chain.h"

#include "retroarch_shader_parser.h"
#include "shader_converter.h"


RetroarchFilterChain::Stage::Stage(int idx, const ConfigNode& params, const Path& basePath, VideoAPI& video)
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

	loadShader(video, params);
}


void RetroarchFilterChain::Stage::loadShader(VideoAPI& video, const ConfigNode& params)
{
	const auto parsed = RetroarchShaderParser::parse(shaderPath);

	const auto outputFormat = fromString<ShaderFormat>(video.getShaderLanguage());
	const auto vertexShader = ShaderConverter::convertShader(parsed.vertexShader, ShaderFormat::GLSL, outputFormat);
	const auto pixelShader = ShaderConverter::convertShader(parsed.pixelShader, ShaderFormat::GLSL, outputFormat);

	shader = ShaderConverter::loadShader(vertexShader, pixelShader, video);
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
		stages.push_back(Stage(i, params, path.parentPath(), video));
	}
}
