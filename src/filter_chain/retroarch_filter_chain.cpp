#include "retroarch_filter_chain.h"


RetroarchFilterChain::Stage::Stage(int idx, const ConfigNode& params, const Path& basePath)
{
	const auto shaderPath = basePath / params["shader" + toString(idx)].asString();
	Logger::logDev("Stage " + toString(idx) + ": " + shaderPath.getNativeString());
}


RetroarchFilterChain::RetroarchFilterChain(Path _path)
	: path(std::move(_path))
{
	const auto params = parsePreset(path);
	loadStages(params);
}

Sprite RetroarchFilterChain::run(const Sprite& src, RenderContext& rc)
{
	// TODO
	return src;
}

ConfigNode RetroarchFilterChain::parsePreset(const Path& path)
{
	ConfigNode::MapType result;
	const auto bytes = Path::readFile(path);

	std::string_view remaining(reinterpret_cast<const char*>(bytes.data()), bytes.size());
	while (!remaining.empty()) {
		auto end = remaining.find('\n');
		std::string_view current = remaining.substr(0, end);
		remaining = remaining.substr(end + 1);

		parsePresetLine(current, result);
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

void RetroarchFilterChain::loadStages(const ConfigNode& params)
{
	const int nShaders = params["shaders"].asInt(0);
	for (int i = 0; i < nShaders; ++i) {
		stages.push_back(Stage(i, params, path.parentPath()));
	}
}
