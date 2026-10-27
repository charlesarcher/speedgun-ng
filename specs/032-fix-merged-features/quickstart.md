# Quickstart: Fix Merged Features

**Branch**: `032-fix-merged-features`

The validation guide for the branch: the daily loop, the gates, the
behavior checks the fixes own, and the coverage proof. Implementation
detail lives in `plan.md` and `tasks.md`; the contracts live in
`contracts/`.

## Prerequisites

- Linux, the repo checked out on the branch, submodules present.
- No `CMakeUserPresets.json` needed: the presets `ci-rocky` and
  `ci-ubuntu` drive everything.
- Before any fix work starts: the full CI run `40438223916` of the
  base head is read (FR-064); `gh` needs a token supplied in the
  session first (PC-11).

## The daily loop

```bash
cmake --preset=ci-rocky
cmake --build build
ctest --test-dir build --output-on-failure
```

Once per feature, the analyzer build:

```bash
cmake --preset=ci-ubuntu
cmake --build build
```

## Behavior checks

Each check proves one fix on the head, through the interface the
contract names.

1. **Probe precedence** (`contracts/cli-options.md`): run the batch
   three ways that differ only in the source of the probe value (the
   command line, an option file, a family call). The same probe
   placement comes out each time, and a family's own probe wins over
   both other sources (User Story 2).
2. **Duplicate cpus**: `--jitter-probe=0,0` and `--jitter-probe=0-1,1`
   each print one line quoting the value and exit 2 (D-04).
3. **Ambient refusal**: set `SPEEDGUN_METRIC`, run `--hunt`. One line
   names the source, exit 2 (User Story 3). Repeat for
   `SPEEDGUN_COUNTER` and `SPEEDGUN_PLACEMENT`.
4. **Hunt with the probe**: `--hunt` with `--jitter-probe` completes
   and the report carries both records (D-08).
5. **Compare quoting**: compare a baseline whose name carries a quote
   character. The name cell and the rule-text cell print quoted once;
   the reason cell is unchanged (User Story 4).
6. **Schema**: a hand-edited report document with a duplicate
   top-level key fails the schema test (FR-017); the one `x-order`
   key names the writer order.
7. **Samples**: the two hunt samples
   (`docs/pages/harness.md:1509`, `example/docs/hunt_rank.expected:2`)
   name the current build version after the re-record (User Story 5).

## Host shape

The suite passes under `taskset -c 0,1` and on a real 2-cpu host
(SC-004). The affinity-cpu scenario runs under `taskset -c 0,1`,
`taskset -c 0-3` and the full mask (FR-009). No test reads a wall
clock, sleeps or makes a cpu up (Constraints).

## Coverage proof

The coverage gate reports 100.0 percent lines and holds the branch
shortfall to the accepted sides of D-13 and the keep-set of D-12
(SC-002):

```bash
cmake --preset=ci-coverage
cmake --build build-coverage
ctest --test-dir build-coverage --output-on-failure
```

After FR-021 lands, the pull-request comment lists each keep-set
pattern and allowance row that no longer matches code, with the trace
and the zero list as evidence, and asks before any removal (FR-026).
The exception-cleanup sides D-13 accepts carry their `gcov` block and
arc data in the spec, and the compiler the coverage job uses is named
there (FR-031).

## Gates and process

- Every commit carries the footer of FR-053, holds the 50-character
  subject and the 72-column body (FR-054), and passes `prose-lint`
  (FR-063).
- Every figure in the records comes from the CI run of the last pushed
  commit, and every job log is read (FR-061).
- The branch rebases only and pushes once with `--force-with-lease`,
  proven with `git ls-remote` (FR-060).
- The one pull-request comment lists each item with its commit or its
  question, the coverage numbers, and the CI run (FR-065).

## Done

Every CI job on the pull-request head passes (SC-001), coverage reads
100 percent for lines and for branches with only the accepted sides
(SC-002), and each item of the request is closed with a commit and a
test, closed with the file:line that shows master already holds the
fix, or carried in the pull-request comment as a question (SC-003).
