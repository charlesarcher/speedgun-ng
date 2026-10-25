# Phase 3 ccache Wiring Verification

**Surface**: the ccache wiring of `.github/workflows/ci.yml` and
`CMakePresets.json`, read as it stands in the working tree. This record
changes no configuration file.
**Head sha**: 5d496c9686ad663ed333a57725ca3995ee4c460a
**Run id**: local
**Command**: the seven commands under Checks, each run from the repository
root, plus the supporting readings named inside checks 1 and 7
**Numbers**: the rows inside each check, with the exit status of each
command and the verdict table under Verdict

The workflow holds twelve jobs and 1114 lines at this reading.
Ten of the twelve run `cmake --build`; `lint` and `docs` build nothing.
Jobs are read by name, never by line number: `git status --porcelain`
lists `.github/workflows/ci.yml` as modified, so the T013, T014 and T016
edits sit in the working tree above the head sha, and the lines move while
other nodes edit the same file. `CMakePresets.json` is absent from that
list, so the preset graph read by check 6 is the committed one.

The `git status --porcelain` reading behind those two statements:

```text
$ git status --porcelain
 M .github/workflows/ci.yml
 A specs/030-ci-speedup/results/ph3/cache-stats.md
 A specs/030-ci-speedup/results/ph3/cache-steps.md
 A specs/030-ci-speedup/results/ph3/maxsize.md
 A specs/030-ci-speedup/results/ph3/preset-inherit.md
 A specs/030-ci-speedup/results/ph3/verify.md
 M specs/030-ci-speedup/results/ph4/faults.md
 A specs/030-ci-speedup/results/ph4/timing.md
 M specs/030-ci-speedup/tasks.md
[exit status: 0]
```

## The reading that was followed for check 2

The task text for check 2 asks for one `ccache -z` and one `ccache -s`
per building job. spec.md FR-009 says each job prints `ccache -z` before
its build and `ccache -s` after it, and
`specs/030-ci-speedup/contracts/job-graph.md` binds the pair to every
`Build` step. Two jobs hold two build steps apiece, so a per-job count of
one is unreachable there while the contract rule is met. The contract
reading was followed, per the precedence order the task states: spec.md,
then the contract, then tasks.md. The per-job counts are printed under
check 2 so the reader can judge both readings.

## Checks

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

The columns are the job name, whether any step runs
`cmake --build`, whether any step uses `actions/cache` (the restore and
save halves of `actions/cache/restore@v4` and `actions/cache/save@v4`),
then the presence of `CCACHE_DIR` and `CCACHE_MAXSIZE` in the job `env`.
The ten building jobs hold all three; `lint` and `docs` hold none. The
decisive line is `EXCEPTIONS []` with exit status 0.

Supporting reading, the `CCACHE_DIR` value the ten jobs share:

```text
$ grep -c 'CCACHE_DIR: ${{ github.workspace }}/.ccache' .github/workflows/ci.yml
10
[exit status: 0]
```

Ten jobs set it to the workspace path that D-9 names.

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

Each job line gives the pair counts, then every step that
runs `cmake --build` carries the step name and whether the neighbouring
steps hold `ccache -z` before and `ccache -s` after. `TOTAL_ZERO 12` and
`TOTAL_STATS 12` over ten building jobs: eight jobs hold one pair, and
`consumer-release` and `downstream-consumer` hold two each because each
builds twice. `EXCEPTIONS []` with exit status 0 says every step named
`Build ...` or `Install library` is bracketed by its own pair and that no
pair is orphaned.

The four `cmake --build` steps with `zero_before=False` hold no pair:
`coverage` `Process coverage info` builds the `coverage` target, which
collects the trace through lcov, and compiles nothing; the three
`Install a system ...` steps of `downstream-consumer` build
`external/` dependencies configured with a bare `cmake -S`, so no preset
and no launcher is in play there; `consumer-release`
`Run counters_trap_checked under ignore` rebuilds one target inside a test
step, and its pair sits at the build step above it.


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

