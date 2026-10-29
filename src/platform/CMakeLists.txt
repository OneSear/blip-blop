# bb_platform: the platform interfaces (headers only). They depend on
# bb::core for the shared value types (Pixmap, input codes).
add_library(bb_platform INTERFACE)
add_library(bb::platform ALIAS bb_platform)
target_include_directories(bb_platform INTERFACE include)
target_link_libraries(bb_platform INTERFACE bb::core)

# bb_platform_sdl2: the SDL 2 backend (Windows, macOS, Linux). The only
# code that includes SDL headers.
add_library(bb_platform_sdl2 STATIC
    sdl2/sdl2_platform.cpp
)
add_library(bb::platform_sdl2 ALIAS bb_platform_sdl2)
target_include_directories(bb_platform_sdl2 PUBLIC sdl2/include)
target_link_libraries(bb_platform_sdl2 PUBLIC bb::platform PRIVATE SDL2::SDL2)
if(WIN32)
    target_link_libraries(bb_platform_sdl2 PRIVATE psapi)  # GetProcessMemoryInfo
endif()
bb_set_strict_warnings(bb_platform_sdl2)

# The process entry point of SDL 2 builds (calls bbMain, see entry.hpp).
# An object library: its main() must be linked into the executable, and
# SDL2main supplies the real entry point where SDL needs one (Windows).
add_library(bb_platform_sdl2_main OBJECT sdl2/sdl2_main.cpp)
target_link_libraries(bb_platform_sdl2_main PUBLIC bb::platform PRIVATE SDL2::SDL2)
if(BB_SDL2MAIN)
    target_link_libraries(bb_platform_sdl2_main INTERFACE ${BB_SDL2MAIN})
endif()
bb_set_strict_warnings(bb_platform_sdl2_main)
