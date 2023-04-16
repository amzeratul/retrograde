#include "controller_config.h"

ControllerConfig::ControllerConfig(const ConfigNode& node)
{
	id = node["id"].asString();
	def = node["default"].asBool(false);
	match = node["match"].asVector<String>({});
}

const String& ControllerConfig::getId() const
{
	return id;
}

bool ControllerConfig::matches(const String& id) const
{
	for (auto& m: match) {
		if (id.contains(m)) {
			return true;
		}
	}
	return false;
}

bool ControllerConfig::isDefault() const
{
	return def;
}
