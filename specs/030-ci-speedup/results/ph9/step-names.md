# Phase 9 Workflow Step Names

**Surface**: the 61 step names of `.github/workflows/ci.yml` that
mention install, analyzers or packages, and the 16 steps that carry no
name (T050, FR-037).

**Head sha**: 24d9814, where `.github/workflows/ci.yml` is 1243 lines.
The rocky block of T048 and the apt block of T049 sit in the working
tree, which is 1459 lines, and every line number below is read from that
tree copy.

**Run id**: local, plus run 40242956401 of branch `030-nocache-evidence`
for the readings that only a runner can give.

**Command**: the three checks under "The checks", the two listings under
"The steps", the `grep -n` reading under "The same lines, printed", and
the `gh run view` readings under "What gh reports".

**Numbers**: 13 jobs before and after, 212 steps, 61 named steps
matching the three words, 22 of them running an install command, 16
steps carrying no name, 0 of those 16 installing anything, 0 lines
changed by this task.

T051 writes the install step times of every run (tasks.md:241 to 242),
and FR-037 at spec.md:759 to 760 asks that every run record the time of
each install step:

```text
759:- **FR-037**: Every run shall record the time of each install step, and a
760:  cache restore miss shall leave the verdict unchanged.
```

Those times reach a reader through the step name, so T050 holds the two
preconditions: a name on every install step, and an install step that
prints only its own progress.

## The checks

```text
$ python3 -c "import yaml; d=yaml.safe_load(open('.github/workflows/ci.yml')); missing=[j for j,v in d['jobs'].items() for s in v['steps'] if 'install' in (s.get('name') or '').lower() and 'name' not in s]; print('unnamed install steps:', missing)"
unnamed install steps: []

$ python3 -c "import yaml,re; d=yaml.safe_load(open('.github/workflows/ci.yml')); bad=[(j,s.get('uses') or s.get('run')) for j,v in d['jobs'].items() for s in v['steps'] if 'name' not in s and re.search(r'apt-get install|dnf |pip3 install|cmake --install', s.get('run') or '')]; print('unnamed steps that install:', bad)"
unnamed steps that install: []

$ python3 -c "import yaml; d=yaml.safe_load(open('.github/workflows/ci.yml')); print('jobs:', len(d['jobs'])); print('steps:', sum(len(v['steps']) for v in d['jobs'].values()))"
jobs: 13
steps: 212
```

The first check is the one the task names. The second is the stronger
form: it drops the name filter and asks whether any step without a name
runs an install command, so it catches an installer that arrives
unnamed. It prints an empty list too. The third gives the job count, 13
before this task and 13 after, since this task edits nothing.

## The steps

Every step whose name mentions install, analyzers or packages, in file
order. `job` is the job id, `step` its index in that job, `line` the
line holding the name, `kind` what the step runs, and `own lines`
whether the body prints a line of its own.

