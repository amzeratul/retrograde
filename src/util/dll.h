#pragma once

#include <halley.hpp>
using namespace Halley;

class DLL {
public:
    ~DLL();

    bool load(std::string_view filename);
    void unload();
    bool isLoaded() const;

    void* getFunction(std::string_view name) const;

private:
    void* handle = nullptr;;
};