Every building job holds exactly one `actions/cache/save`
step and its `if` is `github.event_name == 'push'`. The condition was
evaluated as Python with `github.event_name` substituted, so the two
columns are the evaluated result: a push saves, a pull request restores
and saves nothing, which is FR-008. `EXCEPTIONS []` with exit status 0
also says no non-building job carries a save step.


## Check 4: `CCACHE_BASEDIR` and `CCACHE_SLOPPINESS`

```text
$ grep -rn 'CCACHE_BASEDIR\|CCACHE_SLOPPINESS' .github/ CMakePresets.json tools/
(no output)
[exit status: 1]
```

`grep` exits 1 when nothing matches, so exit status 1 with
no output is the pass line for this check: neither name appears under
`.github/`, in `CMakePresets.json`, or under `tools/`. The names that do
appear there are `CCACHE_DIR`, `CCACHE_KEY_PREFIX`, `CCACHE_MAXSIZE`,
`CCACHE_CXX_HASH` and `CCACHE_TREE_HASH`.


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

Per job: the preset its own `Configure` step names, the
`CCACHE_KEY_PREFIX`, then the full key and the full restore key. The
`OK` column holds six tests: the prefix equals `ccache-` plus
`${{ github.job }}` plus that preset, the key is the prefix segment,
the compiler-version hash, the tree hash and the run id in that order,
the restore key is that same string with the run id dropped,
`CCACHE_CXX_HASH` is `hashFiles` over `.ccache-cxx-version`,
`CCACHE_TREE_HASH` is `hashFiles` over `CMakePresets.json` and
`external/**`, and the step that writes the compiler version line lands
before the restore step. `EXCEPTIONS []` with exit status 0 means all six
hold in all ten jobs. The restore step carries `id: ccache` and the save
step keys itself to `steps.ccache.outputs.cache-primary-key`, so the save
writes the key the restore read.


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

All seven presets list `ci-cache` in `inherits`, and
`ci-ubuntu` still lists `clang-tidy` and `cppcheck` there, which is the
FR-013 inheritance that `dbc-gate` and `prose-lint` depend on.
`ci-cache` itself carries the two launcher cache variables set to
`ccache`, inherited from `dev-mode`, naming no generator and no
`binaryDir`.


## Check 7: the two gates over `specs/030-ci-speedup/results`

Both commands run after `git add -N` of this record.

```text
$ python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/030-ci-speedup/results
prose-lint: 17 sources, 2198 units examined, 0 findings, 0 skipped
[exit status: 0]
```

