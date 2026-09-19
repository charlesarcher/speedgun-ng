# Quickstart: Prose and Commit-Message Lint Gate

**Feature**: `002-prose-commit-lint` | [plan.md](./plan.md) | [contracts/](./contracts/)

This is the run and validation guide. It contains no implementation code:
program internals live in `tasks.md` and the implementation, and the data
shape lives in [contracts/rule-data.md](./contracts/rule-data.md).

## Prerequisites

| Requirement | Check | Notes |
| --- | --- | --- |
| Python 3.12 or newer | `python3 --version` | 3.12 is the floor and the CI interpreter: the `prose-lint` job runs the runner's distribution `python3`, it does not use `actions/setup-python` (decision D1) |
| PyYAML | `python3 -c 'import yaml'` | The package `python3-yaml`, apt-installed in CI. The DBC gates already require it, and their guard message says so (`tools/dbc/dbc_gate_common.py:65-70`) |
| git | `git --version` | Range mode, `diff -U1`, `show --numstat -M`, `rev-list` |
| CMake 3.20 or newer | `cmake --version` | Only for the `cmake -P` and target entry points, never for the Python executable directly |
| A full clone | `git rev-list --count HEAD` greater than 1 | A shallow clone cannot resolve a range. If it is shallow: `git fetch --unshallow` |

## Run the gate locally

The documented single command (FR-019). It checks prose and commits over the
range from the `origin/master` merge-base to `HEAD`, which is what CI will
check:

```sh
cmake -P cmake/prose-lint.cmake
```

Clean result:

```text
prose-lint: 60 sources, 2143 units examined, 0 findings, 0 skipped
```

Result with findings, on stderr:

```text
AGENTS.md:44: XI5.MARKETING family=XI.5 'robust': a performance claim carries a number, a platform, and a distribution (Principle XI.5)
prose-lint: 60 sources, 2143 units examined, 1 finding, 0 skipped
```

Exit status is 1 with findings, 0 without, 2 for a usage error, an invalid
rule file, a missing git, or a range that cannot be resolved (FR-018).

Equivalent invocations, all reaching the same program arguments:

```sh
python3 tools/prose/prose_gate.py                                    # direct
cmake --build build/dev -t prose-lint                                # inside a build dir
```

### Why range mode is the default

Whole-file mode reports the tree as it stands, and the tree carries 225
pre-existing em-dash occurrences across 12 files, 209 of them in the
`specs/001-dbc-facility/` artifacts. Reporting them would fail every pull
request for text its author did not write, which Principle XI.1 explicitly
excludes. Whole-file mode exists for the tree-wide sweep that Principle V
schedules separately, and it stays opt-in:

```sh
cmake -D PROSE_MODE=tree -P cmake/prose-lint.cmake
```

SC-003's bound is measured with this mode: 60 in-scope files (15 Markdown,
45 source, counted by the canonical command recorded in
[plan.md](./plan.md)) in about 0.062 s on this machine against a 10 s
budget; the tree moves, the command is the source of truth.

## Check prose or commits alone

```sh
cmake -D PROSE_CHECK=prose -P cmake/prose-lint.cmake
cmake -D PROSE_CHECK=commit -P cmake/prose-lint.cmake
python3 tools/prose/prose_gate.py --check commit --base v0.1.0 --head HEAD
```

Commit mode needs a range, so combining it with tree mode exits 2.

## Run the gate's own fixtures

FR-023 requires the fixtures to run inside the gate, and SC-001, SC-002, and
SC-004 are measured by them.

```sh
cmake --preset=dev -B build/dev
cmake --build build/dev -t prose-lint-fixtures
ctest --test-dir build/dev -R prose_gate_fixtures --output-on-failure --no-tests=error
```

What they assert, per rule family, in both directions:

