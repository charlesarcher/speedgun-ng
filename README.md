# speedgun-ng

<p align="center">
  <img src="docs/images/sg.jpg" alt="speedgun-ng" width="280">
</p>

A C++23 benchmarking framework in the spirit of Google Benchmark.

The name and the idea come from Speedgun, a tool I worked on at Akuna.
I wanted something like it for personal projects, so this is a
ground-up rethink: Speedgun Next Generation. It shares no code with
that project. Everything here is written by the author and a local AI
army.

## Build

```sh
cmake -S . -B build -D CMAKE_BUILD_TYPE=Release
cmake --build build
```

Install:

```sh
cmake --install build
```

Use from CMake:

```cmake
find_package(speedgun-ng REQUIRED)
target_link_libraries(your_target PRIVATE speedgun-ng::speedgun-ng)
```
