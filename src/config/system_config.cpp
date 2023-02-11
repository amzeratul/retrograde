#include "system_config.h"

SystemRegionConfig::SystemRegionConfig(const ConfigNode& node)
{
	name = node["name"].asString();
	regions = node["regions"].asVector<String>();
}

const String& SystemRegionConfig::getName() const
{
	return name;
}

const Vector<String>& SystemRegionConfig::getRegions() const
{
	return regions;
}


SystemConfig::SystemConfig(const ConfigNode& node)
{
	id = node["id"].asString();
	manufacturer = node["manufacturer"].asString();
	releaseDate = node["releaseDate"].asString();
	cores = node["cores"].asVector<String>();
	regions = node["regions"].asVector<SystemRegionConfig>();
}

const String& SystemConfig::getId() const
{
	return id;
}

const String& SystemConfig::getManufacturer() const
{
	return manufacturer;
}

const String& SystemConfig::getReleaseDate() const
{
	return releaseDate;
}

const SystemRegionConfig& SystemConfig::getRegion(const String& regionId) const
{
	const SystemRegionConfig* worldRegion = nullptr;
	for (const auto& region: regions) {
		if (std_ex::contains(region.getRegions(), regionId)) {
			return region;
		} else if (std_ex::contains(region.getRegions(), "world")) {
			worldRegion = &region;
		}
	}

	if (worldRegion) {
		return *worldRegion;
	} else {
		return regions.front();
	}
}

const Vector<SystemRegionConfig>& SystemConfig::getRegions() const
{
	return regions;
}

const Vector<String>& SystemConfig::getCores() const
{
	return cores;
}
