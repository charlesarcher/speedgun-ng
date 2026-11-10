# Host acceptance on the ci-rocky binary (T033)

**Mode**: read-only over the checkout. No configure and no build ran: the
`build/example/docs_command_line` binary already sitting in the shared
`build` tree served every read. This file is the only write.

**Branch**: `051-raw-events`. **Lane head at read time**:
`69907307afdda43626c19cf5f3281d7c276cbaf4`. **Built head of that binary**:
`da32efe9`. **Host**: the machine the `uname -a` line below names.

Every figure below is copied from the output of the command printed beside
it. The tree, host, binary and CLI blocks come from one batch of reads
taken at one moment. The prose gate blocks of the last section were read
after that batch, when this record was finished.

## The tree and the binary at read time

```
git rev-parse --abbrev-ref HEAD
051-raw-events
```

```
git rev-parse HEAD
69907307afdda43626c19cf5f3281d7c276cbaf4
```

```
git log --oneline -1 da32efe9
da32efe9 Counters: Keep the stamp return live
```

```
git log --oneline da32efe9..HEAD
69907307 Docs: Land the heavy gate chain rows
96d97399 Docs: Bind the CI union read to T041
082c94c9 Docs: Correct the closure record
```

```
git diff --name-only da32efe9..HEAD
specs/051-raw-events/results/commits.md
specs/051-raw-events/results/gates.md
specs/051-raw-events/tasks.md
```

```
git log -1 --format='%h %cd %s' --date=iso da32efe9
da32efe9 2026-11-10 03:36:27 -0600 Counters: Keep the stamp return live
```

```
stat -c '%y %n' build/example/docs_command_line
2026-11-10 03:37:12.722375110 -0600 build/example/docs_command_line
```

```
grep -E '^CMAKE_BUILD_TYPE(:STRING)?=' build/CMakeCache.txt
CMAKE_BUILD_TYPE:STRING=Release
```

The three commits above the built head touch three paths under `specs/`,
so the source that produced the binary matches the source at the head named
above. The binary stamp, `03:37:12`, follows the built head's commit stamp,
`03:36:27`. The cache in `build` names `CMAKE_BUILD_TYPE=Release`, the
build type the `ci-rocky` preset of quickstart Prerequisites sets.

The lane moved while this record was being written, so a late read names
the head at finish time and checks the binary claim against it. The diff
over `source`, `include`, `example` and `CMakeLists.txt` prints no path, so
the binary that served these reads matches the buildable source at both
heads:

```
git rev-parse HEAD; echo '---'; git log --oneline da32efe9..HEAD; echo '---'; git diff --name-only da32efe9..HEAD -- source include example CMakeLists.txt; echo "status $?"
a9642a523624267d7c3e8077d5c30305c0bf6c54
---
a9642a52 Docs: Name the rename head and grep shape
b77ebf2a test: Put the errno tables in constant case
69907307 Docs: Land the heavy gate chain rows
96d97399 Docs: Bind the CI union read to T041
082c94c9 Docs: Correct the closure record
---

status 0
```

## Host identity, read at run time

The positive figures of read 1 are a claim about this host, and the family
and model below are copied from this batch's own reads.

```
uname -a
Linux Gary 7.3.3-1-cachyos #1 SMP PREEMPT_DYNAMIC Sat, 07 Nov 2026 18:39:10 +0000 x86_64 GNU/Linux
```

```
lscpu | grep -E '^(Architecture|CPU family|Model|Model name|Stepping|Thread\(s\) per core|Core\(s\) per socket|Socket\(s\)|CPU\(s\)):'; echo "status $?"
Architecture:                            x86_64
CPU(s):                                  32
Model name:                              AMD Ryzen 9 9950X3D 16-Core Processor
CPU family:                              26
Model:                                   68
Thread(s) per core:                      2
Core(s) per socket:                      16
Socket(s):                               1
Stepping:                                0
status 0
```

