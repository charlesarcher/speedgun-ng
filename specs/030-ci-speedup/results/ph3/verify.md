# Phase 3 ccache Wiring Verification

**Surface**: the ccache wiring of `.github/workflows/ci.yml` and
`CMakePresets.json`, read as it stands in the working tree. This record
changes no configuration file and reports what the commands print.
**Head sha**: 88c649f92f11564eb3fba07acced36e1f03e44ff
**Run id**: local
**Command**: the seven commands under checks 1 to 7, each run from the
repository root, plus the supporting readings named inside checks 1 and 7
**Numbers**: the per-job rows inside each check, the exit status under each
command, and the verdict table

The workflow holds twelve jobs and 1114 lines at this reading. Ten of
the twelve run `cmake --build`; `lint` and `docs` build nothing. Jobs are
read by name, never by line number, because other nodes edit the file.
`.github/workflows/ci.yml` is clean at the head sha above, so the
wiring these commands read is the committed one, and `CMakePresets.json`
is clean too.

```text
$ git status --porcelain
 M specs/030-ci-speedup/results/ph3/verify.md
?? specs/030-ci-speedup/results/ph4/timing.md
[exit status: 0]
```

## The reading that was followed for check 2

The task text for check 2 asks for one `ccache -z` and one `ccache -s` per
building job. spec.md FR-009 says each job prints `ccache -z` before its
build and `ccache -s` after it, and
`specs/030-ci-speedup/contracts/job-graph.md` binds the pair to every
`Build` step. Two jobs hold two build steps apiece, so a per-job count of
one is unreachable there while the contract rule is met. The contract
reading was followed, per the precedence order the task states: spec.md,
then the contract, then tasks.md. The per-job counts are printed under
check 2 so that a reader can weigh either reading.

## Check 1: cache step, `CCACHE_DIR` and `CCACHE_MAXSIZE`

```text
$ python3 -c "import yaml; d=yaml.safe_load(open('.github/workflows/ci.yml')); rows=[(n, any('cmake --build' in (s.get('run') or '') for s in d['jobs'][n]['steps']), any((s.get('uses') or '').startswith('actions/cache') for s in d['jobs'][n]['steps']), 'CCACHE_DIR' in (d['jobs'][n].get('env') or {}), 'CCACHE_MAXSIZE' in (d['jobs'][n].get('env') or {})) for n in sorted(d['jobs'])]; [print(r) for r in rows]; bad=[r[0] for r in rows if r[1] != (r[2] and r[3] and r[4])]; print('EXCEPTIONS', bad); raise SystemExit(1 if bad else 0)"
('consumer-release', True, True, True, True)
('coverage', True, True, True, True)
('dbc-gate', True, True, True, True)
('docs', False, False, False, False)
('downstream-consumer', True, True, True, True)
('lint', False, False, False, False)
('prose-lint', True, True, True, True)
('sanitize', True, True, True, True)
('shared-audit', True, True, True, True)
('test', True, True, True, True)
('test-rocky', True, True, True, True)
('tsan', True, True, True, True)
EXCEPTIONS []
[exit status: 0]
```

The columns hold the job name, whether a step runs `cmake --build`, whether
a step uses `actions/cache` (the halves named
`actions/cache/restore@v4` and `actions/cache/save@v4`), then the presence
of `CCACHE_DIR` and `CCACHE_MAXSIZE` in the job `env`. The ten building
jobs hold all three, `lint` and `docs` hold none, and the decisive line is
`EXCEPTIONS []`.

Supporting reading, the `CCACHE_DIR` value the ten jobs share:

```text
$ grep -c 'CCACHE_DIR: ${{ github.workspace }}/.ccache' .github/workflows/ci.yml
10
[exit status: 0]
```

Ten jobs set it to the workspace path that D-9 names, which is what covers
the rocky lane: that job runs inside a container whose home directory
differs from the runner's.

## Check 2: the `ccache -z` and `ccache -s` pair

