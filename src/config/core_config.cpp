#include "core_config.h"

CoreConfig::CoreConfig(const ConfigNode& node)
{
	id = node["id"].asString();
	if (node.hasKey("options")) {
		options = node["options"].asHashMap<String, String>();
	}
}

const String& CoreConfig::getId() const
{
	return id;
}

const HashMap<String, String>& CoreConfig::getOptions() const
{
	return options;
}
