# Phase 9 Verification

**Surface**: the working tree copy of `.github/workflows/ci.yml` at 1459
lines, and the three phase 9 records in
`specs/030-ci-speedup/results/ph9/` (T048 to T050, FR-014, FR-035,
FR-036, FR-037).

**Head sha**: 7a7dc1f, with `HEAD~3` at 24d9814, the head the phase read
before it began. `.github/workflows/ci.yml` is one blob at both
revisions, `a156f015`, 1243 lines. `git diff --numstat` gives 219 added
and 3 removed lines for that file in the working tree.

**Run id**: local, on the Arch host. No runner ran.

**Command**: `python3 /tmp/ph9/check.py N` with N in 1 to 5, plus the
three gate commands of check 6, each from the repository root. The
script prints its own numbers for each check and both token files under
`/tmp/ph9`.

**Numbers**: 13 jobs at `HEAD~3` and 13 in the tree, 212 steps, 22
install steps, 34 cache steps, 0 unnamed among the 56, 11 apt jobs
touched, 92 package tokens before and after with an empty diff, 4 dnf
transactions before and after, 1 name added and 0 dropped, 3 records
with 5 of 5 header fields and 3 PASS verdicts, 3 gates at exit 0.

Phase 9 asks that package lists survive from one run to the next, so the
install steps stop deciding the wall time (tasks.md:233 to 234). The
phase touched the workflow in three tasks: the rocky dnf cache (T048),
the ubuntu apt archives cache (T049) and the step names (T050). Each
check below reads the tree against `HEAD~3` and prints what it counted.
Line numbers quoted below are `grep -n` readings of the working tree
copy, where the phase work sits.

## Check 1: the file parses and the job count holds

```text
$ python3 /tmp/ph9/check.py 1
tree: parse ok, jobs=13, lines=1459
HEAD: parse ok, jobs=13, lines=1243
HEAD~3: parse ok, jobs=13, lines=1243
job names equal: True
job count tree: 13 HEAD~3: 13
jobs: lint coverage sanitize tsan static-analysis test test-rocky
shared-audit downstream-consumer consumer-release dbc-gate prose-lint
docs
jobs whose content differs from HEAD~3: 12 coverage sanitize tsan
static-analysis test test-rocky shared-audit downstream-consumer
consumer-release dbc-gate prose-lint docs
```

Exit 0. `yaml.safe_load` reads all three copies. The count is 13 at
`HEAD~3` and 13 in the tree, and the 13 ids are the same list in the
same order. The `jobs` line above is wrapped here; the command prints it
as one line. Twelve job bodies differ from `HEAD~3`, which is the phase
work: every job with an install step. `lint` is the thirteenth and holds
its bytes, since it installs through `pip3` and has no apt archives to
keep. The count read at the start of the phase, 13, is the count at the
end of it.

The task statement names 1246 lines for this file at head. The measured
figure is 1243, by `wc -l` and by a newline count of the same bytes, at
both `HEAD` and `HEAD~3`. The difference sits in the statement.

## Check 2: the rocky lane caches a dnf path under a package list key

```text
$ python3 /tmp/ph9/check.py 2
cache steps (dnf path): [2] [{'path': '/var/cache/dnf',
  'key': 'dnf-rockylinux10-${{ steps.dnf-pkgs.outputs.key }}',
  'restore-keys': 'dnf-rockylinux10-'}]
bootstrap install step: [0] ['Install dnf bootstrap packages']
key step: [1] ['Resolve dnf package cache key']
the-rest install step: [3] ['Install toolchain']
bootstrap before the rest: True
cache between them: True
key input names both package lists: True
keepcache enabled in bootstrap step: True
DNF_BOOTSTRAP: tar zstd
DNF_PACKAGES: gcc-c++ cmake git python3 python3-pip python3-pyyaml
  make doxygen lcov gzip diffutils ccache autoconf automake libtool
  patch
dnf names: bootstrap 2 the rest 16 total with repos 20
HEAD~3 first five steps: ['Install toolchain', 'actions/checkout@v4',
  'Record compiler version', 'Restore ccache', 'Configure']
tree first five steps: ['Install dnf bootstrap packages', 'Resolve dnf
  package cache key', 'Restore dnf package cache', 'Install toolchain',
  'actions/checkout@v4']
dnf transactions: HEAD~3 4 tree 4
dnf install arguments: HEAD~3 22 tree 23
distinct names: HEAD~3 22 tree 23
names added: ['zstd']
names dropped: []
tar count HEAD~3/tree: 1 1
```

