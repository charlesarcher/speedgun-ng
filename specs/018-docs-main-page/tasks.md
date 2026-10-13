# Tasks: Docs Main Page (018-docs-main-page)

**Input**: Design documents from `specs/018-docs-main-page/` (plan.md, spec.md, research.md, data-model.md, contracts/, quickstart.md)

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/example-drift.md, contracts/main-page-structure.md, contracts/report-shape.md, quickstart.md

**Tests**: Included and mandatory. Constitution Principle III (R-DCUT) and the plan's test plan pin fixture-first order: fixture runners land red before their tools go green.

**Organization**: Tasks are grouped by user story. The rewritten `README.md` is one atomic artifact (a partial page fails the section-coverage and forbidden-token scans), so the page lands in the User Story 1 phase as the MVP, and later story phases deliver or verify what they own: US2 the first-screen devices, US3 the snippet-compile gate content, US4 the build page, US5 the governance and CI registration. This mapping mirrors `.omo/plans/018-docs-main-page.md` (plan todo N is noted per task); do not re-split todo 11.

**Path conventions**: repository-relative paths throughout, matching plan.md.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1..US5, matching spec.md user stories
- Every task names its exact file path(s)

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: One risk spike before any marker ships.

- [X] T001 Spike the fence-info marker in `docs/pages/`: add one scratch page carrying one fence with info string `text sg-example=x.md#y` and one HTML-comment pre-marker, build docs with `cmake "-DPROJECT_SOURCE_DIR=$PWD" "-DPROJECT_BINARY_DIR=$PWD/build" -P cmake/docs-ci.cmake`, grep the rendered html for visible `sg-example` text (research D-01), then delete the scratch page. Must NOT touch the real `README.md`. Evidence: `.omo/evidence/task-1-018-docs-main-page.log`

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: The text-truth regions both gates compare against, the red fixture runners, and both gate tools.

**⚠️ CRITICAL**: No user story work can begin until this phase is complete.

- [X] T002 [P] Mark regions in `example/benchmark_example.cpp` with `// sg-example:begin <name>` / `// sg-example:end <name>` comment pairs; every region wraps a COMPLETE top-level declaration (whole function, whole class, `main`), never a bare body fragment (contract example-drift R-10, FR-011); regions: `bmSum`, the suite/case named registration family, `bmArgs`, the template benchmark, the `TableFixture` class, the timing-mode functions (`bmProcessCpu`, `bmRealTime`, `bmManual`, `bmPaused`), the chained-registration + `addMetric` block, the `DISABLED_` registration, `main` with `speedgunMain`; names per the FR-009 section list; edits are comments only (FR-036). Evidence: `.omo/evidence/task-3-018-docs-main-page.log` (plan todo 3)
- [X] T003 [P] Create `example/first_benchmark.cpp` (complete program: include list from `benchmark.hpp`, one sum benchmark using `doNotOptimize`, one `SG_BENCHMARK` registration, `main` returning `sg::speedgunMain(argc, argv)`, one region wrapping the whole program as `program`), wire it in `example/CMakeLists.txt` with `add_example(first_benchmark)` and `target_link_libraries(first_benchmark PRIVATE speedgun-ng::harness)` beside the existing `benchmark_example` pair, and create `example/quickstart.sh` with `#`-comment `sg-example` regions named `configure`, `build`, `install`, `consumer-configure`, `consumer-build`, `run` plus `example/quickstart/CMakeLists.txt` with the `package` region (`find_package`, `add_executable`, `target_link_libraries`). `bash -n example/quickstart.sh` exits 0. Evidence: `.omo/evidence/task-4-018-docs-main-page.log` (plan todo 4)
- [X] T004 Create `example/docs_snippets.cpp` with one marked region per FR-009 capability absent from `example/benchmark_example.cpp`: capture macro, template fixture macro family, ranges family (`range`, `ranges`, `rangeMultiplier`, `denseRange`, `argsProduct`, `argName`, `argNames`), `setUp`/`tearDown` + `setup`/`teardown` callbacks, run control chain (`minTime`, `iterations`, `repetitions`, `warmupTime`), `clobberMemory` in a full benchmark function, `skipWithError`, `skipWithMessage`, `setDefaultTimeUnit`; every region wraps a complete top-level declaration (R-10); demonstrative benchmarks registered with the `DISABLED_` prefix; wire via `add_example` + harness link in `example/CMakeLists.txt`. Must NOT create run output the aggregate suite depends on. Evidence: `.omo/evidence/task-5-018-docs-main-page.log` (plan todo 5)
- [X] T005 [P] Write BOTH red fixture runners: `test/example-drift-fixture/run_example_drift_fixtures.py` building scratch trees and asserting every scan case of contracts/example-drift.md R-8 (forbidden-token, prose-surface, section-coverage, shell-fence-skip, documentation-link cases included), and `test/example-drift-fixture/run_report_shape_fixtures.py` carrying the four mask golden pairs and the end-to-end cases of contracts/report-shape.md S-5 (scratch page plus fake-binary script with planted rename, missing-row, extra-aggregate failures), importing mask and comparator from `tools/docs/report_shape.py` (import error is its red state). Runners exit non-zero until T006/T007 land. Evidence: `.omo/evidence/task-6-018-docs-main-page.log` (plan todo 6)
- [X] T006 Implement `tools/docs/example_drift.py` per contracts/example-drift.md R-1..R-7 plus the `--emit-blocks` mode of R-10: block extraction, region balance in `//` and `#` comment syntax, byte-exact compare, four-part failure naming (block, file, region, lines), exit codes 0/1/2, `--names`, `--options`, `--forbidden`, `--sections`, cpp-fence filtering in `--emit-blocks`; standard library only; must NOT read any build directory. Green when T005 exits 0 and `python3 tools/docs/example_drift.py .` exits 0. Evidence: `.omo/evidence/task-8-018-docs-main-page.log` (plan todo 8)
- [X] T007 Implement `tools/docs/report_shape.py` per contracts/report-shape.md S-2/S-3: run the binary argument, capture stdout, fold with the pinned mask, compare masked line sequences to the one `sg-sample-output` block, four-part failure naming, exit codes 0/1/2; CLI `python3 tools/docs/report_shape.py --binary <path> --readme README.md`. Green when the T005 golden pairs and planted-failure cases exit 0. Evidence: `.omo/evidence/task-9-018-docs-main-page.log` (plan todo 9)

