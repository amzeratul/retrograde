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
	
	"src/config/bezel_config.cpp"
	"src/config/core_config.cpp"
	"src/config/screen_filter_config.cpp"
	"src/config/system_config.cpp"

	"src/filter_chain/material_generator.cpp"
	"src/filter_chain/retroarch_filter_chain.cpp"
	"src/filter_chain/retroarch_shader_parser.cpp"
	"src/filter_chain/shader_compiler.cpp"
	"src/filter_chain/shader_converter.cpp"

	"src/game/game_canvas.cpp"
	"src/game/system_bezel.cpp"

	"src/libretro/libretro_core.cpp"
	"src/libretro/libretro_vfs.cpp"

	"src/metadata/game_collection.cpp"

	"src/retrograde/game_stage.cpp"
	"src/retrograde/retrograde_environment.cpp"
	"src/retrograde/retrograde_game.cpp"

	"src/savestate/rewind_data.cpp"

	"src/ui/choose_game_window.cpp"
	"src/ui/choose_system_window.cpp"
	"src/ui/in_game_menu.cpp"

	"src/util/cpu_update_texture.cpp"
	"src/util/dll.cpp"
	"src/util/dx11_state.cpp"
	"src/util/image_cache.cpp"
	"src/util/opengl_interop.cpp"
	)

set (HEADERS
	"prec.h"
	"todo.txt"

	"src/contrib/glslang/include/glslang_c_interface.h"
	"src/contrib/glslang/include/glslang_c_shader_types.h"

	"src/contrib/spirv_cross/GLSL.std.450.h"
	"src/contrib/spirv_cross/spirv_cfg.hpp"
	"src/contrib/spirv_cross/spirv_common.hpp"
	"src/contrib/spirv_cross/spirv_cpp.hpp"
	"src/contrib/spirv_cross/spirv_cross_c.h"
	"src/contrib/spirv_cross/spirv_cross_containers.hpp"
	"src/contrib/spirv_cross/spirv_cross_error_handling.hpp"
	"src/contrib/spirv_cross/spirv_cross_parsed_ir.hpp"
	"src/contrib/spirv_cross/spirv_cross_util.hpp"
	"src/contrib/spirv_cross/spirv_cross.hpp"
	"src/contrib/spirv_cross/spirv_glsl.hpp"
	"src/contrib/spirv_cross/spirv_hlsl.hpp"
	"src/contrib/spirv_cross/spirv_msl.hpp"
	"src/contrib/spirv_cross/spirv_parser.hpp"
	"src/contrib/spirv_cross/spirv_reflect.hpp"
	"src/contrib/spirv_cross/spirv.h"
	"src/contrib/spirv_cross/spirv.hpp"
	
	"src/config/bezel_config.h"
	"src/config/core_config.h"
	"src/config/screen_filter_config.h"
	"src/config/system_config.h"

	"src/filter_chain/filter_chain.h"
	"src/filter_chain/material_generator.h"
	"src/filter_chain/retroarch_filter_chain.h"
	"src/filter_chain/retroarch_shader_parser.h"
	"src/filter_chain/shader_compiler.h"
	"src/filter_chain/shader_converter.h"

	"src/game/game_canvas.h"
	"src/game/system_bezel.h"

	"src/libretro/libretro_core.h"
	"src/libretro/libretro_vfs.h"
	"src/libretro/libretro.h"
	"src/libretro/libretro_d3d.h"
	"src/libretro/libretro_vulkan.h"

	"src/metadata/game_collection.h"

	"src/retrograde/game_stage.h"
	"src/retrograde/retrograde_environment.h"
	"src/retrograde/retrograde_game.h"

	"src/savestate/rewind_data.h"

	"src/ui/choose_game_window.h"
	"src/ui/choose_system_window.h"
	"src/ui/in_game_menu.h"

	"src/util/cpu_update_texture.h"
	"src/util/c_string_cache.h"
	"src/util/dll.h"
	"src/util/dx11_state.h"
	"src/util/image_cache.h"
	"src/util/opengl_interop.h"
	)