```
grep -m1 'vendor_id' /proc/cpuinfo
vendor_id	: AuthenticAMD
```

```
grep -m1 'cpu family' /proc/cpuinfo
cpu family	: 26
```

```
grep -m1 'model\s*:' /proc/cpuinfo
model		: 68
```

```
cat /sys/devices/cpu/type
4
```

```
cat /sys/bus/event_source/devices/cpu/type
4
```

```
cat /sys/bus/event_source/devices/cpu/events/instructions
event=0xc0
```

The `cpu` PMU type reads `4` through both path forms. The `instructions`
alias of that PMU reads `event=0xc0`, and the raw spelling in the
acceptance pair, `r` followed by `c0`, names that same config word.
Kernel `7.3.3-1-cachyos`, x86_64, AMD family 26, model 68,
`vendor_id AuthenticAMD`.

## Read 1: two columns over one config word (SC-001)

```
./build/example/docs_command_line --iterations=1 --counter=cpu/instructions --counter=cpu/rc0
speedgun-ng 0.35.0 build=Release
bmRead/cells:256         iterations=         1  real time/iter (ns)=  300.000  cpu time/iter (ns)=  290.000  overhead floor (ns)=  140.000  endpoint samples=2  cpu/instructions=  773.000 ratio=1.000 scaled=0 gap=0  cpu/rc0=  773.000 ratio=1.000 scaled=0 gap=0
bmWrite/cells:256        iterations=         1  real time/iter (ns)=  430.000  cpu time/iter (ns)=  440.000  overhead floor (ns)=  140.000  endpoint samples=2  cpu/instructions=  773.000 ratio=1.000 scaled=0 gap=0  cpu/rc0=  773.000 ratio=1.000 scaled=0 gap=0
```

Exit status 0.

Expectation: two report columns labeled `cpu/instructions` and `cpu/rc0`,
both carrying a positive figure for config word `0xc0`.

Actual: both labels print on both rows, and each carries the figure
`773.000`:

```
cpu/instructions=  773.000 ratio=1.000 scaled=0 gap=0  cpu/rc0=  773.000 ratio=1.000 scaled=0 gap=0
```

Both figures are positive, and the two figures over one row are equal. The
alias read above gives `event=0xc0` for `cpu/instructions`, and `cpu/rc0`
spells the config word `0xc0`, so the two columns name one config word on
this host. The expectation is met.

## Read 2: the same run written to JSON

```
./build/example/docs_command_line --iterations=1 --counter=cpu/instructions --counter=cpu/rc0 --json=/tmp/sg051.json
speedgun-ng 0.35.0 build=Release
bmRead/cells:256         iterations=         1  real time/iter (ns)=  310.000  cpu time/iter (ns)=  280.000  overhead floor (ns)=  150.000  endpoint samples=2  cpu/instructions=  773.000 ratio=1.000 scaled=0 gap=0  cpu/rc0=  773.000 ratio=1.000 scaled=0 gap=0
bmWrite/cells:256        iterations=         1  real time/iter (ns)=  220.000  cpu time/iter (ns)=  220.000  overhead floor (ns)=  140.000  endpoint samples=2  cpu/instructions=  773.000 ratio=1.000 scaled=0 gap=0  cpu/rc0=  773.000 ratio=1.000 scaled=0 gap=0
```

Exit status 0.

```
python3 -c 'import json
d = json.load(open("/tmp/sg051.json"))
print("schemaVersion", d["schemaVersion"])
for b in d["benchmarks"]:
    print(b["name"], "metricLabels", b["metricLabels"])
    for r in b["rows"]:
        print(" ", r["kind"], [m["label"] for m in r["metrics"]])'
schemaVersion 1.16
bmRead/cells:256 metricLabels ['cpu/instructions', 'cpu/rc0']
  REPETITION ['cpu/instructions', 'cpu/rc0']
bmWrite/cells:256 metricLabels ['cpu/instructions', 'cpu/rc0']
  REPETITION ['cpu/instructions', 'cpu/rc0']
```

