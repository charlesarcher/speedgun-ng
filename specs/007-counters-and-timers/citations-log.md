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

## Corrections the 012 feature made

Ten defects carried stable identifiers `I-01` through `I-10` in
`specs/012-counters-defect-resolution/research.md`, and each correction
restores a requirement a merged feature already published. A
requirement correction moves no line in the frozen record, so these ten
entries keep the discipline of the requirement-correction format above:
the claim as the tree held it, the requirement that governs it now, the
site that carries it, the command that measured it, the head, and what
the correction left alone.

The frozen record holds no figure for nine of the ten. It cites
`source/counters/` for anchor totals, prose verdicts, and the
coverage-exclusion population, and it carries no claim about a numeric
table key, a device-scoped row, a per-target probe, a fast read, a short
group read, catalog resolution, a build-time table path, a fast window's
descriptor lifetime, or a clock leaf's order. The tenth, `I-09`, moves
the record's coverage-exclusion population, so that entry carries the
figure pair the format asks for.

### I-01, a numeric table key carried an encoding obligation

```yaml
date: 2026-10-04
task: T035, T036
section: none; the frozen record holds no claim on this requirement
claim_as_written: the parser recorded every integer-valued key as an
  encoding field, so a sampling key such as SampleAfterValue became a
  config word the device format directory never publishes
requirement_now_governing: 007 FR-037, through 012 FR-016 and FR-018
code_now_governing: kernel_spelling at
  source/counters/linux_pmu/table_parse.cpp records a key as an
  encoding field only where it names a kernel format, under the spelling
  the kernel publishes
command: grep -c kernel_spelling source/counters/linux_pmu/table_parse.cpp
  (2)
head: c507754
must_not_move: the four kernel spellings cmask, inv, edge, and
  offcore_rsp, the AMD row counts, and the closed task lines
```

### I-02, a vendored row reached one device

```yaml
date: 2026-10-04
task: T037
section: none; the frozen record holds no claim on this requirement
claim_as_written: merge_vendored appended every vendored row to one
  device, so a host publishing cpu_core and cpu_atom, or an uncore
  device, received no row of its own scope
requirement_now_governing: 007 FR-024, through 012 FR-019
code_now_governing: merge_vendored at
  source/counters/linux_pmu/provider.cpp routes each row by the
  table scope its device publishes
command: grep -c merge_vendored source/counters/linux_pmu/provider.cpp
  (3)
head: c507754
must_not_move: the core-scoped rows the amdzen4 tables publish, and the
  closed task lines
```

### I-03, every availability probe asked as one target

```yaml
date: 2026-10-04
task: T038, T075
section: none; the frozen record holds no claim on this requirement
claim_as_written: the probe took no target, so a device-scoped entry
  refused the per-task event and published permission_blocked while a
  cpu-target plan over the same entry compiled
requirement_now_governing: 007 FR-024 and FR-031, through 012 FR-021 and
  FR-022
code_now_governing: detail::pmu_probe at
  source/counters/linux_pmu/provider.cpp takes the target kinds the
  entry supports, and the catalog carries them in the fixed-size bitmask
  beside the countability state
command: grep -c pmu_probe source/counters/linux_pmu/provider.cpp (4)
head: c507754
must_not_move: the per-action gap value, and the closed task lines
```

### I-04, the fast read published a masked value and a stale retry

```yaml
date: 2026-10-04
task: T008, T013, T015
section: none; the frozen record holds no claim on this requirement
claim_as_written: fast_decode masked the sum to the published counter
  width, a read the page refused kept the value an earlier action wrote,
  and a window the kernel refused to time reported the ratio of a window
  that ran its whole length
requirement_now_governing: 007 FR-013, FR-019, FR-040, and FR-041,
  through 012 FR-002 through FR-005
code_now_governing: fast_decode at
  source/counters/linux_pmu/fast_read.cpp sign extends as the
  kernel interface header documents, and the managed disclosure column
  carries the gap instead of an earlier value
command: grep -c fast_decode source/counters/linux_pmu/fast_read.cpp (5)
head: c507754
must_not_move: the stated fallback that issues no syscall read, and the
  closed task lines
```

### I-05, a short group read read as a zero count

