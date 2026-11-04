cmake_minimum_required(VERSION 3.16)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(ZOS_TOOLCHAIN sdcc)
include($ENV{ZOS_PATH}/cmake/zos_init.cmake)

project(enigma C ASM)

add_executable(enigma
    src/main.c
    src/assets.c
    src/bullet.c
    src/enemies.c
    src/hud.c
    src/objects.c
    src/player.c
    src/game_world.c
)

zos_use(zvb)
find_package(ZVB_SDK REQUIRED)
zos_use(zgdk)
find_package(ZGDK REQUIRED)
zos_use(zar)
find_package(ZAR REQUIRED)
if(NOT TARGET core)
    zos_use(coreutils)
    find_package(COREUTILS REQUIRED)
endif()
target_link_libraries(enigma PRIVATE zgdk zvb_gfx zvb_sound core zar)

target_compile_options(enigma PRIVATE $<$<COMPILE_LANGUAGE:C>:--nostdlib>)

foreach(flag BREAK EMULATOR FRAMELOCK DEBUG)
    option(${flag} "Enable ${flag}" OFF)
    if(${flag})
        target_compile_definitions(enigma PRIVATE ${flag}=1)
    endif()
endforeach()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(asset_dir "${CMAKE_CURRENT_BINARY_DIR}/assets")
set(asset_outputs)
set(asset_targets)
foreach(asset enemies gates letters numbers player tileset)
    gif2zeal(enigma FILES "assets/${asset}.gif" BIT 4 COLORS 128)
    list(APPEND asset_outputs "${asset_dir}/${asset}.zts" "${asset_dir}/${asset}.ztp")
    list(APPEND asset_targets enigma_${asset}_asset)
endforeach()

# The 8x8 world exports 64 separate rooms, not a single tilemap file.
# Keep rooms first in the sorted archive: TILEMAP_OFFSET is zero.
set(room_outputs)
foreach(room RANGE 0 63)
    set(room_index "000${room}")
    string(LENGTH "${room_index}" index_length)
    math(EXPR index_start "${index_length} - 4")
    string(SUBSTRING "${room_index}" ${index_start} 4 room_index)
    list(APPEND room_outputs "${asset_dir}/__room${room_index}.map")
endforeach()

set(tiled_converter "$ENV{ZVB_SDK_PATH}/tools/tiled2zeal/tiled2zeal.py")
add_custom_command(
    OUTPUT ${room_outputs}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${asset_dir}"
    COMMAND ${Python3_EXECUTABLE} "${tiled_converter}"
        -i "${CMAKE_CURRENT_SOURCE_DIR}/assets/enigma.tmx"
        -o "${asset_dir}/__room.map" --size 40x21 --layer 3 -z
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/enigma.tmx" "${tiled_converter}"
    COMMENT "Exporting Enigma rooms"
    VERBATIM
)
list(APPEND asset_outputs ${room_outputs})

zar_create(enigma
    INPUT "${asset_dir}"
    OUTPUT enigma.zar
    HEADER "${CMAKE_CURRENT_BINARY_DIR}/include/enigma_assets.h"
)
# Finish shared graphics rules before the archive consumes their outputs.
add_dependencies(enigma_enigma_zar ${asset_targets})
# zar_create discovers existing files at configure time. Declare generated
# inputs explicitly so clean builds and later asset edits rebuild the archive.
add_custom_command(
    OUTPUT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/enigma.zar"
    APPEND
    DEPENDS ${asset_outputs}
)

zgdk_add_outputs(enigma)
