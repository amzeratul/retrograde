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
	
	"src/config/core_config.cpp"
	"src/config/system_config.cpp"

	"src/filter_chain/retroarch_filter_chain.cpp"
	"src/filter_chain/retroarch_shader_parser.cpp"
	"src/filter_chain/shader_converter.cpp"

	"src/game/game_stage.cpp"
	"src/game/retrograde_environment.cpp"
	"src/game/retrograde_game.cpp"

	"src/libretro/libretro_core.cpp"
	"src/libretro/libretro_vfs.cpp"

	"src/savestate/rewind_data.cpp"

	"src/ui/choose_game_window.cpp"
	"src/ui/choose_system_window.cpp"
	"src/ui/game_canvas.cpp"
	"src/ui/in_game_menu.cpp"

	"src/util/cpu_update_texture.cpp"
	"src/util/dll.cpp"

	"src/zip/zip_file.cpp"
	"src/zip/miniz.c"
	)

set (HEADERS
	"prec.h"
	"todo.txt"

	"src/contrib/glslang/include/glslang_c_interface.h"
	"src/contrib/glslang/include/glslang_c_shader_types.h"
	
	"src/config/core_config.h"
	"src/config/system_config.h"

	"src/filter_chain/filter_chain.h"
	"src/filter_chain/retroarch_filter_chain.h"
	"src/filter_chain/retroarch_shader_parser.h"
	"src/filter_chain/shader_converter.h"

	"src/game/game_stage.h"
	"src/game/retrograde_environment.h"
	"src/game/retrograde_game.h"

	"src/libretro/libretro_core.h"
	"src/libretro/libretro_vfs.h"
	"src/libretro/libretro.h"
	"src/libretro/libretro_d3d.h"
	"src/libretro/libretro_vulkan.h"

	"src/savestate/rewind_data.h"

	"src/ui/choose_game_window.h"
	"src/ui/choose_system_window.h"
	"src/ui/game_canvas.h"
	"src/ui/in_game_menu.h"

	"src/util/cpu_update_texture.h"
	"src/util/c_string_cache.h"
	"src/util/dll.h"

	"src/zip/zip_file.h"
	"src/zip/miniz.h"
	)

set (GEN_DEFINITIONS
	)

halleyProject(retrograde "${SOURCES}" "${HEADERS}" "" "${GEN_DEFINITIONS}" ${CMAKE_CURRENT_SOURCE_DIR}/${HALLEY_GAME_BIN_DIR})

target_link_libraries(retrograde-game
	optimized "${CMAKE_CURRENT_SOURCE_DIR}/lib/SPIRV.lib"
	optimized "${CMAKE_CURRENT_SOURCE_DIR}/lib/SPVRemapper.lib"
	optimized "${CMAKE_CURRENT_SOURCE_DIR}/lib/OSDependent.lib"
	optimized "${CMAKE_CURRENT_SOURCE_DIR}/lib/OGLCompiler.lib"
	optimized "${CMAKE_CURRENT_SOURCE_DIR}/lib/GenericCodeGen.lib"
	optimized "${CMAKE_CURRENT_SOURCE_DIR}/lib/MachineIndependent.lib"
	optimized "${CMAKE_CURRENT_SOURCE_DIR}/lib/glslang.lib"
	debug "${CMAKE_CURRENT_SOURCE_DIR}/lib/SPIRVd.lib"
	debug "${CMAKE_CURRENT_SOURCE_DIR}/lib/SPVRemapperd.lib"
	debug "${CMAKE_CURRENT_SOURCE_DIR}/lib/OSDependentd.lib"
	debug "${CMAKE_CURRENT_SOURCE_DIR}/lib/OGLCompilerd.lib"
	debug "${CMAKE_CURRENT_SOURCE_DIR}/lib/GenericCodeGend.lib"
	debug "${CMAKE_CURRENT_SOURCE_DIR}/lib/MachineIndependentd.lib"
	debug "${CMAKE_CURRENT_SOURCE_DIR}/lib/glslangd.lib"
)
