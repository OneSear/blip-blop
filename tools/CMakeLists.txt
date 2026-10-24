# Asset tools (migration.md Phase 2). They may use third-party libraries
# (DEC-11): stb for PNG files, nlohmann/json for the source manifests.

add_library(bb_tools_thirdparty INTERFACE)
target_include_directories(bb_tools_thirdparty SYSTEM INTERFACE
    "${CMAKE_SOURCE_DIR}/third_party/stb"
    "${CMAKE_SOURCE_DIR}/third_party/nlohmann")

add_library(bb_asset_tools STATIC
    bbassets/json_text.cpp
    bbassets/png_io.cpp
    bbassets/source_assets.cpp
    bbassets/sprite_ids.cpp
    bbassets/stb_impl.cpp
)
target_include_directories(bb_asset_tools PUBLIC bbassets)
# bb::game for the entity names levels use (its registry).
target_link_libraries(bb_asset_tools PUBLIC bb::core bb::game bb_tools_thirdparty)
bb_set_strict_warnings(bb_asset_tools)

add_executable(bbassets bbassets/main.cpp)
target_link_libraries(bbassets PRIVATE bb_asset_tools)
bb_set_strict_warnings(bbassets)

# stb's PNG coder is third-party code nobody debugs here: optimise it in
# every configuration, so debug builds of the tools stay usable.
if(NOT MSVC)
    set_source_files_properties(bbassets/stb_impl.cpp PROPERTIES COMPILE_OPTIONS "-O2")
endif()

# The runtime pack the game can run from (--pack), built from the source
# assets. Without an assets/source tree, the source assets are first
# extracted from the legacy data into the build directory.
set(BB_ASSET_SOURCE_DIR "${CMAKE_SOURCE_DIR}/assets/source" CACHE PATH
    "Source assets (PNG, JSON, WAV, Ogg, UTF-8 text) the runtime pack is built from")
set(BB_PACK "${BB_RUN_DIR}/blipblop.bbpack" CACHE INTERNAL "")

if(EXISTS "${BB_ASSET_SOURCE_DIR}")
    set(BB_PACK_SOURCE "${BB_ASSET_SOURCE_DIR}" CACHE INTERNAL "")
    file(GLOB_RECURSE pack_inputs CONFIGURE_DEPENDS "${BB_ASSET_SOURCE_DIR}/*")
else()
    set(BB_PACK_SOURCE "${CMAKE_BINARY_DIR}/assets-source" CACHE INTERNAL "")
    set(pack_inputs "${BB_PACK_SOURCE}/.extracted")
    add_custom_command(
        OUTPUT "${BB_PACK_SOURCE}/.extracted"
        COMMAND "${CMAKE_COMMAND}" -E rm -rf "${BB_PACK_SOURCE}"
        COMMAND bbassets extract "${BB_RUN_DIR}/data" "${BB_PACK_SOURCE}"
        COMMAND "${CMAKE_COMMAND}" -E touch "${BB_PACK_SOURCE}/.extracted"
        DEPENDS bbassets legacy_data "${BB_RUN_DIR}/data/.extracted"
        COMMENT "Extracting source assets from the legacy data (no ${BB_ASSET_SOURCE_DIR})"
        VERBATIM)
endif()

add_custom_command(
    OUTPUT "${BB_PACK}"
    COMMAND bbassets pack "${BB_PACK_SOURCE}" "${BB_PACK}"
    DEPENDS bbassets ${pack_inputs}
    COMMENT "Building the runtime pack from ${BB_PACK_SOURCE}"
    VERBATIM)
add_custom_target(assets_pack ALL DEPENDS "${BB_PACK}")
