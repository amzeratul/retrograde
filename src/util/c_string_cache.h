#pragma once

#include <halley.hpp>

using namespace Halley;

class CStringCache {
public:
    const char* operator()(String str)
    {
        buffer.push_back(std::move(str));
        return buffer.back().c_str();
    }

    void clear()
    {
        buffer.clear();
    }

private:
    Vector<String> buffer;
};