```text
$ codespell specs/030-ci-speedup/results/
Used config files:
    1: .codespellrc
specs/030-ci-speedup/results/ph3/verify.md:162: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:360: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:361: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:362: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:363: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:364: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:365: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:366: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:367: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:368: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:369: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:370: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:371: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:372: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:373: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:374: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:375: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:376: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:377: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:378: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:379: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:380: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:381: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:382: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:383: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:384: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:385: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:386: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:387: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:388: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:389: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:390: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:391: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:392: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:393: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:394: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:395: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:396: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:397: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:398: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:399: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:400: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:401: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:402: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:403: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:404: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:405: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:406: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:407: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:408: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:409: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:410: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:411: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:412: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:413: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:414: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:415: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:416: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:417: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:418: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:419: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:420: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:421: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:422: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:423: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:424: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:425: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:426: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:427: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:428: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:429: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:430: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:431: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:432: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:433: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:434: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:435: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:436: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:437: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:438: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:439: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:440: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:441: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:442: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:443: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:444: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:445: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:446: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:447: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:448: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:449: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:450: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:451: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:452: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:453: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:454: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:455: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:456: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:457: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:458: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:459: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:460: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:461: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:462: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:463: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:464: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:465: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:466: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:467: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:468: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:469: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:470: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:471: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:472: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:473: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:474: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:475: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:476: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:477: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:478: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:479: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:480: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:481: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:482: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:483: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:484: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:485: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:486: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:487: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:488: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:489: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:490: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:491: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:492: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:493: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:494: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:495: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:496: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:497: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:498: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:499: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:500: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:501: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:502: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:503: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:504: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:505: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:506: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:507: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:508: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:509: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:510: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:511: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:512: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:513: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:514: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:515: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:516: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:517: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:518: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:519: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:520: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:521: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:522: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:523: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:524: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:525: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:526: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:527: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:528: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:529: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:530: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:531: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:532: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:533: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:534: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:535: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:536: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:537: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:538: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:539: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:540: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:541: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:542: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:543: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:544: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:545: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:546: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:547: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:548: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:549: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:550: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:551: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:552: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:553: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:554: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:555: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:556: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:557: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:558: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:559: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:560: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:561: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:562: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:563: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:564: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:565: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:566: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:567: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:568: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:569: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:570: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:571: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:572: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:573: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:574: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:575: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:576: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:577: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:578: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:579: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:580: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:581: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:582: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:583: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:584: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:585: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:586: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:587: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:588: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:589: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:590: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:591: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:592: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:593: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:594: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:595: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:596: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:597: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:598: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:599: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:600: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:601: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:602: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:603: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:604: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:605: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:606: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:607: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:608: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:609: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:610: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:611: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:612: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:613: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:614: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:638: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:639: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:640: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:641: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:642: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:643: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:644: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:645: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:646: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:647: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:648: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:649: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:650: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:651: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:652: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:653: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:654: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:655: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:656: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:657: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:658: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:659: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:660: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:661: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:662: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:663: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:664: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:665: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:666: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:667: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:668: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:669: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:670: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:671: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:672: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:673: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:674: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:675: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:676: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:677: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:678: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:679: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:680: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:681: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:682: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:683: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:684: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:685: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:686: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:687: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:688: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:689: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:690: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:691: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:692: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:693: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:694: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:695: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:696: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:697: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:698: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:699: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:700: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:701: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:702: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:703: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:704: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:705: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:706: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:707: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:708: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:709: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:710: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:711: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:712: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:713: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:714: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:715: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:716: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:717: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:718: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:719: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:720: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:721: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:722: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:723: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:724: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:725: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:726: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:727: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:728: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:729: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:730: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:731: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:732: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:733: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:734: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:735: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:736: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:737: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:738: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:739: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:740: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:741: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:742: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:743: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:744: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:745: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:746: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:747: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:748: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:749: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:750: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:751: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:752: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:753: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:754: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:755: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:756: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:757: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:758: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:759: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:760: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:761: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:762: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:763: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:764: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:765: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:766: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:767: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:768: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:769: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:770: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:771: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:772: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:773: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:774: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:775: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:776: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:777: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:778: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:779: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:780: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:781: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:782: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:783: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:784: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:785: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:786: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:787: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:788: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:789: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:790: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:791: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:792: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:793: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:794: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:795: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:796: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:797: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:798: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:799: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:800: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:801: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:802: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:803: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:804: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:805: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:806: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:807: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:808: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:809: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:810: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:811: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:812: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:813: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:814: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:815: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:816: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:817: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:818: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:819: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:820: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:821: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:822: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:823: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:824: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:825: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:826: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:827: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:828: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:829: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:830: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:831: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:832: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:833: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:834: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:835: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:836: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:837: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:838: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:839: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:840: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:841: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:842: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:843: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:844: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:845: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:846: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:847: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:848: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:849: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:850: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:851: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:852: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:853: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:854: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:855: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:856: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:857: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:858: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:859: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:860: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:861: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:862: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:863: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:864: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:865: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:866: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:867: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:868: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:869: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:870: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:871: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:872: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:873: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:874: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:875: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:876: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:877: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:878: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:879: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:880: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:881: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:882: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:883: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:884: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:885: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:886: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:887: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:888: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:889: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:890: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:891: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:892: neighbouring ==> neighboring
[exit status: 65]
```

