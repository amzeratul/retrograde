#pragma once

#include <halley.hpp>

using namespace Halley;

class Settings {
public:
	Settings(Path path);
	void load();
	void save() const;

	const Path& getRomsDir() const;

	void setWindowData(const String& windowId, ConfigNode data);
	ConfigNode& getWindowData(const String& windowId);

private:
	const Path path;

	Path romsDir;
	HashMap<String, ConfigNode> windowData;

	void load(const ConfigNode& node);
	ConfigNode toConfigNode() const;
};