**Checkpoint**: Region files compile, both runners green, both tools green standalone. User stories can now proceed independently.

---

## Phase 3: User Story 1 - Write and run a first benchmark from the page (Priority: P1) 🎯 MVP

**Goal**: The reader configures, builds, installs, builds the first program from the page's link line, runs it, and sees a report row; the page's sample output matches a real run in shape.

**Independent Test** (spec US1): On a clean Linux checkout follow the quick start verbatim; success is a report row on stdout with exit 0, reading no file outside the page except the example program the page names. Validate with quickstart.md scenarios 1 and 9.

### Implementation for User Story 1

- [X] T008 [US1] Rewrite `README.md` end to end per contracts/main-page-structure.md: opening + six-fact list; quick start (platform, C++23, CMake >= 3.20, configure/build/install commands, `find_package` block, two-target link line naming both targets a benchmark needs, the whole program block from `example/first_benchmark.cpp#program`, build/run commands); run the built binary once and store its stdout as the `report` region of `example/first_benchmark.expected`, paste those exact bytes as the page's sample block labeled as one host's run (main-page-structure R-2 rule 2, report-shape S-1); twelve H3 feature sections per the FR-009 list each with purpose sentence + marked block (complete declarations, example-drift R-10) + result sentence; quick reference rows per main-page-structure C-C plus the nine C-D option rows; maintainer link near the page end to the build page. Hard caps (D-14, SC-011): `[ "$(wc -l < README.md)" -le 400 ]` and no section over 25 lines counted from the H3 heading. Its region blocks are its text truth; it is their only writer (research D-12). Acceptance: `python3 tools/docs/example_drift.py .` exits 0; `python3 tools/docs/report_shape.py --binary build/example/first_benchmark --readme README.md` exits 0; `cmake -P cmake/prose-lint.cmake` exits 0. Evidence: `.omo/evidence/task-11-018-docs-main-page.log` (plan todo 11)

**Checkpoint**: A reader following only `README.md` builds and runs a benchmark; US1 is the shippable MVP.

---

