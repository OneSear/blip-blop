#!/bin/sh
# Cross-builds the project for 64-bit Windows in a container (MinGW-w64 GCC,
# the official SDL2 MinGW package) and runs every test, the
# regression replays included, under Wine. It checks that the code builds
# and runs on Windows; the MSVC build is checked by CI.
set -e
root=$(cd "$(dirname "$0")/../.." && pwd)
docker build -q -t bb-mingw "$root/tools/docker/mingw" > /dev/null
docker run --rm -v "$root":/src -w /src bb-mingw sh -c '
  cmake -S /src -B /src/build/mingw -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=/opt/mingw-toolchain.cmake > /dev/null &&
  cmake --build /src/build/mingw --parallel "$(nproc)" &&
  ctest --test-dir /src/build/mingw -j "$(nproc)" --output-on-failure'