```text
$ python3 - <<'PY'
$ import yaml
$ d = yaml.safe_load(open('.github/workflows/ci.yml'))
$ bad = []
$ for n, j in sorted(d['jobs'].items()):
$     st = j.get('steps', [])
$     runs = [(s.get('run') or '').strip() for s in st]
$     names = [s.get('name') for s in st]
$     if not any('cmake --build' in r for r in runs):
$         continue
$     z = [i for i, r in enumerate(runs) if r == 'ccache -z']
$     s = [i for i, r in enumerate(runs) if r == 'ccache -s']
$     print(n, 'zero=%d stats=%d' % (len(z), len(s)))
$     for i, r in enumerate(runs):
$         if 'cmake --build' not in r:
$             continue
$         before = i > 0 and runs[i - 1] == 'ccache -z'
$         after = i + 1 < len(runs) and runs[i + 1] == 'ccache -s'
$         print('   %-46s zero_before=%-5s stats_after=%s' % (names[i], before, after))
$         if names[i] and (names[i].startswith('Build') or names[i] == 'Install library'):
$             if not (before and after):
$                 bad.append(n + ':' + str(names[i]))
$     if len(z) != len(s):
$         bad.append(n + ':unpaired')
$     for i in z:
$         if i + 1 >= len(runs) or 'cmake --build' not in runs[i + 1]:
$             bad.append(n + ':zero-not-adjacent')
$     for i in s:
$         if i == 0 or 'cmake --build' not in runs[i - 1]:
$             bad.append(n + ':stats-not-adjacent')
$ print('TOTAL_ZERO', sum((s.get('run') or '').strip() == 'ccache -z'
$                         for j in d['jobs'].values() for s in j['steps']))
$ print('TOTAL_STATS', sum((s.get('run') or '').strip() == 'ccache -s'
$                          for j in d['jobs'].values() for s in j['steps']))
$ print('EXCEPTIONS', bad)
$ raise SystemExit(1 if bad else 0)
$ PY
consumer-release zero=2 stats=2
   Build consumer library                         zero_before=True  stats_after=True
   Build trap fixture and dbc_test                zero_before=True  stats_after=True
   Run counters_trap_checked under ignore         zero_before=False stats_after=False
coverage zero=1 stats=1
   Build                                          zero_before=True  stats_after=True
   Process coverage info                          zero_before=False stats_after=False
dbc-gate zero=1 stats=1
   Build dbc-gate                                 zero_before=True  stats_after=True
downstream-consumer zero=2 stats=2
   Install a system HdrHistogram_c for the consumer premise zero_before=False stats_after=False
   Install a system yaml-cpp for the consumer premise zero_before=False stats_after=False
   Install a system quill for the consumer premise zero_before=False stats_after=False
   Install library                                zero_before=True  stats_after=True
   Build consumer verbose                         zero_before=True  stats_after=True
prose-lint zero=1 stats=1
   Build prose-lint fixtures                      zero_before=True  stats_after=True
sanitize zero=1 stats=1
   Build                                          zero_before=True  stats_after=True
shared-audit zero=1 stats=1
   Build                                          zero_before=True  stats_after=True
test zero=1 stats=1
   Build                                          zero_before=True  stats_after=True
test-rocky zero=1 stats=1
   Build                                          zero_before=True  stats_after=True
tsan zero=1 stats=1
   Build                                          zero_before=True  stats_after=True
TOTAL_ZERO 12
TOTAL_STATS 12
EXCEPTIONS []
[exit status: 0]
```

Each job line gives its two counts, then every step that runs
`cmake --build` carries its name and whether the step before it holds
`ccache -z` and the step after it holds `ccache -s`. `TOTAL_ZERO 12` and
`TOTAL_STATS 12` over ten building jobs: eight jobs hold one pair, and
`consumer-release` and `downstream-consumer` hold two each because each
builds twice. `EXCEPTIONS []` says every step whose name starts with
`Build`, plus `Install library`, sits between its own pair, and that no
pair stands apart from a build.

Four `cmake --build` steps hold no pair. `coverage`
`Process coverage info` builds the `coverage` target, which collects the
trace through lcov and compiles nothing. The three
`Install a system ...` steps of `downstream-consumer` build `external/`
dependencies configured by a bare `cmake -S`, so no preset and no launcher
is in play there. `consumer-release`
`Run counters_trap_checked under ignore` rebuilds one target inside a test
step whose pair sits at the build step above it.

## Check 3: the save step and its condition

