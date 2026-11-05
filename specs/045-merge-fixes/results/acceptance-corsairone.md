# Acceptance record: corsairone (specs/045-merge-fixes T201)

Head under test: `d214be37` for the first pass, then `e1c1b4ef` and `6025a71f`
for the two fixes this run produced.
Host: CorsairOne, kernel `7.3.0-4-cachyos`, gcc `16.2.1 20260810`, cmake
`4.4.2`, lcov `2.6-0`, clang-format `23.1.3`, 24 cpus (Intel Core i9-9920X).
Date: 2026-11-03.
Worktree: `~/sgng-045-accept`, detached at the pushed head. The host also ran
another active session (specs/046 work in `~/code/speedgun-ng`) during this
run; its load average stood between 9 and 22.

## 1. Full suite at `d214be37`

`ctest --test-dir build --output-on-failure --no-tests=error -j 24`:
`98% tests passed, 3 tests failed out of 178`.

| Entry | Time | Cause |
| --- | --- | --- |
| `prose_gate_fixtures` | 16.79 s | the entry bound of 10 s; the entry measured 12.157 s whole tree |
| `counters_kernel_calls_test` | 0.03 s | the real-open premise, named below |
| `harness_jitter_window_test` | 0.40 s | the E-08 assertion, named below |

## 2. `counters_kernel_calls_test`: fixed by `e1c1b4ef`

The entry planted a zero `perf_event_attr` size and expected the kernel to
refuse it. Probed on both hosts:

| Host | `perf_event_paranoid` | zero size | unregistered type | out-of-range config |
| --- | --- | --- | --- | --- |
| thrawn | 2 | EACCES | EACCES | EACCES |
| CorsairOne | -1 | granted, fd 3 | ENOENT | EINVAL |

The premise read a host fact: this host refuses for permission before it
looks at the attribute, and a host with the control at -1 accepts a zero size
and grants a descriptor. The attribute now carries a valid size and a type no
pmu registers, which refuses on both hosts. `counters_kernel_calls_test: ok`
on CorsairOne at `e1c1b4ef`.

## 3. `prose_gate_fixtures`: fixed by `6025a71f`

The pre-existing timing case passed there (`wall:9.822s` against the 10 s
bound). The entry's own bound, added by this lane, reused that same 10 s
figure and failed every run: `10.0s allowed, 12.700s measured`. The entry runs
the whole-tree scan several times over, so it now carries its own bound of
30 s. At `6025a71f`: `prose_gate_fixtures ... Passed 12.69 sec`.

The margin on the pre-existing case stands noted: 9.8 s against a 10 s bound
on this host, where the plan calibrated the figure at 0.062 s. A slower host
than this one can fail that case with no change in the gate.

## 4. `harness_jitter_window_test`: open

The entry fails on this host in every run, standalone included:

```
HARNESS JITTER WINDOW TEST FAIL: the entry whose windows kept no gap loads no fold (FR-027, E-08)
```

The scenario runs `bmQuiet` and expects no jitter fold, because its windows
are expected to keep no gap. On this host `bmQuiet` ends with a fold. The same
assertion moved on thrawn under load (2 failures in 5 standalone runs at load
14.9 in the coverage tree, 6 in 20 solo runs in the dev tree). The condition
is not a contention artifact here: it reproduced 3 of 3 runs at load averages
from 4.7 to 21.7. It stands reported for the owner's ruling, and the entry
stands open for this host.

## 5. Instrumented trees at `6025a71f`

A second run on this host, in a private worktree, completed the instrumented
trees. `76fd4aa3` differs from `6025a71f` by Docs commits only, so these figures
carry the code the pull request stands on.

| Item | Reading |
| --- | --- |
| coverage | lines 98.6% (17359 of 17601), functions 97.9% (1821 of 1860), branches 96.6% (8415 of 8709), 242 dark lines, gate exit 1 on the line check |
| sanitize | 167 of 173, no `AddressSanitizer` line, no `runtime error` line |
| tsan | 169 of 174, no `WARNING: ThreadSanitizer` line |

Five to seven entries hit their recorded wall bounds in the instrumented trees
on this host, and that is the reason for the 242 dark lines: an entry that ends
at its bound leaves the code after the bound unexecuted. The per-file dark-line
list and the entry names stand in `acceptance-corsairone-cov.md`.

## 6. A reading discarded, and its cause

The two-cpu pass in the first run (177 of 178) straddled a set of checkouts I
made in the same worktree at 19:49 and 19:50 while that pass was running, so
binaries changed mid-run. That reading is discarded, and the cause is mine: the
worktree was another session's, and I used it for verification. The pass is
being repeated in a private worktree.

The marker count and the five tools of SC-005 were not run on this host. The
thrawn record carries them at `d214be37`, and the CI matrix carries them at the
PR head.
