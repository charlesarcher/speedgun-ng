# The NPS evidence plan

The acceptance runs stand on thrawn, the Threadripper 9960X lane
host, one record per NPS state the board's menu offers. The record
set covers every state the menu offers; a state this lane cannot
produce stands recorded as out of reach, with the gap named
(FR-019, D-7, R-11).

## The state list

The board's menu offers three NPS states: `NPS1`, `NPS2` and
`NPS4`. The `nps` field of the record names the state the boot ran
on, and the listing the run printed states the nodes the read
carried: `NPS1` reads one node, `NPS2` reads two, `NPS4` reads
four.

## The BIOS change and the reboot

1. Save the work and note the current state from the last record or
   the listing (`NUMA node(s)` of `lscpu`).
2. Enter the BIOS setup at power-on, walk `AMD CBS` → `NBIO Common
   Options` → `NPS Setting` (the menu names the field `NPS`), set
   the state, save and exit.
3. The board reboots twice on a memory-map change; wait for the
   second boot before running anything.
4. Confirm the state with the listing before any run: the node
   family names the nodes the read carried.

## The command set per state

The commands run from the repository root on the built tree; the
record carries each command, its exit code and the listing it
printed:

```
./build/example/docs_topology_placement --topology
./build/example/docs_numa_placement --placement=node
./build/example/docs_numa_placement --placement=node '--jitter-probe=idle,mode=precheck,precheck=50'
ctest --test-dir build -R harness_numa_placement_test
```

The pre-check run stands beside the placement runs in every record,
the merged pre-check the lane's later lanes read (R-11).

## The record format

One file per state, `nps<value>.md`, beside this plan:

- the state, the boot date, and the `lscpu` NUMA line;
- each command, its exit code, and the listing or report it printed;
- the pre-check block summary and the warning lines it printed;
- the gap that stands between the state and the runs, when one
  stands, named in the state's own record.