set (GEN_DEFINITIONS
	)

if (MSVC)
	set (RESOURCES
		"res/retrograde.rc"
		"res/resource.h"
		"res/retrograde.ico"
	)
endif ()

halleyProject(retrograde "${SOURCES}" "${HEADERS}" "${RESOURCES}" "${GEN_DEFINITIONS}" ${CMAKE_CURRENT_SOURCE_DIR}/${HALLEY_GAME_BIN_DIR})

target_link_libraries(retrograde-game
	optimized "${CMAKE_CURRENT_SOURCE_DIR}/lib/SPIRV-Tools-opt.lib"
	optimized "${CMAKE_CURRENT_SOURCE_DIR}/lib/SPIRV-Tools.lib"
	optimized "${CMAKE_CURRENT_SOURCE_DIR}/lib/SPIRV.lib"
	optimized "${CMAKE_CURRENT_SOURCE_DIR}/lib/SPVRemapper.lib"
	optimized "${CMAKE_CURRENT_SOURCE_DIR}/lib/OSDependent.lib"
	optimized "${CMAKE_CURRENT_SOURCE_DIR}/lib/OGLCompiler.lib"
	optimized "${CMAKE_CURRENT_SOURCE_DIR}/lib/GenericCodeGen.lib"
	optimized "${CMAKE_CURRENT_SOURCE_DIR}/lib/MachineIndependent.lib"
	optimized "${CMAKE_CURRENT_SOURCE_DIR}/lib/glslang.lib"
	optimized "${CMAKE_CURRENT_SOURCE_DIR}/lib/spirv-cross-cpp.lib"
	optimized "${CMAKE_CURRENT_SOURCE_DIR}/lib/spirv-cross-core.lib"
	optimized "${CMAKE_CURRENT_SOURCE_DIR}/lib/spirv-cross-glsl.lib"
	optimized "${CMAKE_CURRENT_SOURCE_DIR}/lib/spirv-cross-hlsl.lib"
	optimized "${CMAKE_CURRENT_SOURCE_DIR}/lib/spirv-cross-reflect.lib"
	optimized "${CMAKE_CURRENT_SOURCE_DIR}/lib/spirv-cross-util.lib"
	debug "${CMAKE_CURRENT_SOURCE_DIR}/lib/SPIRV-Tools-optd.lib"
	debug "${CMAKE_CURRENT_SOURCE_DIR}/lib/SPIRV-Toolsd.lib"
	debug "${CMAKE_CURRENT_SOURCE_DIR}/lib/SPIRVd.lib"
	debug "${CMAKE_CURRENT_SOURCE_DIR}/lib/SPVRemapperd.lib"
	debug "${CMAKE_CURRENT_SOURCE_DIR}/lib/OSDependentd.lib"
	debug "${CMAKE_CURRENT_SOURCE_DIR}/lib/OGLCompilerd.lib"
	debug "${CMAKE_CURRENT_SOURCE_DIR}/lib/GenericCodeGend.lib"
	debug "${CMAKE_CURRENT_SOURCE_DIR}/lib/MachineIndependentd.lib"
	debug "${CMAKE_CURRENT_SOURCE_DIR}/lib/glslangd.lib"
	debug "${CMAKE_CURRENT_SOURCE_DIR}/lib/spirv-cross-cppd.lib"
	debug "${CMAKE_CURRENT_SOURCE_DIR}/lib/spirv-cross-cored.lib"
	debug "${CMAKE_CURRENT_SOURCE_DIR}/lib/spirv-cross-glsld.lib"
	debug "${CMAKE_CURRENT_SOURCE_DIR}/lib/spirv-cross-hlsld.lib"
	debug "${CMAKE_CURRENT_SOURCE_DIR}/lib/spirv-cross-reflectd.lib"
	debug "${CMAKE_CURRENT_SOURCE_DIR}/lib/spirv-cross-utild.lib"
)
