#pragma once

#include <halley.hpp>
using namespace Halley;

class ControllerConfig {
public:
    ControllerConfig() = default;
    ControllerConfig(const ConfigNode& node);

    const String& getId() const;
    bool matches(const String& id) const;
    bool isDefault() const;

private:
    String id;
    Vector<String> match;
    bool def = false;
};
