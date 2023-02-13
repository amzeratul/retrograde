cmake_minimum_required (VERSION 3.10)

project (retrograde)

add_subdirectory(test_cores)

set(REQUIRE_WINDOWS_10 1)
set(HALLEY_PROJECT_EMBED 1)
set(HALLEY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/halley)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${HALLEY_PATH}/cmake/")
include(HalleyProject)

if(NOT (DEFINED HALLEY_GAME_BIN_DIR))
	set(HALLEY_GAME_BIN_DIR "bin")
endif()

set (SOURCES
	"prec.cpp"
	
	"src/config/config_database.cpp"
	"src/config/core_config.cpp"
	"src/config/system_config.cpp"

	"src/game/game_stage.cpp"
	"src/game/retrograde_environment.cpp"
	"src/game/retrograde_game.cpp"

	"src/libretro/libretro_core.cpp"
	"src/libretro/libretro_vfs.cpp"

	"src/savestate/rewind_data.cpp"

	"src/util/cpu_update_texture.cpp"
	"src/util/dll.cpp"

	"src/zip/zip_file.cpp"
	"src/zip/miniz.c"
	)

set (HEADERS
	"prec.h"
	"todo.txt"
	
	"src/config/config_database.h"
	"src/config/core_config.h"
	"src/config/system_config.h"

	"src/game/game_stage.h"
	"src/game/retrograde_environment.h"
	"src/game/retrograde_game.h"

	"src/libretro/libretro_core.h"
	"src/libretro/libretro_vfs.h"
	"src/libretro/libretro.h"
	"src/libretro/libretro_d3d.h"
	"src/libretro/libretro_vulkan.h"

	"src/savestate/rewind_data.h"

	"src/util/cpu_update_texture.h"
	"src/util/c_string_cache.h"
	"src/util/dll.h"

	"src/zip/zip_file.h"
	"src/zip/miniz.h"
	)

set (GEN_DEFINITIONS
	)

halleyProject(retrograde "${SOURCES}" "${HEADERS}" "" "${GEN_DEFINITIONS}" ${CMAKE_CURRENT_SOURCE_DIR}/${HALLEY_GAME_BIN_DIR})
