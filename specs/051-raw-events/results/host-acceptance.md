# The acceptance reading in brief (T035)

The compact reading of the host acceptance run. Every figure below is copied
from `specs/051-raw-events/results/acceptance-host.md`, the T033 log over the
`ci-rocky` binary of this host. That file is the full log: the whole output of
every read, the expectation and the actual text beside each read, the lane
documents quoted for the two reads that answer differently, and the verdict
table.

## Host identity behind the positive figures (SC-001, FR-013)

`/proc/cpuinfo` answers family 26 and model 68 over a `vendor_id` of
AuthenticAMD, the `cpu` PMU device type reads 4 through both path forms, and
the `instructions` alias file answers `event=0xc0`. The six command lines and
their six answer lines stand in six blocks of the full log, gathered here in
read order:

```
grep -m1 'vendor_id' /proc/cpuinfo
vendor_id	: AuthenticAMD
grep -m1 'cpu family' /proc/cpuinfo
cpu family	: 26
grep -m1 'model\s*:' /proc/cpuinfo
model		: 68
cat /sys/devices/cpu/type
4
cat /sys/bus/event_source/devices/cpu/type
4
cat /sys/bus/event_source/devices/cpu/events/instructions
event=0xc0
```

## Two columns over one config word (SC-001)

The pair run of the full log prints both labels on both report rows, each
label with a positive figure, and its command line reads:

```
./build/example/docs_command_line --iterations=1 --counter=cpu/instructions --counter=cpu/rc0
```

The two columns over one report row carry the same figure:

```
cpu/instructions=  773.000 ratio=1.000 scaled=0 gap=0  cpu/rc0=  773.000 ratio=1.000 scaled=0 gap=0
```

The same run written with `--json=/tmp/sg051.json` answers
`schemaVersion 1.16`, and `cpu/rc0` stands in the `metricLabels` of both
benchmarks and in the labels of their rows.

## The four refusal reads (SC-002)

Each command line, each printed line and each status below is copied from the
read that printed it, and those four reads stand in four blocks of the full
log:

```
./build/example/docs_command_line --counter=nosuch/r4000; echo "status $?"
--counter: nosuch/r4000: no object at path 'nosuch'
status 2
./build/example/docs_command_line --counter=cpu/r12345678901234567; echo "status $?"
--counter: cpu/r12345678901234567: raw event code exceeds 16 hex digits
status 2
./build/example/docs_command_line --counter=nosuch/r12345678901234567; echo "status $?"
--counter: nosuch/r12345678901234567: raw event code exceeds 16 hex digits
status 2
./build/example/docs_command_line --counter=cpu/r4000 --list; echo "status $?"
bmRead/cells:256
bmWrite/cells:256
status 0
```

Reads 3.1 and 3.2 print the line their expectation names, each at `status 2`.
Read 3.3 prints the width line at `status 2`: the width gate runs ahead of the
device path lookup, and quickstart section 3 names that answer for that
command line. Read 3.4 answers `status 0` with the two benchmark names and no
address line, which quickstart section 3 names too: the listing answers before
any address is walked. The full log holds both expectations as given.

## The option-file run and its carriers (FR-006, SC-003)

The option file `/tmp/sg051_opts.txt` holds the one line `counter=cpu/rc0`.
The block below is the grep read of the full log in its own order, with the
second report row left out: the run answers `status 0`, the disclosure line
prints ahead of the two report rows, and the row carries the `cpu/rc0=`
column with the positive figure `657.000`:

```
options from a file or the environment: --counter=cpu/rc0
status 0
bmRead/cells:256         iterations=         1  real time/iter (ns)=  280.000  cpu time/iter (ns)=  270.000  overhead floor (ns)=  130.000  endpoint samples=2  cpu/rc0=  657.000 ratio=1.000 scaled=0 gap=0
status 0
```

The python read of that JSON counts `cpu/rc0` twice among the `metricLabels`
of the two benchmarks and twice among the row labels: 2 labels and 2 row
entries, over `schemaVersion 1.16`:

```
schemaVersion 1.16
metricLabels: ['cpu/rc0', 'cpu/rc0']
row labels: ['cpu/rc0', 'cpu/rc0']
carriers ok: 2 labels, 2 row entries
```

## The catalog count (FR-015)

```
./build/example/docs_command_line --catalog | wc -l; echo "status ${PIPESTATUS[0]}"
467
status 0
```

The `--catalog` figure reads 467 with the binary answering 0 at the head of
the pipeline. `specs/051-raw-events/results/base-readings.md:89-90` records
the base figure of 467, so the count stands at its base figure.

## The scripted pins of type 4 and word 0xc0 (SC-001, FR-013)

SC-001 requires the scripted unit to pin the exact PMU type number and config
word. `test/source/counters_raw_event_test.cpp:91` sets `kCoreType = 4U`, and
`test/source/counters_raw_event_test.cpp:488-490` weighs
`openedWith(kCoreType, 0xC0U)` under the check message "the scripted kernel
saw an attribute of type 4 and config 0xc0 (SC-001)".
`test/source/harness_raw_events_test.cpp:204` holds `kSmallRawWord = 0xc0`,
and that file's header records a scripted `cpu` device of PMU type 4. The
scripted units pin type 4 and word `0xc0`, the device type and the config
word read from this host above.
