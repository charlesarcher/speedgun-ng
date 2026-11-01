# Research: B-1 move-only `FastContext`, B-2 counting-reader seam

Sources: the constitution (Principles II, X, XI), `spec.md`,
`diagnosis-b2.md`, and the code the two items touch:
`source/counters/detail/pmu.hpp`,
`source/counters/linux_pmu/fast_read.cpp`,
`test/source/counters_linux_pmu_seam_test.cpp`,
`test/source/harness_jitter_window_test.cpp`,
`source/harness/jitter_probe.cpp`,
`source/harness/detail/jitter_probe.hpp`,
`include/speedgun-ng/counters_fake.hpp`,
`source/counters/fake_provider.cpp`, and
`source/counters/plan.cpp`.
Line numbers name the base `d454bcff`.

## B-1 approach: the move-only FastContext design

### The two defects, restated from the code

`FastContext` (`pmu.hpp:337`) holds a descriptor, a mapped page, its
length, the owning thread, and the pinned processor. Its destructor
delegates to `fastContextClose` (`pmu.hpp:534-537`), and that release
calls `::munmap` and `::close` and empties the fields
(`fast_read.cpp:385-399`). The type declares a destructor and no
constructor, so the compiler writes a copy assignment. The test feeds
the context a stack address at four sites (`:722-723`, `:781-782`,
`:897-898`, `:911-912`), so the destructor's `munmap` addresses the
live stack; a page-aligned stack address unmaps part of it, and the
stack canary then reads zero and aborts. The same missing rule doubles
the release at `:1019-1020`: the temporary's destructor closes the
descriptor and unmaps the mapping, and the array element holds the same
values and releases them a second time at scope exit.

### The operations the type declares and deletes

The change completes the rule of five around the destructor that
already exists:

- Deleted: the copy constructor and the copy assignment, both
  `= delete`. Any accidental copy becomes a compile-time error, which
  is the spec's edge-case answer (spec.md, Edge Cases, first item).
- Added: a move constructor and a move assignment, both user-defined
  and `noexcept`. The moves perform no allocation and the release arm
  performs syscalls, so `noexcept` costs nothing and states the truth.
- Added: an explicitly defaulted default constructor,
  `FastContext() = default;`.

The defaulted default constructor needs a reason, and the reason is a
C++23 rule the design must surface: an aggregate carries no
user-declared constructor, and a move constructor is one
([dcl.init.aggr]). Once the move constructor stands, the type leaves
aggregate status, the six designated-initializer constructions in the
test (`:720`, `:779`, `:896`, `:910`, `:965`, `:1019`) stop compiling,
and the value-initialized array at `:1010` and the default
constructions at `:1039` and `:1831` need a default constructor that
exists by name. The defaulted default constructor keeps the non-static
member initializers (`fd = -1`, `map = nullptr`, the rest), so a
default-constructed context owns nothing exactly as today.

The construction sites take the form the library itself already uses:
`fastContextOpen` builds its context with
`std::make_unique<FastContext>()` and member assignment
(`fast_read.cpp:228-232`). The test sites follow that shape:
default-construct, then assign `fd`, `map`, `mapLength`, `owner`, and
`pinnedCpu` by name. The members stay public, so the assignment reads
like the designated form it replaces.

### What the move operations transfer and reset

Both operations copy all five fields from the source and then reset the
source's three resource fields: `fd = -1`, `map = nullptr`,
`mapLength = 0`. The moved-from `owner` and `pinnedCpu` stay as the
copy left them: the spec's assumption (spec.md, Assumptions) records
that no moved-from member needs a value beyond "owns nothing", and no
use of a moved-from context exists.

The move assignment releases the target first: it calls
`fastContextClose(*this)` before it takes the source's fields
(FR-002). That close empties the target's fields
(`fast_read.cpp:392-398`), so the target never holds two descriptors or
two mappings, and the release runs through the one release path the
destructor already documents. Self-assignment needs no guard: the
release empties the fields, the transfer that follows moves an emptied
source onto itself, and the value ends owning nothing with its
resources released exactly once. A guard would add a branch no call
site takes (X.2) and a branch the coverage gate would then have to
measure (FR-015).

### Where the definitions live

Declarations stand in `pmu.hpp` beside the destructor, with the
doxygen contracts. Definitions stand out of line in
`fast_read.cpp`, beside `fastContextClose`, outside the
`#if SG_PMU_FAST_X86` / `#else` platform branch, so one definition
serves the x86 build and the stub build.

The enforcement sits in `fast_read.cpp` for three reasons:

