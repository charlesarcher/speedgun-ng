# Contract: the `hwlat` lines of the probe and tracer blocks

Phase 1 output. The delta over the standing console contracts:
`specs/034-host-noise-precheck/contracts/console-report.md` for the
pre-check block and the probe block it borrows its field names from.
`printJitterReport` at `source/harness/report.cpp:987` and
`printPrecheckReport` at `:1469` gain the lines below, and every
standing line stands byte-identical (spec FR-004, FR-016, FR-027,
FR-030). The block order, the field names and the `not available`
wording of the standing contracts stand unchanged; the amendments this
delta names join the standing files in the commit that lands the code.

## Line shape

```text
[probe block, printJitterReport, after the last series line]
  smiDelta: <figure> [<figure> ...]                  [one column per probe cpu, cpu order]
  nmiDelta: <figure> [<figure> ...]                  [the three lines stand for the hwlat
  smiGapCount: <figure> [<figure> ...]                method alone]
[probe block, disclosure region, after the kept-gaps-only line]
  jitter: SMI count not available                    [once per run, once for that source]
  jitter: NMI count not available                    [once per run, once for that source]
  jitter: probe runs with interrupts enabled         [once per run, the limits line]
[pre-check block, printPrecheckReport, per measured cpu, after the switch
 and schedule line]
  smi delta: <figure> (source: /dev/cpu/<n>/msr)
  nmi delta: <figure> (source: /proc/interrupts)
  smi gaps: <count>
[tracer lines, after the standing engine header]
  tracer: engine hwlat cpus <list>                   [the standing line, the third word]
  tracer: windowUs=<us> widthUs=<us> thresholdUs=<us>
[tracer lines, after the standing hardware line of each tracer cpu]
  tracer cpu <n> maxGapNs: <figure>
  tracer cpu <n> gapCount: <figure>
  tracer cpu <n> nmiCount: <figure>
[the not-available case, and the case the kernel already runs the engine]
  tracer: hwlat already stands as the current tracer   [only when the read
                                       `current_tracer` answered `hwlat`
                                       ahead of the run, once per run,
                                       ahead of the standing line below]
  tracer: not available                                [the standing line]
[restore defect path, at the restore call site, ahead of the pre-check block]
  tracer: restore failed: <file> = <saved value>     [one line per file whose write failed]
```

## Lines, sections and counts

| Line | Section | Prints | Requirement |
| --- | --- | --- | --- |
| `smiDelta:`, `nmiDelta:`, `smiGapCount:` | the probe block's field group, after `droppedGaps:` | one line per field, one column per probe cpu, a `hwlat` run only | spec FR-016 |
| `jitter: SMI count not available` | the probe block's disclosure region | once per run when that source answers nothing | spec FR-011 |
| `jitter: NMI count not available` | the probe block's disclosure region | once per run when that source answers nothing | spec FR-011 |
| `jitter: probe runs with interrupts enabled` | the probe block's disclosure region | once per run of the `hwlat` method | spec FR-030 |
| `tracer: hwlat already stands as the current tracer` | the tracer block, ahead of the standing `tracer: not available` line | once per run whose `current_tracer` read answered `hwlat` before the setup wrote | spec FR-023, plan P-15 |
| `smi delta: <figure> (source: /dev/cpu/<n>/msr)` | the pre-check block, after the standing attribution lines | once per measured cpu of a `hwlat` run | spec FR-016 |
| `nmi delta: <figure> (source: /proc/interrupts)` | the pre-check block, after the standing attribution lines | once per measured cpu of a `hwlat` run | spec FR-016 |
| `smi gaps: <count>` | the pre-check block, after the standing attribution lines | once per measured cpu of a `hwlat` run | spec FR-016 |
| `tracer: windowUs=<us> widthUs=<us> thresholdUs=<us>` | the tracer lines, after the standing engine header | once per run whose detector ran | spec FR-020, FR-027 |
| `tracer cpu <n> maxGapNs: <figure>` | the tracer lines | once per tracer cpu, cpus ascending | spec FR-027 |
| `tracer cpu <n> gapCount: <figure>` | the tracer lines | once per tracer cpu, cpus ascending | spec FR-027 |
| `tracer cpu <n> nmiCount: <figure>` | the tracer lines | once per tracer cpu, cpus ascending | spec FR-027 |
| `tracer: restore failed: <file> = <saved value>` | the restore call site | once per file whose restore write failed, zero to six lines | spec FR-022 |

## Rules

