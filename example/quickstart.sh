#!/usr/bin/env bash
# The command lines the root page quotes. Each region below is one block a
# reader copies, so every flag stands spelled out and the file stays
# runnable from a scratch directory with network access.

set -eu

# sg-example:begin configure
cmake -S . -B build -D CMAKE_BUILD_TYPE=Release
# sg-example:end configure

# sg-example:begin build
cmake --build build -j
# sg-example:end build

# sg-example:begin install
cmake --install build --prefix "$HOME/.local"
# sg-example:end install

# sg-example:begin consumer-configure
cmake -S . -B build -DCMAKE_PREFIX_PATH="$HOME/.local"
# sg-example:end consumer-configure

# sg-example:begin consumer-build
cmake --build build
# sg-example:end consumer-build

# sg-example:begin run
./build/first_benchmark
# sg-example:end run

# sg-example:begin options
./build/first_benchmark --list
./build/first_benchmark --filter='bmSum' --min-time=200000000
./build/first_benchmark --counter=machine/monotonic --dry-run
./build/first_benchmark --catalog
# sg-example:end options
