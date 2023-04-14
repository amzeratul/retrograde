#pragma once

#include <halley.hpp>
using namespace Halley;

class Settings {
public:
	void load(const Path& path);
	void load(const ConfigNode& node);

	const Path& getRomsDir() const;

private:
	Path romsDir;
};
