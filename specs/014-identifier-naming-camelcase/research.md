# Phase 0 research: identifier naming

Working tree `HEAD` is the audit point
`6d32efcab3c13c3d41470c1c621d78839ce11543`. Citations below were read on
that tree. A later edit re-anchors a line that moves.

## Method

Each decision cites a command or a file:line. A tool that is missing is
recorded as absent from `command -v`. No SHA is invented.

## Corrections to the specification assumptions

The specification assumes `BUILD_SHARED_LIBS` stays off and that the
shared-object version is dormant. That assumption does not hold.

`cmake/variables.cmake:9` defaults the option to `OFF`. It does not force
the parent library. `CMakeLists.txt:137`, `:232`, `:384`, and `:558` force
the option off only around vendored `add_subdirectory` calls.
`.github/workflows/ci.yml:251` configures
`cmake --preset=ci-linux-audit -B build-shared -D BUILD_SHARED_LIBS=ON`.
`SOVERSION` at `CMakeLists.txt:47` is live in that job.

The bump from 1 to 2 still records the ABI break. The shared-audit job
exercises it. The plan does not edit the closed follow-up text that calls
the number dormant. That sentence is a separate stale claim. This feature
corrects the two sentences FR-020 names, and it leaves the dormant-number
sentence for a later logged edit.

The version list lives in closed plans:
`specs/012-counters-defect-resolution/plan.md:170` and
`specs/013-counters-defect-followup/plan.md:126`. FR-016 forbids an edit
there. This feature records the 0.5.0 entry in its own plan. The successor
log points at the rename map, which points at that entry.

## Decisions

### D-01 Rename mechanism

Decision: The rename mechanism is one fixer pass per commit group. Each rename commit runs `clang-tidy` 23.1.1 with a temporary
copy of the new naming options, exports fixes, and applies them with
`clang-apply-replacements` 23.1.1. `clang-format` then reformats only the
files that commit touches. A name the fixer misses is edited by hand in
that same commit.

Rationale: `command -v clang-rename` fails. The two LLVM tools above are
on `/usr/bin`. A fix-it renames an identifier token. A string literal is
not an identifier. FR-016 requires those literals to keep their text. The
first rename commit confirms that on one fixture whose literal text
matches an old spelling, and the literal is unchanged after the apply
step.

Alternatives considered: A tree-wide textual substitution. It rewrites
literals, catalog names, and comments, which FR-016 forbids. A single
tree-wide `clang-tidy` fix. It produces one commit, and FR-007 requires
one coherent group per commit. Hand edits alone. They remain the fallback
for a missed name, and they are the wrong default on a tree this size.

### D-02 Commit-group order

Decision: Each commit group moves one header family and its call sites
in one commit. The order follows the include graph measured under
`include/speedgun-ng/`.

1. `dbc.hpp`, `tools/dbc/macros.yaml`, and the compile-fail tests that
   name `dbc` identifiers.
2. `counters_core.hpp` and every call site of a name it declares.
3. `counters_provider.hpp` and its remaining call sites.
4. `counters_measurement.hpp` and its remaining call sites.
5. `counters_clock.hpp`, `counters_pmu.hpp`, `counters_push.hpp`, and
   `counters_fake.hpp`, one commit each, with that header's users.
6. `counters_system.hpp` and its users.
7. `simulation.hpp` and its users.
8. File-local helpers in `source/` that no header declares, one commit
   per source family (`linux_pmu`, `fold`, `plan`, and any family the
   search still finds).
9. A sweep of `tools/`, `test/`, and `.github/workflows/` for a match
   FR-017 names that an earlier commit did not update.
10. The closing commit: constitution 2.14.0, `.clang-tidy` keys,
    `WarningsAsErrors`, version fields, the rename map, and the document
    edits.

`counters.hpp` and `speedgun-ng.hpp` are umbrellas. They gain no spelling
change unless they declare a name. They ride the commit of the header
they include when a name in that header changes.

Rationale: `counters_core.hpp` includes only the export header.
`counters_provider.hpp` includes core and `dbc.hpp`.
`counters_measurement.hpp` includes core, provider, and `dbc.hpp`.
The clock, pmu, push, and fake headers include provider or measurement.
`counters_system.hpp` includes core, measurement, and provider.
`simulation.hpp` includes only the export header. A declaration that
moves without its call sites does not compile. The core commit is large
because almost every translation unit names a core type. Splitting it
would break the build FR-007 requires.