- `dbc.hpp` is already included there (`fast_read.cpp:26`); `pmu.hpp`
  carries no contract macro today, and the inline-definition route
  would pull the include into the header for no gain.
- Every other enforcement of this component stands in the same file
  (`SG_REQUIRE` at `fast_read.cpp:272`, `:279`, `:320`, `:323` and
  `SG_ENSURE` at `:124`), so the moves keep the component's contracts
  in one place.
- The pair gate attributes each macro invocation to the innermost
  function region (`tools/dbc/dbc_pair_gate.py`, `scan_tree` and
  `innermost`), so an out-of-line definition pairs with the header's
  doxygen rows the same way an inline one does.

Rejected placements:

- Header-inline definitions beside the destructor. The destructor is
  header-inline because it only delegates (`pmu.hpp:535-537`); the
  move operations add `SG_ENSURE` enforcement, and the header route
  changes the header's include set.
- Definitions inside each platform branch. The release arm is
  platform-independent, and a second copy of the same definition is
  the drift the contract-facility rule (II) forbids in spirit.

Rejected operation shapes:

- Defaulted move operations (`= default`). The defaulted forms
  transfer the fields but state no contract and enforce nothing;
  FR-001 requires the `SG_ENSURE` pairs.
- A moved-from flag that gates the destructor's release. It adds
  state, a second release path, and drift beside the single
  `fastContextClose` release the destructor documents.
- Shared ownership (a reference count) or a pimpl. Both rewrite a
  plain kernel-facing aggregate for a defect the move-only rule
  fixes; X.2 rules them out.
- Deleting the copy assignment alone. The destructor suppresses the
  implicit move operations, so the type would move not at all, and
  `fastContextOpen`'s unique_ptr hand-back and the array element
  would stop compiling.

### The DBC contracts

Each new operation documents `\pre none` and two `\post` rows, and
enforces the moved-from row with one `SG_ENSURE`.

- Move constructor. `\pre none`. `\post The context holds the
  descriptor, the mapping, its length, the owner, and the pinned
  processor the source held.` `\post The source owns no descriptor
  and no mapping (descriptor -1, no mapping, length 0).`
  Enforcement: one `SG_ENSURE` over the source's emptied triple after
  the transfer.
- Move assignment. `\pre none`. `\post The context released what it
  held, then holds what the source held.` `\post The source owns no
  descriptor and no mapping.` Enforcement: one `SG_ENSURE` over the
  source's emptied triple; the release-before-take is structural,
  because `fastContextClose` empties the fields before the transfer.

The contract facility drops the checks from release builds (II), so
the read path gains nothing. The move bodies carry no branch, the new
tests cover every line, and no `LCOV_EXCL` marker is added (FR-015);
the `SG_ENSURE` machinery itself sits outside the coverage
measurement (VI).

### The test-side changes

1. Four owned anonymous mappings (FR-004). The four sites move the
   fake event page off the stack: the multiplex page (`:720-724`),
   the raw page (`:779-783`), and the two pinned-read pages, one per
   contract build (`:896-900`, `:910-914`). Each site maps one page
   with `mmap(nullptr, length, PROT_READ | PROT_WRITE,
   MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)`, builds the fake page in the
   mapping, puts the mapping's address and the page size
   (`::sysconf(_SC_PAGESIZE)`, the recipe `lifecycleCycle` already
   uses at `:959-962`) into the context, and lets the destructor
   release it. The write permission is required: the multiplex
   scenario mutates the page's fields while the context holds it
   (`:741`, `:751`), and the setup writes `makeEventPage`'s value
   into the mapping at every site. The field mutations go through
   the mapped pointer. The work-around FR-004 forbids (clearing
   `map` before destruction) appears nowhere. A small helper beside
   `makeEventPage` that maps the page, builds the page in it, and
   hands back the pointer and the length keeps the four sites one
   call each; four inline blocks satisfy FR-004 with the same
   checks, and both forms are acceptable.
2. The partial-open scenario (`:1010-1027`). The assignment at
   `:1019-1020` stays as written: once the copy assignment is
   deleted, it selects the move assignment, and the array element is
   value-initialized, so it holds nothing and releases nothing while
   the source's pair moves in. The array's destruction releases each
   descriptor and each mapping exactly once (FR-005, acceptance
   scenario 5). Rejected alternative: an in-place construction
   through `std::destroy_at` and `std::construct_at`. The machinery
   buys no new guarantee, the `construct_at` call needs the
   parenthesized-aggregate form, and the move assignment is the
   operation whose correctness the defect class needs.