- The three `stats` figure lines print for a `hwlat` run alone, so a
  `tsc`, `fwq` or `ftq` run prints the bytes it printed before (spec
  FR-025, SC-006).
- The window a figure answers follows the block that prints it: the
  pre-check block answers the pre-check window, and the probe block
  answers the probe's own run span (spec FR-016).
- Each figure line names its source file beside it, the shape the
  standing attribution lines carry at `source/harness/report.cpp:1510`
  to `:1520`. The marked-gap count names no source, because no file
  backs it (spec FR-016).
- A figure whose source answers nothing prints the standing
  `not available` token in the figure position, the token
  `kUnavailableFigure` holds at `source/harness/report.cpp:1094` (spec
  FR-011, FR-027).
- The two disclosure lines print once per run per source, and the run
  keeps status 0 (spec FR-011). The same facts ride the
  `jitter.disclosure` text, so no new key carries them (plan P-12).
- The limits line states that the user-space probe runs with the cpu's
  interrupts enabled, and the document carries the same sentence under
  `jitter.limits` (spec FR-030, FR-031).
- The tracer figure labels stand the JSON key names, the way the
  standing five labels stand the five source names at
  `source/harness/report.cpp:1569` to `:1583`.
- The detector ran in the kernel's `per-cpu` mode, because the run
  writes `hwlat_detector/mode` with `per-cpu` before the engine word
  (spec D-9, P-14). Each tracer cpu therefore carries its own buffer, so
  a cpu the detector watched and found nothing on holds no detector line
  and prints the `not available` token. A zero prints for the `nmiCount`
  figure of a cpu whose lines carry no `nmi-count` field (spec FR-027).
- The restore writes `tracing_on` at `0`, then each saved value in the
  reverse of the write order, so the engine word goes back before the
  threshold (spec FR-021). The saved set holds `current_tracer`,
  `tracing_cpumask`, `hwlat_detector/mode`, `hwlat_detector/width`,
  `hwlat_detector/window` and `tracing_thresh` (spec D-8).
- A restore-failure line names the file below the tracefs root and the
  value the run saved, the shape the standing host-settings line uses
  for its `key=value` pairs at `source/harness/report.cpp:1525`. The
  line prints at the restore call site of
  `source/harness/cli.cpp:1258`, ahead of the pre-check block (spec
  FR-022).
- `tracing_on` stands outside the saved set, so no restore-failure line
  can name it, and the run leaves tracing off the way the standing
  restore does (spec D-8, SC-007).
- A run that names neither new word prints no line above, and every
  standing line stands byte-identical (spec FR-004, FR-029).
- The wording of a message line stays unpinned where this file states
  no exact shape; the tests assert the presence, the order and the
  named figures, the standing rule of the pre-check block.

## Standing contract lines this delta amends

| File and line | Standing text | New text |
| --- | --- | --- |
| `specs/034-host-noise-precheck/contracts/console-report.md` lines 4 to 5 | the field names are the sixteen the console probe block uses | the field names are the nineteen the console probe block uses |
| `specs/034-host-noise-precheck/contracts/console-report.md:18` | `<the sixteen stats lines, one value>` | the sixteen stats lines stand, and the three detector figures join the attribution group after the `switch delta` line, the first two naming their source and the third naming none (FR-016) |
| `specs/034-host-noise-precheck/contracts/console-report.md:21` | the `switch delta` line ends the per-cpu group | three `hwlat` lines follow it, each naming its source |
| `specs/036-osnoise-timerlat-tracers/contracts/console-report.md:15` | `tracer: engine <word> cpus <list>` | unchanged text, and a `hwlat` run adds the settings line below it |
| `specs/036-osnoise-timerlat-tracers/contracts/console-report.md:20` | `tracer cpu <n> hardware: <figure>`, the last figure line | three further figure lines follow it for the `hwlat` engine |

## Consumers

- `test/source/harness_jitter_window_test.cpp`: the three probe figure
  lines, the two disclosures and the limits line over a fake file root
  (spec FR-016, FR-030, FR-031).
- `test/source/harness_noise_report_test.cpp`: the settings line, the
  three tracer figure lines, the not-available line and the
  restore-failure line (spec FR-020, FR-022, FR-027).
- `test/source/harness_hwlat_tracer_test.cpp`: the reverse restore order
  and the file and value each failure line names (spec FR-021, FR-022).
- `specs/034-host-noise-precheck/contracts/console-report.md` and
  `specs/036-osnoise-timerlat-tracers/contracts/console-report.md` carry
  the rows above in the commit that lands the printer (spec FR-029).
