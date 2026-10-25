#!/bin/sh
# Runs the present benchmark (tests/bench/present_bench.cpp) on Linux in a
# container, on an Xvfb display: SDL's X11 driver with Mesa's software
# OpenGL, no GPU. Uses the build of linux-check.sh (build/linux).
# Usage: tools/docker/linux-bench.sh [frames] [reverse]
set -e
root=$(cd "$(dirname "$0")/../.." && pwd)
docker build -q -t bb-linux "$root/tools/docker" > /dev/null
docker run --rm -v "$root":/src -w /src bb-linux sh -c '
  cmake -S /src -B /src/build/linux -DCMAKE_BUILD_TYPE=Release > /dev/null &&
  cmake --build /src/build/linux --parallel "$(nproc)" --target bb_present_bench > /dev/null &&
  xvfb-run -a -s "-screen 0 1024x768x24" /src/build/linux/tests/bb_present_bench "$@"' sh "$@"
