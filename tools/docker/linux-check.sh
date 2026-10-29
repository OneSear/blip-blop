#!/bin/sh
# Builds and tests the project on Linux in a container (case-sensitive file
# system, GCC) and compares the regression replays with the expected
# fingerprints recorded on any other platform.
# The job count is the container's CPU count: an unlimited count made the
# compilers run out of memory in Docker Desktop's default 4 GB VM.
set -e
root=$(cd "$(dirname "$0")/../.." && pwd)
docker build -q -t bb-linux "$root/tools/docker" > /dev/null
docker run --rm -v "$root":/src -w /src bb-linux sh -c '
  cmake -S /src -B /src/build/linux -DCMAKE_BUILD_TYPE=Release > /dev/null &&
  cmake --build /src/build/linux --parallel "$(nproc)" &&
  ctest --test-dir /src/build/linux --output-on-failure'
