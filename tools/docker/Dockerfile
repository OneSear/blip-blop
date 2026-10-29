# Linux build environment matching the CI's Ubuntu runner.
#   docker build -t bb-linux tools/docker
#   tools/docker/linux-check.sh            (build + all tests, Release, GCC)
#   tools/docker/linux-bench.sh            (present benchmark under Xvfb)
FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update -qq \
 && apt-get install -y -qq cmake g++ clang make pkg-config libsdl2-dev \
    xvfb xauth libgl1-mesa-dri \
 && rm -rf /var/lib/apt/lists/*
