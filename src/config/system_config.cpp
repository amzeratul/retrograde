#include "system_config.h"

SystemConfig::SystemConfig(const ConfigNode& node)
{
	cores = node["cores"].asVector<String>({});
}

const Vector<String>& SystemConfig::getCores() const
{
	return cores;
}
