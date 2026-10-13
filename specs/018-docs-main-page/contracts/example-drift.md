# Contract: Example Drift Gate

Normative behavior of the gate that keeps prose code blocks equal to the
example regions they quote. FR-031, FR-033, FR-035, SC-004, SC-009.
The gate is one python tool with a fixture harness, following the
`tools/prose/prose_gate.py` plus `test/prose-gate-fixture/` split.

## R-1: Region markers in example files

Line comments, one per line, at any indentation, in the file's own
comment syntax (`//` in C++, `#` in shell and CMake files), one token
spelling everywhere:

```text
// sg-example:begin <region>
// sg-example:end <region>
```

1. `<region>` matches `[a-z0-9][a-z0-9-]*` and is unique within the file.
2. Markers balance: each begin has a later end with the same name; a
   duplicate begin, an end without a begin, or a nesting violation is a
   gate error naming the file and marker line.
3. Regions may repeat and sit in any order; they may not nest.
4. The region content is every line strictly between the two marker lines.
5. Marker lines never appear on a page block; they are tooling comments.
   They hold no identifier, so the naming rules bind nothing there.
6. Region files: C++ regions live in compiled example sources
   (`first_benchmark.cpp`, `docs_snippets.cpp`, `benchmark_example.cpp`);
   quick-start command regions live in `example/quickstart.sh`; the
   `find_package` and link-line region lives in
   `example/quickstart/CMakeLists.txt`; the sample output region
   (`report`) lives in `example/first_benchmark.expected`, captured
   once from the real binary and re-captured only by the shape gate's
   todo-11 procedure (FR-031 holds every main-page
   block to a region; FR-032's build obligation binds example targets
   only, so the shell, CMake, and expected-output regions carry text
   truth, while the shape rule S-2 holds the region equal to the live
   run in shape).

## R-2: Page-block fence info

A marked block opens its fence with an info string whose first token is
the display language and whose remaining tokens carry the source:

````text
```cpp sg-example=first_benchmark.cpp#run-loop
```text sg-sample-output=first_benchmark.expected#report
````

1. `sg-example=<file>#<region>` names one file under `example/`
   (directly, or in a subdirectory such as `example/quickstart/`)
   and one region in it (FR-031).
2. `sg-sample-output=<file>#<region>` marks the one sample output block.
   The reference follows the same grammar as rule 1: the block is held
   byte-equal to its region (a region of
   `example/first_benchmark.expected`, captured once from the real
   binary), so FR-031's region obligation binds it too; the live shape
   comparison runs under `report-shape.md` S-2, and the block is checked
   for uniqueness on the main page by this gate.
3. A block with no `sg-` token is unmarked.
4. GitHub uses the first info token for highlighting and renders the rest
   invisibly. One early task renders both forms through the `docs` target
   and the GitHub preview and stops if a renderer shows the token text;
   the fallback is an HTML-comment pre-marker line.

## R-3: Scan scope

The gate scans, as prose pages in scope, the same set
`tools/prose/prose_gate.py` treats as prose: the root `README*`, the root
`AGENTS.md`, and every file under `docs/` with a `.md` suffix.

1. Marking obligation: every fenced code block on the main page carries
   `sg-example` or `sg-sample-output` (FR-031, FR-033).
2. A marked block on any other prose page is compared under the same
   rules (clarify session 2026-10-13).
3. An unmarked block on a prose page other than the main page is left
   alone (FR-033).

## R-4: Comparison

1. The block body (fence lines excluded) must equal the region lines
   exactly, line for line: same order, same characters, same trailing
   whitespace state, same line count. The gate trims nothing.
2. A one-character difference fails (US5).
3. A region that moved inside its file with identical text passes.

## R-5: Failure output and exit codes

1. One summary line per failure naming, in order: the prose file, the
   block (its region name or fence line number), the example file, the
   region, and each differing line pair with line numbers on both sides.
2. A main-page block without a region marker fails with the file and the
   fence line.
3. Exit 0 clean, exit 1 with any finding, exit 2 on a usage error, the
   prose gate convention.

## R-6: Build independence

The tool reads repository sources only and takes the repo root as its one
argument. It never opens a build directory. It is registered
unconditionally in `test/CMakeLists.txt`:

```cmake
add_test(NAME example_drift
         COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/../tools/docs/example_drift.py
                 ${CMAKE_SOURCE_DIR})
```

The plain `python3` command line follows the established script-test
idiom (`dbc_dependency_scan`, `hwloc_purity_scan`, the pmu-events
fixtures); the tree resolves no `Python3_EXECUTABLE` variable.

so it runs with `BUILD_EXAMPLES` on or off (FR-035). The `lint` job,
which configures nothing, carries one direct step running the same
command as the build-independent proof.

## R-7: Name, option, forbidden, and section scans

The same tool carries four checks used by the quickstart and by
`main-page-structure.md`:

