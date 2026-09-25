# Contract: Privacy Surfaces and Audits

**Feature**: `005-vendor-hdrhistogram` | [plan.md](../plan.md) | [data-model.md](../data-model.md) | [research.md](../research.md)

The privacy contract states what a downstream `find_package(speedgun-ng)` consumer can observe. Every row is one command with a binary verdict; CI runs each one (plan Test Plan). Two audit patterns per FR-019: the HdrHistogram_c pattern is the case-insensitive regex `hdr[-_]?histogram` (covering `hdr_histogram`, `hdr-histogram`, `hdrhistogram`); the zlib pattern is the case-insensitive pair `zlib`/`libz`. A repo-spelling-only search (`hdrhistogram`) is insufficient: upstream names its artifacts `hdr_histogram`, so a real leak must trip the check. On a shared build the audits additionally flag any exported symbol starting with `hdr_`, and name `inflate`/`deflate`/`compress`/`uncompress` for the zlib surface.

## 1. Surface audits

| # | Surface | Audit command | Pass condition | Requirements |
|---|---|---|---|---|
| A1 | Install tree (HdrHistogram_c) | `find prefix/ -iname '*hdr*histogram*'` | empty output (covers `hdr_histogram`/`hdr-histogram`/`hdrhistogram`) | FR-015, SC-002 |
| A1z | Install tree (zlib) | `find prefix/ -iname '*zlib*' -o -iname '*libz*'` | empty output | FR-015, SC-002 |
| A2 | Package files (HdrHistogram_c) | `grep -riE 'hdr[-_]?histogram' prefix/lib/cmake/speedgun-ng/*.cmake` | empty output (covers `speedgun-ngConfig.cmake`, `speedgun-ngTargets.cmake`) | FR-016, SC-003 |
| A2z | Package files (zlib) | `grep -riE 'zlib\|libz' prefix/lib/cmake/speedgun-ng/*.cmake` | empty output | FR-016, SC-003 |
| A3 | Shared link interface | `grep -riE 'hdr[-_]?histogram\|zlib\|libz' prefix/lib/cmake/speedgun-ng/speedgun-ngTargets*.cmake` on a shared build | empty output; no `INTERFACE_LINK_LIBRARIES` entry resolves to either | FR-016 |
| A4 | Dynamic symbols (HdrHistogram_c) | `nm -D --defined-only libspeedgun-ng.so \| grep -iE 'hdr[-_]?histogram'`; and `nm -D --defined-only libspeedgun-ng.so \| grep -wE '^_?hdr_'` | empty output (hidden members not promoted; no `hdr_`-prefixed export) | FR-017, SC-004 |
| A4z | Dynamic symbols (zlib) | `nm -D --defined-only libspeedgun-ng.so \| grep -iE 'zlib\|libz'`; and `grep -wE '(inflate\|deflate\|compress\|uncompress)'` | empty output (hidden, `z_`-prefixed members absent from `.dynsym`) | FR-022, SC-011 |
| A5 | Runtime dependencies | `ldd prefix/lib/libspeedgun-ng.so \| grep -iE 'hdr\|libz\|zlib'` | empty output (both vendored builds forced static: no separate object) | SC-004, SC-010, R-007 |
| A6 | Static archive contents: HdrHistogram_c (build tree) | `nm build/.../libspeedgun-ng.a`: `hdr_` vendored members present, plus the gate object's `hdr_alloc` U reference resolving into a different member | nonzero: the gate reference resolves into merged members | FR-007, SC-009, R-006 |
| A6z | Static archive contents: zlib (build tree) | `nm build/.../libspeedgun-ng.a`: zlib vendored members present (`z_`-prefixed under `Z_PREFIX`), plus the gate object's `zlibVersion`/`z_zlibVersion` U reference resolving into a different member | nonzero: the gate reference resolves into merged members | FR-021, SC-010, R-006 |
| A7 | Build files (HdrHistogram_c discovery) | `grep -rniE 'find_package\( *hdr[-_]?histogram\|pkg_check_modules\( *hdr[-_]?histogram' --include='*.cmake' --include='CMakeLists.txt' .` with `external/` excluded | zero hits | FR-004, SC-008 |
| A8 | Public headers | `grep -riE 'hdr[-_]?histogram' include/` and `grep -riE 'zlib\|libz' include/` | zero hits | FR-014 |
| A9 | Host zlib never resolved | configure record check: the configured `ZLIB_INCLUDE_DIR`/`ZLIB_LIBRARY` names the `external/zlib` path | vendored path recorded; host copy resolved zero times | FR-020, SC-010 |

