# Evidence check: specs/044-fix-rocky-pmu-stack

The spec's factual claims against the code on the base branch. Every line
number is the file's own numbering on the current checkout.

## Findings

- `source/counters/detail/pmu.hpp:337-361`: Claim: `FastContext` has a
  double-release copy-assignment defect. Verdict: confirmed. The class
  declares a destructor and no copy or move operations, so the implicit
  copy constructor and copy assignment exist and copy `fd` and `map`
  member-wise. Two values that hold one descriptor and one mapping each
  release them twice.

- `source/counters/detail/pmu.hpp:361` and
  `source/counters/linux_pmu/fast_read.cpp:385-401`: Claim: the
  destructor calls `munmap` on whatever address `map` holds. Verdict:
  confirmed. The destructor delegates to `fastContextClose`, which
  munmaps `context.map` with `context.mapLength` whenever `map` is not
  null. A stack address in `map` is therefore unmapped at destruction.

- `source/counters/linux_pmu/fast_read.cpp:385-401`: Claim: the release
  is safe against double close. Verdict: confirmed, and it refines the
  spec. `fastContextClose` resets `map`, `mapLength`, and `fd`, so an
  explicit close followed by destruction releases once
  (`test/source/counters_linux_pmu_seam_test.cpp:3796` does exactly
  this). The live double-release comes from copies; a second close would
  release once already.

- `test/source/counters_linux_pmu_seam_test.cpp:720-724`: Claim: the
  multiplex-window scenario gives `FastContext` a stack address. Verdict:
  confirmed. `context.map = &page`, where `page` is a local
  `perf_event_mmap_page`; the destructor at the end of the scenario
  (line 846) munmaps the stack page that holds `page`.

- `test/source/counters_linux_pmu_seam_test.cpp:779-783`: Claim: a
  separate raw-page site. Verdict: confirmed with a taxonomy note. The
  raw-page context (`rawContext`, `map = &rawPage`) sits inside
  `multiplexWindowScenario` and no scenario of its own. The spec's
  "multiplex-window, raw-page, and the two pinned-read sites" names four
  context sites across three scenario functions.

- `test/source/counters_linux_pmu_seam_test.cpp:896-900 and 910-914`:
  Claim: two pinned-read sites. Verdict: confirmed. Both branches of the
  compile-time `#if` in `migratedThreadScenario` build a context with
  `map = &page` and never clear `map`. Only one branch compiles per
  build, so three of the four stack-address contexts are live in any one
  binary. In the contract-emitting branch the forked child exits with
  `::_exit(0)` (line 926), which skips the child's destructor; the
  parent's context still munmaps the stack address.

- `test/source/counters_linux_pmu_seam_test.cpp:1019-1020`: Claim: the
  partial-open scenario exercises the double-release copy assignment.
  Verdict: confirmed, and it is the only live copy site.
  `acquired[index] = sg::counters::detail::FastContext{...}` copy-assigns
  from a temporary; the temporary's destructor munmaps and closes at the
  end of the statement, and the array element repeats both releases at
  scope exit. The second `munmap` fails with `EINVAL` and the mapping
  count returns to its baseline, so the scenario's count checks still
  pass today; the double `close` is the real hazard, because an
  intervening `open` can hand the same descriptor number to something
  else.

- Whole tree: Claim: no place outside the four test sites hands
  `FastContext` an unowned address, and no library code copies a
  context. Verdict: confirmed. The library owns contexts through
  `std::unique_ptr` only (`source/counters/linux_pmu/group_io.cpp:376`,
  `source/counters/linux_pmu/fast_read.cpp:228`). The other test
  contexts own real mappings (`:965-966`, `:1019`, and the kernel-granted
  mappings at `:1839` and `:3788`). One latent gap the spec's FR-001
  already covers: because the destructor suppresses nothing, the
  deprecated implicit move operations still exist and move the scalars by
  copying without emptying the source, so any future move would
  double-release. No current code moves a context.

- `test/source/harness_jitter_window_test.cpp:138-171, 227-256`: Claim:
  the gates are `bmGate` and `bmPassGate` and both wait on a read count.
  Verdict: confirmed. Each body spins on `provider.readActions()` with
  `std::this_thread::yield()` and a 1e9 spin bound that calls `fail()`.
  `bmGate` waits twice (margins 20000 and 20000); `bmPassGate` waits
  twice (margins 2 and `passGateBase() * 2 + 64`).

- `source/counters/fake_provider.cpp:103`,
  `include/speedgun-ng/counters_fake.hpp:252-254, 334`: Claim: the
  counter the gates wait on is shared between the probe cpus and the
  runner. Verdict: confirmed. `readActions()` reads one process-wide
  atomic that every fake window's `readPoints` increments, on every
  thread, for every leaf set. The wait cannot tell which thread produced
  an increment.

- `test/source/harness_jitter_window_test.cpp:817-820`: Claim: the CI
  failure message is "the gate entry loads the folds of the probe cpus
  whose windows kept gaps (FR-027, E-08)". Verdict: confirmed. The
  message is verbatim the round-trip scenario's check, so the observed
  failure is the fold check and no wait's spin bound.

- Spec FR-012: Claim: a fix makes the wait end on a count of the probe
  thread's own reads, kept apart from the runner's reads through the fake
  provider's seam. Verdict: consistent with the code. The seam exists:
  the test already installs a custom `WindowReader`
  (`sg::detail::installTestReader`, used in the raw-gap scenario) and
  carries per-thread state in `thread_local GapThreadState`, so a
  per-thread read count is realizable. The characterization of the
  current mechanism as a margin where the design wants an event
  matches `bmPassGate`'s documented drift reasoning.

## Questions

1. FR-004 says "the four scenarios", but the sites are four contexts in
   three scenario functions, and only three are live per build (the two
   pinned-read sites are `#if`-exclusive). Should FR-004 name the four
   context sites beside scenarios, and require both
   `migratedThreadScenario` branches to use an owned mapping? Best
   answer: yes; the fix must cover both branches even though one build
   runs one, because the build flag selects the branch and the defect
   sits in both.

2. The partial-open scenario's count checks pass today despite the
   double release, because the second `munmap` fails quietly and the
   double `close` lands on a descriptor number nothing else holds in
   that window. Does FR-006's move-assignment test measure the release
   at the moment of the assignment and again at scope exit? Best answer:
   it should: take the descriptor and mapping counts after the
   move-assignment and again after the element's destruction, so a
   single release and a double release both fail the count.

3. FR-012's fix separates "the probe thread's own reads" from "the
   runner's reads". With two probe cpus the shared counter mixes the two
   cpus' reads with each other, which is the interference the
   `bmPassGate` comment describes as drift; separating probe from runner
   alone leaves that in place. Should the fix count each probe cpu's
   reads separately? Best answer: yes, with per-cpu counts beside one
   probe total; the failing scenario runs `--jitter-probe` with two
   cpus.

4. Does the moved-from state need a defined `owner` and `pinnedCpu`?
   Best answer: no. The spec's assumption holds: every use of a
   moved-from context is already a contract violation, and the release
   path reads only `map`, `mapLength`, and `fd`.

5. Off-x86 builds: `fastContextClose` is a no-op there
   (`fast_read.cpp:174`), so the stack-address defect cannot fire off
   Linux. Do the fixed scenarios need to keep working there? Best
   answer: they do, and they can: an anonymous mapping and a closed
   `/dev/null` descriptor behave the same on every platform, so the
   fixed scenarios are platform-neutral and need no `#if`.