```text
$ python3 - <<'PY'
$ import yaml
$ d = yaml.safe_load(open('.github/workflows/ci.yml'))
$ bad = []
$ for n, j in sorted(d['jobs'].items()):
$     builds = any('cmake --build' in (s.get('run') or '') for s in j['steps'])
$     save = [s for s in j['steps'] if (s.get('uses') or '').startswith('actions/cache/save')]
$     if not builds:
$         bad += [n + ':unexpected-save'] if save else []
$         continue
$     cond = save[0].get('if')
$     v = lambda e: eval(cond.replace('github.event_name', repr(e)), {})
$     print(n, '| save if =', cond, '| push ->', v('push'), '| pull_request ->', v('pull_request'))
$     if len(save) != 1 or cond != "github.event_name == 'push'" \
$             or v('push') is not True or v('pull_request') is not False:
$         bad.append(n)
$ print('EXCEPTIONS', bad)
$ raise SystemExit(1 if bad else 0)
$ PY
consumer-release | save if = github.event_name == 'push' | push -> True | pull_request -> False
coverage | save if = github.event_name == 'push' | push -> True | pull_request -> False
dbc-gate | save if = github.event_name == 'push' | push -> True | pull_request -> False
downstream-consumer | save if = github.event_name == 'push' | push -> True | pull_request -> False
prose-lint | save if = github.event_name == 'push' | push -> True | pull_request -> False
sanitize | save if = github.event_name == 'push' | push -> True | pull_request -> False
shared-audit | save if = github.event_name == 'push' | push -> True | pull_request -> False
test | save if = github.event_name == 'push' | push -> True | pull_request -> False
test-rocky | save if = github.event_name == 'push' | push -> True | pull_request -> False
tsan | save if = github.event_name == 'push' | push -> True | pull_request -> False
EXCEPTIONS []
[exit status: 0]
```

Every building job holds exactly one `actions/cache/save` step and its
`if` is `github.event_name == 'push'`. The condition was evaluated as
Python with `github.event_name` substituted, so the two columns are the
evaluated result: a push saves, a pull request restores and saves nothing,
which is the FR-008 split. `EXCEPTIONS []` also says no non-building job
carries a save step.

## Check 4: `CCACHE_BASEDIR` and `CCACHE_SLOPPINESS`

```text
$ grep -rn 'CCACHE_BASEDIR\|CCACHE_SLOPPINESS' .github/ CMakePresets.json tools/
(no output)
[exit status: 1]
```

`grep` exits 1 when nothing matches, so exit status 1 with no output is
the pass line: neither name appears under `.github/`, in
`CMakePresets.json`, or under `tools/`. The names that do appear there are
`CCACHE_DIR`, `CCACHE_KEY_PREFIX`, `CCACHE_MAXSIZE`, `CCACHE_CXX_HASH` and
`CCACHE_TREE_HASH`.

## Check 5: the key of D-9

