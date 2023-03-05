#pragma once

#include <halley.hpp>
using namespace Halley;

class SystemRegionConfig {
public:
    SystemRegionConfig() = default;
    SystemRegionConfig(const ConfigNode& node);

    const String& getName() const;
    const Vector<String>& getRegions() const;

private:
    String name;
    Vector<String> regions;
};

class SystemConfig {
public:
    SystemConfig() = default;
    SystemConfig(const ConfigNode& node);

    const String& getId() const;
    const String& getManufacturer() const;
    const String& getReleaseDate() const;
    const SystemRegionConfig& getRegion(const String& region) const;
	const Vector<SystemRegionConfig>& getRegions() const;
	const Vector<String>& getCores() const;
    const Vector<String>& getScreenFilters() const;
    const Vector<String>& getBezels() const;
    int getGeneration() const;

private:
    String id;
    String manufacturer;
    String releaseDate;
    Vector<SystemRegionConfig> regions;
    Vector<String> cores;
    Vector<String> screenFilters;
    Vector<String> bezels;
    int generation;
};