```yaml
date: 2026-10-04
task: T009, T016
section: none; the frozen record holds no claim on this requirement
claim_as_written: a group read returning fewer bytes than the group
  header filled zeros and continued, so a fold across the action read a
  delta no read produced
requirement_now_governing: 007 FR-011, through 012 FR-006
code_now_governing: group_read_short at
  source/counters/linux_pmu/group_io.cpp marks the action, and the
  managed disclosure column carries the mark
command: grep -c group_read_short source/counters/linux_pmu/group_io.cpp
  (2)
head: c507754
must_not_move: put's one-integer signature, and the closed task lines
```

### I-06, catalog resolution wrote shared state

```yaml
date: 2026-10-04
task: T021
section: none; the frozen record holds no claim on this requirement
claim_as_written: two threads resolving one canonical address both
  reached the emplace that writes the handle back, and plan compilation
  wrote the open flag on every call
requirement_now_governing: 007 FR-009 and FR-031, through 012 FR-010
  through FR-012
code_now_governing: system::handle_for at source/counters/system.cpp
  reads the map under the guard and writes once, and the thread
  sanitizer preset reports no race
command: grep -c handle_for source/counters/system.cpp (5)
head: c507754
must_not_move: the thread sanitizer preset's separation from the address
  and undefined-behavior preset, and the closed task lines
```

### I-07, the tables resolved through a build-time path

```yaml
date: 2026-10-04
task: T042, T043, T044
section: none; the frozen record holds no claim on this requirement
claim_as_written: SG_PMU_EVENTS_DIR named a directory inside the source
  tree, so an installed archive published no vendored row
requirement_now_governing: 007 FR-050 and SC-008, through 012 FR-023
code_now_governing: the vendored JSON travels as static bytes generated by
  cmake/embed_pmu_blob.cmake, and the existing simdjson path decodes them
  at run time
command: grep -rn SG_PMU_EVENTS_DIR include/ source/ test/ (exit 1, no
  hit)
head: c507754
must_not_move: the simdjson parse path, the added codec count at zero,
  and the closed task lines
```

### I-08, a fast window released nothing

```yaml
date: 2026-10-04
task: T029
section: none; the frozen record holds no claim on this requirement
claim_as_written: fast_context held a descriptor and a mapping and
  declared no destructor, so every destroyed fast-mode plan leaked one of
  each per member leaf
requirement_now_governing: 007 FR-040 and FR-041, through 012 FR-013
  through FR-015
code_now_governing: fast_context_close at
  source/counters/linux_pmu/group_io.cpp serves the destructor and
  the partial-open arms
command: grep -c fast_context_close
  source/counters/linux_pmu/group_io.cpp (1)
head: c507754
must_not_move: the syscall window's existing destructor, and the closed
  task lines
```

### I-09, the calibration sat inside a coverage exclusion

```yaml
date: 2026-10-04
task: T046
section: The coverage-exclusion population at this head
figure_as_written: 301 marker lines and 301 tokens over 11 files
figure_measured: 384 marker lines and 384 tokens over 12 files at this
  feature's head, and 387 tokens over 12 files at this feature's base
  6aafd2d
command: rg -c 'LCOV_EXCL' source/counters
  include/speedgun-ng/counters*.hpp
claim_as_written: the whole calibration region sat inside one exclusion
  pair, so no registered test could reach the code that subtracts the
  bracketing clock reads
requirement_now_governing: 011 R-006, through 012 FR-025 and FR-026
code_now_governing: calibrate at source/counters/plan.cpp subtracts
  its bracket with no exclusion around it, and a registered test
  exercises it
head: c507754
must_not_move: the exclusion population outside the lines this feature
  changed, which falls from 387 to 384 tokens over the feature
```

### I-10, two leaves documented an order their clock does not provide

```yaml
date: 2026-10-04
task: T049, T052
section: none; the frozen record holds no claim on this requirement
claim_as_written: the clock class stated one order guarantee for every
  leaf, so the per-thread CPU clock read as ordered across threads and
  the timestamp counter read as ordered across processors
requirement_now_governing: 011 FR-006 and FR-007, through 012 FR-028
  through FR-031
code_now_governing: include/speedgun-ng/counters_clock.hpp states each
  leaf's own guarantee, and the timestamp counter's states the
  precondition that one thread takes both endpoints
command: grep -c order include/speedgun-ng/counters_clock.hpp (4)
head: c507754
must_not_move: the timestamp leaf's per-read cost, which rises by no
  ordering fence, and the closed task lines
```

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