```text
$ python3 - <<'PY'
$ import re, yaml
$ d = yaml.safe_load(open('.github/workflows/ci.yml'))
$ E = chr(36) + '{{'
$ SHAPE = '-'.join([E + ' env.CCACHE_KEY_PREFIX }}', E + ' env.CCACHE_CXX_HASH }}',
$                   E + ' env.CCACHE_TREE_HASH }}', E + ' github.run_id }}'])
$ bad = []
$ for n, j in sorted(d['jobs'].items()):
$     st = j['steps']
$     runs = [(s.get('run') or '') for s in st]
$     if not any('cmake --build' in r for r in runs):
$         continue
$     ri = next(i for i, s in enumerate(st)
$               if (s.get('uses') or '').startswith('actions/cache/restore'))
$     rstep, w = st[ri], st[ri]['with']
$     pre = (j.get('env') or {})['CCACHE_KEY_PREFIX']
$     preset = re.search(r'--preset=([a-z0-9-]+)', ' '.join(runs)).group(1)
$     key, rk = w['key'], w['restore-keys']
$     chk = {'prefix': pre == 'ccache-' + E + ' github.job }}-' + preset,
$            'shape': key == SHAPE,
$            'run_id_dropped_in_restore_keys': rk == SHAPE[: -len(E + ' github.run_id }}')],
$            'cxx_hash': rstep['env']['CCACHE_CXX_HASH'] == E + " hashFiles('.ccache-cxx-version') }}",
$            'tree_hash': rstep['env']['CCACHE_TREE_HASH'] == E + " hashFiles('CMakePresets.json', 'external/**') }}",
$            'version_step_first': any('--version | head -n 1 > .ccache-cxx-version' in runs[i]
$                                      for i in range(ri))}
$     print('%-20s preset=%-16s prefix=%s' % (n, preset, pre))
$     print('                     key=%s' % key)
$     print('                     restore=%s  %s' % (rk, 'OK' if all(chk.values()) else chk))
$     bad += [n + ':' + k for k, v in chk.items() if not v]
$ print('EXCEPTIONS', bad)
$ raise SystemExit(1 if bad else 0)
$ PY
consumer-release     preset=ci-linux-ignore  prefix=ccache-${{ github.job }}-ci-linux-ignore
                     key=${{ env.CCACHE_KEY_PREFIX }}-${{ env.CCACHE_CXX_HASH }}-${{ env.CCACHE_TREE_HASH }}-${{ github.run_id }}
                     restore=${{ env.CCACHE_KEY_PREFIX }}-${{ env.CCACHE_CXX_HASH }}-${{ env.CCACHE_TREE_HASH }}-  OK
coverage             preset=ci-coverage      prefix=ccache-${{ github.job }}-ci-coverage
                     key=${{ env.CCACHE_KEY_PREFIX }}-${{ env.CCACHE_CXX_HASH }}-${{ env.CCACHE_TREE_HASH }}-${{ github.run_id }}
                     restore=${{ env.CCACHE_KEY_PREFIX }}-${{ env.CCACHE_CXX_HASH }}-${{ env.CCACHE_TREE_HASH }}-  OK
dbc-gate             preset=ci-ubuntu        prefix=ccache-${{ github.job }}-ci-ubuntu
                     key=${{ env.CCACHE_KEY_PREFIX }}-${{ env.CCACHE_CXX_HASH }}-${{ env.CCACHE_TREE_HASH }}-${{ github.run_id }}
                     restore=${{ env.CCACHE_KEY_PREFIX }}-${{ env.CCACHE_CXX_HASH }}-${{ env.CCACHE_TREE_HASH }}-  OK
downstream-consumer  preset=ci-linux-audit   prefix=ccache-${{ github.job }}-ci-linux-audit
                     key=${{ env.CCACHE_KEY_PREFIX }}-${{ env.CCACHE_CXX_HASH }}-${{ env.CCACHE_TREE_HASH }}-${{ github.run_id }}
                     restore=${{ env.CCACHE_KEY_PREFIX }}-${{ env.CCACHE_CXX_HASH }}-${{ env.CCACHE_TREE_HASH }}-  OK
prose-lint           preset=ci-ubuntu        prefix=ccache-${{ github.job }}-ci-ubuntu
                     key=${{ env.CCACHE_KEY_PREFIX }}-${{ env.CCACHE_CXX_HASH }}-${{ env.CCACHE_TREE_HASH }}-${{ github.run_id }}
                     restore=${{ env.CCACHE_KEY_PREFIX }}-${{ env.CCACHE_CXX_HASH }}-${{ env.CCACHE_TREE_HASH }}-  OK
sanitize             preset=ci-sanitize      prefix=ccache-${{ github.job }}-ci-sanitize
                     key=${{ env.CCACHE_KEY_PREFIX }}-${{ env.CCACHE_CXX_HASH }}-${{ env.CCACHE_TREE_HASH }}-${{ github.run_id }}
                     restore=${{ env.CCACHE_KEY_PREFIX }}-${{ env.CCACHE_CXX_HASH }}-${{ env.CCACHE_TREE_HASH }}-  OK
shared-audit         preset=ci-linux-audit   prefix=ccache-${{ github.job }}-ci-linux-audit
                     key=${{ env.CCACHE_KEY_PREFIX }}-${{ env.CCACHE_CXX_HASH }}-${{ env.CCACHE_TREE_HASH }}-${{ github.run_id }}
                     restore=${{ env.CCACHE_KEY_PREFIX }}-${{ env.CCACHE_CXX_HASH }}-${{ env.CCACHE_TREE_HASH }}-  OK
test                 preset=ci-ubuntu        prefix=ccache-${{ github.job }}-ci-ubuntu
                     key=${{ env.CCACHE_KEY_PREFIX }}-${{ env.CCACHE_CXX_HASH }}-${{ env.CCACHE_TREE_HASH }}-${{ github.run_id }}
                     restore=${{ env.CCACHE_KEY_PREFIX }}-${{ env.CCACHE_CXX_HASH }}-${{ env.CCACHE_TREE_HASH }}-  OK
test-rocky           preset=ci-rocky         prefix=ccache-${{ github.job }}-ci-rocky
                     key=${{ env.CCACHE_KEY_PREFIX }}-${{ env.CCACHE_CXX_HASH }}-${{ env.CCACHE_TREE_HASH }}-${{ github.run_id }}
                     restore=${{ env.CCACHE_KEY_PREFIX }}-${{ env.CCACHE_CXX_HASH }}-${{ env.CCACHE_TREE_HASH }}-  OK
tsan                 preset=ci-tsan          prefix=ccache-${{ github.job }}-ci-tsan
                     key=${{ env.CCACHE_KEY_PREFIX }}-${{ env.CCACHE_CXX_HASH }}-${{ env.CCACHE_TREE_HASH }}-${{ github.run_id }}
                     restore=${{ env.CCACHE_KEY_PREFIX }}-${{ env.CCACHE_CXX_HASH }}-${{ env.CCACHE_TREE_HASH }}-  OK
EXCEPTIONS []
[exit status: 0]
```

