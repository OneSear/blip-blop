# Windows cross-build environment (MinGW-w64, GCC) with the official SDL2
# and SDL2_mixer MinGW development packages; Wine runs the tests.
#   tools/docker/windows-cross-check.sh
FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive
RUN dpkg --add-architecture i386 \
 && apt-get update -qq \
 && apt-get install -y -qq cmake make mingw-w64 curl ca-certificates wine64 wine \
 && rm -rf /var/lib/apt/lists/*
ARG SDL2_VERSION=2.30.8
ARG SDL2_MIXER_VERSION=2.8.0
RUN mkdir -p /opt/sdl && cd /tmp \
 && curl -fsSL -o sdl2.tar.gz https://github.com/libsdl-org/SDL/releases/download/release-${SDL2_VERSION}/SDL2-devel-${SDL2_VERSION}-mingw.tar.gz \
 && curl -fsSL -o mixer.tar.gz https://github.com/libsdl-org/SDL_mixer/releases/download/release-${SDL2_MIXER_VERSION}/SDL2_mixer-devel-${SDL2_MIXER_VERSION}-mingw.tar.gz \
 && tar xzf sdl2.tar.gz && tar xzf mixer.tar.gz \
 && cp -r SDL2-${SDL2_VERSION}/x86_64-w64-mingw32/. /opt/sdl/ \
 && cp -r SDL2_mixer-${SDL2_MIXER_VERSION}/x86_64-w64-mingw32/. /opt/sdl/ \
 && rm -rf /tmp/*
COPY mingw-toolchain.cmake /opt/mingw-toolchain.cmake
# A prepared Wine prefix (it must be created before WINEPATH is set: one
# created with it fails with "could not load kernel32.dll"), then the
# directories where Wine finds the MinGW runtime and the SDL DLLs.
RUN WINEDEBUG=-all wineboot --init && wineserver --wait
ENV WINEPATH="/usr/lib/gcc/x86_64-w64-mingw32/13-posix;/usr/x86_64-w64-mingw32/lib;/opt/sdl/bin" \
    WINEDEBUG=-all