## Phase 4: User Story 2 - Learn what the library does in one screen (Priority: P2)

**Goal**: First screen answers what the library is, where it runs, where numbers come from; measured on the spec's two devices.

**Independent Test** (spec US2): Fresh-model pass plus three-reader pass on the first screen alone; success is all three answers correct on both devices. Validate with quickstart.md scenario 2.

### Implementation for User Story 2

- [X] T009 [US2] Run the first-screen devices, recording all results in `.omo/evidence/task-12-018-docs-main-page.log`: extract the first screen of `README.md` (everything before the second H2), run the fresh-model pass (one completion, no other repository access, the three quickstart scenario 2 questions), then three independent reader passes with the same three questions; record all four answer sets, the cap check (400 total, 25 per section), and the six-fact first-screen precondition (main-page-structure C-A rule 9) in a PR body text file. An answer missing C++23 / Linux / counters-source sends T008 back for revision. Must NOT add a CI gate for these checks. Evidence: `.omo/evidence/task-12-018-docs-main-page.log` (plan todo 12)

**Checkpoint**: Both devices answer all three questions; the revision loop is the check and CI stays out of it.

---

## Phase 5: User Story 3 - Find one feature and copy a snippet that builds (Priority: P2)

**Goal**: Every FR-009 capability owns a section; every shipped snippet compiles; each section's block equals its region.

**Independent Test** (spec US3): Compile every section snippet in CI (100%), and the drift gate fails any drifted block. Validate with quickstart.md scenarios 3 and 4.

### Verification for User Story 3 (implementation shipped in T002/T004/T008)

- [X] T010 [US3] Run the section-coverage audit against `README.md`: `python3 tools/docs/example_drift.py . --sections` names every FR-009 capability with a missing H3 section as a failure, and each quick-reference row links to its section (grep loop over the C-C row set, all hit). Evidence: `.omo/evidence/task-13a-018-docs-main-page.log`
- [X] T011 [US3] Run the snippet compile proof per contracts/example-drift.md R-10 against `README.md`: `python3 tools/docs/example_drift.py . --emit-blocks <dir>` (cpp fences only), then compile each emitted snippet over the canonical quick-start includes; 100% compile. Evidence: `.omo/evidence/task-13b-018-docs-main-page.log`

**Checkpoint**: Feature audit green; the CI-enforced versions land with T015.

---

## Phase 6: User Story 4 - Recover the maintainer facts on the build page (Priority: P2)

**Goal**: One link from the main page reaches a build page carrying every fact of the nine dropped sections.

**Independent Test** (spec US4): Walk the plan.md section map; each dropped section's commands, tool names, tripwire descriptions, version-pin mechanics present on the new page. Validate with quickstart.md scenario 7.

### Implementation for User Story 4

- [X] T012 [US4] Create `docs/pages/building.md` holding all nine dropped sections per the plan.md section map (one `## Contract checks`, one `## Dependency re-pinning` with seven H3s, one `## Quality gates`), copying commands and facts from the current pre-rewrite `README.md` (record `git rev-parse HEAD` in the evidence log as the snapshot); edit `docs/conf.py.in` `LINKS_NAVBAR1` to the user group (about, building, harness) before the internals group; retitle `docs/pages/harness.md` H1 per plan todo 2. Acceptance: per-map-row grep loops hit (`update_pmu_events.py --check`, `ZLIB_VERNUM`, `quill_dependency_check`, `perl-gd`), `cmake -P cmake/docs-ci.cmake` exits 0, `python3 tools/prose/prose_gate.py --check all` exits 0. Evidence: `.omo/evidence/task-2-018-docs-main-page.log` (plan todo 2)

**Checkpoint**: Maintainer facts walk complete; main page carries only the link.

---

## Phase 7: User Story 5 - Docs ship with the feature (Priority: P3)

**Goal**: A stale block cannot merge: constitution rule IV.1 plus registered CI gates fail drift with a named region.

**Independent Test** (spec US5): Edit one character inside a marked block, run the suite, expect failure naming file/region/lines; delete a marker, expect the unmarked-block failure too. Validate with quickstart.md scenarios 5, 6, 8.

### Implementation for User Story 5