Exit 0. One cache step in `test-rocky`, step 2, over the dnf cache
directory `/var/cache/dnf`. The key names the package list through a
step that hashes it: line 591 holds
`echo "key=$(printf '%s' "$DNF_BOOTSTRAP $DNF_PACKAGES" | sha256sum |
cut -c1-16)" >> "$GITHUB_OUTPUT"`, so both lists feed the digest, and
line 596 reads `key: dnf-rockylinux10-${{ steps.dnf-pkgs.outputs.key }}`
with `restore-keys: dnf-rockylinux10-` at line 597. Repeating the digest
here over the two `env` values at lines 576 and 577 prints
`14743b10cb3e6167`, so the key moves when a list moves.

Order: the bootstrap install is step 0, ahead of the key step at 1, the
restore at 2 and `Install toolchain` at 3, which holds the rest of the
names. At `HEAD~3` that job opened with `Install toolchain`. The
`sed` at line 585 turns on `keepcache=1` in the same step as the
bootstrap install at line 586, so the RPMs are left behind for the
restore at line 595 to find. The `env` split is 2 bootstrap names and
16 toolchain names.

The package set: 4 `dnf` transactions at `HEAD~3` and 4 in the tree, 22
install arguments against 23, with `zstd` the one added name, nothing
dropped, and `tar` counted once on each side, the one name that changed
which step installs it.

## Check 3: the apt cache steps precede the installs and the lists hold

```text
$ python3 /tmp/ph9/check.py 3
coverage             prepare=[1] cache=[2] install=[3] before=True
sanitize             prepare=[1] cache=[2] install=[3] before=True
tsan                 prepare=[1] cache=[2] install=[3] before=True
static-analysis      prepare=[1] cache=[2] install=[3] before=True
test                 prepare=[1] cache=[2] install=[3] before=True
shared-audit         prepare=[1] cache=[2] install=[3] before=True
downstream-consumer  prepare=[1] cache=[2] install=[3] before=True
consumer-release     prepare=[1] cache=[2] install=[3] before=True
dbc-gate             prepare=[1] cache=[2] install=[3] before=True
prose-lint           prepare=[1] cache=[2] install=[3] before=True
docs                 prepare=[3] cache=[4] install=[5] before=True
uses=['actions/cache@v4'] path=['/var/cache/apt/archives'] for all 11
jobs with an apt install step: HEAD~3 11 tree 11
jobs touched by a cache step: 11
every cache step precedes its install: True
package tokens: HEAD~3 92 tree 92 distinct 14
diff exit: 0
diff output: (empty)
install step names and command bodies identical to HEAD~3: True
```

Exit 0. Eleven jobs install through `apt-get` at `HEAD~3` and the same
eleven in the tree. Each of the eleven carries a `Prepare apt package
cache` step and a `Restore apt package cache` step ahead of its install
step, indices 1, 2 and 3 in ten of them and 3, 4 and 5 in `docs`, where
two `actions` steps come first. Every cache step is
`actions/cache@v4` over `/var/cache/apt/archives`, and the file holds 11
lines reading `key: apt-${{ github.job }}-ubuntu-26.04-...`, the first
of them at line 79.

The package list check reads the sorted token list of every install
command in both copies, options dropped, into two files, and `diff -u`
over them exits 0 with no output. Both sides carry 92 tokens over 14
distinct names, and the install step names and command bodies are
identical to `HEAD~3` line for line, so the versions CI installs are the
ones it installed before.

## Check 4: no install step lacks a name

```text
$ python3 /tmp/ph9/check.py 4
steps: 212 named: 196 unnamed: 16
install or cache steps: 56 by kind: {'pip': 2, 'cache': 34, 'apt': 11,
  'cmake-install': 7, 'dnf': 2}
unnamed install or cache steps: 0 []
```

Exit 0. 212 steps across the 13 jobs, 196 of them named. Of those, 22
run an install command, 11 `apt-get install`, 7 `cmake --install`, 2
`dnf` install, 2 `pip3 install`, and 34 are `actions/cache` steps. The
22 and the 34 together are 56, and none of the 56 lacks a name. The 16
unnamed steps are 13 `actions/checkout@v4`, 2
`actions/setup-python@v5` and the `prose-lint` step that runs
`ctest --test-dir build/dev -R prose_gate_fixtures`, which installs
nothing.

## Check 5: the three records carry a header block and a verdict

```text
$ python3 /tmp/ph9/check.py 5
specs/030-ci-speedup/results/ph9/rocky-cache.md: lines=206
  headers=5/5 missing=[] verdict=['PASS']
specs/030-ci-speedup/results/ph9/apt-cache.md: lines=310
  headers=5/5 missing=[] verdict=['PASS']
specs/030-ci-speedup/results/ph9/step-names.md: lines=288
  headers=5/5 missing=[] verdict=['PASS']
records checked: 3
```

