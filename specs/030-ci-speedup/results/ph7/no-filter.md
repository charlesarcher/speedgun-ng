# Phase 7 CI ctest Lines Carry No Label Filter

**Surface**: every `ctest` line in `.github/workflows/ci.yml` (T040).
**Head sha**: 16ffda368107126fc0420a2dea00bfdfe606da64
**Run id**: local
**Command**: the command block below, each line run from the repository
root
**Numbers**: the table under Results, one row per numbered check

The workflow has twelve jobs and eight steps that invoke `ctest`. None of
the eight passes a label filter, and none runs `ctest` through a preset,
so no `labelInclude` reaches a CI run. CI therefore runs every registered
entry, whatever tier it carries (FR-027).

## The ctest lines

Eight steps, each with its job, its step name and the command as the file
spells it. The `consumer-release` step chains a build and a `ctest` over
two continuation lines, so the chain is shown whole. The `prose-lint`
step carries no name.

```text
coverage / Test (line 71)
  ctest -j 1 --output-on-failure --no-tests=error

sanitize / Test (line 123)
  ctest --output-on-failure --no-tests=error -j "$(nproc)"

tsan / Test (line 153)
  ctest --output-on-failure --no-tests=error -j "$(nproc)"

test / Test (line 222)
  ctest --output-on-failure --no-tests=error -C Release -j "$(nproc)"

test-rocky / Test (line 249)
  ctest --output-on-failure --no-tests=error -C Release -j "$(nproc)"

consumer-release / Run counters_trap_checked under ignore (line 653)
  cmake --build build/consumer-release-dev -j "$(nproc)" \
    --target counters_trap_checked_test
  ctest --test-dir build/consumer-release-dev -R counters_trap \
    --output-on-failure

dbc-gate / Test dbc gate fixtures (line 696)
  ctest -R dbc_gate_fixtures --output-on-failure --no-tests=error -j "$(nproc)"

prose-lint / unnamed step (line 754)
  ctest --test-dir build/dev -R prose_gate_fixtures --output-on-failure --no-tests=error
```

## Filter state

Every cell below is filled from the parsed step. A folded `run: >-` block
whose flag lands mid-chain is therefore read on the same footing as a
one-line `run:`.

| Job / step | `-L` | `-LE` | `-E` | ctest preset | other selector |
| --- | --- | --- | --- | --- | --- |
| coverage / Test | none | none | none | none | none |
| sanitize / Test | none | none | none | none | none |
| tsan / Test | none | none | none | none | none |
| test / Test | none | none | none | none | none |
| test-rocky / Test | none | none | none | none | none |
| consumer-release / Run counters_trap_checked under ignore | none | none | none | none | `-R counters_trap` |
| dbc-gate / Test dbc gate fixtures | none | none | none | none | `-R dbc_gate_fixtures` |
| prose-lint / unnamed step | none | none | none | none | `-R prose_gate_fixtures` |

Three steps carry `-R`, a regex over test names that picks out one gate
fixture entry each. `-R` selects by name, so a tier label plays no part in
them. The five suite steps carry no selector of any kind and run the whole
registered suite of their build tree.

## Test presets

`CMakePresets.json` holds four `testPresets` entries. One carries a label
filter, and CI runs no `ctest --preset` line, so none of them is in
effect in a CI job.

| Test preset | `configurePreset` | `filter` | Used by CI |
| --- | --- | --- | --- |
| `ci-tsan` | `ci-tsan` | none | no |
| `ci-sanitize` | `ci-sanitize` | none | no |
| `prepush-quick` | `prepush` | `labelInclude: quick` | no |
| `prepush-full` | `prepush` | none | no |

`prepush-quick` is the local quick tier that `tools/ci/prepush.sh` uses.
It is the only place in the repository where a tier label selects what
runs, and it sits outside CI.

## The commands

```text
cd /home/archerc/code/speedgun-ng
python3 - <<'PY'
import json, re, yaml
text = open('.github/workflows/ci.yml').read()
jobs = yaml.safe_load(text)['jobs']
n = 0
for job, spec in jobs.items():
    for step in spec.get('steps', []):
        blob = str(step.get('run', '')) + str(step.get('uses', ''))
        if 'ctest' in blob:
            n += 1
            print(n, job, '/', step.get('name'))
        for ln in str(step.get('run', '')).splitlines():
            if re.search(r'\bctest\b', ln) and re.search(r'(^|\s)-(L|LE|E)(\s|$)', ln):
                print('LABEL FILTER', job, ln.strip())
print('jobs', len(jobs), 'ctest steps', n)
print('label flag matches:', re.findall(r'ctest[^\n]*(?:^|\s)-(?:L|LE|E)(?:\s|$)', text))
presets = json.load(open('CMakePresets.json')).get('testPresets', [])
for p in presets:
    print('test preset', p['name'], p.get('filter'))
print('ctest presets CI uses:', sorted(set(re.findall(r'ctest --preset[= ]([\w.-]+)', text))))
PY
grep -n 'ctest' .github/workflows/ci.yml | wc -l
python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/030-ci-speedup/results
codespell specs/030-ci-speedup/results/
```

## Results

| Check | Expect | Exit | Decisive line | Verdict |
| --- | --- | --- | --- | --- |
| 1 parsed job count | 12 | 0 | `jobs 12 ctest steps 8` | PASS |
| 2 parsed steps that invoke `ctest` | 8 | 0 | `8 prose-lint / None` | PASS |
| 3 label flags on a parsed ctest line | none | 0 | `label flag matches: []` | PASS |
| 4 `ctest` lines in the raw file | 8 | 0 | `8` | PASS |
| 5 `ctest --preset` lines in CI | none | 1 | grep exits 1 on a zero count | PASS |
| 6 parsed `testPresets` entries | 4 | 0 | `test preset prepush-full None` | PASS |
| 7 `testPresets` with `labelInclude` | 1, `prepush-quick` | 0 | `test preset prepush-quick {'labelInclude': 'quick'}` | PASS |
| 8 test presets a CI job runs | none | 0 | `ctest presets CI uses: []` | PASS |
| 9 prose gate | 0 findings | 0 | see the run below the table | PASS |
| 10 codespell | 0 hits | 0 | see the run below the table | PASS |

Checks 2 and 3 read the parsed job graph; check 4 reads the raw file. A
filter hidden inside a folded or continued `run:` block would show up in
check 3 and leave the raw line count of check 4 at eight. The two counts
agree, which is the pairing that makes the absence meaningful.

## Verdict

No CI `ctest` line passes `-L`, `-LE` or `-E`, no CI line runs `ctest`
through a preset, and the only `labelInclude` in the tree sits in
`prepush-quick`, which CI never invokes. No tier can be dropped from a CI
run: every job runs the full registered suite of its tree, and the tier
labels change only what the local pre-push script runs (FR-027, US5
scenario 2).

The path here is the one the driver named for this node. T040 in
`specs/030-ci-speedup/tasks.md` names
`specs/030-ci-speedup/results/ph7/no-filter.md` for the same reading; a
downstream node that opens that path finds nothing until the driver
repoints it.
