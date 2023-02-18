#pragma once

#include <halley.hpp>
using namespace Halley;

#include "filter_chain.h"

class RetroarchFilterChain : public FilterChain {
public:
	RetroarchFilterChain() = default;
	RetroarchFilterChain(Path path);

	Sprite run(const Sprite& src, RenderContext& rc) override;
};