```text
job                   step  line  kind         own lines  name
lint                     4    29  pip install  quiet      Install codespell and clang-format 18
coverage                 2    69  other        quiet      Prepare apt package cache
coverage                 3    75  cache action quiet      Restore apt package cache
coverage                 4    82  apt install  quiet      Install LCov
sanitize                 2   163  other        quiet      Prepare apt package cache
sanitize                 3   169  cache action quiet      Restore apt package cache
sanitize                 4   176  apt install  quiet      Install clang, Doxygen, and python3-yaml
tsan                     2   246  other        quiet      Prepare apt package cache
tsan                     3   252  cache action quiet      Restore apt package cache
tsan                     4   259  apt install  quiet      Install clang, Doxygen, and python3-yaml
static-analysis          2   338  other        quiet      Prepare apt package cache
static-analysis          3   344  cache action quiet      Restore apt package cache
static-analysis          4   351  apt install  quiet      Install analyzers
test                     2   469  other        quiet      Prepare apt package cache
test                     3   475  cache action quiet      Restore apt package cache
test                     4   482  apt install  quiet      Install Doxygen and python3-yaml
test                    11   506  cmake install quiet      Install
test                    12   508  other        quiet      Audit install tree for hwloc
test                    13   510  other        quiet      Audit package files for hwloc
test                    14   512  other        quiet      Audit install tree for simdjson
test                    15   514  other        quiet      Audit package files for simdjson
test                    16   516  other        quiet      Audit install tree for hdrhistogram
test                    17   518  other        quiet      Audit package files for hdrhistogram
test                    18   520  other        quiet      Audit install tree for zlib
test                    19   522  other        quiet      Audit package files for zlib
test                    20   524  other        quiet      Audit install tree for yaml-cpp
test                    21   526  other        quiet      Audit package files for yaml-cpp
test                    22   533  other        quiet      Audit install tree for quill
test                    23   535  other        quiet      Audit package files for quill
test-rocky               1   582  dnf install  quiet      Install dnf bootstrap packages
test-rocky               2   587  other        own lines  Resolve dnf package cache key
test-rocky               3   592  cache action quiet      Restore dnf package cache
test-rocky               4   598  dnf install  quiet      Install toolchain
test-rocky              12   628  cmake install quiet      Install
shared-audit             2   657  other        quiet      Prepare apt package cache
shared-audit             3   663  cache action quiet      Restore apt package cache
shared-audit             4   670  apt install  quiet      Install autotools and audit tools
shared-audit            11   694  cmake install quiet      Install
downstream-consumer      2   800  other        quiet      Prepare apt package cache
downstream-consumer      3   806  cache action quiet      Restore apt package cache
downstream-consumer      4   813  apt install  quiet      Install autotools, libhwloc-dev, libsimdjson-dev, zlib1g-dev, and build tools
downstream-consumer      7   833  cmake install quiet      Install a system HdrHistogram_c for the consumer premise
downstream-consumer      8   843  cmake install quiet      Install a system yaml-cpp for the consumer premise
downstream-consumer      9   856  cmake install quiet      Install a system quill for the consumer premise
downstream-consumer     10   864  other        quiet      Zero ccache before Install library
downstream-consumer     11   866  cmake install quiet      Install library
downstream-consumer     12   870  other        own lines  ccache stats after Install library
downstream-consumer     18   896  other        own lines  Compare the installed count against the build tree's
consumer-release         2   999  other        quiet      Prepare apt package cache
consumer-release         3  1005  cache action quiet      Restore apt package cache
consumer-release         4  1012  apt install  quiet      Install Doxygen and python3-yaml
dbc-gate                 2  1235  other        quiet      Prepare apt package cache
dbc-gate                 3  1241  cache action quiet      Restore apt package cache
dbc-gate                 4  1248  apt install  quiet      Install Doxygen and python3-yaml
prose-lint               2  1314  other        quiet      Prepare apt package cache
prose-lint               3  1320  cache action quiet      Restore apt package cache
prose-lint               4  1327  apt install  quiet      Install PyYAML, Doxygen, and lcov
docs                     3  1426  pip install  quiet      Install m.css dependencies
docs                     4  1434  other        quiet      Prepare apt package cache
docs                     5  1440  cache action quiet      Restore apt package cache
docs                     6  1447  apt install  quiet      Install Doxygen
```

22 of the 61 run an install command: 11 with `apt-get install`, 7 with
`cmake --install`, 2 with `dnf -y install`, 2 with `pip3 install`. 12
restore a package cache through `actions/cache@v4`, 11 prepare the apt
archives directory, 12 audit the install tree and the package files of
the `test` job, and the last 4 resolve a cache key, zero the ccache
counters, print the ccache stats and compare the installed catalog
count.

## What the steps print

The 22 installer bodies hold no `echo`, no `printf`, no `cat`, no
listing command and no `set -x`. Each one is the install command alone,
or the configure, build and install chain in one line, as `Install
library` at line 866 shows. So the output of an install step is what the
tool itself prints.

A runner gives the reading. The `coverage` job of run 40242956401, job
id 121318613752, prints 414 log lines for `Install LCov`:

```text
239  apt install progress (Selecting, Unpacking, Setting up, Fetched)
 79  already-the-newest-version lines and state information
 76  apt download lines (Hit, Get, Ign)
  7  blank separators
  4  environment variables the runner lists before the command
  3  apt list progress (Reading package lists, Building dependency tree)
  3  the needrestart trailer that apt's hook appends
  2  runner group markers
  2  runner headers (shell, env)
414  total
```

Those lines come from apt, from `needrestart` behind apt's own hook, or
from the runner's step scaffolding. A step authored line is absent,
which is the second clause of the task: an install step prints its own
progress and carries nothing else.

Three of the 61 named steps print a line of their own, and each is a
measurement step with a reason: `Resolve dnf package cache key` sends a
`printf` into `GITHUB_OUTPUT` and leaves stdout alone, `ccache stats
after Install library` runs `ccache -s` to publish the hit rate, and
`Compare the installed count against the build tree's` echoes the two
counts it matched. The 12 audit steps print nothing while they pass,
because each one is a `test -z` over a `find` or a `grep -q`, as lines
509 and 511 show:

