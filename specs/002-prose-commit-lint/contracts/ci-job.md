# Contract: CI Job `prose-lint`

**Feature**: `002-prose-commit-lint` | [plan.md](../plan.md) | [cli.md](./cli.md)

One new job in `.github/workflows/ci.yml`, the ninth, following the
`dbc-gate` job conventions at `ci.yml:237-263`: `needs: [lint]`,
`runs-on: ubuntu-26.04`, no `if:`, no `paths` filter, so it runs on every
pull request targeting `master` and on every push to `master`.

## Shape

```yaml
  prose-lint:
    needs: [lint]
    runs-on: ubuntu-26.04
    steps:
      - uses: actions/checkout@v4
        with:
          fetch-depth: 0          # R-07: default 1 cannot support a range
      - name: Install PyYAML, Doxygen, and lcov
        run: sudo apt-get update -q && sudo apt-get install -q -y python3-yaml doxygen lcov
      - name: Resolve range
        env:
          EVENT_NAME: ${{ github.event_name }}
          PR_BASE: ${{ github.event.pull_request.base.sha }}
          PR_HEAD: ${{ github.event.pull_request.head.sha }}
          PUSH_BEFORE: ${{ github.event.before }}
          PUSH_AFTER: ${{ github.event.after }}
        run: |
          set -euo pipefail
          if [ "$EVENT_NAME" = pull_request ]; then
            base="$PR_BASE"; head_sha="$PR_HEAD"
          else
            base="$PUSH_BEFORE"; head_sha="$PUSH_AFTER"
            if ! git cat-file -e "${base}^{commit}" 2>/dev/null \
               || ! git merge-base --is-ancestor "$base" "$head_sha"; then
              first="${{ github.event.commits[0].id }}"
              base="${first}^"
            fi
          fi
          git cat-file -e "${base}^{commit}" \
            || { echo "::error::cannot resolve a base commit"; exit 2; }
          if [ "$(git rev-parse "$base")" = "$(git rev-parse "$head_sha")" ]; then
            echo "::error::resolved range is empty (base equals head)"
            exit 2
          fi
          echo "PROSE_BASE=$(git rev-parse "$base")" >> "$GITHUB_ENV"
          echo "PROSE_HEAD=$(git rev-parse "$head_sha")" >> "$GITHUB_ENV"
      - name: Prose and commit lint
        run: >
          cmake -D PROSE_MODE=range
          -D PROSE_BASE="${PROSE_BASE}"
          -D PROSE_HEAD="${PROSE_HEAD}"
          -P cmake/prose-lint.cmake
      - name: Configure
        run: cmake --preset=ci-ubuntu -B build/dev
      - run: cmake --build build/dev -t prose-lint-fixtures || true
      - run: ctest --test-dir build/dev -R prose_gate_fixtures
             --output-on-failure --no-tests=error
```

The `Resolve range` step exports `PROSE_BASE` and `PROSE_HEAD` through
`$GITHUB_ENV`; its logic is the event resolution below. The build step's
`|| true` keeps configure and build noise from masking a fixture failure:
the `ctest` step re-runs the fixture harness and is the fixture verdict.
The job runs the runner's distribution `python3` with the apt
`python3-yaml` package (decision D1), plus `doxygen` and `lcov`:
the `ci-ubuntu` preset enters developer mode, whose
`cmake/dbc-gate.cmake` and `cmake/coverage.cmake` fail configure
at include time without their tools on PATH. It does not use
`actions/setup-python`: that step belongs to the `lint` job
(`ci.yml:19-20`) and installs a standalone CPython without PyYAML, so
importing it here would break the `yaml` import. The fixture steps follow
the `dbc-gate` pattern of configure, build, then `ctest -R <fixtures>
--output-on-failure --no-tests=error` (`ci.yml:248-260`).

## Event input resolution

| Event | Left edge | Right edge | Notes |
| --- | --- | --- | --- |
| `pull_request` | `github.event.pull_request.base.sha` | `github.event.pull_request.head.sha` | SHAs from the event, so refs moving during review cannot change what was checked; `git merge-base` of the two defines the diff edge |
| `push` to `master` | `github.event.before` | `github.event.after` | Checks the landing commit, which is how a squash result is checked separately from the commits it collapsed (FR-016) |
| `push` with all-zeros or unresolvable `before`, new branch, or force-push | `github.event.commits[0].id^`, the parent of the first pushed commit; if `commits` is empty or that parent is unresolvable (`git cat-file -e` fails), the job exits 2 | `github.event.after` | Fail loudly: an unresolved range must never be reported as an empty range that passes (`cmake/coverage.cmake:6-13`, R-07) |

