#include "retroarch_shader_parser.h"

RetroarchShaderParser::Result RetroarchShaderParser::parse(const Path& path)
{
	const auto lines = Path::readFileLines(path);
	if (lines.empty()) {
		Logger::logError("Shader not found: " + path.getString());
		return {};
	}

	// TODO
	Logger::logDev("Parsing Retroarch shader " + path.getString());
	return {};
}
