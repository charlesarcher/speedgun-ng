# Contract: Privacy Surfaces and Audits

**Feature**: `006-vendor-yaml-cpp` | [plan.md](../plan.md) | [data-model.md](../data-model.md) | [research.md](../research.md)

The privacy contract states what a downstream `find_package(speedgun-ng)` consumer can observe. Every row is one command with a binary verdict; CI runs each one (plan Test Plan). The audit pattern per FR-018 is the case-insensitive regex `yaml[-_]?cpp` (covering `yaml-cpp`, `yaml_cpp`, `yamlcpp`). A repo-spelling-only search suffices for paths here, yet the pattern is kept broad so any real leak trips it: upstream names its target, package, artifacts, and pkg-config file `yaml-cpp`, and its namespace `YAML`. On a shared build the audits additionally flag any exported symbol whose mangled name contains `4YAML`, the namespace encoding of `YAML::` (FR-018).

## 1. Surface audits

| # | Surface | Audit command | Pass condition | Requirements |
|---|---|---|---|---|
| A1 | Install tree | `find prefix/ -iname '*yaml*cpp*'` | empty output (the glob covers `yaml-cpp`, `yaml_cpp`, `yamlcpp`, and any interleaving) | FR-015, SC-002 |
| A2 | Package files | `grep -riE 'yaml[-_]?cpp' prefix/lib/cmake/speedgun-ng/*.cmake` | empty output (covers `speedgun-ngConfig.cmake`, `speedgun-ngTargets.cmake`) | FR-016, SC-003 |
| A3 | Shared link interface | `grep -riE 'yaml[-_]?cpp' prefix/lib/cmake/speedgun-ng/speedgun-ngTargets*.cmake` on a shared build | empty output; no `INTERFACE_LINK_LIBRARIES` entry resolves to yaml-cpp | FR-016 |
| A4 | Dynamic symbols | `nm -D --defined-only libspeedgun-ng.so \| grep -iE 'yaml[-_]?cpp'`; and `nm -D --defined-only libspeedgun-ng.so \| grep '4YAML'` | empty output both (hidden members with `YAML_CPP_API` resolved to nothing are not promoted; no `YAML::`-namespace export) | FR-017, SC-004 |
| A5 | Runtime dependencies | `ldd prefix/lib/libspeedgun-ng.so \| grep -iE 'yaml[-_]?cpp'` | empty output (vendored build forced static: no separate object exists) | SC-004, R-005 |
| A6 | Static archive contents (build tree) | `nm build/.../libspeedgun-ng.a`: members defining `4YAML` symbols present, plus the gate object's `_ZN4YAML4Load` `U` reference resolving into a different member | nonzero: the gate reference resolves into merged members | FR-007, SC-009, R-003/R-010 |
| A7 | Build files (discovery) | `grep -rniE 'find_package\( *yaml[-_]?cpp\|pkg_check_modules\([^)]*yaml[-_]?cpp' --include='*.cmake' --include='CMakeLists.txt' .` with `external/` excluded | zero hits | FR-004, SC-008 |
| A8 | Public headers | `grep -riE 'yaml[-_]?cpp' include/` | zero hits | FR-014 |

A1 through A5 and A7 run in CI after `cmake --install`; A6 runs against the build tree in `test`; A7 and A8 run in the registered ctest purity scan (`tools/yaml/yaml_purity_scan.sh`, modeled on `tools/simdjson/simdjson_purity_scan.sh`). The nm proof (A6) is `tools/yaml/yaml_nm_proof.sh`, modeled on `tools/zlib/zlib_nm_proof.sh`.

## 2. Downstream consumer test (FR-019, SC-005)

The authoritative proof, run in the `downstream-consumer` CI job and reproducible locally (quickstart section 8). The existing `test/consumer/` project from specs/003 is reused unchanged: it `find_package(speedgun-ng)`, links, and runs, and names no vendored dependency, so its success plus the log greps prove yaml-cpp invisibility alongside hwloc's, simdjson's, HdrHistogram_c's, and zlib's.

**Machine state**: the runner carries yaml-cpp as a system install, so discovery would succeed were any to exist. The premise step builds the vendored tree out-of-tree into `/usr/local` (the `sys-hdr` pattern of the existing job: `cmake -S external/yaml-cpp -B sys-yaml -DYAML_CPP_BUILD_TESTS=OFF -DYAML_CPP_BUILD_TOOLS=OFF && cmake --build sys-yaml && sudo cmake --install sys-yaml --prefix /usr/local`); apt has no stable `libyaml-cpp-dev` guarantee across runners, and building from the pinned tree installs the exact version the tripwire pins. The vendored copy sits in-tree at the pinned commit (FR-001); the system copy is irrelevant to the outcome by design.

**Steps and verdicts**:

1. Install: `cmake --install build --prefix prefix` (static, defaults) → exit 0.
2. Configure: `cmake -S test/consumer -B build-consumer -D CMAKE_PREFIX_PATH=$PWD/prefix -D CMAKE_BUILD_TYPE=Release` → exit 0, log captured.
3. Build: `cmake --build build-consumer -v` → exit 0, log captured.
4. Run: the consumer binary → exit 0.
5. Grep verdict (empty): the consumer's configure log and link command (captured via `-v`) grep `yaml[-_]?cpp` zero times.

Any yaml-cpp path or target leaking into the package files surfaces as a configure failure or a grep hit at step 5.

## 3. Boundary semantics

- **Static library self-containment (A6)**: merged vendored members inside `libspeedgun-ng.a` are contract-legal: an archive's member list is a build input to consumer links; a consumer-observable dependency expression would be a package-file or link-interface entry, and those stay empty (A2, A3). The merged archive is what makes consumer links succeed once a gate object is pulled (research R-010).
- **Hidden visibility and the export macro (A4)**: the vendored objects compile with `-fvisibility=hidden` and `CMAKE_VISIBILITY_INLINES_HIDDEN` (research R-007), and the static target propagates `YAML_CPP_STATIC_DEFINE`, which resolves `YAML_CPP_API` to nothing on every yaml-cpp declaration, so nothing requests default visibility. yaml-cpp carries no symbol-prefix mechanism; hidden visibility plus static absorption is the posture the HdrHistogram_c import took. A consumer process loading a system yaml-cpp in the same address space as speedgun-ng sees no name collision from our hidden, non-exported copy.
- **Forced-static vendored build (A5)**: `YAML_BUILD_SHARED_LIBS OFF` under a `CMP0077 NEW` default (research R-005) guarantees no `libyaml-cpp.so` exists to become a runtime dependency, in the static and shared builds alike.
- **Install-rule suppression (A1/A2)**: every upstream install rule, the export set, the package-config files, the `.pc` file, and the `uninstall` target sit behind `YAML_CPP_INSTALL`, switched off (research R-006); the package/pc generation writes only into the vendored binary directory. The HdrHistogram_c `install()` override upstream in the file aborts loudly on any rule hypothetically escaping the guard: no leak can pass silently.
- **Include paths**: the vendored include directories are `SYSTEM` usage requirements inside the bracket (research R-009) of a `BUILD_INTERFACE`-only link edge; the installed interface carries the project's own header root alone.
- **Out of scope**: runtime behavior of any yaml-cpp call: no speedgun-ng code calls it beyond the gate address-of in this feature (Scope boundaries); the contract covers presence, linkage, and export, and parsing semantics arrive with the future configuration feature.
