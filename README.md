# speedgun-ng

<p align="center">
  <img src="docs/images/sg.jpg" alt="speedgun-ng" width="280">
</p>

A C++23 benchmarking framework in the spirit of Google Benchmark.

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