- [X] T013 [P] [US5] Amend `.specify/memory/constitution.md` to 2.19.0 per research D-08: sub-rule IV.1 carrying the FR-027 text, a Principle VIII hard-gate entry for the example compile gate, lineage row 2.19.0, footer version and ISO-date, the Sync Impact Report comment in the file's existing form; add the main-page checklist line to `.specify/templates/spec-template.md` and `.specify/templates/tasks-template.md` exactly as D-08 pins it. Must NOT renumber existing principles; must NOT touch project version fields. Evidence: `.omo/evidence/task-7-018-docs-main-page.log` (plan todo 7)
- [X] T014 [US5] Register the gates in `test/CMakeLists.txt`: `example_drift`, `example_drift_fixtures`, `report_shape_fixtures`, `snippet_compile` (plain `python3`/`bash` idiom, unconditional) and `report_shape` inside `if(TARGET first_benchmark)` with `TIMEOUT 120`; write `test/snippet-compile/run.sh` per R-10 (emit cpp-fence blocks, first fail the planted broken probe, then syntax-compile each real snippet with the configured compiler); lands only after T008 so no intermediate commit carries a failing gate. Acceptance: `ctest --test-dir build -N` lists all five names; `ctest --test-dir build -R "example_drift|report_shape|snippet_compile" --output-on-failure` exits 0. Evidence: `.omo/evidence/task-10-018-docs-main-page.log` (plan todo 10)
- [X] T015 [US5] Add one step `python3 tools/docs/example_drift.py .` after checkout in the lint job of `.github/workflows/ci.yml`. Must NOT add a job; must NOT touch other jobs. Evidence: `.omo/evidence/task-10b-018-docs-main-page.log` (plan todo 10)

**Checkpoint**: Drift, shape, snippet, and fixture gates run in CI; US5 depends on T008 completion by plan order.

---

## Phase 8: Polish & Cross-Cutting Concerns