| Family | Must be reported | Must stay silent |
| --- | --- | --- |
| XI.1 | an em-dash in prose; ASCII `--` in prose | `P0–P3` style numeric ranges using the en-dash |
| XI.2 | `, not ` and ` rather than ` constructions | a load-bearing distinction stated in two sentences |
| XI.3 | a voucher word | `candidate` and `adjust`, the longer-word cases at `spec.md:53` and `spec.md:103` |
| XI.4 | a self-describing artifact sentence | a plain statement of content |
| XI.5 | a filler word and a marketing word | the same words inside a code span |
| Commit template | one violation per rule: title format, title length 51, unknown section, non-imperative title, trailing period (`CM.TITLE-FORMAT`), missing body at 40 changed lines, missing `Approved-by:`, vague title, wrapped body over 72 columns | a conforming commit; a body-less commit at 3 changed lines; a marked body line that fits 72 columns once the marker is stripped |
| Exemptions | a marker with an empty reason | a marker with a reason; a fenced block; an inline code span; a URL; a shell command line; a comment that is a license header |

The harness builds its commit fixtures in a throwaway repository under
`/tmp`, so no malformed commit ever enters this repository's history, which
is immutable once merged.

Expected result: `100% tests passed, 0 tests failed out of 1`, and the
harness prints one assertion line per case.

## Reproduce the CI job exactly

```sh
git fetch origin master
base=$(git merge-base origin/master HEAD)
cmake -D PROSE_MODE=range -D PROSE_BASE="$base" -D PROSE_HEAD=HEAD -P cmake/prose-lint.cmake
ctest --test-dir build/dev -R prose_gate_fixtures --output-on-failure --no-tests=error
```

The only differences from CI are that a developer resolves the base by hand
where CI reads it from the event payload, and that CI configures with
`--preset=ci-ubuntu` while a developer uses `--preset=dev`. The finding set,
its ordering, and its text are identical, which is the parity claim in
[contracts/cli.md](./contracts/cli.md) and US3 scenario 3.

## Load-validate the rule data

```sh
python3 tools/prose/prose_gate.py --check prose --paths README.md
```

Any load-time validation failure exits 2 and names the field, including a
wildcard pattern, a missing canonical rule id, an empty `sections` list, or
an out-of-bounds threshold. The checks and their reasons are in
[contracts/rule-data.md](./contracts/rule-data.md). `--paths` narrows the
prose check; combining it with `--check commit` is a usage error that exits
2 ([contracts/cli.md](./contracts/cli.md)).

## Exempt one quoted line

Add the marker with a reason on the line itself. This is the P2 exception
site Principle X.2 and XI.6 require, so the reason is mandatory and an empty
one becomes a finding of its own (FR-005):

```c
int x = 1;  /* prose-lint: allow reason="verbatim quotation from the spec" */
```

```md
The gate bans the phrase. prose-lint: allow reason="quotation from Principle XI.2"
```

There is no block form. A region toggle would let a forgotten end marker
silently stop checking the rest of a file.

## Add a section token (FR-010)

Append one line to `sections` in `tools/prose/prose_rules.yaml`, in the
pull request that first uses the new token, keeping the case the token will
appear with in `git log`. The load-time checks must still pass, and a new
token needs no constitution change because Pull Request Quality defines the
section list as data the repository maintains.

## What green means

| Signal | Meaning |
| --- | --- |
| `prose-lint` job passes on a pull request | No discourse violation on any line the pull request adds or modifies, and every commit in its range satisfies the template |
| `prose_gate_fixtures` passes | The gate detects what it claims to detect, in both directions, per FR-023 |
| Exit 2 anywhere | The check did not run: fix the rule file, the range, or the environment. It is never a pass |
| Constitution 2.5.0 with both deferrals resolved | The Principle XI.6 machine check and the commit-template lint exist (FR-022, SC-005) |

## Out of scope for this guide

The tree-wide em-dash sweep, editor and pre-commit integrations, spell
checking, which stays with `spell-check`, and enforcement on interactive
chat replies, which leave no artifact to check.