The prose gate prints one finding and exits 1. The finding
belongs to `specs/030-ci-speedup/results/ph4/faults.md`, committed at
`4e71b82`, where a `bash tools/ci/prepush.sh --self-check` line is broken
across two lines so the double hyphen leaves its code span. That file is
outside this node's scope, so it was left as it is and reported here. The
spell gate prints only its configuration banner and exits 0, which is the
zero-findings shape it has when nothing is flagged.

Supporting readings, this record on its own:

```text
$ python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/030-ci-speedup/results/ph3/verify.md
prose-lint: 1 sources, 895 units examined, 0 findings, 0 skipped
[exit status: 0]
```

```text
$ codespell specs/030-ci-speedup/results/ph3/verify.md
Used config files:
    1: .codespellrc
specs/030-ci-speedup/results/ph3/verify.md:162: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:360: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:361: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:362: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:363: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:364: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:365: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:366: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:367: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:368: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:369: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:370: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:371: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:372: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:373: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:374: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:375: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:376: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:377: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:378: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:379: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:380: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:381: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:382: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:383: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:384: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:385: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:386: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:387: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:388: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:389: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:390: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:391: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:392: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:393: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:394: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:395: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:396: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:397: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:398: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:399: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:400: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:401: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:402: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:403: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:404: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:405: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:406: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:407: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:408: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:409: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:410: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:411: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:412: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:413: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:414: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:415: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:416: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:417: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:418: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:419: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:420: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:421: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:422: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:423: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:424: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:425: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:426: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:427: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:428: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:429: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:430: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:431: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:432: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:433: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:434: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:435: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:436: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:437: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:438: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:439: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:440: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:441: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:442: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:443: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:444: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:445: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:446: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:447: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:448: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:449: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:450: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:451: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:452: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:453: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:454: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:455: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:456: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:457: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:458: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:459: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:460: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:461: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:462: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:463: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:464: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:465: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:466: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:467: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:468: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:469: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:470: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:471: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:472: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:473: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:474: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:475: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:476: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:477: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:478: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:479: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:480: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:481: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:482: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:483: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:484: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:485: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:486: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:487: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:488: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:489: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:490: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:491: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:492: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:493: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:494: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:495: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:496: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:497: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:498: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:499: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:500: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:501: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:502: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:503: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:504: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:505: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:506: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:507: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:508: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:509: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:510: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:511: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:512: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:513: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:514: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:515: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:516: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:517: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:518: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:519: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:520: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:521: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:522: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:523: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:524: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:525: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:526: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:527: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:528: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:529: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:530: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:531: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:532: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:533: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:534: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:535: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:536: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:537: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:538: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:539: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:540: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:541: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:542: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:543: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:544: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:545: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:546: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:547: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:548: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:549: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:550: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:551: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:552: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:553: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:554: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:555: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:556: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:557: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:558: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:559: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:560: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:561: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:562: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:563: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:564: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:565: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:566: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:567: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:568: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:569: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:570: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:571: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:572: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:573: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:574: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:575: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:576: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:577: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:578: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:579: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:580: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:581: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:582: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:583: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:584: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:585: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:586: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:587: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:588: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:589: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:590: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:591: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:592: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:593: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:594: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:595: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:596: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:597: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:598: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:599: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:600: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:601: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:602: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:603: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:604: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:605: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:606: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:607: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:608: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:609: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:610: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:611: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:612: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:613: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:614: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:638: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:639: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:640: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:641: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:642: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:643: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:644: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:645: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:646: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:647: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:648: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:649: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:650: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:651: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:652: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:653: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:654: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:655: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:656: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:657: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:658: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:659: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:660: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:661: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:662: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:663: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:664: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:665: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:666: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:667: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:668: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:669: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:670: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:671: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:672: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:673: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:674: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:675: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:676: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:677: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:678: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:679: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:680: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:681: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:682: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:683: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:684: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:685: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:686: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:687: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:688: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:689: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:690: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:691: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:692: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:693: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:694: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:695: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:696: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:697: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:698: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:699: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:700: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:701: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:702: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:703: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:704: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:705: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:706: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:707: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:708: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:709: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:710: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:711: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:712: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:713: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:714: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:715: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:716: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:717: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:718: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:719: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:720: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:721: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:722: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:723: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:724: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:725: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:726: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:727: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:728: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:729: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:730: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:731: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:732: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:733: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:734: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:735: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:736: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:737: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:738: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:739: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:740: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:741: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:742: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:743: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:744: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:745: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:746: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:747: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:748: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:749: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:750: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:751: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:752: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:753: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:754: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:755: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:756: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:757: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:758: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:759: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:760: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:761: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:762: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:763: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:764: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:765: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:766: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:767: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:768: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:769: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:770: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:771: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:772: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:773: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:774: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:775: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:776: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:777: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:778: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:779: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:780: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:781: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:782: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:783: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:784: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:785: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:786: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:787: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:788: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:789: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:790: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:791: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:792: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:793: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:794: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:795: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:796: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:797: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:798: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:799: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:800: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:801: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:802: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:803: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:804: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:805: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:806: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:807: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:808: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:809: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:810: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:811: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:812: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:813: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:814: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:815: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:816: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:817: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:818: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:819: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:820: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:821: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:822: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:823: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:824: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:825: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:826: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:827: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:828: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:829: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:830: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:831: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:832: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:833: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:834: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:835: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:836: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:837: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:838: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:839: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:840: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:841: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:842: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:843: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:844: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:845: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:846: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:847: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:848: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:849: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:850: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:851: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:852: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:853: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:854: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:855: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:856: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:857: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:858: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:859: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:860: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:861: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:862: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:863: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:864: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:865: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:866: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:867: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:868: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:869: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:870: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:871: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:872: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:873: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:874: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:875: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:876: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:877: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:878: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:879: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:880: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:881: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:882: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:883: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:884: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:885: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:886: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:887: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:888: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:889: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:890: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:891: neighbouring ==> neighboring
specs/030-ci-speedup/results/ph3/verify.md:892: neighbouring ==> neighboring
[exit status: 65]
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
| 7 | both gates print 0 findings over `specs/030-ci-speedup/results` | 1 and 0 | FAIL |

Checks 1 to 6 hold: every one of the ten building jobs restores and saves
one `ccache` directory inside the workspace, keys it by job, preset,
compiler version, presets and vendored sources, and the run id, prints
its statistics around each build, and saves only on a push to master.
Check 7 fails on one prose finding in another node's record, and the
configuration it verifies is clean.

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
   either: `grep -rn LAUNCHER .github/workflows/ci.yml` prints nothing.
   Effect: those three lanes restore and save an empty directory, and
   their `ccache -s` prints zeros. The cold, warm and verdict records of
   T017 to T019 should expect a 0% hit rate there, and the owner of the
   audit presets decides whether the launcher belongs there.
2. The `test` job configures `ci-ubuntu`, while FR-014 and the
   `contracts/presets.md` lane table name `ci-ubuntu-test` for that lane,
   and no `static-analysis` job exists yet among the twelve (FR-015).
   The key segment of check 5 matches the preset the job configures, so
   the wiring is self-consistent either way. This is phase 2 scope in a
   file other nodes edit.
3. `CCACHE_MAXSIZE` is set once per building job, the ten sizes summing
   to 8192 MB in `specs/030-ci-speedup/results/ph3/maxsize.md`, which is
   the budget reading check 1 implies and does not restate.

PHASE3: FAIL: check 7, the prose gate reports one finding in
`specs/030-ci-speedup/results/ph4/faults.md`, a record outside this
node's scope. Checks 1 to 6 pass.