A1 through A5, A7, and A9 run in CI after `cmake --install`; A6/A6z run against the build tree in `test`; A8 runs in the registered ctest purity scans (`tools/hdrhistogram/hdrhistogram_purity_scan.sh`, `tools/zlib/zlib_purity_scan.sh`, modeled on `tools/simdjson/simdjson_purity_scan.sh`). The nm proofs (A6/A6z) are `tools/hdrhistogram/hdrhistogram_nm_proof.sh` and `tools/zlib/zlib_nm_proof.sh`.

## 2. Downstream consumer test (FR-018, SC-005)

The authoritative proof, run in the `downstream-consumer` CI job and reproducible locally (quickstart section 8). The existing `test/consumer/` project from specs/003 is reused unchanged: it `find_package(speedgun-ng)`, links, and runs, and names no vendored dependency, so its success plus the log greps prove HdrHistogram_c and zlib invisibility as well as hwloc's and simdjson's.

**Machine state**: the runner carries both dependencies as system packages (on Debian-family runners: `apt-get install libhdrhistogram-c-dev zlib1g-dev`), so discovery would succeed were any to exist. `zlib1g-dev` is present on almost every runner, which makes the host-zlib non-selection concrete (SC-010). The vendored copies sit in-tree at the pinned commits (FR-001, FR-001a); the system copies are irrelevant to the outcome by design.

**Steps and verdicts**:

1. Install: `cmake --install build --prefix prefix` (static, defaults) → exit 0.
2. Configure: `cmake -S test/consumer -B build-consumer -D CMAKE_PREFIX_PATH=$PWD/prefix -D CMAKE_BUILD_TYPE=Release` → exit 0.
3. Build: `cmake --build build-consumer -v` → exit 0.
4. Run: the consumer binary → exit 0.
5. Grep verdicts (all empty): the consumer's configure log and link command (captured via `-v`) grep `hdr[-_]?histogram` zero times and `zlib`/`libz` zero times.

Any HdrHistogram_c or zlib path or target leaking into the package files surfaces as a configure failure or a grep hit at step 5.

## 3. Boundary semantics

- **Static library self-containment (A6/A6z)**: merged vendored members inside `libspeedgun-ng.a` are contract-legal: an archive's member list is a build input to consumer links; a consumer-observable dependency expression would be a package-file or link-interface entry, and those stay empty (A2, A3). The merged archive is also what makes consumer links succeed once a gate object is pulled (research R-006).
- **Hidden visibility and zlib renaming (A4/A4z)**: both vendored object sets compile with `-fvisibility=hidden` (research R-009); zlib additionally renames its public symbols via `Z_PREFIX`, so `inflate`/`deflate`/`compress`/`uncompress` do not exist under their bare names, and the hidden `z_`-prefixed members are not promoted to the shared library's `.dynsym`. A consumer process loading a system HdrHistogram_c or zlib alongside speedgun-ng sees no name collision from our hidden, non-exported, renamed copies.
- **Forced-static vendored builds (A5)**: the scoped `BUILD_SHARED_LIBS OFF` plus the per-tree build switches (research R-007) guarantee no `libz.so` or `hdr_histogram.so` exists to become a runtime dependency, in the static and shared builds alike.
- **Host zlib (A9)**: the `find_package(ZLIB)` inside HdrHistogram_c is redirected to the vendored copy by the pre-seeded cache and the parent-scope `ZLIB::ZLIB` handle (research R-008); the audit records the vendored path and proves the host copy is resolved zero times.
- **Include paths**: the vendored include directories are `SYSTEM` usage requirements of the library targets (research R-012); the installed interface carries the project's own header root alone.
- **Out of scope**: runtime behavior of any HdrHistogram_c or zlib call: no speedgun-ng code calls either beyond the gate address-of in this feature (Scope boundaries); the contract covers presence, linkage, and export, and analysis semantics arrive with the future data-analysis feature.