Exit status 0. The console block and the JSON block come from two runs,
and both runs printed `773.000` in both columns.

Expectation: `schemaVersion` `1.16`, with `cpu/rc0` named in `metricLabels`
and in each row's `metrics[].label`.

Actual: `schemaVersion 1.16`; `metricLabels ['cpu/instructions', 'cpu/rc0']`
under both `bmRead/cells:256` and `bmWrite/cells:256`; and the one
`REPETITION` row of each benchmark carries the row labels
`['cpu/instructions', 'cpu/rc0']`. The expectation is met.

## Read 3: the four refusal reads (SC-002)

The four command lines are copied from quickstart section 3 and from the
T033 task line, and each carries its own `echo "status $?"`.

```
./build/example/docs_command_line --counter=nosuch/r4000; echo "status $?"
--counter: nosuch/r4000: no object at path 'nosuch'
status 2
```

Read 3.1. Expectation: the no-object line at status 2. Actual: that
line, at `status 2`, naming the missing path `nosuch`. The expectation is
met.

```
./build/example/docs_command_line --counter=cpu/r12345678901234567; echo "status $?"
--counter: cpu/r12345678901234567: raw event code exceeds 16 hex digits
status 2
```

Read 3.2. Expectation: the width line at status 2, described in the
expectation as naming 17 digits. Actual: the width line at `status 2`. Its
text names the bound, `16 hex digits`:

```
raw event code exceeds 16 hex digits
```

The offending leaf carries 17 hex digits, and the command line above shows
that leaf. The status matches the expectation, and the printed line carries
the digit count of the bound.

```
./build/example/docs_command_line --counter=nosuch/r12345678901234567; echo "status $?"
--counter: nosuch/r12345678901234567: raw event code exceeds 16 hex digits
status 2
```

Read 3.3. Expectation: the no-object line at status 2. Actual: the width
line at `status 2`, over a leaf that names both an unpublished device and a
17-digit code. The status matches and the line differs: the width gate runs
ahead of the device path lookup.

The lane's own documents name this answer for this exact command.
`specs/051-raw-events/quickstart.md:29-30` reads: "The width line wins when
a wide leaf also names an unpublished device
(`--counter=nosuch/r12345678901234567` answers the width line)." The T033
Verify line of `specs/051-raw-events/tasks.md` names the three answers as
the no-object line, the width line and the width line. The binary matches
the documents of its own lane, and the expectation handed to this worker
holds the no-object line here. That expectation is recorded as given and
stands unchanged.

```
./build/example/docs_command_line --counter=cpu/r4000 --list; echo "status $?"
bmRead/cells:256
bmWrite/cells:256
status 0
```

Read 3.4. Expectation: status 2, refusing the listing with the
raw-in-listing wording. Actual: `status 0`, the two benchmark names, and no
address line.

The lane's own documents name the actual answer. quickstart section 3
expects "the listing still printing at status 0 with no address line", over
the standing order pinned at `test/source/harness_cli_test.cpp:769-775`,
whose check message reads "the listing answers before any address is
walked (FR-087, FR-009)". The T033 Verify line of `specs/051-raw-events/tasks.md`
states the same: "the listing read exits 0 and prints no address line". The
raw-in-listing refusal wording belongs to the hunter surface:
`docs/pages/harness.md:2577` sits under the `Hunter mode` section, and the
pinned hunter refusal stands at `test/source/harness_hunt_cli_test.cpp:394-398`.
No `--hunt` read ran in this record, because T033 names none. The block
below repeats the read with the two flags reversed.

```
./build/example/docs_command_line --list --counter=cpu/r4000; echo "status $?"
bmRead/cells:256
bmWrite/cells:256
status 0
```

## Read 4: the disclosure reads (FR-006, SC-003)

