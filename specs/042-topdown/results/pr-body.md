# Vendor-neutral top-down analysis

The branch reads the top-down tree of a running benchmark from the metric tables
the host already carries. `--topdown`, with a level from 1 to 6 and a default of
1, asks for a level. The console prints one block per benchmark, the written
record carries one `topdown` object per entry at schema `1.13`, and every node
the host cannot answer carries a refusal with a reason in place of a figure. A
host that holds no source for the tree answers the refusal and nothing else. The
reader, the writer and the checker of a written document all read the same schema
file.

Roadmap item N10. Spec `specs/042-topdown/spec.md`.

## The lane

- 82 commits over `master`, at tip `eda6f2d6`.
- 81 files changed, 17,447 added lines, 211 removed lines.
- Source: 2,829 added lines in 27 files, with `counters_topdown.hpp` new at the
  public surface and `topdown_select.cpp` beside the metric tables.
- Tests: 9,315 added lines in 27 files, with seven new executables and one new
  fixture header.
- Two new examples: `example/docs/topdown.cpp` prints the block, and
  `example/memory_walk.cpp` chases a 32 MiB dependent chain so a memory-bound
  figure reaches the block.
- Version `0.30.0`, schema `1.13`, harness `SOVERSION 20`, counters archive
  `SOVERSION 5`. specs/043-fwq-ftq took `0.29.0` and `1.12` while this branch
  stood open, and the two archive numbers stand where specs/043 and
  specs/044-fix-rocky-pmu-stack put them, so no number moves twice in one
  unreleased window.
- The DBC documentation gate and the pairing gate read 345 interfaces and report
  no gap.

## The host

- Host `CorsairOne`, kernel `Linux 7.3.0-4-cachyos x86_64`.
- Intel Core i9-9920X, model 85, 12 cores, 2 threads per core, 24 cpus.
- Four general counters and three fixed counters, read from CPUID leaf 0xA.
- `kernel.perf_event_paranoid` at -1, set by the owner for this work.
- gcc 16.2.1, clang 23.1.3, cmake 4.4.2.

## The acceptance record

`specs/042-topdown/quickstart.md` carries the gate order end to end with the
figures one run on this host printed: the level-1 shares of both examples, the
twelve refusals a level-1 run carries here, the closed-refusal section, the tier
audit line, and the troubleshooting rows. `specs/042-topdown/tasks.md` carries
the measured evidence per task on its tick line. PC-15 in the spec's prior-condition
register records the host fact behind the refusals: the spine of a level-1 share
needs five general events and this part schedules four, so one event stands
unscheduled and the share answers `not available`. On a part with eight general
counters all four shares publish.

## What this host cannot close

- The coverage gate cannot start here: `lcov` dies on a perl XS version mismatch
  between `~/.local/lib/perl5` and `/usr/bin/perl`, so T097 stands open and the
  coverage ratio comes from the `coverage` job.
- Three suite entries fail here and fail the same way on a fresh `origin/master`
  worktree: `counters_kernel_calls_test` (two checks that ask a real
  `perf_event_open` to refuse), `counters_pmu_provider_test` (one check that
  compares a description with the kernel's text), and
  `harness_jitter_window_test` (`theSeriesRoundTripsThroughLoadReport`). specs/043
  and specs/044 added all three, and none of them reads top-down code.
- `prose_gate_fixtures` bounds its own run at 10 seconds. It passes alone in
  16.74 seconds here and fails when the suite runs the entries in parallel.
- The `lint` job gates every other job and stands red on `origin/master`: its
  spell step answers six findings, three on the British spelling of the
  mean-removed local in `source/harness/spectrum.cpp` and three on split
  literals inside `test/source/harness_jitter_bytes_test.cpp`, all of them from
  specs/043. This branch carries the commit that clears them, because no
  coverage, sanitizer or suite verdict is readable behind a skipped `lint`.