```text
509       run: test -z "$(find prefix/ -iname '*hwloc*')"
511       run: "! grep -qi hwloc prefix/lib/cmake/speedgun-ng/*.cmake"
```

## The same lines, printed

The same names come out of `grep -nE` over the pattern `name:
.*(install|Install|analyz|Analyz|package|Package)`. First rows and last
rows:

```text
29:    - name: Install codespell and clang-format 18
69:    - name: Prepare apt package cache
75:    - name: Restore apt package cache
82:    - name: Install LCov
... 55 rows in between, in the same file order ...
1434:    - name: Prepare apt package cache
1440:    - name: Restore apt package cache
1447:    - name: Install Doxygen
```

That grep prints 62 lines. 61 are step names, matching the listing above
row for row. The odd one is line 442, the artifact name
`analyzer-findings-${{ matrix.scope }}` inside the `with` block of an
upload step, an artifact label that belongs to no step.

## What gh reports

`gh run view` exposes steps under `jobs`, so the read is `--json jobs`
with a step projection. From run 40242956401:

```text
$ gh run view 40242956401 --json jobs -q '.jobs[] | select(.name=="coverage") | .steps[] | "\(.number)\t\(.name)"'
1       Set up job
2       Run actions/checkout@v4
3       Install LCov
4       Record compiler version
...

$ gh run view 40242956401 --json jobs -q '.jobs[] | select(.name=="coverage") | .steps[] | select(.name=="Install LCov")'
{"completedAt":"2026-10-25T23:07:24Z","conclusion":"success","name":"Install LCov","number":3,"startedAt":"2026-10-25T23:07:11Z","status":"completed"}
```

Two readings matter. A named step reaches the reader with its yaml name
verbatim, and every step carries `startedAt` and `completedAt`, the pair
T051 turns into a duration. An unnamed step still gets a label: gh
prints `Run actions/checkout@v4` for a bare `uses` step, and the unnamed
ctest step of `prose-lint` reads `Run ctest --test-dir build/dev`
followed by the rest of its command line, so that label carries the
whole command. A missing name costs readability and nothing else, which
is why this task adds none.

## The steps with no name

16 of the 212 steps carry no `name:`. None of them installs a package.

```text
job                   step  line  what it runs
lint                     1    17  actions/checkout@v4
lint                     2    20  actions/setup-python@v5
coverage                 1    61  actions/checkout@v4
sanitize                 1   155  actions/checkout@v4
tsan                     1   238  actions/checkout@v4
static-analysis          1   330  actions/checkout@v4
test                     1   462  actions/checkout@v4
test-rocky               5   606  actions/checkout@v4
shared-audit             1   650  actions/checkout@v4
downstream-consumer      1   793  actions/checkout@v4
consumer-release         1   991  actions/checkout@v4
dbc-gate                 1  1227  actions/checkout@v4
prose-lint               1  1305  actions/checkout@v4
prose-lint              13  1386  run: ctest --test-dir build/dev -R prose_gate
docs                     1  1420  actions/checkout@v4
docs                     2  1423  actions/setup-python@v5
```

13 are `actions/checkout@v4`, 2 are `actions/setup-python@v5`, and 1 is
the `prose_gate_fixtures` ctest run at line 1386. All three kinds
already reach the reader through the label gh derives, each one unique
inside its job, so the projection names every step of every job.

## What changed

`.github/workflows/ci.yml` left this task with the same bytes it arrived
with, and no step was added: every install step already carries a name,
which is the condition the task sets for leaving the file alone. The
working tree still holds the 219 added and 3 removed lines of the T048
rocky block and the T049 apt block. `README.md`, `docs/tiers.md` and
`tools/docs/example_drift.py` stayed clean through the task, which is
the file set the other two agents hold.

## Outside this reading

The T049 pair has no runner reading yet: those steps exist in the
working tree only, so their printing rests on the script text, where the
`Prepare apt package cache` body holds two `sudo` commands that print
nothing on success and the restore is one `actions/cache@v4` call that
prints its own restore report. T051 reads both off a warm run.

The `static-analysis` job runs four matrix instances that share one step
list, so a step name repeats across those four job entries. gh separates
them by job name, printed as `static-analysis (library)`, `(tests)` and
`(examples)` in the same run, so a timing row is keyed on the job name
together with the step name.

The prose gate over this record:

```text
$ python3 /tmp/chk.py specs/030-ci-speedup/results/ph9/step-names.md
evaluate_prose -> (1, 243, 0, 0)
```

VERDICT: PASS
