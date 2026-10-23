# Contract: Thread Counts and Interleaving

**Surfaces:** `--threads=<list>`, `--interleave[=<seed>]`,
`detail::readThreadList`, `detail::readSeed`, `detail::shuffledOrder`
(research D-14, D-15, D-16, D-17; data model E-10, E-11, E-13).
**Covers:** FR-044 to FR-060; SC-004, SC-005, SC-012.

## `--threads=<list>`

```cpp
auto detail::readThreadList(std::string_view text)
    -> std::expected<std::vector<int>, std::string>;
```

**Precondition:** `text` is the option value as typed. **Postcondition:**
the answer holds one count per item, in item order, or one fault line.

The list is a comma-separated sequence of items (FR-045):

| Item | Reading |
| --- | --- |
| `<count>` | one count; a count below one is refused |
| `<min>:<max>` | the counts of `detail::buildThreadCounts` (`source/harness/detail/internal.hpp:315`): doubling from `min` while at most `max`; `min` above `max` is refused |

The fault line names the option and the text as FR-046 asks, at status
2:

```text
--threads: expected counts and min:max ranges, got '<text>'
```

The same reader answers `SPEEDGUN_THREADS` items.

The list is written into `RegistryEntry::threadCounts`
(`source/harness/detail/internal.hpp:199`) for every entry of
`detail::registry()` (`:244`) in one loop that stands before
`detail::expandRegistry` at `source/harness/cli.cpp:570` (D-14). The
expansion at `source/harness/family.cpp:404` and `:425` then crosses the
argument lists with the new counts, so `threads`, `threadRange` and
`denseThreadRange` all yield to the option (FR-050). The instance names
follow the master rule at `source/harness/family.cpp:246`: no
`/threads:` segment at one thread, `/threads:<n>` above one (FR-048).

The option caps nothing: a count above the cpu count of
`sched_getaffinity` runs, and the console discloses the count (FR-047).
A preset that cannot seat a count answers the master line of
`detail::placementFitStatus` (`source/harness/placement_preset.cpp:173`,
read at `source/harness/cli.cpp:607`) naming the preset, the place count
and the requested count, at status 2 before any run (FR-049). The
warning of `checkFamilySize` (`source/harness/family.cpp:290`) against
`kMaxFamilySize` (`source/harness/detail/internal.hpp:35`) keeps its
text when a list grows a family past one hundred instances.

## `--interleave[=<seed>]`

The row's argument kind is `OPTIONAL`, which is what lets the bare form
stand and keeps a following word a separate argument: `--interleave 7`
interleaves with seed 0 and leaves `7` an unclaimed positional, which
the entry point ignores the way it ignores a stray word today (FR-052).

```cpp
auto detail::readSeed(std::string_view text)
    -> std::expected<std::uint64_t, std::string>;
```

**Precondition:** `text` is the attached value or the environment value.
**Postcondition:** the answer holds the seed, or one fault line.

A seed text is unsigned decimal digits alone, consumed whole, with a
value at or below the largest 64-bit unsigned integer (FR-054). The
fault line names the option and the text:

```text
--interleave: the seed '<text>' is not an unsigned decimal value
```

The environment form starts `SPEEDGUN_INTERLEAVE: ` and carries the same
reader line (FR-022, FR-027). `-1`, `0x10`, `7x`, `true` and an
over-limit digit string all answer it at status 2;
`SPEEDGUN_INTERLEAVE=0` means given with seed 0. The bare `--interleave`
means given with seed 0 (FR-051).

## The order

```cpp
auto detail::shuffledOrder(std::size_t count, std::uint64_t seed)
    -> std::vector<std::size_t>;
```

**Precondition:** `count` is the number of selected instances.
**Postcondition:** the answer is a permutation of `0 .. count - 1`, and
the same `count` and `seed` answer the same permutation under every
standard library (FR-053).

The fill is `0 .. count - 1`; the walk runs `i` from `count - 1` down to
1 and swaps the entry at `i` with the entry at `engine() % (i + 1)` from
a `std::mt19937_64` seeded with the seed (spec D-12). No distribution
template is involved, which is what makes the claim travel across
libraries (research D-15, D-28). The pinned readings for `count = 12`:

| Seed | Permutation |
| --- | --- |
| 7 | `4 0 2 6 10 9 1 5 11 8 7 3` |
| 8 | `7 0 6 10 8 2 11 5 9 4 3 1` |

Both were measured under `g++` 16 and `clang++` 23 in the pass that
wrote research D-15.

The passes and repetitions of one instance stay together: the walk moves
whole instances, and the runner keeps its own repetition loop inside
`runner.run(*entry)` (FR-055).

## The print order

Without `--interleave` the loop prints each row as it runs, the master
path at `source/harness/cli.cpp:646`, and the exit status keeps the
master answer (FR-059).

With it, the loop holds the rows (E-13) and prints nothing; after the
loop one cold pass walks the instances in registration order and prints
each held row, and prints a family's fit after the last held row of that
family (FR-056). The held rows reach `ReportData` in that same order, so
the console and the JSON document agree. An interrupt during a shuffled
run prints the rows that ran in registration order and answers the
master interrupt status (FR-057).

The console states the seed in the disclosure line, and the `run` object
states it as `interleave` (FR-058,
[contracts/disclosure-and-schema.md](./disclosure-and-schema.md)).

## The loop shape

`const auto result = runner.run(*entry);` stands in the run loop at
`source/harness/cli.cpp:645`, and the entry-point shape gate stays green
with its one interrupt-flag reading inside the loop range
(`test/cli_shape.sh:32` and `:39`, the reading at `cli.cpp:652`)
(FR-060, FR-083). The walk order is computed before the loop and the
held-row print stands after it, so the gate needs no edit (research
D-16).

## Oracle

`harness_cli_threads_test` drives `readThreadList` with texts, asserts
the expansion of `min:max` against `detail::buildThreadCounts`, the
refusal line, the registry override and the resulting instance names
through a run, the master fit refusal line, and the disclosure of a
count above the affinity count read from the mask (FR-081: no test names
a cpu count). `harness_cli_interleave_test` asserts the two pinned
permutations, the seed faults and the attached-form rule (a run of
`--interleave 7` answers seed 0 and sets nothing from `7`), the held-row
print order and the fit position, and the interrupt case through a body
that calls `std::raise(SIGINT)` at a named instance (research D-17),
with no sleep and no clock. `test/cli_shape.sh` runs unchanged over the
branch.
