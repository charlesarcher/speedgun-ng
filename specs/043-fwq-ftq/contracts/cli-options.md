# Contract: the `--jitter-probe` method keys

Phase 1 output. The delta over the standing value grammar of
`specs/027-jitter-probe/contracts/cli-options.md`: the value gains
`method=`, `quantum=<us>`, `work=<n>` and `noise=<percent>` (spec
FR-001, FR-002, FR-003, D-1). The option table gains no row:
`kJitterProbeOptionValue` stays `260` at
`source/harness/detail/options.hpp:87`, inside the `260` to `269` block
the comment at `:77` records (spec FR-034). Every other row
of the standing contract stands unchanged. The standing file is amended
in the commit that lands the parser (spec FR-030).

## Grammar delta

Line 23 of the standing file becomes:

```text
--jitter-probe=<cpu-list|idle>[,threshold=<ns>][,mode=alongside|precheck|both][,precheck=<ms>|serial][,gaps=<n>][,strict][,method=tsc|fwq|ftq][,quantum=<us>][,work=<n>][,noise=<percent>]
```

Four rows join the part table after line 34, in the standing columns
`| Part | Meaning | Default | Requirement |`:

| Part | Meaning | Default | Requirement |
| --- | --- | --- | --- |
| `method=` | `tsc`, `fwq`, or `ftq`: one method per run, the words the value takes | `tsc` | specs/043 FR-001, FR-002, D-1 |
| `quantum=<us>` | the FTQ quantum in microseconds, 10 to 100,000 | 100 us | specs/043 FR-002, FR-015, D-4 |
| `work=<n>` | the FWQ work count of one unit, 1 to 16,777,216 | none: the calibration measures the count on the probe cpu | specs/043 FR-002, FR-010, D-2 |
| `noise=<percent>` | the noise percent at which a strict run refuses, 0 to 100 | 1 | specs/043 FR-002, FR-021, D-9 |

The option and the public field carry microseconds, and the JSON key
carries nanoseconds (spec FR-002, FR-006, FR-019).

One sentence joins after line 40:

```text
The head stands first and mandatory: the parser reads token 0 as the
head, and a head token that carries `=` answers the standing
cpu-token line (specs/043 FR-002, the round 2 answer).
```

Each key stands once in one value, the standing rule of line 36: a
second occurrence of one key is an invalid value (spec FR-002).

## Parse point rows

Two rows join the input table after line 48, in the standing columns
`| Input | Behavior |`:

| Input | Behavior |
| --- | --- |
| `--jitter-probe=idle,method=fwq` | the FWQ method runs on one probe cpu per L3 group the plans touch (specs/043 D-1) |
| `--jitter-probe=idle,method=ftq,quantum=1000` | the FTQ method runs with a 1,000 us quantum (specs/043 FR-015) |

The value parses where the standing contract fixes it: the command
line's value parses in `speedgunMain`, and an option file's value
parses where the gather settles it (specs/027 FR-027). A bad file line
names the path, the line and the token (spec FR-003).

## Error-surface rows

Five rows join the error table after line 86, in the standing columns
`| Case | The line names | Status |`:

| Case | The line names | Status |
| --- | --- | --- |
| A method word outside the three | the word | 2 |
| `quantum=0`, a value outside 10 to 100,000, a value that is not a number | the value | 2 |
| `work=0`, a value above 16,777,216, a value that is not a number | the value | 2 |
| `noise=` a value outside 0 to 100, a value that is not a number | the value | 2 |
| A `quantum` key with `method=fwq`, and a `work` key with `method=ftq` | the key | 2 |

Each row prints the standing `--jitter-probe: <message>` line, one
line, with the offending token quoted, and the run exits 2 before the
first run starts (spec FR-003, SC-005). The parser reads the head
first, so a head token that carries `=` answers the standing cpu-token
row of line 74.

## The disclosure row

One row joins the disclosure table after line 106, in the standing
columns `| Case | Behavior |`:

| Case | Behavior |
| --- | --- |
| A `noise=` value with no `strict`, and a `noise=` value with `method=tsc` | the run accepts the value, discloses it as unused, and continues with status 0 (specs/043 FR-021) |

## The standing rows this delta changes none of

- The cpu-list head, the `idle` preset, the mode words, the
  `threshold`, `gaps` and `precheck` keys and the overlap refusal
  stand as specs/027 states them (spec FR-031).
- The option table gains no row and no value: the keys ride the
  standing value (spec FR-034, D-1).
- `strict` keeps its standing meaning in `mode=alongside`, where no
  noise gate fires (spec FR-021, D-9).
- The exit statuses stand unchanged: 0 for a run and its disclosures,
  2 for every case of the error surface, and 1 for the standing run
  failures of specs/015.

## Gate

- `harness_jitter_cli_test` reads the grammar and the 13 refusal rows,
  each with one line and status 2 (spec FR-003, SC-005).
- `harness_cli_table_test` reads the option table: no new row, the
  value `260`, and the version rows at
  `test/source/harness_cli_table_test.cpp:1382-1393` (spec FR-005,
  FR-034).
- The prose gate reads every line of this file, at
  `tools/ci/prepush.sh:203` and in the `prose-lint` job at
  `.github/workflows/ci.yml:1312`.

## Consumers

- `test/source/harness_jitter_cli_test.cpp`: the four keys, the head
  rule, and every status-2 row above (spec FR-002, FR-003).
- `test/source/harness_precheck_test.cpp`: the unused `noise=`
  disclosure and the pre-check refusal (spec FR-021).
- `specs/027-jitter-probe/contracts/cli-options.md` gains these rows
  in the same commit as the parser (spec FR-030).
