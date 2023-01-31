#include "libretro_core.h"

#define RETRO_IMPORT_SYMBOLS
#include "libretro.h"

bool LibretroCore::loadCore(std::string_view filename)
{
	bool loaded = dll.load(filename);
	if (loaded) {
		const auto retroAPIVersion = static_cast<decltype(&retro_api_version)>(dll.getFunction("retro_api_version"));
		const auto dllVersion = retroAPIVersion();
		if (dllVersion != RETRO_API_VERSION) {
			dll.unload();
			return false;
		}
	}
	return true;
}