The resolution step prints the resolved `BASE`, `HEAD`, the merge-base, and
the commit count, so a reviewer reading the log can tell at a glance that
the range was real. The commit count comes from `git rev-list BASE..HEAD`,
two-dot enumeration, so base-side-only commits never enter the checked set
while the prose diff stays three-dot `BASE...HEAD` (R-07 amendment). A
count of zero commits is printed as `range resolved, 0 commits` and exits 0
only when the range resolved cleanly (`spec.md:110`). A resolved range whose
`BASE` equals `HEAD` exits 2 with a message: a vacuous pass is never taken
in CI; locally the same condition is a warning, per [cli.md](./cli.md).

## Checkout requirement

`fetch-depth: 0` is not optional. Quoted from the tool's README (doc):
"Number of commits to fetch. 0 indicates all history for all branches and
tags. Default: 1" and "Only a single commit is fetched by default, for the
ref/SHA that triggered the workflow. Set `fetch-depth: 0` to fetch all
history for all branches and tags." Measured on this repository, a
`--depth 1` clone reports `git rev-list --count HEAD` as 1 and fails
`git merge-base HEAD HEAD~1` with `fatal: Not a valid object name HEAD~1`.
A range check against the default checkout therefore either errors or, if
errors are swallowed, silently checks nothing.

## Dependency position

`needs: [lint]` places the job beside `coverage`, `sanitize`, `test`,
`test-rocky`, `consumer-release`, and `dbc-gate`, which all wait on the
`lint` root (`ci.yml` job graph). Nothing depends on `prose-lint`, so it
adds latency to no other job. It installs `python3-yaml`,
`doxygen`, and `lcov` through apt, packages existing jobs already
install (python3-yaml at `ci.yml:48`, `81`, `110`, `156`, `247`;
doxygen and lcov in every job that configures with `ci-ubuntu`),
and nothing through pip, so SC-006 holds: no runner installs
anything beyond what existing jobs already install.

## Failure behavior

| Condition | Result |
| --- | --- |
| Prose findings on added or modified lines | Job fails: the gate step runs through `cmake -P`, so findings surface as a CMake `FATAL_ERROR` and process exit 8, with one `::error file=,line=` annotation per finding, so the finding appears on the diff line that caused it |
| Commit-template findings | Job fails through the same wrapper (process exit 8), findings printed with short hashes, no annotation available since a commit has no diff line |
| Rule data invalid, git unavailable, or range unresolved | Job fails: through the gate step as a CMake `FATAL_ERROR` (process exit 8), through the `Resolve range` step as exit 2, always with a message naming the offending input, never reported as success. The 1-versus-2 exit distinction is observable only through direct `python3` invocation ([cli.md](./cli.md)) |
| Empty range or all paths excluded | Job succeeds, summary reports `0 commits` or `0 sources` explicitly |
| Fixture failure | Job fails from the `ctest` step independently of the lint step, so a broken gate is distinguishable from a violating pull request |

## Gate closure and the companion amendment

FR-022 and SC-005 require the same landing change to amend the constitution:
resolve both Sync Impact Report deferrals, name the delivered check in
Principle XI.6 in place of the deferral sentence, add the gate to Principle
VIII's gate list, and add the version lineage row for 2.5.0. The CI job and
the amendment land in one pull request, so there is no window in which the
tree enforces a gate the constitution does not list, and no window in which
the constitution lists a gate the CI does not run.

## Reproduction by a developer

The job's steps are the commands a developer runs, which is Principle VIII's
traceability requirement that CI artifacts be re-creable interactively with
the same presets:

```sh
git fetch origin master
base=$(git merge-base origin/master HEAD)
cmake -D PROSE_MODE=range -D PROSE_BASE="$base" -D PROSE_HEAD=HEAD -P cmake/prose-lint.cmake
cmake --preset=dev -B build/dev && cmake --build build/dev -t prose-lint-fixtures
ctest --test-dir build/dev -R prose_gate_fixtures --output-on-failure --no-tests=error
```

The two differences from CI are the base resolution, performed by hand where
CI reads it from the event payload, and `--preset=dev` where CI uses
`--preset=ci-ubuntu`, the same pair of presets the DBC gate job uses.

## Requirement mapping

| Requirement | Contract element |
| --- | --- |
| FR-007 added and modified lines only | `range` mode with the merge-base edge |
| FR-016 pull-request range plus newly pushed commits | Event input resolution table |
| FR-019 one command, documented | Reproduction block, mirrored in `quickstart.md` |
| FR-020 Linux job on pull request and push, existing conventions, re-creable | Job shape, `needs: [lint]`, reproduction block |
| FR-021, SC-006 no new dependency | apt `python3-yaml`, `doxygen`, and `lcov`, all already installed by existing jobs; no pip, no Node, no new binary |
| FR-023 fixtures run inside the gate | `ctest -R prose_gate_fixtures` step |
| SC-001 refused before merge | Landing marks the job a required status check in branch protection, same standing as `dbc-gate` |