The new `.clang-tidy` keys land in the closing commit. Until that commit,
`WarningsAsErrors` stays empty, so a mid-sequence spelling does not fail
the build. FR-007 requires compile, tests, and the format check at each
commit. It does not require a clean name check before the head.

Alternatives considered: Flip the naming keys in the first commit. Every
later commit would fail the name check until the last name moved.
Rename call sites in a later commit than the declaration. The tree would
not compile between those commits.

### D-03 Machine-code comparison

Decision: Build the release preset twice with contracts ignored. Compare
normalized disassembly of each owned object file. The permitted
difference is the string an always-on contract makes from a renamed
predicate.

Command shape, one line each:

`cmake --preset=ci-ubuntu -Dspeedgun-ng_CONTRACTS=ignore`

`cmake --build build`

`objdump -d --no-show-raw-insn <owned-object>`

Normalization replaces each owned mangled symbol with a stable token
from `nm` and `llvm-cxxfilt` (LLVM 23.1.1). Strip the address column
before the diff. Instruction text and immediates stay. An address
column is not a difference. An immediate that changes is a difference.
`diff` of the two normalized texts is empty, apart from the logged
contract-predicate strings.

`cmake/dbc.cmake:15` defines `ignore` as semantic 0. Always-on macros
stay compiled in that semantic. The consumer-release job already selects
`ignore`.

Rationale: FR-002 asks for identical machine-code text after symbol
normalization, with that one permitted string difference. `objdump` is
GNU Binutils 2.47. `clang-rename` is absent, so the comparison does not
depend on it.

Alternatives considered: Compare raw object bytes. Mangled names differ
on every symbol, so the comparison would fail on the rename itself.
Compare with contracts enforced. An enforce build emits the predicate
text FR-002 already allows, and it also emits check code the ignore
build elides. The specification names the ignore build.

### D-04 Macro-collision check

Decision: At the rename head, preprocess a translation unit that includes
every public header and the Linux headers a library translation unit
includes. The command is `clang++ -dM -E -std=c++23` on that unit. The
check fails when an enumerator token or a `kPascalCase` constant token
is a defined macro.

The risk tokens from the specification are `NONE`, `GAP`, `SYSCALL`,
`CPU`, `THREAD`, `ABSENT`, `BYTES`, and `OPS`. The check covers every
new enumerator and every new constant. The risk list is the minimum set. The result is
recorded in the feature directory before the closing commit.

Rationale: A macro replaces the token before the compiler reads the
enumerator. `kPascalCase` keeps constants off that surface. Enumerators
stay on it, which is why the check exists (FR-006).

Alternatives considered: Skip the check and rely on a later compile
error. A macro collision can change meaning and still compile.
`UPPER_SNAKE_CASE` for constants as well as enumerators. The
specification rejected that reading because it widens the collision
surface.

### D-05 clang-tidy mapping

Decision: `.clang-tidy` implements the constitution. The option names
below come from `clang-tidy` 23.1.1
`clang-tidy -checks=-*,readability-identifier-naming --dump-config`.
`CamelCase` is the check's spelling for PascalCase. `camelBack` is its
spelling for lowerCamelCase.

| Rule | Options |
| --- | --- |
| N-1 types | `ClassCase`, `StructCase`, `EnumCase`, `UnionCase`, `TypeAliasCase`, `TypedefCase` set to `CamelCase` |
| N-2 functions | `FunctionCase`, `MethodCase`, `ClassMethodCase`, `ConstexprFunctionCase`, `ConstexprMethodCase`, `PrivateMethodCase`, `ProtectedMethodCase`, `PublicMethodCase`, `VirtualMethodCase`, `GlobalFunctionCase` set to `camelBack` |
| N-3 macros | `MacroDefinitionCase` `UPPER_CASE`, `MacroDefinitionPrefix` `SG_` |
| N-4 enumerators | `EnumConstantCase` and `ScopedEnumConstantCase` set to `UPPER_CASE` |
| N-5 variables | `VariableCase`, `ParameterCase`, `LocalVariableCase`, `LocalConstantCase` set to `camelBack` |
| N-6 members | `PublicMemberCase` `camelBack` with an empty prefix. `PrivateMemberCase` and `ProtectedMemberCase` `camelBack` with prefix `m_` |
| N-7 namespaces | `NamespaceCase` `lower_case` |
| N-8 template parameters | `TemplateParameterCase`, `TypeTemplateParameterCase`, `ValueTemplateParameterCase`, `TemplateTemplateParameterCase` stay `CamelCase` |
| N-9 file names | No identifier option. File names stay. The check does not govern them. |
| Constants | `ConstexprVariableCase`, `StaticConstantCase`, `GlobalConstantCase`, `ClassConstantCase` set to `CamelCase` with prefix `k` |