quickstart section 4 writes the disclosure run with `--json=/tmp/sg051.json`.
T033 names `/tmp/sg051b.json` for this record, and the record follows T033.
The two greps re-run the same command line into the shell variable `out`,
the shape quickstart section 4 writes, so the run block and the grep block
hold two separate runs.

```
printf 'counter=cpu/rc0\n' > /tmp/sg051_opts.txt; cat /tmp/sg051_opts.txt
counter=cpu/rc0
```

The option file holds one line.

```
out=$(./build/example/docs_command_line --iterations=1 --options=/tmp/sg051_opts.txt --json=/tmp/sg051b.json 2>&1); echo "status $?"; printf '%s\n' "$out"
status 0
speedgun-ng 0.35.0 build=Release
options from a file or the environment: --counter=cpu/rc0
bmRead/cells:256         iterations=         1  real time/iter (ns)=  280.000  cpu time/iter (ns)=  280.000  overhead floor (ns)=  130.000  endpoint samples=2  cpu/rc0=  657.000 ratio=1.000 scaled=0 gap=0
bmWrite/cells:256        iterations=         1  real time/iter (ns)=  200.000  cpu time/iter (ns)=  190.000  overhead floor (ns)=  130.000  endpoint samples=2  cpu/rc0=  657.000 ratio=1.000 scaled=0 gap=0
```

```
out=$(./build/example/docs_command_line --iterations=1 --options=/tmp/sg051_opts.txt --json=/tmp/sg051b.json 2>&1); grep -F 'options from a file or the environment: --counter=cpu/rc0' <<<"$out"; echo "status $?"; grep -F 'cpu/rc0=' <<<"$out"; echo "status $?"
options from a file or the environment: --counter=cpu/rc0
status 0
bmRead/cells:256         iterations=         1  real time/iter (ns)=  280.000  cpu time/iter (ns)=  270.000  overhead floor (ns)=  130.000  endpoint samples=2  cpu/rc0=  657.000 ratio=1.000 scaled=0 gap=0
bmWrite/cells:256        iterations=         1  real time/iter (ns)=  260.000  cpu time/iter (ns)=  250.000  overhead floor (ns)=  130.000  endpoint samples=2  cpu/rc0=  657.000 ratio=1.000 scaled=0 gap=0
status 0
```

Both greps answer `status 0`. The option line prints ahead of the two
report rows, and each row carries the `cpu/rc0=` column with the positive
figure `657.000`. One column prints in this run, the raw one.

```
python3 -c 'import json
doc = json.load(open("/tmp/sg051b.json"))
print("schemaVersion", doc["schemaVersion"])
listed = [label for b in doc["benchmarks"] for label in b["metricLabels"]]
rows = [m["label"] for b in doc["benchmarks"] for r in b["rows"] for m in r["metrics"]]
print("metricLabels:", listed)
print("row labels:", rows)
print("carriers ok:", listed.count("cpu/rc0"), "labels,", rows.count("cpu/rc0"), "row entries")'
schemaVersion 1.16
metricLabels: ['cpu/rc0', 'cpu/rc0']
row labels: ['cpu/rc0', 'cpu/rc0']
carriers ok: 2 labels, 2 row entries
```

Exit status 0.

Expectation: the console prints the option line, the row carries the
`cpu/rc0=` column, and the JSON answers `1.16`. Actual: the disclosure line
`options from a file or the environment: --counter=cpu/rc0` names the
address the option file held, the two grep reads return that line and both
rows carrying `cpu/rc0=`, and the python read answers `schemaVersion 1.16`
with `cpu/rc0` in `metricLabels` twice and in the row labels twice. The
expectation is met.

## Read 5: the catalog listing count

```
./build/example/docs_command_line --catalog | wc -l; echo "status ${PIPESTATUS[0]}"
467
status 0
```

The listing counts `467` lines, and the binary answers 0 at the head of
the pipeline. `results/base-readings.md:89-90` records 467 at the lane
base, so the figure stands at the base figure, which T033 Verify asks for
(FR-015).

