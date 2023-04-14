#include "system_config.h"

Date::Date(const ConfigNode& node)
{
	year = 0;
	month = 0;
	day = 0;

	const auto split = node.asString("").split('-');
	if (split.size() >= 1) {
		year = split[0].toInteger();
	}
	if (split.size() >= 2) {
		month = split[1].toInteger();
	}
	if (split.size() >= 3) {
		day = split[2].toInteger();
	}
}

Date::Date(int year, int month, int day)
	: year(year)
	, month(month)
	, day(day)
{
}

String Date::toString() const
{
	if (year) {
		if (month) {
			if (day) {
				return Halley::toString(year) + "-" + Halley::toString(month) + "-" + Halley::toString(day);
			}
			return Halley::toString(year) + "-" + Halley::toString(month);
		}
		return Halley::toString(year);
	} else {
		return "?";
	}
}

bool Date::operator<(const Date& other) const
{
	return std::tuple(year, month, day) < std::tuple(other.year, other.month, other.day);
}

bool Date::operator==(const Date& other) const
{
	return std::tuple(year, month, day) == std::tuple(other.year, other.month, other.day);
}

bool Date::operator!=(const Date& other) const
{
	return std::tuple(year, month, day) != std::tuple(other.year, other.month, other.day);
}


SystemRegionConfig::SystemRegionConfig(const ConfigNode& node)
{
	name = node["name"].asString();
	logo = node["logo"].asString("");
	machine = node["machine"].asString("");
	regions = node["regions"].asVector<String>();
}

const String& SystemRegionConfig::getName() const
{
	return name;
}

const String& SystemRegionConfig::getLogo() const
{
	return logo;
}

const String& SystemRegionConfig::getMachine() const
{
	return machine;
}

const Vector<String>& SystemRegionConfig::getRegions() const
{
	return regions;
}

String SystemRegionConfig::getLogoImage() const
{
	if (logo.isEmpty()) {
		return "";
	} else {
		return "systems/logos/" + logo;
	}
}

String SystemRegionConfig::getMachineImage() const
{
	if (machine.isEmpty()) {
		return "";
	} else {
		return "systems/machines/" + machine;
	}
}


SystemConfig::SystemConfig(const ConfigNode& node)
{
	id = node["id"].asString();
	manufacturer = node["manufacturer"].asString();
	releaseDate = Date(node["releaseDate"]);
	generation = node["generation"].asInt(0);
	unitsSold = node["unitsSold"].asInt(0);
	category = node["category"].asEnum<SystemCategory>();
	cores = node["cores"].asVector<String>();
	regions = node["regions"].asVector<SystemRegionConfig>();
	screenFilters = node["screenFilters"].asVector<String>({});
	bezels = node["bezels"].asVector<String>({});
	capabilities = node["capabilities"].asHashMap<SystemCapability, bool>();
}

const String& SystemConfig::getId() const
{
	return id;
}

const String& SystemConfig::getManufacturer() const
{
	return manufacturer;
}

const Date& SystemConfig::getReleaseDate() const
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

const Vector<String>& SystemConfig::getScreenFilters() const
{
	return screenFilters;
}

const Vector<String>& SystemConfig::getBezels() const
{
	return bezels;
}

int SystemConfig::getGeneration() const
{
	return generation;
}

int SystemConfig::getUnitsSold() const
{
	return unitsSold;
}

SystemCategory SystemConfig::getCategory() const
{
	return category;
}

String SystemConfig::getDescriptionKey() const
{
	return "system_description_" + id;
}

bool SystemConfig::hasCapability(SystemCapability capability) const
{
	const auto iter = capabilities.find(capability);
	if (iter != capabilities.end()) {
		return iter->second;
	}
	return true;
}
