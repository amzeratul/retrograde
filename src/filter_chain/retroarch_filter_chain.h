#pragma once

#include <halley.hpp>
using namespace Halley;

#include "filter_chain.h"

class RetroarchFilterChain : public FilterChain {
public:
	struct Stage {
		Stage() = default;
		Stage(int idx, const ConfigNode& params, const Path& basePath);
	};

	RetroarchFilterChain() = default;
	RetroarchFilterChain(Path path);

	Sprite run(const Sprite& src, RenderContext& rc) override;

private:
	Path path;
	Vector<Stage> stages;

	static ConfigNode parsePreset(const Path& path);
	static void parsePresetLine(std::string_view str, ConfigNode::MapType& dst);

	void loadStages(const ConfigNode& params);
};
