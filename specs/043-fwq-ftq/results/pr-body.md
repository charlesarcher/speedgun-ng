# Fixed work quanta and fixed time quanta

The branch adds two probe methods to the standing jitter probe. FWQ prices a
fixed body of work. FTQ prices a fixed span of time. Each run calibrates a
work count on its own probe cpu, then keeps a series of stamps. The kernel
removes the window mean, runs a plain DFT, and reports the top five periods.
The console prints 30 series lines for each probe cpu. The JSON record
carries the series at schema `1.12`. The compare table reads 32 jitter
fields. A strict run refuses a noisy cpu and exits 2.

Roadmap item N13c. Spec `specs/043-fwq-ftq/spec.md`.

## The lane

- 89 commits over `master`, at tip `f8ad4f9c`.
- 61 files changed, 15,581 added lines, 219 removed lines.
- Source: 2,135 added lines in 12 files, with two new files for the spectrum.
- Tests: 5,083 added lines in 16 files, with three new executables.
- The suite reads 171 entries and passes at 100%.
- Version `0.29.0`, harness `SOVERSION 20`, archive `SOVERSION 5`, schema
  `1.12`.

## The host

- Host `chilly`, kernel `Linux 7.3.1-1-cachyos x86_64`.
- AMD Ryzen 9 7950X3D, 16 cores, 2 threads per core, 32 cpus, two L3 groups.
- `CONFIG_HZ` 1000, read from `/proc/config.gz`.
- gcc 16.2.1, clang 23.1.4, cmake 4.4.4.

## The acceptance sections

The script `tools/noise/fwq_ftq_acceptance.sh` runs eight sections: two
methods, two L3 groups and two load states. The load is a busy loop pinned to
the SMT sibling thread. Each section names its command line, exit status,
calibration count, measured unit time, noise fraction, `gaps=` value, kept
span in milliseconds, drop count and detected periods.

The clean run reads eight sections at exit status 0, a drop count of 0 in
every section, an FWQ work count of 1136 at rest and 640 under load, and a
unit time of 1,008 ns against the 1,000 ns target. The FTQ sections keep
100,000 samples each.

- [Clean run](specs/043-fwq-ftq/results/acceptance-2026-11-02-rerun.md)
- [First run, the defects as read](specs/043-fwq-ftq/results/acceptance-2026-11-02.md)

The first run found two defects, and the branch fixes both. The FWQ spectrum
bound a span to a temporary vector, so the transform read freed memory and a
long run died on a signal. The calibration divided the body time by the count,
so the search read the target per iteration and kept a count of 1.
AddressSanitizer named the first one in a standing test.

## Local gates

- Full suite: 100% of 171 entries, twice.
- Time-source gate: `time_source_gate: clean`.
- Format gate at the pinned clang-format 18.1.8: clean.
- Prose gate over the lane: 0 findings.
- AddressSanitizer on the window test and the FWQ test: exit 0, no report.
- Coverage: open. The head reads 44 dark lines and 52 short branch positions,
  and the rows that reach them land under T070.

## Open work

Phase 10 runs the hard gates at the head: the release build and the full
suite, the suite under a two-cpu mask, coverage, sanitizers, thread sanitizer,
clang-tidy, cppcheck, the time-source gate, the docs gates, the shape folds,
prose, format, spell and the pre-push script. Phase 11 reads the CI run, names
the run reference beside the acceptance evidence, and re-checks the landed
tree. The CI run reference stands recorded in
`specs/043-fwq-ftq/results/ci-run.md` when that run lands.

## Artifacts

- [spec.md](specs/043-fwq-ftq/spec.md),
  [plan.md](specs/043-fwq-ftq/plan.md),
  [tasks.md](specs/043-fwq-ftq/tasks.md),
  [quickstart.md](specs/043-fwq-ftq/quickstart.md)
- Contracts: `specs/027-jitter-probe/contracts/cli-options.md`,
  `public-api.md`, `report-json.md`, `rule-names.md`,
  `specs/034-host-noise-precheck/contracts/console-report.md`,
  `report-shape.md`