## Corrections taken by specs/013-counters-defect-followup

One correction here names the arithmetic. The figure stays as recorded.
D-05, the register-filter decision, leaves the encodable-row count
unmoved: `MSRValue` and `MSRIndex` were already dropped at parse
time, so a row was already counted encodable whatever register formats
the device published. Every count that moved in this feature moved
under D-06, the counter-constraint and deprecation decision. A reader
who attributes a moved count to the register filter would credit the
wrong decision for it.

Every entry below was measured at head `0dea082` on the machine the
quickstart's step 1 records. The frozen record at
`specs/007-counters-and-timers/citations.md` is unedited.

```yaml
date: 2026-10-06
task: T017
section: The encodable-row counts the seam test pins
figure_as_written: skylake 576, icelake 342, alderlake 521, sapphirerapids 1685
figure_measured: skylake 581, icelake 346, alderlake 563, sapphirerapids 1993
command: ./build/dev/test/counters_linux_pmu_seam_test
head: 0dea082
must_not_move: the amdzen4 and amdzen5 figures, both unchanged
```

```yaml
date: 2026-10-06
task: T017
section: The encodable-row counts the seam test pins
figure_as_written: alderlake 564 after both decisions land
figure_measured: alderlake 563
command: ./build/dev/test/counters_linux_pmu_seam_test
head: 0dea082
must_not_move: the sapphirerapids figure on the same line
```

```yaml
date: 2026-10-06
task: T017
section: The encodable-row counts the seam test pins
figure_as_written: sapphirerapids 2141 after both decisions land
figure_measured: sapphirerapids 1993
command: ./build/dev/test/counters_linux_pmu_seam_test
head: 0dea082
must_not_move: the 566 of 579 rows the placement scenario measures
```

```yaml
date: 2026-10-06
task: T017
section: The rows still unencodable after both decisions land
figure_as_written: no key named, so every sapphirerapids row encodes
figure_measured: 293 rows name a PortMask and 250 name an FCMask
command: ./build/dev/test/counters_linux_pmu_seam_test
head: 0dea082
must_not_move: the four directories whose counts moved
```

```yaml
date: 2026-10-06
task: T027
section: The core catalog the availability probe reports
figure_as_written: 8 pmu catalog entries on the cpu device
figure_measured: 385 pmu catalog entries, 353 on the cpu device
command: ./build/dev/test/counters_pmu_test
head: 0dea082
must_not_move: the kernel-alias count, which stays 8
```

```yaml
date: 2026-10-06
task: T029
section: The package config compatibility the install writes
figure_as_written: MinorVersion
figure_measured: SameMinorVersion, the only spelling CMake defines
command: cmake --preset=ci-ubuntu
head: 0dea082
must_not_move: the generated PACKAGE_VERSION, which reads 0.4.0
```

```yaml
date: 2026-10-06
task: T029
section: The shared-object version the 0.x line carries
figure_as_written: the major position, which is 0 on this line
figure_measured: 1, declared and dormant while BUILD_SHARED_LIBS is off
command: grep SOVERSION CMakeLists.txt
head: 0dea082
must_not_move: the project VERSION line, which reads 0.4.0
```

```yaml
date: 2026-10-06
task: T048
section: The encodable-row counts the seam test pins
figure_as_written: skylake 581 and sapphirerapids 1993, with PortMask and FCMask unpublished
figure_measured: skylake 587 and sapphirerapids 2222 against the synthetic list
command: ./build/dev/test/counters_linux_pmu_seam_test
head: 0dea082
must_not_move: the reference-host pins, which stay at 581 and 1993
```

```yaml
date: 2026-10-06
task: T049
section: Device scope from published per-task context
figure_as_written: every device starts per-task capable, and a probe errno settles scope
figure_measured: a device publishing cpumask or cpus is device-scoped before any probe
command: ./build/dev/test/counters_linux_pmu_seam_test
head: 0dea082
must_not_move: the msr thread-target path, which still takes the per-task probe
```

