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

## Quality gates

One command runs the prose and commit-message gate over the range from
the merge base with `origin/master` to `HEAD`, the same verdict CI
produces:

```sh
cmake -P cmake/prose-lint.cmake
```

Direct and build-target equivalents:

```sh
python3 tools/prose/prose_gate.py --check all
cmake --build build/dev -t prose-lint
cmake --build build/dev -t prose-lint-fixtures
```

Exit 0 is clean, 1 reports findings, and 2 signals a usage error or an
unusable environment. The vocabulary and thresholds live in
`tools/prose/prose_rules.yaml`, a mechanical projection of constitution
Principle XI and the Pull Request Quality template. The gate's own
fixtures run through `ctest -R prose_gate_fixtures`.
