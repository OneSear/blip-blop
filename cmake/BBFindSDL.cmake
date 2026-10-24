# Locates SDL2 and SDL2_mixer and exposes the imported targets
#   SDL2::SDL2  and  SDL2_mixer::SDL2_mixer
# CMake package configs are tried first (vcpkg, Homebrew, recent distros);
# pkg-config is the fallback for distributions that do not ship them.

find_package(SDL2 CONFIG QUIET)
if(NOT TARGET SDL2::SDL2)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(BB_PC_SDL2 REQUIRED IMPORTED_TARGET sdl2)
    add_library(SDL2::SDL2 INTERFACE IMPORTED)
    target_link_libraries(SDL2::SDL2 INTERFACE PkgConfig::BB_PC_SDL2)
    message(STATUS "SDL2 found through pkg-config: ${BB_PC_SDL2_VERSION}")
else()
    message(STATUS "SDL2 found through its CMake package: ${SDL2_VERSION}")
endif()

if(TARGET SDL2::SDL2main)
    set(BB_SDL2MAIN SDL2::SDL2main)
    # SDL2main calls SDL2, which its package does not declare: with it,
    # CMake puts SDL2 after SDL2main, as GNU ld (MinGW) needs.
    get_target_property(bb_sdl2main_alias SDL2::SDL2main ALIASED_TARGET)
    if(NOT bb_sdl2main_alias)
        set_property(TARGET SDL2::SDL2main APPEND PROPERTY INTERFACE_LINK_LIBRARIES SDL2::SDL2)
    endif()
else()
    set(BB_SDL2MAIN "")
endif()

find_package(SDL2_mixer CONFIG QUIET)
if(NOT TARGET SDL2_mixer::SDL2_mixer)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(BB_PC_SDL2_MIXER REQUIRED IMPORTED_TARGET SDL2_mixer)
    add_library(SDL2_mixer::SDL2_mixer INTERFACE IMPORTED)
    target_link_libraries(SDL2_mixer::SDL2_mixer INTERFACE PkgConfig::BB_PC_SDL2_MIXER)
    message(STATUS "SDL2_mixer found through pkg-config: ${BB_PC_SDL2_MIXER_VERSION}")
else()
    message(STATUS "SDL2_mixer found through its CMake package: ${SDL2_mixer_VERSION}")
endif()
