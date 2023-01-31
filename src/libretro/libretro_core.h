#pragma once

#include <halley.hpp>

#include "src/util/dll.h"
using namespace Halley;

class LibretroCore {
public:
	bool loadCore(std::string_view filename);

private:
	DLL dll;
};