Exit 0, with the wrapped lines above printed as one line each. All three
files exist, each with `Surface`, `Head sha`, `Run id`, `Command` and
`Numbers` in its header block and exactly one line matching
`VERDICT: PASS` or `VERDICT: FAIL`, which reads PASS in all three.

## Check 6: the three source gates

```text
$ cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake
lint exit=0
$ cmake -P cmake/spell.cmake
spell exit=0
Used config files:
    1: .codespellrc

$ python3 tools/prose/prose_gate.py --check prose --base origin/master
prose-lint: 63 sources, 7592 units examined, 0 findings, 0 skipped
prose exit=0

$ python3 tools/prose/prose_gate.py --check prose --mode tree \
    --paths .github/workflows/ci.yml
prose-lint: 0 sources, 0 units examined, 0 findings, 0 skipped
prose exit=0

$ python3 /tmp/chk.py specs/030-ci-speedup/results/ph9/rocky-cache.md
evaluate_prose -> (1, 173, 0, 0)
$ python3 /tmp/chk.py specs/030-ci-speedup/results/ph9/apt-cache.md
evaluate_prose -> (1, 265, 0, 0)
$ python3 /tmp/chk.py specs/030-ci-speedup/results/ph9/step-names.md
evaluate_prose -> (1, 243, 0, 0)
$ python3 /tmp/chk.py specs/030-ci-speedup/results/ph9/verify.md
evaluate_prose -> (1, 241, 0, 0)
$ python3 /tmp/chk.py .github/workflows/ci.yml
evaluate_prose -> (0, 0, 0, 0)
```

Every gate exits 0. The format gate is the `Lint` step that the name at
ci.yml:32 declares, `cmake -D FORMAT_COMMAND=clang-format -P
cmake/lint.cmake`, and it printed nothing, which is its clean reading.
The spell gate is the `Spell check` step named at ci.yml:40, `cmake -P
cmake/spell.cmake`, which runs codespell over the tree from
`.codespellrc`. Its skip list covers build trees, `prefix` and
`external`, so `specs/030-ci-speedup/results/ph9/` sits inside what
codespell reads and the three records arrived at it with 0 hits.

The prose gate in its CI form, range mode against `origin/master`, read
63 sources and 7592 units with 0 findings and exit 0. The unit count
moves while sibling lanes edit tracked files, and the reading above is
the one this run took. Tree mode with `--paths` read 0 sources for
`.github/workflows/ci.yml`, because
`classify_source` holds no language for `.yml`, and 0 sources for the
three records, because tree mode works from `git ls-files` and the phase
9 files sit untracked in the tree. The single-path form of the same
engine reads a named path whatever its tracking, and the tuples above
are its per-file readings, with findings and skipped both 0 everywhere.

## Verdict

| Check | Asks | Command | Exit | Count read | Verdict |
| --- | --- | --- | --- | --- | --- |
| 1 | parses, job count holds | `check.py 1` | 0 | 13 and 13 | PASS |
| 2 | rocky dnf cache | `check.py 2` | 0 | 1 path, 0 dropped | PASS |
| 3 | apt cache order | `check.py 3` | 0 | 92, diff empty | PASS |
| 4 | install steps named | `check.py 4` | 0 | 22, none unnamed | PASS |
| 5 | records complete | `check.py 5` | 0 | 3, 15 fields | PASS |
| 6 | source gates | `lint spell prose` | 0 | 3, 0 findings | PASS |

`check.py N` is `python3 /tmp/ph9/check.py N` and the three gate
commands are printed in full under check 6.

VERDICT: PASS

## What these checks leave out

The checks read the yaml text and the tree. A warm run is the reading
they cannot give: the install step times, the cache hit rate and the
upload size of the new steps belong to T051, which takes them off the
runs FR-052 counts. `dnf` and `apt-get` ran on neither side of the
comparison here, since this host has no RPM toolchain and the ubuntu
runner image sits beyond it.

Two facts about the tree carry forward. The phase 9 workflow edits and
the three records are uncommitted: `ci.yml` differs from head by 219
added and 3 removed lines, and the record directory is untracked, so the
prose gate in its tracked modes reads none of those files and the
single-path readings above cover them. The working tree also holds
`test/CMakeLists.txt`, `example/CMakeLists.txt` and the phase
8 records from the lanes beside this one; the format, spell and prose
gates read them as part of the tree and arrived at exit 0 with them.