1. `--names`: every API name in the quick-reference API table appears in
   `include/speedgun-ng/benchmark.hpp` or `include/speedgun-ng/barrier.hpp`.
2. `--options`: every option row appears in the long-option table of
   `source/harness/cli.cpp`, and every long option there has a row.
3. `--forbidden`: the main page carries zero tokens of the classes in
   `main-page-structure.md` C-B (FR-038, SC-005), over prose text only
   (fence bodies and inline code exempt, the `prose_gate.py` mechanism).
4. `--sections`: each of the twelve FR-009 section titles (the fixed
   list compiled into the tool from the spec at the audit point) appears
   as an H3 of "How you do it", and each such section holds at least one
   marked block. A missing section fails by name (SC-003's coverage
   half; `snippet_compile` R-10 carries the compile half).

All four scans run inside the default `example_drift` test invocation so
one ctest test owns page freshness end to end.

## R-8: Fixture harnesses

Two runners, one per gate, so each gate's fixtures gate only their own
tool. Both build a scratch tree (a copied mini page plus mini example
files; the real `README.md` is never modified) and assert one verdict
per case.

`test/example-drift-fixture/run_example_drift_fixtures.py` covers the
scan cases:

| Case | Expected |
| --- | --- |
| planted one-character drift in a marked block | fail, names file, region, line |
| main-page block with no `sg-` token | fail, names the block |
| detail-page marked block with drift | fail |
| detail-page unmarked block | pass, ignored |
| region moved inside its file, text equal | pass |
| unbalanced region markers | fail, names the marker |
| `--names` row absent from the headers | fail |
| `--options` table missing one row | fail |
| planted forbidden token (a commit-hash-shaped string) on the mini page | fail |
| planted literal header path (`include/speedgun-ng/x.hpp`) in mini-page prose, and the same spelling inside a fenced block | prose hit fails; the fenced copy passes (scan surface is prose only) |
| mini page missing one required H3 section | fail, names the section |
| `--emit-blocks` on a mini page holding a shell fence and a cpp fence | shell fence skipped, cpp fence emitted (R-10 rule 2) |
| relative documentation link (`docs/pages/building.md`) on the mini page | pass (C-G link is legal) |
| clean scratch tree | pass (liveness: every fail case above proves the scan inspects something, per the planted-probe discipline of `test/time_source_gate.sh`) |

Registration (the report-shape runner registers in `report-shape.md`
S-5):

```cmake
add_test(NAME example_drift_fixtures
         COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/example-drift-fixture/run_example_drift_fixtures.py
                 ${CMAKE_SOURCE_DIR})
```

Both runners print the TDD-red line naming the missing gate when their
tool is absent, the `test/prose-gate-fixture/` precedent.

## R-9: Coverage posture

The gate is tooling under `tools/docs/`, checked by its fixture harness
through CTest, the posture `tools/prose/prose_gate.py` holds: the 100%
gcov gates measure application code, and the fixture suite is the tool's
covering test set, deterministic and fast enough for every job
(Principle VI).

## R-10: Snippet compile check (FR-011, SC-003)

The spec's independent test compiles each page snippet with the includes
the quick start shows. Three rules carry it:

1. Region granularity: every C++ region wraps a complete top-level
declaration (a function, a class, `main`) or the one complete program
(the `first_benchmark.cpp#program` region: includes, registration,
`main`), so its page block compiles pasted under the canonical include
prelude and nothing else. Fragments
(bare loop bodies) are not regions; the enclosing declaration is.
2. `example_drift.py --emit-blocks <dir>` writes every main-page
   `sg-example` block whose fence language is `cpp` to
   `<dir>/NN_<region>.snippet`; shell and cmake fences are skipped by
   rule (a command fence is no translation unit). Registered test
   `snippet_compile` emits to a scratch directory and compiles each
   snippet as `$CXX -std=c++23 -fsyntax-only -I
   ${CMAKE_SOURCE_DIR}/include` over a prelude of the canonical include.
   A snippet that is neither a complete declaration nor the complete
program region fails there.
3. Liveness: `snippet_compile` first compiles one planted broken probe
   snippet (a declaration calling a nonexistent name) inside the scratch
   directory and must fail that probe; the run passes only when the
   probe fails and every real snippet passes. The emitted snippet count
   MUST equal the count of `sg-example` fences whose fence language is
   `cpp` on the main page (shell and cmake fences stay outside this
   count by rule 2; a zero-block run on the shipped page is then
   impossible; a mismatch
   fails, naming the `cpp` fences without a snippet). Skipping shell and
   cmake
   fences is pinned by the fixture case in R-8.

```cmake
add_test(NAME snippet_compile
         COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/snippet-compile/run.sh
                 ${CMAKE_SOURCE_DIR} ${CMAKE_CXX_COMPILER})
```

The test is unconditional: source text, public headers, and a compiler,
none of which depends on `BUILD_EXAMPLES`. The `report_shape` guard of
`report-shape.md` S-4 is the only build-conditional registration.
