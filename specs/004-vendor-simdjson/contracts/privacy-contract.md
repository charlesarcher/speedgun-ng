# Contract: Privacy Surfaces and Audits

**Feature**: `004-vendor-simdjson` | [plan.md](../plan.md) | [data-model.md](../data-model.md) | [research.md](../research.md)

The privacy contract states what a downstream `find_package(speedgun-ng)` consumer can observe. Every row is one command with a binary verdict; CI runs each one (plan Test Plan). The audit pattern is the substring `simdjson`, case-insensitive. It matches the mangled vendored symbols (`_ZN8simdjson...`), the discovery-call pattern (`find_package(simdjson`, `pkg_check_modules(simdjson`), and `include/` cleanliness, so the audits read uniformly on both sides of the contract: present where required (SC-009), absent everywhere a consumer can see (SC-002 through SC-005). There is no symbol prefix (research R-007), so the pattern is the bare library name.

## 1. Surface audits

| # | Surface | Audit command | Pass condition | Requirements |
|---|---|---|---|---|
| A1 | Install tree | `find prefix/ -iname '*simdjson*'` | empty output | FR-014, SC-002 |
| A2 | Package files | `grep -i simdjson prefix/lib/cmake/speedgun-ng/*.cmake` | empty output (covers `speedgun-ngConfig.cmake` and `speedgun-ngTargets.cmake`) | FR-015, SC-003 |
| A3 | Shared link interface | `grep -i simdjson prefix/lib/cmake/speedgun-ng/speedgun-ngTargets*.cmake` on a shared build | empty output; no `INTERFACE_LINK_LIBRARIES` entry resolves to simdjson | FR-015 |
| A4 | Dynamic symbols | `nm -D --defined-only libspeedgun-ng.so \| grep -i simdjson` | empty output (hidden members are not promoted to `.dynsym`) | FR-016, SC-004 |
| A5 | Runtime dependencies | `ldd prefix/lib/libspeedgun-ng.so \| grep -i simdjson` | empty output (vendored build forced static: no `libsimdjson` object) | SC-004, R-005 |
| A6 | Static archive contents (build tree) | `nm build/.../libspeedgun-ng.a` listing vendored members, plus a defined `get_active_implementation` resolving the gate's U reference | nonzero: the gate object's reference resolves into merged members | FR-007, SC-009 |
| A7 | Build files | `grep -rniE 'find_package\( *simdjson\|pkg_check_modules\( *simdjson' --include='*.cmake' --include='CMakeLists.txt' .` with `external/` excluded | zero hits | FR-004, SC-008 |
| A8 | Public headers | `grep -rni 'simdjson' include/` | zero hits | FR-013 |

A1 through A5 and A7 run in CI after `cmake --install`; A6 runs against the build tree in `test`; A8 runs in the registered ctest purity scan (`tools/simdjson/simdjson_purity_scan.sh`, modeled on `tools/hwloc/hwloc_purity_scan.sh`). The nm proof (A6) is `tools/simdjson/simdjson_nm_proof.sh`, modeled on `tools/hwloc/hwloc_nm_proof.sh`; with no prefix, it matches the demangled/mangled `simdjson` name.

## 2. Downstream consumer test (FR-017, SC-005)

The authoritative proof, run in the `downstream-consumer` CI job and reproducible locally (quickstart section 8). The existing `test/consumer/` project from specs/003 is reused unchanged: it `find_package(speedgun-ng)`, links, and runs, and names no vendored dependency, so its success plus the log greps prove simdjson invisibility as well as hwloc's.

**Machine state**: the runner carries simdjson as a system package (on Debian-family runners: `apt-get install libsimdjson-dev`), so discovery would succeed were any to exist. The vendored copy sits in-tree at the pinned commit (FR-001); the system copy is optional for the project's own build and irrelevant to the outcome by design.

**Steps and verdicts**:

1. Install: `cmake --install build --prefix prefix` (static, defaults) → exit 0.
2. Configure: `cmake -S test/consumer -B build-consumer -D CMAKE_PREFIX_PATH=$PWD/prefix -D CMAKE_BUILD_TYPE=Release` → exit 0.
3. Build: `cmake --build build-consumer -v` → exit 0.
4. Run: the consumer binary → exit 0.
5. Grep verdicts (all empty): the consumer's configure log greps `-i simdjson` zero times; its link command (captured via `-v`) greps `-i simdjson` zero times.

Any simdjson path or target leaking into the package files surfaces as a configure failure or a grep hit at step 5.

## 3. Boundary semantics

- **Static library self-containment (A6)**: merged vendored members inside `libspeedgun-ng.a` are contract-legal: an archive's member list is a build input to consumer links, a consumer-observable *dependency expression* would be a package-file or link-interface entry, and those stay empty (A2, A3). The merged archive is also what makes consumer links succeed once the gate object is pulled (research R-009).
- **Hidden visibility (A4)**: the vendored objects compile with `-fvisibility=hidden` (research R-007); merged or absorbed, they are not promoted to the shared library's `.dynsym`. There is no prefix, so collision removal rests on hidden visibility plus static absorption (FR-016); a consumer process loading a system simdjson alongside speedgun-ng sees no name collision from our hidden, non-exported copy.
- **Forced-static vendored build (A5)**: the scoped `BUILD_SHARED_LIBS OFF` (research R-005) guarantees no `libsimdjson` shared object exists to become a runtime dependency, in the static build and the shared build alike.
- **Include paths**: the vendored include directory is a `SYSTEM` usage requirement of the library target (research R-008); the installed interface carries the project's own header root alone.
- **Out of scope**: runtime behavior of simdjson calls: no speedgun-ng code calls simdjson beyond the wrapper's address-of in this feature (Scope boundaries); the contract covers presence, linkage, and export, and semantics arrive with the future serialization feature.