3. Two new run-time tests (FR-006), counted with
   `openDescriptorCount()` (`:172`) and `mappingCount()` (`:191`),
   registered beside the lifecycle scenarios (`:3819-3821`):
   - The moved-from destruction test. Baseline counts; a context
     holding a real descriptor and an owned anonymous mapping; a
     second context move-constructed from it; a check that the
     target holds the source's fields and the source's triple reads
     `-1`, `nullptr`, `0`; scope exit destroys both; the counts
     stand at the baseline (FR-003).
   - The move-assignment test. Baseline counts; a target holding a
     real descriptor and an owned mapping; a source holding its own;
     the assignment; a mid-block check that the counts stand exactly
     one above the baseline (the target's old pair left at the
     assignment, the source's pair still held); a check that the
     target holds exactly the source's fields; scope exit; the
     counts stand at the baseline. The two count checks pin the
     release to exactly one: a missed release shows at the mid check
     as a retained descriptor or mapping, and the final check shows
     the process returned to its starting counts.
4. The compile-time check (FR-006, spec edge case 1):
   `static_assert(!std::is_copy_constructible_v<
   sg::counters::detail::FastContext> &&
   !std::is_copy_assignable_v<sg::counters::detail::FastContext>)`
   in the test file, with `<type_traits>` included.

The internal seam stays internal: `git grep -n FastContext include/`
returns nothing at the base, and the change adds no public name
(FR-008, FR-014).

### Alternatives rejected (B-1)

- Fixing the test alone and leaving the compiler-generated copy
  assignment: the library defect survives, and the next caller
  repeats it. The fix goes where the cause sits.
- Heap or static storage for the fake pages: the destructor munmaps
  whatever address the context holds, so only a real anonymous
  mapping makes the release address a mapping the test owns. A heap
  or static address would put the `munmap` on memory of the wrong
  kind, the same defect class with a different victim.
- Copy-eliminating the temporary at `:1019-1020` through a refactor
  of the loop: the assignment is the statement the defect lives on,
  and the move-assignment reading fixes it where it stands (X.3).
- A self-move guard: the structural argument above.
- `std::optional<FastContext>` or a `unique_ptr` re-wrap at the call
  sites: indirection for one caller, X.2.

## B-2 approach: the counting-reader seam design

### Where the wait stands today

`bmGate` (`:138`) and `bmPassGate` (`:227`) wait on
`provider.readActions()` (`:146-148`, `:162-164`, `:235-237`,
`:244-246`). That value is the provider-wide atomic every
`FakeWindow::readPoints` increments (`fake_provider.cpp:63`), so one
count carries the probe cpus' rides, the runner's sampling reads, and
any stale reader a former scenario left live. The margins (20,000
actions in `bmGate`; `passGateBase() * 2 + 64` in `bmPassGate`'s ride
wait) stand against a composition that does not hold under load, and
`diagnosis-b2.md` measured the margin unblocking on reads the probe
never made: the pass's timed window closed with no kept gap at the
pass's magnitude, and the fold carried only the earlier magnitude.

### The seam

`installTestReader` (`source/harness/detail/jitter_probe.hpp:144`,
namespace `sg::detail`) installs a counters `WindowReader` in place of
the one the probe opens over the `machine/tsc` leaf. `JitterProbe::start`
moves the installed reader into the session (`jitter_probe.cpp:434`),
and every probe item resolves its read thunk from that one reader
(`:218-223`), so the reader is shared by all probe cpus and its count
covers every cpu the run probes. The runner's reads take a different
route: the plans the system compiled hand each group the provider
window's own thunk (`plan.cpp:604`), and those reads never pass the
installed reader. The seam exists for this shape (specs/027 R-02), and the
change adds no public name and touches no installed header (FR-014).

### The counting reader

A test-local decorator, the shape `GapSeriesReader` (`:535-545`)
already uses, named `CountingReader`:

- It holds one delegate window per probe thread, created on the
  thread's first read through the fake provider's public `open()`
  over the machine/tsc leaf (`counters_fake.hpp:308-310`,
  `fake_provider.cpp:303-338`): a `LeafSet` whose addresses name
  `machine/tsc` and whose disclosure column stands at
  `kNoDisclosureColumn`. The fake ignores the target, so the test
  passes any valid `Target`. The per-thread shape is the probe's own
  default path: the probe's default opens one window per item, whose
  cursor advances by that item's reads alone (FR-018), so a cpu's
  gap stays the scripted tail. One shared delegate would make the
  two cpus' gaps measure their interleaving, and the quiet entry's
  windows would keep the interleaving gaps the run must not carry;
  the loaded diagnosis measured exactly that failure before the
  per-thread shape stood in.
- Its `readPoints` counts one increment on a relaxed atomic (the
  `m_readActions` shape, `counters_fake.hpp:334`) and delegates to
  the reading thread's own window. The scripted series, the
  `setPoints` tail jumps, the per-window playback hand-off, and the
  disclosure behave exactly as today, so every scenario's assertions
  keep holding.
- The test keeps a raw pointer to the decorator in function-local
  state (the `seriesJump()` / `passGateCalls()` pattern, `:115-123` and
  `:177-185`) and clears that pointer when it clears the install.
  The gate bodies read the count through an accessor on that
  pointer; the accessor's precondition is an installed reader, and a
  run that reaches a body with none installed reports the failure
  through the accessor, and the accessor dereferences no null
  pointer.
- The decorator keeps the `WindowReader` default thunk
  (`counters_provider.hpp:369`): the read reaches `readPoints`
  through one virtual call and delegates once. The `FakeWindow`
  thunk specialization (`fake_provider.cpp:54-64`) serves the
  library's sampling-path cost; the decorator is test code with no
  such bound to keep.

The install follows the specs/027 FR-028 gap scenario's pattern
(`:863-865`): the scenario arms the decorator after its `setPoints`
calls, runs `captureRun`, and clears the install and the pointer
afterwards. The decorator constructs its underlying window at
install time, after the scenario's `setPoints` calls, so the window
opens on the series the scenario scripted.

### The waits

Both bodies keep their two waits, their yield, and the
1,000,000,000-spin bound (`:148-156`, `:166-172`, `:237-241`,
`:246-250`). Each wait now ends on the counting reader's count, and
the targets derive from the pass structure the probe documents: one
measurement pass takes `kPassReads = 1024` reads per probe cpu
(`jitter_probe.cpp:50`, the initial read plus the loop to `:245`),
so one pass's worth of rides stands at `kPassReads` times the cpus
the scenario armed.

- `bmGate` pass wait: ends after one pass's worth of the probe's
  reads ride the one-tick tail inside the window, so the jump stands
  only after the probe's reads demonstrably flow at the unjumped
  series (specs/027 E-05, FR-007, R-05).
- `bmGate` jump wait: after `setPoints(seriesJump())`, ends after
  one pass's worth of rides at the jumped tail, so each probe cpu
  keeps records at the jump's magnitude and the close edge stands
  above every kept stamp (specs/027 FR-039, R-04).
- `bmPassGate` rebase wait: after `setPoints(1)`, ends after one
  read per armed probe cpu, the hand-off the close edge forks from
  (`:99-108`).
- `bmPassGate` ride wait: after `setPoints(pass * kSeriesJump)`,
  ends after one pass's worth of rides at the pass's magnitude. The
  drift margin (`passGateBase() * 2 + 64`) goes away with its
  reason: the decorator gives both cpus one window and one cursor,
  so the inter-cpu cursor drift the margin sized against cannot
  arise, and `passGateBase()` loses its only use and leaves the test
  (X.3).

The armed cpu count travels beside the reader pointer in the same
function-local state; the scenarios already build it from `pickCpus`
(`:570-572`). `kPassReads` itself moves from `jitter_probe.cpp:50`
into `detail/jitter_probe.hpp`, so the test and the probe spell the
value once; the header is internal (specs/027 R-01) and the move touches no
installed header (FR-014). The alternative, a restated 1024 in the
test, leaves one fact with two spellings.

### What stays

The spin bound and the yield stay (specs/027 FR-039): a stalled series still
reports a failure and never hangs. No sleep, no clock read, no
timeout, and no retry is added anywhere (FR-012, the brief's hard
rules). The provider's `readActions` keeps its published meaning
(`counters_fake.hpp:245-254`) for the counters tests that rely on
it; the gate bodies stop reading it.

### Rejected alternatives (B-2)

- Larger or re-derived margins. The margin is the defect: it
  unblocks on reads the probe never made (`diagnosis-b2.md`), and a
  larger constant is the same defect with more slack. The brief
  names the shape the fix takes: a wait that ends on an event the
  test controls.
- Per-provider or per-window counters in the library.
  `FakeProvider` is exported with no pimpl, so a member changes the
  layout of a published type (`fake_provider.cpp:40-42`), and
  FR-014 forbids the ABI change. `readActions`' own contract pins
  the count's current meaning ("the `readPoints` actions performed
  by readers this provider opened", `counters_fake.hpp:245-247`),
  and the counters tests rely on that meaning (US1 scenario 5).
- Public API additions (a public install seam, a public probe
  accessor). The internal seam exists at `jitter_probe.hpp:144`, and
  the change keeps it internal (FR-014); a bug fix that widens the
  public surface fails the scope the spec sets (User Story 3).
- Waiting on the probe's kept counts or the fold output. The
  benchmark body has no route to the probe holder's states, and the
  checks the waits serve (the entry-level fold check at `:816-820`,
  the per-pass magnitude checks at `:1064-1068`, the run-level
  folds) need an ordering signal, never a fold value.
- Per-cpu attribution inside the decorator (a thread-local count per
  cpu, the `GapSeriesReader` shape). The waits serve fold-level
  checks, no check needs a per-cpu wait signal, and machinery that
  serves no existing check is X.2's definition of speculative
  generality.
- A sleep, a timeout, or a retry. Forbidden by FR-012 and the
  brief's hard rules.

## Risks and unknowns

1. The seam reader is shared by all probe cpus (`jitter_probe.cpp:434`,
   `:218-223`). The count is probe-wide, so a wait cannot tell which
   cpu produced a read, and a stalled cpu hides behind the other
   cpu's rides. The spin bound still reports a stall, and the checks
   the waits serve read the run's folds after `stopAndJoin`, never a
   per-cpu wait signal. The implementation confirms this stays true
   for every check the waits serve.
2. The runner's reads still count in `readActions`
   (`fake_provider.cpp:63`), and no wait may use it after the
   change. The decorator's delegation also increments it once per
   probe read. The implementation greps the test for `readActions`
   after the change and confirms the only readers left are the
   counters tests' own scenarios.
3. Every run whose filter names a gate body must install the
   decorator. The gate-body runs stand at `:741` (the roundtrip run,
   two cpus), `:941` (the split run, two cpus, no filter), `:1004`
   (repetitions, one cpu), `:1044` (pass-split, one cpu), `:1081`
   (kept buffer, one cpu), and `:1260`, `:1268` (the comparison
   runs, one cpu each). A body without an installed reader reports a
   failure through the accessor; it never dereferences null and
   never hangs.
4. The armed cpu count varies per run: two cpus at `:741` and
   `:941`, one cpu
   elsewhere. The wait targets multiply `kPassReads` by the count
   the scenario armed, and the body learns that count from the
   test-side state beside the reader pointer. The implementation
   confirms the count at install time matches the run's
   `--jitter-probe` cpu list, which each scenario builds from
   `pickCpus`.
5. One underlying window serves both cpus, so a cpu's kept gap can
   carry a multiple of the pass's magnitude when the other cpu rides
   in between. The magnitude-exact checks (`minNs == kSeriesJump`,
   `maxNs == 2 * kSeriesJump` at `:1064-1068`, `p99Ns` at
   `:1279-1282`) run on single-cpu scenarios, where no interleaving
   happens. The implementation confirms no two-cpu run asserts an
   exact magnitude; the two-cpu roundtrip run asserts fold
   existence, ascending cpus, and round-trip shape only.
6. The fake window's playback hand-off stays shared with the
   runner's windows (`fake_provider.cpp:97-100`): the decorator
   isolates the count only. The tail rises per read, so the close
   edge, read after the ride reads the wait ensured, stands above
   every kept stamp. The suite under `ci-tsan` covers the shared
   leaf state; the decorator adds one shared atomic and one
   delegated window.
7. The decorator's raw pointer dangles once the run stops the probe
   and destroys the session. The count is read only inside the gate
   bodies, which run while the session is live; the pointer clears
   beside the install. The implementation keeps any other use out.
8. The four B-1 sites need a writable mapping, and the mapped page
   must hold `sizeof(perf_event_mmap_page)`. The supported platform
   is x86-64 Linux, whose pages are 4096 bytes or more, so one page
   holds the kernel's page type. The mapping length is the page size
   `sysconf` reports, the same figure `fastContextOpen` maps with
   (`fast_read.cpp:232`).
9. The partial-open wording: the brief names "in-place
   construction"; FR-005 admits "construct each element in place or
   move into it". The research recommends the move-assignment
   reading above. The implementer records the chosen reading in the
   commit body (X.1).
10. The Rocky container check (FR-017) covers both items, and the
    1000-run seam count and the 100-run preset loops (SC-001,
    SC-002) supply the pull request body's evidence. The B-2 fix
    lands in one commit with the test it changes (FR-012); the B-1
    library change and its tests land in one commit (FR-007).