```yaml
date: 2026-10-06
task: T050
section: Version lineage for the 0.4.0 bump
figure_as_written: the spec 012 plan records 0.3.0 and names neither cd5cbd1 removal
figure_measured: the Version lineage names both removals and the move from 0.3.0 to 0.4.0
command: grep -n cd5cbd1 specs/012-counters-defect-resolution/plan.md include/speedgun-ng/counters_measurement.hpp
head: 0dea082
must_not_move: the project VERSION line, which reads 0.4.0
```

The seven entries below are requirement corrections. Each names the 007
or 012 requirement the correction restores. The commands were run on the
working tree at `c059b0f`. That commit is the base. The corrections
themselves are uncommitted, because this pass does not commit.

### I-01, a non-zero register value whose index names no format

```yaml
date: 2026-10-06
task: T053
section: none; the frozen record holds no claim on this requirement
claim_as_written: a non-zero register value whose index named no format
  encoded as the base event, so the count appeared without its filter
requirement_now_governing: 007 FR-037, through 012 FR-017
code_now_governing: register_filter_of at
  source/counters/linux_pmu/table_parse.cpp marks the row, and
  composition refuses the field no device publishes
command: grep -c kUnnamedRegister source/counters/linux_pmu/table_parse.cpp
  (2)
head: c059b0f, working tree
must_not_move: the pinned encodable-row counts, and the closed task lines
```

### I-02, a suffixed unit reached no device

```yaml
date: 2026-10-06
task: T054
section: none; the frozen record holds no claim on this requirement
claim_as_written: scope_reaches stripped the device suffix and compared
  it to the whole unit, so a unit that already carried an instance suffix
  reached no device
requirement_now_governing: 007 FR-024, through 012 FR-019
code_now_governing: unit_names_instance in scope_reaches at
  source/counters/linux_pmu/provider.cpp keeps the suffix when the unit
  already carries one, so cbox_0, imc_free_running_0, and
  imc_free_running_1 each reach that one device
command: grep -c unit_names_instance source/counters/linux_pmu/provider.cpp
  (2)
head: c059b0f, working tree
must_not_move: the class-wide rule for a unit that names no instance, and
  the closed task lines
```

### I-03, device scope came from the device name

```yaml
date: 2026-10-06
task: T049
section: none; the frozen record holds no claim on this requirement
claim_as_written: every device other than the three core names was marked
  device-scoped by spelling, and the per-task probe was skipped
requirement_now_governing: 007 FR-024 and FR-031, through 012 FR-021 and
  FR-022
code_now_governing: load_device at source/counters/linux_pmu/provider.cpp
  marks a device scoped when it publishes cpumask or cpus, and a device
  that publishes neither takes the per-task probe
command: grep -c device_scoped source/counters/linux_pmu/provider.cpp (6)
head: c059b0f, working tree
must_not_move: the msr thread-target path, and the closed task lines
```

### I-04(a), one host-wide fast verdict covered every device

```yaml
date: 2026-10-06
task: T059
section: none; the frozen record holds no claim on this requirement
claim_as_written: one core-event probe set the fast mode on every
  countable entry, including a device whose own page refuses the read.
  The T027 entry named a cap_user_rdpmc read in provider.cpp that the
  file did not perform; the verdict was the host probe and a sysfs
  rdpmc file
requirement_now_governing: 007 FR-013, through 012 FR-001 and 013 FR-017
code_now_governing: device_page_fast_verdict at
  source/counters/linux_pmu/provider.cpp opens one event of that device
  and reads cap_user_rdpmc from the page the kernel maps. The host-wide
  instructions probe and the sysfs rdpmc file do not decide the verdict
command: grep -c cap_user_rdpmc source/counters/linux_pmu/provider.cpp (6)
head: 7ea0535, working tree
must_not_move: the sampling-time refusal, which still discloses a gap and
  issues no syscall read, and the closed task lines
```

### I-04(d), the fast window copied the raw time pair

```yaml
date: 2026-10-06
task: T052
section: none; the frozen record holds no claim on this requirement
claim_as_written: the enabled and running pair was copied from the page,
  and the scale, offset, shift, index, and short-counter fields were read
  after the sequence comparison
requirement_now_governing: 007 FR-019, through 012 FR-005
code_now_governing: fast_context_time_pair at
  source/counters/linux_pmu/fast_read.cpp reads those fields inside the
  sequence snapshot, before the stability comparison
command: grep -c 'page->time_offset' source/counters/linux_pmu/fast_read.cpp
  (1)
head: c059b0f, working tree
must_not_move: the short-counter correction in fast_time_pair, and the
  closed task lines
```