## Verdict over the read set

| Read | Command shape | Expectation | Actual at this read | Verdict |
| --- | --- | --- | --- | --- |
| 1 | `--counter=cpu/instructions --counter=cpu/rc0` | two positive columns over config word `0xc0` | `cpu/instructions=  773.000` and `cpu/rc0=  773.000` on both rows | met |
| 2 | read 1 with `--json=/tmp/sg051.json` | `schemaVersion` `1.16`, `cpu/rc0` in `metricLabels` and in the row labels | `1.16`, and both carriers name `cpu/rc0` | met |
| 3.1 | `--counter=nosuch/r4000` | the no-object line at status 2 | that line at status 2 | met |
| 3.2 | `--counter=cpu/r12345678901234567` | the width line at status 2 | the width line at status 2, naming the 16-digit bound over a 17-digit leaf | met on the status and on the width line |
| 3.3 | `--counter=nosuch/r12345678901234567` | the no-object line at status 2 | the width line at status 2 | the line differs; the status holds |
| 3.4 | `--counter=cpu/r4000 --list` | status 2 with a refusal of the listing | status 0, two benchmark names, no address line | both differ |
| 4 | `--options=/tmp/sg051_opts.txt --json=/tmp/sg051b.json` | the option line, the `cpu/rc0=` column, JSON `1.16` | all three, with `cpu/rc0=  657.000` on both rows | met |
| 5 | `--catalog \| wc -l` | the line count recorded | `467`, equal to the base figure | met |

Reads 3.3 and 3.4 are the two places where the binary answers differently
from the expectation handed to this worker. Both answers match the lane's
own documents, quoted in section Read 3 above: the width gate runs ahead of
the device lookup, and the listing answers before any address is walked.
No expectation, figure or wording in this record was adjusted to fit
another source.

## The prose gate over this record

Tree mode discovers its candidates from `git ls-files`
(`tools/prose/prose_gate.py:598`), and the reads below come from two
moments of this session.

At the first gate read this record stood untracked, so the single-path read
answered zero sources and a clean read over a path list said nothing about
this file:

```
python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/051-raw-events/results/acceptance-host.md
prose-lint: 0 sources, 0 units examined, 0 findings, 0 skipped
status 0
```

```
git status --porcelain -- specs/051-raw-events
 M specs/051-raw-events/results/gates.md
?? specs/051-raw-events/results/acceptance-host.md
status 0
```

The gate kernel then read the record on its own terms.
`/tmp/sg051_gate_one.py` holds a six-line call into `evaluate_prose` of
`tools/prose/prose_gate.py` with `collect_candidates` pointed at this one
path, over the project's own `tools/prose/prose_rules.yaml`. The repository
holds no scratch file:

```
python3 /tmp/sg051_gate_one.py specs/051-raw-events/results/acceptance-host.md
prose-lint: 1 sources, 372 units examined, 0 findings, 0 skipped
status 0
```

This record entered the index between those reads and the next ones, so the
command-line gate reaches it, and the three reads below carry the finished
record:

```
python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/051-raw-events/results/acceptance-host.md
prose-lint: 1 sources, 372 units examined, 0 findings, 0 skipped
status 0
```

```
python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/051-raw-events/results
prose-lint: 11 sources, 3010 units examined, 0 findings, 0 skipped
status 0
```

```
python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/051-raw-events
prose-lint: 18 sources, 5232 units examined, 0 findings, 0 skipped
status 0
```

```
git status --porcelain -- specs/051-raw-events
 M specs/051-raw-events/results/acceptance-host.md
status 0
```

0 findings at this file, 0 findings over the `results` list, and 0 findings
over the lane list. The record's own count stands at 372 units in the two
reads that reach it. The path-list counts move while the lane edits its
documents: two reads of the lane list during this session answered 4879
units and 5232 units, the second one after this record joined the index.
