cmake_minimum_required (VERSION 3.10)

project (retrograde)

add_subdirectory(test_cores)

set(HALLEY_PROJECT_EMBED 1)
set(HALLEY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/halley)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${HALLEY_PATH}/cmake/")
include(HalleyProject)

if(NOT (DEFINED HALLEY_GAME_BIN_DIR))
	set(HALLEY_GAME_BIN_DIR "bin")
endif()

set (SOURCES
	"prec.cpp"
	
	"src/game/retrograde_game.cpp"
	"src/game/game_stage.cpp"

	"src/libretro/libretro_core.cpp"
	"src/libretro/libretro_environment.cpp"

	"src/util/dll.cpp"
	)

set (HEADERS
	"prec.h"
	
	"src/game/retrograde_game.h"
	"src/game/game_stage.h"

	"src/libretro/libretro_core.h"
	"src/libretro/libretro_environment.h"
	"src/libretro/libretro.h"

	"src/util/dll.h"
	)

set (GEN_DEFINITIONS
	)

halleyProject(retrograde "${SOURCES}" "${HEADERS}" "" "${GEN_DEFINITIONS}" ${CMAKE_CURRENT_SOURCE_DIR}/${HALLEY_GAME_BIN_DIR})
