#pragma once

#include <halley.hpp>

#include "src/ui/game_canvas.h"
using namespace Halley;

class FilterChain {
public:
	virtual ~FilterChain() = default;
	virtual Sprite run(const Sprite& src, RenderContext& rc) = 0;
};
