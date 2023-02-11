#include "config_database.h"

void ConfigDatabase::load(Resources& resources, const String& prefix)
{
	for (const auto& configName: resources.enumerate<ConfigFile>()) {
		if (configName.startsWith(prefix)) {
			loadFile(*resources.get<ConfigFile>(configName));
		}
	}
}

void ConfigDatabase::loadFile(const ConfigFile& configFile)
{
	observers.push_back(ConfigObserver(configFile));
	loadConfig(configFile.getRoot());
}

void ConfigDatabase::loadConfig(const ConfigNode& node)
{
	for (const auto& [k, v]: node.asMap()) {
		for (auto& db: dbs) {
			if (db.second->getKey() == k) {
				db.second->loadConfigs(v);
				break;
			}
		}
	}
}

void ConfigDatabase::update()
{
	for (auto& o: observers) {
		if (o.needsUpdate()) {
			o.update();
			loadConfig(o.getRoot());
		}
	}
}
