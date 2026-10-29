# Installation and packages (CPack): the executable, the runtime pack
# (blipblop.bbpack; the 2002 data directory when the asset tools are not
# built), the licence and the README, which carries the credits.
#
#   Windows: the executable and the SDL DLLs in one folder; NSIS installer
#            and zip.
#   macOS:   "Blip and Blop.app" (the pack in Contents/Resources, the SDL
#            libraries and their dependencies in Contents/Frameworks) on a disk
#            image, with the licence and the README beside it.
#   Linux:   a .tar.gz of the folder; SDL 2 comes from the system.
#
# The version in the package names and the macOS app: the project's, or
# the release's (-DBB_PACKAGE_VERSION=1.2.0, .github/workflows/release.yml).
#
# The game finds its data next to the executable (bb::app::locateGameData);
# in an app bundle SDL reports Contents/Resources as that directory.

set(BB_PACKAGE_VERSION "${PROJECT_VERSION}" CACHE STRING "Version of the packages (default: the project's)")
set(BB_PACKAGE_DOCS "${CMAKE_SOURCE_DIR}/LICENSE" "${CMAKE_SOURCE_DIR}/README.md")

if(APPLE)
    set(BB_BUNDLE "Blip and Blop.app")
    set(BB_DATA_DESTINATION "${BB_BUNDLE}/Contents/Resources")
else()
    set(BB_DATA_DESTINATION .)
endif()

# First: on macOS the bundle is signed last.
if(BB_BUILD_TOOLS)
    install(FILES "${BB_PACK}" DESTINATION "${BB_DATA_DESTINATION}")
else()
    install(DIRECTORY "${BB_RUN_DIR}/data" DESTINATION "${BB_DATA_DESTINATION}" PATTERN ".extracted" EXCLUDE)
endif()

if(APPLE)
    configure_file("${CMAKE_SOURCE_DIR}/cmake/Info.plist.in" "${CMAKE_BINARY_DIR}/Info.plist" @ONLY)
    install(PROGRAMS "$<TARGET_FILE:blipblop>" DESTINATION "${BB_BUNDLE}/Contents/MacOS")
    install(FILES "${CMAKE_BINARY_DIR}/Info.plist" DESTINATION "${BB_BUNDLE}/Contents")
    install(FILES ${BB_PACKAGE_DOCS} DESTINATION .)

    # sdl2-compat (Homebrew's SDL 2 since 2025) loads SDL 3 at run time,
    # first from @loader_path/libSDL3.dylib: the bundle needs a copy there,
    # which the link-time dependencies do not show.
    find_library(BB_SDL3_DYLIB NAMES SDL3)
    install(CODE "
        include(BundleUtilities)
        set(BU_CHMOD_BUNDLE_ITEMS ON)
        set(bundle \"\${CMAKE_INSTALL_PREFIX}/${BB_BUNDLE}\")
        fixup_bundle(\"\${bundle}\" \"\" \"\")
        set(sdl2 \"\${bundle}/Contents/Frameworks/libSDL2-2.0.0.dylib\")
        if(EXISTS \"\${sdl2}\")
            file(STRINGS \"\${sdl2}\" compat REGEX \"@loader_path/libSDL3.dylib\")
            if(compat)
                if(NOT EXISTS \"${BB_SDL3_DYLIB}\")
                    message(FATAL_ERROR \"SDL 2 is sdl2-compat but SDL 3 was not found\")
                endif()
                get_filename_component(sdl3 \"${BB_SDL3_DYLIB}\" REALPATH)
                execute_process(COMMAND \"${CMAKE_COMMAND}\" -E copy \"\${sdl3}\"
                                        \"\${bundle}/Contents/Frameworks/libSDL3.dylib\")
                execute_process(COMMAND chmod 644 \"\${bundle}/Contents/Frameworks/libSDL3.dylib\")
                execute_process(COMMAND install_name_tool -id @loader_path/libSDL3.dylib
                                        \"\${bundle}/Contents/Frameworks/libSDL3.dylib\")
            endif()
        endif()
        # Changed binaries lose their signatures; Apple silicon runs only
        # signed code (an ad hoc signature is enough).
        execute_process(COMMAND codesign --force --deep --sign - \"\${bundle}\")
    ")
else()
    install(TARGETS blipblop RUNTIME DESTINATION .)
    install(FILES ${BB_PACKAGE_DOCS} DESTINATION .)
    if(WIN32 AND NOT CMAKE_VERSION VERSION_LESS 3.21)
        install(FILES "$<TARGET_RUNTIME_DLLS:blipblop>" DESTINATION .)
    endif()
    if(CMAKE_HOST_WIN32)
        # And the DLLs those load (cmake/BBInstallWindowsDlls.cmake). Only on
        # a Windows host: the MinGW cross-build has no tool to read them.
        install(CODE "set(BB_GAME \"$<TARGET_FILE:blipblop>\")")
        install(SCRIPT "${CMAKE_CURRENT_LIST_DIR}/BBInstallWindowsDlls.cmake")
    endif()
    if(MSVC)
        # The Microsoft C++ runtime the game and vcpkg's DLLs are built
        # against, for machines without the redistributable.
        set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION .)
        include(InstallRequiredSystemLibraries)
    endif()
endif()

set(CPACK_PACKAGE_NAME "BlipBlop")
set(CPACK_PACKAGE_VENDOR "LOADED Studio")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Blip & Blop, a 2D shooter by LOADED Studio (2002)")
set(CPACK_PACKAGE_VERSION "${BB_PACKAGE_VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Blip and Blop")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_EXECUTABLES "blipblop" "Blip & Blop")
set(CPACK_STRIP_FILES ON)
if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_DISPLAY_NAME "Blip & Blop")
    set(CPACK_NSIS_PACKAGE_NAME "Blip & Blop")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "Blip and Blop")
    # The licence is beside the app; no click-through agreement on opening.
    set(CPACK_DMG_SLA_USE_RESOURCE_FILE_LICENSE OFF)
else()
    set(CPACK_GENERATOR "TGZ")
endif()
include(CPack)
