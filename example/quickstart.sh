#!/usr/bin/env bash
# The command lines the root page quotes. Each region below is one block a
# reader copies, so every flag stands spelled out and the file stays
# runnable from a scratch directory with network access.

set -eu

# sg-example:begin library
cmake -S . -B build -D CMAKE_BUILD_TYPE=Release
cmake --build build -j
cmake --install build --prefix "$HOME/.local"
# sg-example:end library

# sg-example:begin consumer
cmake -S . -B build -DCMAKE_PREFIX_PATH="$HOME/.local"
cmake --build build
./build/first_benchmark
# sg-example:end consumer

# sg-example:begin options
./build/first_benchmark --list
./build/first_benchmark --filter='bmSum' --min-time=0.2
./build/first_benchmark --counter=machine/monotonic --dry-run
./build/first_benchmark --catalog
# sg-example:end options
