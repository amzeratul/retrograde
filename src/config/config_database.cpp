#include "config_database.h"

void ConfigDatabase::load(Resources& resources, const String& prefix)
{
	for (const auto& configName: resources.enumerate<ConfigFile>()) {
		if (configName.startsWith(prefix)) {
			const auto& configFile = resources.get<ConfigFile>(configName);
			for (const auto& [k, v]: configFile->getRoot().asMap()) {
				for (auto& db: dbs) {
					if (db.second->getKey() == k) {
						db.second->loadConfigs(v);
						break;
					}
				}
			}
		}
	}
}
