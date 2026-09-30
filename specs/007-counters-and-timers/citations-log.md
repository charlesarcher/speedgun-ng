# citations log: the successor to the frozen record

`specs/007-counters-and-timers/citations.md` is frozen at the tree of
commit `49f809d`, where it holds 1385 lines. From that commit forward it
receives no edit. Task `T330` in
`specs/007-counters-and-timers/tasks.md` decided the freeze, and this
file carries every correction the record can no longer take.

## The freeze boundary

The frozen tree is the tree commit `49f809d` names. The record holds
1385 lines at that tree, and
`wc -l specs/007-counters-and-timers/citations.md` returns 1385. From
`49f809d` forward the record takes no edit of any kind, so its bytes
stand and its line numbering is permanent.

One line added to the record would move every line below it. The record
carries 201 occurrences of the `citations.md:NNN` self-anchor form
across the 12 feature Markdown files at `49f809d`, and every one of them
names a line the record itself holds, so every one of them would move
with it.

## Why the record is frozen

The record cites its own line numbers. A correction that adds lines
above a cited line moves that line, so the record's own tables go stale
the moment the commit carrying the correction lands. The driver is the
record addressing its own tables by line number, and the line count is
one sufficient cause among several.

Two tasks measured the mechanism before the freeze. `T323` recorded the
commit `1a25e51` caused, and found that it inserted 4 lines at
`citations.md:167` and 35 at `:1077`, so every record line at or above
167 stands 4 higher than the number an earlier text wrote. `T323`
corrected the target column of the re-anchor table and the Site column
of the constitution table, and it left 16 sites for a later pass
because its own added section moved the numbers again.

`T327` closed that loop over the 10 live site pointers, and found the
re-pointing free. Every correction it made was an equal-length in-place
edit, so it added no line to the record and moved no line that any
anchor names. The same commit re-anchored 8 table cells in place, and
that line-neutral edit invalidated 4 further site pointers in the
record's prose, which is the sharper form of the loop.

## The addressing convention

Cite a target by its section heading. Do not cite one by a line number.

The record names the drift criteria at
`specs/007-counters-and-timers/citations.md:106-111`. An entry in this
file that corrects them names the section of the frozen record as
`Method and population`, and the number stays as the frozen position
the entry found, pinned to the head it was measured at.

The record names its own coverage-exclusion section as
`specs/007-counters-and-timers/citations.md:438-458`. An entry in this
file names that same target `The coverage-exclusion population at this
head`. The heading form lands on the section wherever a later commit
places it.

## Entry format

Every correction lands here as one entry. Each entry names the fields
below, in this order.

| Field | Content |
| --- | --- |
| Date | the day the entry was written |
| Task | the `T` task in `tasks.md` that produced the correction |
| Section in the frozen record | the heading of the record section the entry corrects |
| Figure as written | the figure or anchor as the record holds it |
| Figure measured | the figure the command returned |
| Command | the command that measured it |
| Head | the commit the command was run against |
| Must not move | the lines, tables, closed task lines, and preambles the correction left alone |

A filled entry, the first correction taken after the freeze, follows.

```yaml
date: 2026-09-28
task: T327
section: The constitution re-anchored after amendment 2.10.0
figure_as_written: citations.md:455
figure_measured: citations.md:477
command: git diff -U0 1a25e51 1b89b06 -- specs/007-counters-and-timers/citations.md
head: 1b89b06
must_not_move: the 8 Site values of that table, the closed task
  lines, and the dated phase preambles
```

The frozen record holds the full evidence for every figure an entry
corrects, so an entry points at the record's section and repeats only
what a reader needs to re-measure it.

## Requirement corrections

A correction to a merged requirement has no figure to correct. The entry
format above carries line-number drift, where the figure as written names
a number and the figure measured names the number it became. A supersession
names a requirement, and no number moves, so it lands here as a sibling
section and keeps the same discipline: the date, the task, the section of
the frozen record, the claim as the record holds it, what governs it now,
the command that measured it, the head it was measured at, and what the
correction left alone.

### FR-034 of 007, superseded for the time-stamp counter

```yaml
date: 2026-09-29
task: T031
section: Journal sentences a later requirement withdrew
site: citations.md:218, the `:959` row, at the frozen position the entry
  found, pinned to the record's freeze at 49f809d
claim_as_written: the fast `tsc` leaf's frequency is calibrated at
  system-open from sysfs and CPUID
requirement_now_governing: FR-011 at
  specs/008-timestamp-counter/spec.md:211, which records the withdrawal as
  superseding the calibration and publication obligations of FR-034 of 007
  for this counter
code_now_governing: the catalog seed publishes the entry under the
  `SG_COUNTERS_X86` build guard at source/counters/clock_provider.cpp:236-253,
  one build-time condition decides it at :45-49, and the constructor is
  `= default` at :207, reading no file and no instruction identifier
command: grep -rn "tsc_khz" include/ source/ test/ (exit 1, no hit)
head: 08e42fe
must_not_move: the five other rows of that table, the closed task lines, and
  the dated phase preambles
```

A reader who meets the `:959` row reads it as describing the tree at
`49f809d`. FR-011 of 008 withdrew the calibration, and the sysfs read and
the instruction-identifier read went with it. The shipped provider attaches
no rate to the count, and the entry publishes wherever the build executes
the instruction.

## The three routes T330 measured

Route one drops the `citations.md:NNN` Site-column values across the
feature's files and cites by section heading everywhere. It costs the
rewrite of 201 self-anchor occurrences over 181 sites across 12 files.
It breaks the re-anchor table and the constitution table, whose first
column then holds no machine-checkable landing, and it leaves the 118
dated sites `T328` measures with no correction target.

Route two, which this file is, freezes the record at its current state
and puts every future correction in a new dated sibling. It costs one
new file. It breaks nothing, because the 201 self-anchors freeze with
the record and each becomes historical.

Route three pins the record's line count and appends only. It costs one
rule. It forbids the in-place table correction that `T323` performed
and that the tip commit performed, so the 8 Site values the tip commit
wrote become impossible and the 16 sites `T323` found stay
uncorrected.

Route two is the one that reaches a fixed point at once, and the
heading citation removes the driver the measurement found. Route two
won.

## Which mechanism governs

The drift criteria in the frozen record, at its section `Method and
population`, grant an exemption. A sentence describing a past
revision's line numbering lies outside the population those criteria
read, because the sentence reports numbering as the subject of the
claim.

The 201 self-anchors the record holds at `49f809d` fall under that
exemption, and they are historical from this commit forward. A pass
that reads the drift criteria and finds one of them reports no drift,
and the finding is correct.
