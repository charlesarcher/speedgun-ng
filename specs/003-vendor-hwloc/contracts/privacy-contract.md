# Contract: Privacy Surfaces and Audits

**Feature**: `003-vendor-hwloc` | [plan.md](../plan.md) | [data-model.md](../data-model.md) | [research.md](../research.md)

The privacy contract states what a downstream `find_package(speedgun-ng)` consumer can observe. Every row is one command with a binary verdict; CI runs each one (plan Test Plan). The audit pattern is the substring `hwloc`, case-insensitive: the adopted symbol prefix `sg_` yields `sg_hwloc_*` symbols (research R-003), which the same pattern still matches, so the audits read uniformly on both sides of the contract: present where required (SC-011), absent everywhere a consumer can see (SC-002 through SC-005).

## 1. Surface audits

| # | Surface | Audit command | Pass condition | Requirements |
|---|---|---|---|---|
| A1 | Install tree | `find prefix/ -iname '*hwloc*'` | empty output | FR-022, SC-002 |
| A2 | Package files | `grep -i hwloc prefix/lib/cmake/speedgun-ng/*.cmake` | empty output (covers `speedgun-ngConfig.cmake` and `speedgun-ngTargets.cmake`) | FR-023, SC-003 |
| A3 | Shared link interface | `grep -i hwloc prefix/lib/cmake/speedgun-ng/speedgun-ngTargets*.cmake` on a shared build | empty output; no `INTERFACE_LINK_LIBRARIES` entry resolves to hwloc | FR-023 |
| A4 | Dynamic symbols | `nm -D --defined-only libspeedgun-ng.so \| grep -i hwloc` | empty output | FR-024, SC-004 |
| A5 | Runtime dependencies | `ldd prefix/lib/libspeedgun-ng.so \| grep -i hwloc` | empty output | SC-004 |
| A6 | Static archive contents (build tree) | `nm build/.../libspeedgun-ng.a \| grep -c 'sg_hwloc_get_api_version'` plus `nm` listing vendored members | nonzero: the gate object's reference resolves into merged members | FR-007, SC-011 |
| A7 | Build files | `grep -rniE 'find_package\( *hwloc\|pkg_check_modules\( *hwloc' --include='*.cmake' --include='CMakeLists.txt' .` with `external/` excluded | zero hits | FR-004, SC-009 |
| A8 | Public headers | `grep -rni 'hwloc' include/` | zero hits | FR-021 |

A1 through A5 and A7 run in CI after `cmake --install`; A6 runs against the build tree in `test`; A8 runs in the registered ctest purity scan (`tools/hwloc/hwloc_purity_scan.sh`, same script as A7, modeled on `tools/dbc/dependency_scan.sh`).

## 2. Downstream consumer test (FR-026, SC-005)

The authoritative proof, run in the `downstream-consumer` CI job and reproducible locally (quickstart section 8).

**Machine state**: the runner carries hwloc as a system package (`apt-get install libhwloc-dev`), so discovery would succeed were any to exist. The vendored copy sits in-tree at the pinned commit (FR-001); the system copy is optional for the project's own build and irrelevant to the outcome by design.

**Steps and verdicts**:

1. Install: `cmake --install build --prefix prefix` (static, defaults) → exit 0.
2. Configure: `cmake -S test/consumer -B build-consumer -D CMAKE_PREFIX_PATH=$PWD/prefix -D CMAKE_BUILD_TYPE=Release` → exit 0.
3. Build: `cmake --build build-consumer` → exit 0.
4. Run: the consumer binary → exit 0.
5. Grep verdicts (all empty): the consumer's configure log greps `-i hwloc` zero times; its link command (captured via `-DCMAKE_VERBOSE_MAKEFILE=ON` or the build log) greps `-i hwloc` zero times.

`test/consumer/` is a two-file project (CMakeLists.txt, main.cpp) that calls the public speedgun-ng API exactly as the README instructs; it never names hwloc, and any hwloc path or target leaking into the package files would surface as a configure failure or a grep hit at step 5.

## 3. Boundary semantics

- **Static library self-containment (A6)**: merged vendored members inside `libspeedgun-ng.a` are contract-legal: an archive's member list is a build input to consumer links, a consumer-observable *dependency expression* would be a package-file or link-interface entry, and those stay empty (A2, A3). The merged archive is also what makes consumer links succeed once the gate object is pulled (research R-010).
- **Hidden visibility (A4)**: `CXX_VISIBILITY_PRESET hidden` plus `VISIBILITY_INLINES_HIDDEN` (CMakeLists.txt:38-39) keeps merged members out of the shared library's dynamic table; the prefix additionally removes name collisions when a consumer process loads a system hwloc alongside speedgun-ng (FR-025).
- **Include paths**: the staged hwloc include directory is a `PRIVATE` usage requirement of the library target; the installed interface carries the project's own header root alone.
- **Out of scope**: `Fixes`-style runtime behavior of hwloc calls: no speedgun-ng code calls hwloc beyond the wrapper's reference in this feature (Scope boundaries); the contract covers presence and linkage, semantics arrive with the future pinning feature.