`ConceptCase` is absent from the 23.1.1 dump. A concept is still
PascalCase under N-1. The closing commit records whether the check
diagnoses a concept. If it does not, review enforces N-1 for concepts,
and the constitution says so.

N-10 (an acronym is one word) has no option. Review enforces it. The
rename applies it (`Pmu`, `Tsc`, `Dbc`).

N-11 (drop `_t`, tag objects in lowerCamelCase) has no option that
separates a tag object from another constexpr variable. Each tag object
is listed in `ConstexprVariableIgnoredRegexp`, and the list entry cites
N-11. The implementation enumerates the objects. This research does not
invent that list.

`HeaderFilterRegex` is absent from `.clang-tidy`. An empty filter shows
diagnostics for the main file only. The closing commit sets
`HeaderFilterRegex` so diagnostics from `include/speedgun-ng/` and
`source/` are reported, and diagnostics from `external/` are not.
SC-011 plants one misnamed identifier in a header and one in a `.cpp`.
Both plantings fail the name-check step. A planting that stays silent
means the filter is wrong, and the closing commit stays open.

Exceptions use the per-kind `IgnoredRegexp` options the dump lists, plus
`IgnoreMainLikeFunctions: true`. The export macro `SPEEDGUN_NG_EXPORT`
is listed in `MacroDefinitionIgnoredRegexp` because it has no `SG_`
prefix. Protocol names (`begin`, `end`, `empty`, and the rest of the
exception list) are listed on the kind that declares them. A suppression
that remains cites the exception entry (FR-019).

`WarningsAsErrors` is `''` at `.clang-tidy:20`. The closing commit sets
it to `readability-identifier-naming`. Other checks stay on the existing
baseline. A naming finding then fails the compile. That is the mechanism
FR-004 requires. The 2.10.0 report measured that a global
`WarningsAsErrors: '*'` fails the build on vendored findings. This
change names one check.

Rationale: The dump is the option list the installed binary accepts.
Guessing a key the dump omits would make the closing commit fail at
configure time.

Alternatives considered: A custom clang-tidy check for N-10 and N-11.
The specification does not ask for a new tool. A second prose document
as the source of the keys. FR-011 says the keys derive from the
constitution.

### D-06 Gate baseline

Decision: No gate baseline exists yet. The plan names the predecessor
work, and it names the SHAs that were observed. It does not name a
future SHA.

Observed runs on `master`, from `gh run list --branch master --limit 12`:

