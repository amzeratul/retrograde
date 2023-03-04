#pragma once

#include <halley.hpp>

#include "src/game/game_canvas.h"
using namespace Halley;

class FilterChain {
public:
	virtual ~FilterChain() = default;
	virtual Sprite run(const Sprite& src, RenderContext& rc, Vector2i viewPortSize) = 0;
	virtual const String& getId() const = 0;
};
