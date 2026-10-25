# Phase 5 Test Job Configures Without the Analyzers

**Surface**: the `test` job of `.github/workflows/ci.yml` (T027).
**Head sha**: 4a8047a10d43fe582599db7583696f526a44e822
**Run id**: local
**Command**: the command block under Proof, each line run from the
repository root
**Numbers**: 3 changed lines in the `test` job, 0 analyzer names in it, 1
preset line, 4 `matrix.scope` references in the shard job

## The change

Two steps of the `test` job changed and nothing else did.

The install step keeps the eight packages the build needs and drops the
two analyzer packages from its `apt-get install` argument list. Its name
now matches the name the sibling jobs give the same package list.

```text
    - name: Install Doxygen and python3-yaml
      run: >-
        sudo apt-get update -q &&
        sudo apt-get install ccache doxygen python3-yaml lcov autoconf automake libtool patch -y -q
```

The configure step names the analyzer-free preset:

```text
    - name: Configure
      run: cmake --preset=ci-ubuntu-test
```

`ci-ubuntu-test` already exists in `CMakePresets.json`, at line 154. It
inherits `ci-build`, `ci-linux`, `dev-mode` and `ci-cache`, and it omits
the `clang-tidy` and `cppcheck` presets that `ci-ubuntu` inherits, so the
analyzer hooks leave the build while the compiler, the flags, the
developer mode and the ccache launcher stay. `ci-build` sets the binary
directory to `build`, so the Build, Install and Test steps keep the
paths they already spell. The preset also turns on
`SPEEDGUN_DBC_MATRIX`, the lane flag of phase 6.

Every other step keeps its exact text and its position. The step order
around the build is unchanged:

```text
19:    - name: Restore ccache
29:    - name: Configure
31:    - name: Zero ccache before Build
33:    - name: Build
35:    - name: ccache stats after Build
37:    - name: Install
```

The `Save ccache` step stays last in the job, gated on
`github.event_name == 'push'` as before.

## Proof

The first command extracts the `test` job alone, from its own key to the
`test-rocky` key, and counts the two analyzer tool names. The second
prints the preset line. Both ran against the working tree at the head
sha above. The branch moved twice while this node ran, from 88c649f to
6767c20 to 4a8047a, and neither move touched the `test` job.

```console
$ awk '/^  test:/{f=1} /^  test-rocky:/{f=0} f' .github/workflows/ci.yml \
    | grep -cE 'clang-tidy|cppcheck'
0
$ awk '/^  test:/{f=1} /^  test-rocky:/{f=0} f' .github/workflows/ci.yml \
    | grep -nE 'cmake --preset'
30:      run: cmake --preset=ci-ubuntu-test
```

Zero matching lines: neither analyzer name appears in any step of the job,
so no step installs either tool and no step runs either one. The single
preset line names `ci-ubuntu-test`.

## The shard job holds the analyzers

T026 owns the `static-analysis` job. It was absent when this node opened
and it arrived in the same working tree partway through this pass, so the
final state of the file carries it at line 255, uncommitted next to this
node's own 3 lines. The job is outside this node's scope and nothing in it
was touched here.

```console
$ grep -n 'static-analysis' .github/workflows/ci.yml
255:  static-analysis:
$ awk '/^  static-analysis:/{f=1} /^  test:/{f=0} f' .github/workflows/ci.yml \
    | grep -c 'matrix.scope'
4
```

The job declares `needs: [lint]`, a three-value `scope` matrix over
`library`, `tests` and `examples`, its own install step naming both
analyzer packages, and the `ci-ubuntu` configure that carries the analyzer
hooks. So the pair holds: the analyzers run in the sharded job and the
`test` job builds without them (FR-015, FR-045).
