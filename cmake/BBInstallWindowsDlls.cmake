# Install-time script (Windows, run by install(SCRIPT) in BBPackaging.cmake):
# copies the DLLs the game loads and the DLLs those load in turn next to
# the installed executable. $<TARGET_RUNTIME_DLLS> lists only the direct
# ones (SDL2, SDL2_mixer); SDL2_mixer's codecs (ogg, vorbis, vorbisfile,
# mpg123, wavpack from vcpkg) were missing from the packages, and the
# installed game did not start (0xc0000135, found by the package replays in
# CI, 28 October 2026). SDL2_mixer is no longer used since Phase 9; the rule
# stays for whatever the DLLs load. Expects BB_GAME, the built executable;
# the DLLs are looked for next to it, where the build puts them.

get_filename_component(bb_game_dir "${BB_GAME}" DIRECTORY)
file(GET_RUNTIME_DEPENDENCIES
    EXECUTABLES "${BB_GAME}"
    RESOLVED_DEPENDENCIES_VAR bb_dlls
    UNRESOLVED_DEPENDENCIES_VAR bb_missing
    DIRECTORIES "${bb_game_dir}"
    # The system's DLLs (the API sets resolve to none).
    PRE_EXCLUDE_REGEXES "^api-ms-" "^ext-ms-"
    POST_EXCLUDE_REGEXES "[/\\\\][Ss][Yy][Ss][Tt][Ee][Mm]32[/\\\\]")
foreach(dll IN LISTS bb_dlls)
    file(INSTALL "${dll}" DESTINATION "${CMAKE_INSTALL_PREFIX}")
endforeach()
if(bb_missing)
    message(FATAL_ERROR "DLLs of the game not found next to ${BB_GAME}: ${bb_missing}")
endif()
