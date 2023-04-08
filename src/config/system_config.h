#pragma once

#include <halley.hpp>
using namespace Halley;

enum class SystemCategory {
	Console,
    Handheld,
    Arcade,
    Computer
};

namespace Halley {
	
	template <>
	struct EnumNames<SystemCategory> {
		constexpr std::array<const char*, 4> operator()() const {
			return{{
				"console",
				"handheld",
				"arcade",
                "computer"
			}};
		}
	};
}

class Date {
public:
    Date() = default;
    Date(const ConfigNode& node);
    Date(int year, int month, int day);

    String toString() const;

    bool operator<(const Date& other) const;
    bool operator==(const Date& other) const;
    bool operator!=(const Date& other) const;

    int year;
    int month;
    int day;
};

class SystemRegionConfig {
public:
    SystemRegionConfig() = default;
    SystemRegionConfig(const ConfigNode& node);

    const String& getName() const;
    const String& getLogo() const;
    const Vector<String>& getRegions() const;
    String getLogoImage() const;

private:
    String name;
    String logo;
    Vector<String> regions;
};

class SystemConfig {
public:
    SystemConfig() = default;
    SystemConfig(const ConfigNode& node);

    const String& getId() const;
    const String& getManufacturer() const;
    const Date& getReleaseDate() const;
    const SystemRegionConfig& getRegion(const String& region) const;
	const Vector<SystemRegionConfig>& getRegions() const;
	const Vector<String>& getCores() const;
    const Vector<String>& getScreenFilters() const;
    const Vector<String>& getBezels() const;
    int getGeneration() const;
    int getUnitsSold() const;
    SystemCategory getCategory() const;

	String getDescriptionKey() const;
    String getInfoImage() const;

private:
    String id;
    String manufacturer;
    Date releaseDate;
    SystemCategory category;
    Vector<SystemRegionConfig> regions;
    Vector<String> cores;
    Vector<String> screenFilters;
    Vector<String> bezels;
    int generation;
    int unitsSold;
};
