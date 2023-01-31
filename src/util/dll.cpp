#include "dll.h"


#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>

	#ifdef min
		#undef min
		#undef max
	#endif
#else
	#include <dlfcn.h>
#endif


DLL::DLL(DLL&& other) noexcept
	: handle(other.handle)
{
	other.handle = nullptr;
}

DLL::~DLL()
{
	unload();
}

DLL& DLL::operator=(DLL&& other) noexcept
{
	unload();
	handle = other.handle;
	other.handle = nullptr;
	return *this;
}

bool DLL::load(std::string_view filename)
{
#ifdef _WIN32
	handle = LoadLibraryW(String(filename).getUTF16().c_str());
#else
	handle = dlopen(filename.data(), RTLD_LAZY);
#endif
	return !!handle;
}

void DLL::unload()
{
	if (handle) {
#ifdef _WIN32
		FreeLibrary(static_cast<HMODULE>(handle));
#else
		dlclose(handle);
#endif
		handle = nullptr;
	}
}

bool DLL::isLoaded() const
{
	return handle != nullptr;
}

void* DLL::getFunction(std::string_view name) const
{
	if (!handle) {
		return nullptr;
	}

#ifdef _WIN32
	return GetProcAddress(static_cast<HMODULE>(handle), name.data());
#else
	return dlsym(handle, name.data());
#endif
}
