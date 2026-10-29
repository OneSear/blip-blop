#!/bin/sh
# Builds SDL 2 for the macOS packages: universal (Apple silicon and Intel)
# and for macOS 11 and later, from the official source release. Homebrew's
# SDL 2 is built for the machine's processor and macOS version only: the
# first release's disk image, made on an Apple silicon runner, did not open
# on an Intel Mac ("not supported by this Mac", 29 October 2026).
#
#   tools/macos/sdl2-universal.sh <install prefix>
#
# Then configure the game with -DCMAKE_PREFIX_PATH=<install prefix>
# -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
# (the release and CI workflows do).
set -e
prefix=$1
[ -n "$prefix" ] || { echo "usage: $0 <install prefix>" >&2; exit 2; }

version=2.32.10  # the version vcpkg gives the Windows build
sha256=5f5993c530f084535c65a6879e9b26ad441169b3e25d789d83287040a9ca5165

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
curl -sSfL -o "$work/sdl.tar.gz" \
    "https://github.com/libsdl-org/SDL/releases/download/release-$version/SDL2-$version.tar.gz"
echo "$sha256  $work/sdl.tar.gz" | shasum -a 256 -c -
tar -xzf "$work/sdl.tar.gz" -C "$work"

cmake -S "$work/SDL2-$version" -B "$work/build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
    -DCMAKE_INSTALL_PREFIX="$prefix" \
    -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TEST=OFF > /dev/null
cmake --build "$work/build" --parallel
cmake --install "$work/build" > /dev/null
echo "SDL $version (arm64, x86_64, macOS 11) installed in $prefix"
