#include "retroarch_shader_parser.h"

// Details: https://github.com/libretro/slang-shaders

RetroarchShaderParser::Parameter::Parameter(const String& str)
{
	std::string_view view = str;

	auto marchToNonSpace = [&]()
	{
		while (!view.empty() && view.front() == ' ') {
			view = view.substr(1);
		}
	};

	// Parse name
	{
		marchToNonSpace();
		const auto end = view.find(' ');
		name = view.substr(0, end);
		view = view.substr(end + 1);
		marchToNonSpace();
	}

	// Parse double quoted description
	{
		const auto start = view.find('"');
		const auto end = view.find('"', start + 1);
		description = view.substr(start + 1, end - start - 1);
		view = view.substr(end + 2);
		marchToNonSpace();
	}

	// Split remaining parameters
	auto split = String(view).split(' ');
	std_ex::erase(split, "");
	initial = split[0].toFloat();
	minimum = split[1].toFloat();
	maximum = split[2].toFloat();
	if (split.size() >= 4) {
		step = split[3].toFloat();
	}
}

RetroarchShaderParser::Result RetroarchShaderParser::parse(const Path& path)
{
	auto lines = readFileWithIncludes(path);
	if (lines.empty()) {
		Logger::logError("Shader not found: " + path.getString());
		return {};
	}
	
	Result result;
	String stage = "";
	for (auto& line: lines) {
		if (line.startsWith("#pragma ")) {
			if (line.startsWith("#pragma stage ")) {
				stage = line.mid(14);
				stage.trimBoth();
			} else if (line.startsWith("#pragma name ")) {
				result.name = line.mid(13);
				result.name.trimBoth();
			} else if (line.startsWith("#pragma format ")) {
				result.format = line.mid(15);
				result.format.trimBoth();
			} else if (line.startsWith("#pragma parameter ")) {
				result.parameters.push_back(Parameter(line.mid(18)));
			}
		} else {
			if (stage == "" || stage == "vertex") {
				result.vertexShader += line + "\n";
			}
			if (stage == "" || stage == "fragment") {
				result.pixelShader += line + "\n";
			}
		}
	}

	return result;
}

Vector<String> RetroarchShaderParser::readFileWithIncludes(const Path& path)
{
	return processIncludes(Path::readFileLines(path), path.parentPath());
}

Vector<String> RetroarchShaderParser::processIncludes(Vector<String> orig, const Path& origDir)
{
	Vector<String> result;
	result.reserve(orig.size());

	for (auto& line: orig) {
		if (line.startsWith("#include \"")) {
			const auto includePath = line.mid(10, line.size() - 11);
			auto toIncludeLines = readFileWithIncludes(origDir / includePath);
			for (auto& incLine: toIncludeLines) {
				result.push_back(std::move(incLine));
			}
		} else {
			result.push_back(std::move(line));
		}
	}

	return result;
}
