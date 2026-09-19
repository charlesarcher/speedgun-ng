# Contract: Gate Command Line and Entry Points

**Feature**: `002-prose-commit-lint` | [plan.md](../plan.md) | [data-model.md](../data-model.md)

The gate has one implementation and three entry points: the Python
executable, a `cmake -P` script module, and a CMake target. Every entry
point reaches the same code path, which is what makes the local verdict
identical to the CI verdict (FR-019, US3 scenario 3).

## 1. Python executable

```text
python3 tools/prose/prose_gate.py [OPTIONS]
```

| Option | Value | Default | Contract |
| --- | --- | --- | --- |
| `--check` | `prose`, `commit`, `all` | `all` | FR-001 and FR-016 run together by default; either alone for targeted debugging |
| `--mode` | `range`, `tree` | `range` | `range` implements FR-007, `tree` implements FR-008 |
| `--base` | git sha or ref | merge-base with `origin/master`, else exit 2 | Left edge of the range, exclusive |
| `--head` | git sha or ref | `HEAD` | Right edge, inclusive |
| `--paths` | one or more repo-relative paths | in-scope discovery | Narrows prose scope for local debugging only; CI never uses it. Combining `--paths` with `--check commit` is a usage error that exits 2, matching the `--mode tree` plus `--check commit` rule below |
| `--rules` | path | `tools/prose/prose_rules.yaml` | Rule data file, validated at load |
| `--version` | flag | off | Prints the gate version and the rule-data `version` field |

`--mode tree` combined with `--check commit` is a usage error and exits 2:
commit checking needs a range, and inventing a whole-history default would
fail on pre-gate history (R-11, immutability).

### Exit codes

| Code | Meaning | Basis |
| --- | --- | --- |
| 0 | No findings. Also the result of an empty range and of an all-excluded path set | FR-018, `spec.md:110` |
| 1 | At least one finding | FR-018 |
| 2 | Usage error, unreadable or invalid rule data, git missing or failing, or a range that cannot be resolved | `tools/dbc/dbc_gate_common.py:10-14`, `cmake/coverage.cmake:6-13`, R-07 |

Exit 2 is never collapsed into 0. A run that could not determine what to
check has not checked anything, and reporting success would be the vacuous
pass the repository treats as a defect.

The table above is the gate script's own status. The CMake wrapper is
coarser: `cmake -P cmake/prose-lint.cmake` turns any nonzero script status
into a CMake `FATAL_ERROR` with process exit 8, so callers of the wrapper
cannot distinguish 1 from 2. Exit 2 observability is therefore limited to
direct `python3` runs and to the CI event-resolution shell step.

### Output contract

Findings go to stderr, one per line, ordered by location then rule id:

```text
specs/002-prose-commit-lint/spec.md:12: XI1.EMDASH family=XI.1 '\u2014': use a colon, semicolon, comma, or parentheses (Principle XI.1)
AGENTS.md:44: XI5.MARKETING family=XI.5 'robust': a performance claim carries a number, a platform, and a distribution (Principle XI.5)
commit 1a2b3c4: CM.FOOTER-APPROVAL: missing Approved-by footer (Pull Request Quality)
```

The summary line goes to stdout and always prints, clean or not:

```text
prose-lint: 60 sources, 2143 units examined, 3 findings, 0 skipped
```

Fields: sources examined, units examined, findings raised, sources
skipped. The count of sources examined is what US1 scenario 7 asserts.

**Skipped sources** print one line each to stderr before the summary:

```text
tools/dbc/overhead.cpp: skipped (invalid UTF-8 at byte 812)
```

which is the `spec.md:111` requirement to handle it without a crash and to
say why.

### CI annotations

When the environment sets `GITHUB_ACTIONS=true`, each prose finding
additionally emits a workflow command so the finding annotates the diff in
the pull request UI:

```text
::error file=specs/002-prose-commit-lint/spec.md,line=12::XI1.EMDASH use a colon, semicolon, comma, or parentheses (Principle XI.1)
```