### I-05, a fold computed a value from a gap mark

```yaml
date: 2026-10-06
task: T010
section: none; the frozen record holds no claim on this requirement
claim_as_written: a fold subtracted across a gap mark and reported the
  zero that mark wrote as a count
requirement_now_governing: 007 FR-011, through 012 FR-006
code_now_governing: the delta paths in source/counters/fold.cpp read the
  disclosure mark at both end points and report availability::gap with no
  value when either end point carries it
command: grep -c 'availability::gap' source/counters/fold.cpp (3)
head: c059b0f, working tree
must_not_move: point_sink::put's one-integer signature, and the closed
  task lines
```

### I-07, the installed catalog count had no comparison

```yaml
date: 2026-10-06
task: T031
section: none; the frozen record holds no claim on this requirement
claim_as_written: the continuous-integration consumer step printed the
  installed catalog count and compared it to nothing
requirement_now_governing: 007 SC-008, through 012 FR-023 and 012 SC-008
code_now_governing: the comparison step in .github/workflows/ci.yml runs
  both figures on the same runner and exits non-zero on a difference
command: grep -c 'installed and build-tree counts agree'
  .github/workflows/ci.yml (1)
head: c059b0f, working tree
must_not_move: the embedding of the vendored tables, and the closed task
  lines
```

## Corrections after the 013 merge

A review of `daa4b6d` found two high defects and a set of small
corrections. The entries below record them. The project version for the
patch is 0.4.1. `SOVERSION` stays 1.

### Hybrid core devices were marked device-scoped

```yaml
date: 2026-10-07
task: review of daa4b6d
section: Device scope from published per-task context
claim_as_written: a device publishing cpumask or cpus is device-scoped
figure_measured: a cpus file does not mark a device scoped. cpu_core and
  cpu_atom stay per-task capable
command: ./build/dev/test/counters_linux_pmu_seam_test
head: daa4b6d
must_not_move: the msr path, which still takes the per-task probe
```

### Offcore rows encoded event 0

```yaml
date: 2026-10-07
task: review of daa4b6d
section: none; the frozen record holds no claim on this requirement
claim_as_written: an EventCode pair such as 0xB7, 0xBB encoded with event
  bits of 0
figure_measured: the first code reaches bits 0-7. The encodable-row counts
  on the synthetic list stay 587, 346, 563, and 2222
command: ./build/dev/test/counters_linux_pmu_seam_test
head: daa4b6d
must_not_move: the synthetic-list Intel pins
```

### The reference-host format list was not this host's list

```yaml
date: 2026-10-07
task: review of daa4b6d
section: The encodable-row counts the seam test pins
figure_as_written: amdzen4 326 and amdzen5 322 against an Intel format list
figure_measured: amdzen4 344 and amdzen5 353 against this host's format
  list, event config:0-7,32-35 plus umask, edge, inv, and cmask
command: ./build/dev/test/counters_linux_pmu_seam_test
head: daa4b6d
must_not_move: the synthetic Intel pins 587, 346, 563, and 2222
```

### The 014 rename map carries the renamed spellings

```yaml
date: 2026-10-07
task: specs/014-identifier-naming-camelcase closing commit
section: none; the map carries every changed shipped-header spelling
claim_as_written: the counters and dbc names as this log spells them
  predate the rename
figure_measured: specs/014-identifier-naming-camelcase/rename-map.md
  lists old spelling, new spelling, declaring header, and kind for
  every changed shipped-header name, detail names marked
command: sed -n '1,20p' specs/014-identifier-naming-camelcase/rename-map.md
head: fb7ba67
must_not_move: every closed spec directory except the two sentences
  below
```

The two sentences naming the shared-object rule in a closed directory
were corrected beside this entry:
`specs/013-counters-defect-followup/spec.md:891` and
`specs/013-counters-defect-followup/checklists/requirements.md:44`
now call the shared-object version a hand-kept number, the live rule
`specs/013-counters-defect-followup/spec.md:657` already states. No
other closed directory was edited.
