#pragma once

#include <halley.hpp>
using namespace Halley;

class DLL {
public:
    DLL() = default;
    DLL(const DLL& other) = delete;
    DLL(DLL&& other) noexcept;
    ~DLL();

	DLL& operator=(const DLL& other) = delete;
    DLL& operator=(DLL&& other) noexcept;

    bool load(std::string_view filename);
    void unload();
    bool isLoaded() const;

    void* getFunction(std::string_view name) const;

private:
    void* handle = nullptr;
    String filename;
};

#define DLL_FUNC(dll, FUNC_NAME) static_cast<decltype(&(FUNC_NAME))>((dll).getFunction(#FUNC_NAME))