Per job the preset its own `Configure` step names, the
`CCACHE_KEY_PREFIX`, then the full key and the full restore key. The `OK`
column holds six tests: the prefix equals `ccache-` plus
`${{ github.job }}` plus that preset; the key is that prefix, the
compiler-version hash, the tree hash and the run id in that order; the
restore key is the same string with the run id dropped and a trailing
hyphen; `CCACHE_CXX_HASH` is `hashFiles` over `.ccache-cxx-version`;
`CCACHE_TREE_HASH` is `hashFiles` over `CMakePresets.json` and
`external/**`; and the step that writes the compiler version line lands
before the restore step, which is what makes that hash read a file the
step just wrote. `EXCEPTIONS []` means all six hold in all ten jobs. The
restore step carries `id: ccache` and the save step keys itself to
`steps.ccache.outputs.cache-primary-key`, so a save writes the key the
restore read.

## Check 6: `ci-cache` in the preset inheritance

```text
$ python3 -c "import json; p=json.load(open('CMakePresets.json')); b={e['name']:e for e in p['configurePresets']}; f=lambda n: [b[n]['inherits']] if isinstance(b[n].get('inherits'), str) else (b[n].get('inherits') or []); [print(n, f(n), 'ci-cache' in f(n)) for n in ('ci-ubuntu','ci-rocky','ci-ubuntu-test','ci-sanitize','ci-tsan','ci-coverage','prepush')]; u=f('ci-ubuntu'); print('ci-ubuntu analyzers', 'clang-tidy' in u, 'cppcheck' in u); raise SystemExit(0 if all('ci-cache' in f(n) for n in ('ci-ubuntu','ci-rocky','ci-ubuntu-test','ci-sanitize','ci-tsan','ci-coverage','prepush')) and 'clang-tidy' in u and 'cppcheck' in u else 1)"
ci-ubuntu ['ci-build', 'ci-linux', 'clang-tidy', 'cppcheck', 'dev-mode', 'ci-cache'] True
ci-rocky ['ci-build', 'ci-linux', 'dev-mode', 'ci-cache'] True
ci-ubuntu-test ['ci-build', 'ci-linux', 'dev-mode', 'ci-cache'] True
ci-sanitize ['ci-linux', 'dev-mode', 'ci-cache'] True
ci-tsan ['ci-linux', 'dev-mode', 'ci-cache'] True
ci-coverage ['coverage-linux', 'dev-mode', 'ci-cache'] True
prepush ['ci-build', 'dev-mode', 'ci-cache'] True
ci-ubuntu analyzers True True
[exit status: 0]
```