| Run | Head | Conclusion | Why it is not the baseline |
| --- | --- | --- | --- |
| [37553123469](https://github.com/charlesarcher/speedgun-ng/actions/runs/37553123469) | `6d32efcab3c13c3d41470c1c621d78839ce11543` | failure | Audit point. The lint job failed. Every other job was skipped. |
| [37533305027](https://github.com/charlesarcher/speedgun-ng/actions/runs/37533305027) | `daa4b6db8c46783d7f0ad05190d246ffc5abcfef` | failure | `test`, `test-rocky`, `sanitize`, `coverage`, and `tsan` failed. |
| [37175047162](https://github.com/charlesarcher/speedgun-ng/actions/runs/37175047162) | `6aafd2dc8a1310ee335820ad8dc4dedd0eb0ae17` | success | Last green run. It predates the counters work FR-013 renames. |

Predecessor, before any rename commit:

1. Repair the 21 format-check files the specification lists. Change no
   identifier.
2. Land that repair on the default branch and wait for a green
   Continuous Integration run.
3. Record that run's head SHA in this plan, with the passing test names,
   the gate results, and the name-check finding count per translation
   unit (FR-021).
4. Start the rename sequence from that SHA.

No rename commit is opened before step 3 is in this plan.

Rationale: FR-001, FR-002, and FR-003 compare against a commit where
every hard gate passes. The audit point has no such record. The last
green commit does not contain the identifiers this feature renames.

Alternatives considered: Use the audit point and ignore the format
failure. The specification forbids that comparison. Use
`6aafd2dc8a1310ee335820ad8dc4dedd0eb0ae17` as the baseline. The machine
code and the test set there omit the counters surface.

### D-07 Reference host and noise

Decision: The reference host is `Linux 7.2.4-1-cachyos x86_64`, AMD
Ryzen 9 9950X3D, 16 cores, at
`docs/pages/counters-overhead.md:117-118`. The noise bound is five
percent. The page uses that bound at `:406-411` for the sampling-path
median and the core-PMU median. The page does not label a figure with
the word noise (`grep -n noise` exits 1).

FR-003 passes when a re-measure on that host stays within five percent
of the gate-baseline figures for the rows the page already gates. The
gate-baseline figures are recorded when D-06 step 3 lands. The stale
name `pmu_probe_fast` is at `:299` and is removed in the closing commit.
The name is absent from `include/`, `source/`, and `test/`.

Rationale: The specification says the plan records the host and the
noise figure the overhead page already uses. Five percent is that
figure.

Alternatives considered: Treat the 0.3 ns agreement at `:87` as the
noise bound. That sentence compares two timing methods on one run. It
is not the pass-to-pass bound the page gates.

### D-08 Constitution amendment placement

Decision: The amendment is MINOR, version 2.14.0, on the reading the
specification chose. It lands in the closing commit, in the same change
as the `.clang-tidy` flip, because a gate item whose check does not yet
fail the build is a gate no commit can pass. The 2.13.0 report stated
that shape for the thread-sanitizer item.

Edits, anchored on the working tree:

| Site | Edit |
| --- | --- |
| Head of `.specify/memory/constitution.md` | Sync Impact Report for 2.14.0, before the 2.13.0 report |
| Principle V, after the formatting rule at `:384-385` | Rules N-1 to N-11, the `kPascalCase` spelling, and the exception list |
| `:553-555` | Replace the local-style naming sentence. The replacement is in `plan.md`. |
| After the static-analysis bullet at `:444-449` | One hard-gate bullet for `readability-identifier-naming` |
| Lineage table, new row above `:858` | 2.14.0 row |
| `:877` | Version 2.14.0 and the Last Amended date of the amendment commit |

The Last Amended date is the date of that commit. This plan's date,
2026-10-07, is not that date.

Rationale: Governance at `:847-851` requires the rationale, the version
bump, and the date. FR-010 requires the Sync Impact Report and the
lineage row. FR-009 puts the rules in the constitution text, and
Principle V is the style principle those rules govern.

Alternatives considered: Land the amendment in the first commit. The new
gate would fail every commit until the closing tidy flip, or the gate
item would name a check that does not yet fail the build. MAJOR 3.0.0.
The specification rejected that reading. Governance reserves MAJOR for
an incompatible principle removal or redefinition. This amendment adds
rules and a gate, and it narrows one X.3 clause. That is the MINOR
pattern of 2.13.0 and 2.11.0's narrower cases. 2.11.0 was MAJOR because
it withdrew a platform obligation. This amendment withdraws no gate.

## Repository constraints this plan records

- C++23, Linux, GCC and Clang. No new runtime dependency.
- `CMakeLists.txt:7` moves from `0.4.1` to `0.5.0`.
- `CMakeLists.txt:47` moves from `SOVERSION 1` to `SOVERSION 2`.
- `cmake/install-rules.cmake:39` stays `SameMinorVersion`.
- `include/speedgun-ng/counters_measurement.hpp:25` and `:31` record the
  new version. No other header carries `0.4.1`.
- Twelve public headers. The include edges are listed under D-02.
- `clang-rename` is absent. `clang-tidy` and `clang-apply-replacements`
  are LLVM 23.1.1.
