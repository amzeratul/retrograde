#include "core_config.h"

CoreConfig::CoreConfig(const ConfigNode& node)
{
	if (node.hasKey("options")) {
		options = node["options"].asHashMap<String, String>();
	}
}

const HashMap<String, String>& CoreConfig::getOptions() const
{
	return options;
}