All seven presets list `ci-cache` in `inherits`, and `ci-ubuntu` still
lists `clang-tidy` and `cppcheck` there, the FR-013 inheritance that
`dbc-gate` and `prose-lint` depend on. `ci-cache` carries the two launcher
cache variables set to `ccache`, inherits `dev-mode`, and names no
generator and no `binaryDir`.

## Check 7: the two gates over `specs/030-ci-speedup/results`

Both commands run after `git add -N` of this record, so the gate counts
this file among its sources.

```text
$ python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/030-ci-speedup/results
prose-lint: 16 sources, 1509 units examined, 0 findings, 0 skipped
[exit status: 0]
```

```text
$ codespell specs/030-ci-speedup/results/
Used config files:
    1: .codespellrc
[exit status: 0]
```

Both gates print zero findings and exit 0. The prose gate summary line
above counts this record among its sources. The spell gate prints its
configuration banner and nothing else, which is the shape it has when no
word is flagged. Other nodes write records into the same directory while
this node works, so a later re-run can print a different `units examined`
count; the finding count and the two exit statuses carry the verdict.

Supporting readings, this record on its own:

```text
$ python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/030-ci-speedup/results/ph3/verify.md
prose-lint: 1 sources, 374 units examined, 0 findings, 0 skipped
[exit status: 0]
```

```text
$ codespell specs/030-ci-speedup/results/ph3/verify.md
Used config files:
    1: .codespellrc
[exit status: 0]
```

## Verdict

| Check | Question | Exit status | Result |
| --- | --- | --- | --- |
| 1 | cache step, `CCACHE_DIR`, `CCACHE_MAXSIZE` in every building job and none of the three in `lint` and `docs` | 0 | PASS |
| 2 | `ccache -z` before and `ccache -s` after each build step | 0 | PASS |
| 3 | save step exists and its condition admits push and refuses pull request | 0 | PASS |
| 4 | `CCACHE_BASEDIR` and `CCACHE_SLOPPINESS` set nowhere | 1, which is grep for no match | PASS |
| 5 | the key carries the D-9 segments in order and the restore keys drop the run id | 0 | PASS |
| 6 | `ci-cache` in the inheritance of the seven presets, analyzers still in `ci-ubuntu` | 0 | PASS |
| 7 | both gates print 0 findings over `specs/030-ci-speedup/results` | 0 and 0 | PASS |

Checks 1 to 6 hold: each of the ten building jobs restores and saves one
`ccache` directory inside the workspace, keys it by job, preset, compiler
version, presets plus vendored sources and the run id, prints its
statistics around every build, and saves only on a push to master. Check 7 holds for this record and for the
directory it sits in.

## Observations outside the seven checks

1. Three of the ten cached lanes configure a preset that reaches no
   launcher. `shared-audit` and `downstream-consumer` configure
   `ci-linux-audit` and `consumer-release` configures
   `ci-linux-ignore`; the ancestry of those two presets ends at `ci-std`
   and `flags-gcc-clang` and never passes `ci-cache`, so
   `CMAKE_CXX_COMPILER_LAUNCHER` stays unset there. The presets that do
   reach the launcher are `ci-cache`, `ci-coverage`, `ci-rocky`,
   `ci-sanitize`, `ci-tsan`, `ci-ubuntu`, `ci-ubuntu-test` and
   `prepush`. No step in `.github/workflows/ci.yml` names a launcher
   either, so those three lanes restore and save a directory that no
   compile writes into, and their `ccache -s` prints zeros. The cold,
   warm and verdict records of T017 to T019 should expect a 0% hit rate
   there, and the owner of the audit presets decides whether the launcher
   belongs there.
2. The `test` job configures `ci-ubuntu`, while FR-014 and the
   `contracts/presets.md` lane table name `ci-ubuntu-test` for that lane,
   and no `static-analysis` job exists yet among the twelve (FR-015). The
   key segment that check 5 reads matches the preset the job configures,
   so the cache wiring is self-consistent either way. This is phase 2
   scope in a file other nodes edit.
3. `CCACHE_MAXSIZE` is set once per building job and the ten sizes sum to
   8192 MB in `specs/030-ci-speedup/results/ph3/maxsize.md`, the budget
   reading that check 1 implies and does not restate.

PHASE3: PASS

