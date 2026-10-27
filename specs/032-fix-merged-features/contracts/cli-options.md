# Contract: the option, precedence and exit-2 surface

Phase 1 output. The standing grammar of
`specs/027-jitter-probe/contracts/cli-options.md` and the hunter
surface of spec 029 hold as they are; this file records the deltas the
fixes land. No option row is added and no option value moves (FR-003).

## Probe value precedence (FR-028, D-01)

| Source pair | Value that runs |
| --- | --- |
| family probe vs command line | the family's own probe |
| family probe vs option file | the family's own probe |
| option file vs command line | the command-line value |

The option-file `jitter-probe=` value carries the same standing as a
command-line value. One test covers each source.

## Duplicate cpu refusal (FR-029, D-04)

| Input shape | Behavior |
| --- | --- |
| `--jitter-probe=0,0` | one line quoting `0`, exit 2 |
| `--jitter-probe=0-1,1` | one line quoting `1`, exit 2 |

The refusal stands regardless of the source of the value.

## Ambient-value refusal for the hunt (FR-034, D-07)

| Ambient variable holding a value | Behavior |
| --- | --- |
| `SPEEDGUN_COUNTER` | one line naming the source, exit 2, no hunt |
| `SPEEDGUN_METRIC` | one line naming the source, exit 2, no hunt |
| `SPEEDGUN_PLACEMENT` | one line naming the source, exit 2, no hunt |

More than one variable holding a value stops on the first, and the
line names that source. The refusal works from every source: the
environment, the option file and the command line each get a test
(FR-045).

## Counters refusals (FR-032, FR-033)

| Input | Behavior |
| --- | --- |
| a `--counter` address this host cannot resolve, from every source | one quoted line, exit 2 |
| a valid PMU path this host lacks | one quoted line, exit 2 |
| a leaf the catalog lists but this host cannot count | exit 2 |

## Combination and mode rules (FR-035, FR-037, FR-043 to FR-050)

| Input | Behavior |
| --- | --- |
| `--hunt` with `--jitter-probe` | the run completes and the report carries both records; a docs row names the combination |
| the hunt's first pass | no retry runs |
| a refused combination given from the environment or the option file | refused, one line, exit 2 |
| `--hunt=none` | the standing behavior, with a test |
| `--hunt-noise` grammar values | the standing behavior, with a test |
| the hunt with `--interleave`, `--threads`, `--jitter-probe` | the standing behavior, with tests |
| a raw-code rule with globs | the standing behavior, with a test |
| the `ABSENT` relabel abort | the standing behavior, with a test |
| a failing device lookup | the standing behavior, with a test |

## The idle preset and the fact checks (SC-007)

The idle preset prints one line naming the preset word and returns 2
when no cpu is free (`source/harness/cli.cpp:805` to `:818`,
confirmed). `SPEEDGUN_JSON` matches no environment row, so the
unmatched-name rule at `options.cpp:1310` to `:1325` leaves it unread
(confirmed). No behavior changes at these sites; the evidence stands
in the record.

## Compare table quoting (FR-030)

| Cell | Format |
| --- | --- |
| name | quoted once with `detail::quotedText` |
| rule text | quoted once with `detail::quotedText` |
| reason | unquoted, unchanged |