Commit findings have no file or line to annotate and print as plain error
lines. Annotation emission is the only behavior in the program that depends
on the environment, so the finding set and ordering are identical with and
without it.

## 2. CMake script module

`cmake/prose-lint.cmake`, invoked in script mode, following the
`cmake/spell.cmake:1-7` `default()` and exit-code-mapping style:

```sh
cmake -P cmake/prose-lint.cmake
cmake -D PROSE_MODE=range -D PROSE_BASE="$base" -D PROSE_HEAD="$head" -P cmake/prose-lint.cmake
cmake -D PROSE_MODE=tree -P cmake/prose-lint.cmake
```

CMake script mode silently ignores `-D` arguments written after `-P`, so
every `-D` definition must precede `-P` in the invocations above. That the
values land is verifiable: `cmake -D PROSE_RULES=/nonexistent.yaml -P
cmake/prose-lint.cmake` exits nonzero, because unreadable rule data is a
usage error, which proves the `-D` value reached the script; the same
command with `-D` written after `-P` exits 0, documenting the trap.

| Variable | Default | Contract |
| --- | --- | --- |
| `PROSE_MODE` | `range` | Passed to `--mode` |
| `PROSE_BASE` | unset | Passed to `--base` when defined |
| `PROSE_HEAD` | `HEAD` | Passed to `--head` |
| `PROSE_CHECK` | `all` | Passed to `--check` |
| `PROSE_RULES` | `tools/prose/prose_rules.yaml` | Passed to `--rules` |
| `PYTHON_COMMAND` | `python3` | Interpreter, mirroring how `SPELL_COMMAND` names the tool in `cmake/spell.cmake:9` |

The module runs `execute_process` at `CMAKE_SOURCE_DIR` and maps the exit
code with `message(FATAL_ERROR ...)` for nonzero results, the way
`cmake/spell.cmake:23-29` maps codespell's 64 and 65. There is no `FIX=YES`
mode: the gate has no auto-fix, and adding a `FIX` flag that did nothing
would be a misleading surface.

## 3. CMake target

`cmake/prose-lint-targets.cmake`, included from `cmake/dev-mode.cmake`
beside `lint-targets.cmake` and `spell-targets.cmake`
(`cmake/dev-mode.cmake:19-20`), defines:

```sh
cmake --build build/dev -t prose-lint
```

which re-invokes the script module in `range` mode. It exists for the
developer who works inside a configured build directory; the script module
exists for CI and for a checkout with no build directory. Neither is
preferred over the other, because both resolve to the same program
arguments.

## 4. Parity rule

The contract that US3 scenario 3 tests: for identical repository state,
identical range, and identical rule data, the local command and the CI job
produce byte-identical output apart from the optional `::error` annotation
lines. Therefore:

- No CI-only rule set, no CI-only exclusions, no CI-only severity.
- Rule data is read from the tracked file at the checked-out commit, so the
  pull request's own rule data edit is what runs.
- Discovery uses `git ls-files` at the checked-out commit, so untracked
  scratch files never change a verdict in either direction.
- Ordering is by location then rule id, with no map iteration order leaking
  into output.

## 5. Requirement mapping

| Requirement | Contract element |
| --- | --- |
| FR-017 report names source, rule id, token, family | Finding line format |
| FR-018 binary exit | Exit codes table |
| FR-019 one documented local command | Section 2 and 3, documented in `quickstart.md` |
| FR-005 marker with mandatory reason | `MARKER.NO-REASON` finding line, data-model entity |
| FR-007, FR-008 modes | `--mode range` and `--mode tree` |
| FR-015 prose rules over commit text | `commit` findings carry the prose rule id, for example `commit 1a2b3c4: XI1.EMDASH` |
| FR-021, SC-006 no new dependency | Only `python3`, `git`, and PyYAML appear anywhere in the contract |

Marker grammar is defined in `contracts/rule-data.md`. The
`MARKER.NO-REASON` built-in (family XI.6, constitution reference
Principle XI.6) is defined in `contracts/rule-data.md`.