- [X] T016 [P] Full-suite parity in `build/`: `cmake --preset=ci-ubuntu`, full build, `ctest --test-dir build --output-on-failure` with the five new tests green; clean-room consumer proof per quickstart scenario 9 (install to a scratch prefix, out-of-tree directory, paste the page's `package` region and first program via `example_drift.py --emit-blocks`, configure with `-DCMAKE_PREFIX_PATH`, build, run, report row, exit 0); docs build `cmake -P cmake/docs-ci.cmake` with index order verified (building under user pages before internals). Evidence: `.omo/evidence/task-13-018-docs-main-page.log` (plan todo 13)
- [X] T017 [P] Plan compliance audit (plan F1): check every Must have / Must NOT have line of `.omo/plans/018-docs-main-page.md` against the working tree; verdict in `.omo/evidence/f1-018-docs-main-page.log`
- [X] T018 [P] Code quality review (plan F2): clang-format clean on `example/first_benchmark.cpp` and `example/docs_snippets.cpp`; verdict in `.omo/evidence/f2-018-docs-main-page.log`
- [X] T019 [P] Real-surface QA (plan F3, agent-executed): run quickstart.md scenarios 1, 3, 4, 5, 6, 7, 9 end to end; verdict per scenario in `.omo/evidence/f3-018-docs-main-page.log`
- [X] T020 [P] Ideal-state fidelity (plan F4): check IS-1..IS-9 rows 1:1 against the delivered tree; verdict in `.omo/evidence/f4-018-docs-main-page.log`

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies.
- **Foundational (Phase 2)**: After Phase 1; BLOCKS all user stories. Within it: T006/T007 after T005 (runners define their red state); T002/T003/T004 mutually independent except T003 and T004 both edit `example/CMakeLists.txt` (serialize those two).
- **User Stories**: All start after Foundational.
  - US1 (T008) after T002-T007 (blocks its acceptance commands).
  - US2 (T009) after T008. US3 (T010, T011) after T006 + T008. US4 (T012) independent of T008 (plan order W1).
  - US5: T013 independent from T008; T014/T015 MUST follow T008 (no red intermediate commit) and T006/T007.
- **Polish (Phase 8)**: After all desired stories.

### Within Each User Story

- Fixture runners precede their tools (T005 before T006/T007); text truth precedes the page (T002-T004 before T008); the page precedes gate registration (T008 before T014/T015).

### Parallel Opportunities

- T002, T003+T004 (serialized with each other), T005: three independent tracks in Phase 2.
- T006 and T007: different files, both after T005.
- US2, US3, US4, and T013 proceed in parallel once T008 lands.
- T016-T020: full parallel final wave.

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Phase 1 spike, then Phase 2 foundation (regions, red runners, green tools).
2. T008 rewrites `README.md`; validate US1 via quickstart scenarios 1 and 9.
3. **STOP and VALIDATE**: a clean-checkout reader builds and runs the first benchmark from the page alone.
4. Ship: the page is correct at this point even before CI registration.

### Incremental Delivery

1. MVP lands → add US4 build page (content already dropped by the rewrite; restore it first if shipping separately).
2. Add US2 devices, US3 audits → page quality evidence in the PR body.
3. Add US5 constitution amendment + gate registration → staleness becomes impossible to merge.
4. Final wave T016-T020, then the plan's commit strategy applies (one commit per `Commit: Y` task, `<Section>: <one-line imperative>` form).

### Parallel Team Strategy

1. One developer runs Phase 1-2 (T002/T003+T004/T005 tracks, then T006/T007).
2. T008 is a single-writer task (the page has one author per pass).
3. After T008: US2 devices, US3 audits, US4 page, US5 amendment in parallel; T014/T015 by whoever holds the gates.

---

## Plan Traceability

| tasks.md | plan todo | | tasks.md | plan todo |
| --- | --- | --- | --- | --- |
| T001 | 1 | | T010 | 13 (audit part) |
| T002 | 3 | | T011 | 13 (emit part) |
| T003 | 4 | | T012 | 2 |
| T004 | 5 | | T013 | 7 |
| T005 | 6 | | T014 | 10 |
| T006 | 8 | | T015 | 10 (CI step) |
| T007 | 9 | | T016 | 13 |
| T008 | 11 | | T017-T020 | F1-F4 |
| T009 | 12 | | | |

## Notes

- `[P]` tasks touch different files and have no open dependencies.
- US5 registration (T014/T015) is the one deliberate cross-phase ordering: plan pins it after the page rewrite.
- Evidence logs go to `.omo/evidence/`; commits follow the plan's commit strategy (planner role here makes no commits).

## Phase 9: Convergence

- [X] T021 Teach capture, template registration, and the suite/case name split in the Registration and Fixtures sections their quick-reference rows link to (prose sentences, or new marked regions in `example/docs_snippets.cpp` if a snippet is needed), inside the 25-line section cap; keep `example_drift`, `snippet_compile`, and the prose gate green per FR-009, FR-018, spec PC-4 (partial). Prose form: the 25-line cap and FR-010's one snippet per section leave no room for a new block, and the macro forms sit in the compiled regions `registration` and `fixture`. Evidence: `.omo/evidence/task-21-018-docs-main-page.log`
- T022 removed: a human first-screen study is out of scope for this branch;
  SC-001 rests on the structure checks. The SC-001 second device, three
  readers new to the repository, asked the
  scenario-2 questions of the first screen alone - is a pull-request review
  step: an open box for it would report the feature
  as unfinished on work no agent can do, and a checked box would claim an
  answer set that does not exist. The packet (the first screen alone) is
  `.omo/evidence/sg018-first-screen.md`, the procedure and pass rule are
  `.omo/evidence/task-22-018-docs-main-page.log`, and the three answer sets
  belong in the pull-request body per SC-001.
- [X] T023 Bring the opening part to the pinned D-10 shape: two or three sentences plus the six FR-002 facts as the list inside "What speedgun does" (the facts may leave the title-block intro once the list holds them), then rerun the fresh-model device; or record the accepted first-screen reading in the PR body and leave the page as shipped per plan: research D-10, contract main-page-structure C-A rule 9 (partial). Reshape branch taken; both model devices pass on the new screen. Evidence: `.omo/evidence/task-23-018-docs-main-page.log`
- [X] T024 Justify the `compilability` ignore-word addition to `.codespellrc` in the PR body, or drop the addition if the spell gate stays green without it per plan: Project Structure touch-point list (unrequested). Keep branch: the gate fails with three hits and exit 65 once the entry is removed, proof and PR-body text on record. Evidence: `.omo/evidence/task-24-018-docs-main-page.log`
