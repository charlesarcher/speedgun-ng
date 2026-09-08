# speedgun-ng

<p align="center">
  <img src="docs/images/sg.jpg" alt="speedgun-ng" width="280">
</p>

Speedgun Next Generation - a modern C++20 benchmarking framework
(benchmarking in the spirit of Google Benchmark, with additional
features). CMake build system, BSD 3-Clause license.

This project is developed **spec-driven** and is governed by a
**constitution** that is the highest authority for how code is written,
tested, and reviewed.

- **Standards & governance** —
  [`.specify/memory/constitution.md`](.specify/memory/constitution.md):
  coding standards, Design By Contract, the R-DCUT design process,
  coverage and CI gates, and the commit/PR standard. Read this before
  contributing.
- **AI-agent guide** — [AGENTS.md](AGENTS.md): the spec-driven workflow
  for coding agents working in this repository.

## Building and installing

The project uses [CMake][1] with [presets][2] and requires no special
flags for a standard build.

### Build

Single-configuration generator (e.g. the Unix Makefiles one), release
mode:

```sh
cmake -S . -B build -D CMAKE_BUILD_TYPE=Release
cmake --build build
```

Multi-configuration generator (e.g. the Visual Studio ones), release
mode:

```sh
cmake -S . -B build
cmake --build build --config Release
```

- **MSVC** is not standards-compliant by default; pass the flags in the
  `flags-msvc` preset in [`CMakePresets.json`](CMakePresets.json) during
  configuration to make it behave properly.
- **Apple Silicon** is built correctly by CMake since 3.20.1; install
  the [latest CMake][1] to be safe.

### Install

Build first, then install the release artifacts. These commands require
CMake ≥ 3.15 ([Install a Project][3]).

```sh
cmake --install build                  # single-configuration
cmake --install build --config Release # multi-configuration
```

#### CMake package

The project exports a CMake package for the [`find_package`][4] command:

- Package name: `speedgun-ng`
- Target name: `speedgun-ng::speedgun-ng`

```cmake
find_package(speedgun-ng REQUIRED)
target_link_libraries(
    project_target PRIVATE
    speedgun-ng::speedgun-ng
)
```

#### Note to packagers

`CMAKE_INSTALL_INCLUDEDIR` is set to a path other than just `include`
when configured as a top-level project, to avoid indirectly including
other libraries when installed to a common prefix. See
[`cmake/install-rules.cmake`](cmake/install-rules.cmake) for the full
set of install rules.

## Development

Build-system targets that only developers need are hidden unless the
`speedgun-ng_DEVELOPER_MODE` option is enabled. Enabling it exposes the
tests and other developer targets and options. CI always builds with
developer mode on; a consumer of the library does not need it.

### Presets and developer mode

Create a `CMakeUserPresets.json` at the project root. This file is
machine-local and must **not** be committed. The recommended `dev`
preset inherits a developer-mode preset and an OS-specific CI preset
(`ci-linux`, `ci-darwin`, or `ci-win64`) and is configured as Debug:

```json
{
  "version": 2,
  "cmakeMinimumRequired": { "major": 3, "minor": 14, "patch": 0 },
  "configurePresets": [
    {
      "name": "dev",
      "binaryDir": "${sourceDir}/build/dev",
      "inherits": ["dev-mode", "ci-<os>"],
      "cacheVariables": { "CMAKE_BUILD_TYPE": "Debug" }
    }
  ],
  "buildPresets": [
    { "name": "dev", "configurePreset": "dev", "configuration": "Debug" }
  ],
  "testPresets": [
    {
      "name": "dev",
      "configurePreset": "dev",
      "configuration": "Debug",
      "output": { "outputOnFailure": true }
    }
  ]
}
```

Replace `<os>` with `win64`, `linux`, or `darwin`. See
[`CMakePresets.json`](CMakePresets.json) for what the inherited presets
map to.

### Configure, build, and test

With the presets file in place:

```sh
cmake --preset=dev
cmake --build --preset=dev
ctest --preset=dev
```

The build and test commands accept `-j <n>` (use your CPU's thread
count); you can also set `jobs` on the preset. A compatible editor (VS
Code, CLion, Visual Studio) can pick up the same user presets.

### Developer-mode targets

Invoke these with the build command plus `-t <target>`:

- `coverage` (if `ENABLE_COVERAGE`) — processes the output of a
  previously coverage-configured test run into an info file (submittable
  to CI services) and, by default, an HTML report in
  `<binary-dir>/coverage_html`.
- `docs` (if `BUILD_MCSS_DOCS`) — builds documentation with Doxygen and
  m.css into `<binary-dir>/docs`.
- `format-check` / `format-fix` — run clang-format to check or fix the
  codebase.
- `run-examples` — run all examples created by `add_example`.
- `spell-check` / `spell-fix` — run codespell to check or fix the
  codebase.

## Contributing

Changes are developed **spec-driven** through
[Spec Kit](https://github.com/github/spec-kit). The flow is
specification → technical plan → task list → implementation, governed by
the [constitution](.specify/memory/constitution.md). See
[AGENTS.md](AGENTS.md) for the exact workflow and command list.

- Bug fixes and trivial changes (typos, formatting, build fixes) may
  bypass the full workflow. Changes touching public API, behavior, or
  build configuration must not.
- Every change must pass the hard CI quality gates in the constitution
  (build, tests, sanitizers, static analysis, `format-check`,
  `spell-check`, and 100% line/branch/DBC coverage).
- Commits and pull requests follow the commit/PR standard in the
  [constitution](.specify/memory/constitution.md) (Pull Request
  Quality): `<Section>: <description>` title, a why-body, an
  `Approved-by:` footer, and a linear base-branch history.

## Licensing

This project is licensed under the BSD 3-Clause License. See the
[LICENSE](LICENSE) file for details.

[1]: https://cmake.org/download/
[2]: https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html
[3]: https://cmake.org/cmake/help/latest/manual/cmake.1.html#install-a-project
[4]: https://cmake.org/cmake/help/latest/command/find_package.html
