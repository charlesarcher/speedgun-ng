# Data Model: Lifecycle of the Programmable Limit Table

**Feature**: `050-limit-table-lifecycle` | **Spec**: `specs/050-limit-table-lifecycle/spec.md`

The entity this lane moves, the field it adds, the objects that carry a record,
the states one table passes through, and the split between two `System` objects
in one process. `FR-nnn`, `D-n` and `SC-nnn` cite `spec.md`. The shipped rule
stands in `contracts/limit-table-ownership.md`, the modules, the commit order
and the FR coverage map stand in `plan.md`, and the five cases stand in SC-001.
The house shape comes from `specs/047-loader-cross-field/data-model.md` and
`specs/042-topdown/data-model.md`.

One member changes owner and one field joins one record: the limit table moves
into `System::Impl`, and a tree node gains the address of the `System` that
owns it. No public layout moves. `Object` keeps its one member
(`include/speedgun-ng/counters_system.hpp:194`) and `System` keeps its one
member (`:383`), so no exported layout moves. Hard rule 12 moves the counters
archive to `SOVERSION 8` (`CMakeLists.txt:168`), because this feature changes
the public header (D-2).

## Evidence

Code facts carry a `path:line` opened on branch `050-limit-table-lifecycle`,
read at head `fba814d2` (`git rev-parse --short HEAD`). The rebase to that
head moved code files, so each number below names the head line that now
carries that reading. A reading that names code this lane removed keeps the
base `858c55d9` in place of a line number, because no head line carries it.
Probe facts name the probe and the note that records the run. Paths
stand short here: `system.cpp` is `source/counters/system.cpp`, `core.hpp` is
`source/counters/detail/core.hpp`, `counters_system.hpp` and
`counters_fake.hpp` stand in `include/speedgun-ng/`, and `clock_provider.cpp`,
`fake_provider.cpp` and the two `linux_pmu/` files stand in
`source/counters/`. `spec.md` and `plan.md` carry no line number here: the lane
edits both while it plans, so each citation names a requirement, a decision, a
scenario or a section instead.

| Fact | Figure | Command |
| --- | --- | --- |
| The table at the base | one `std::map<std::string, std::size_t>` in a function-local static, behind an anonymous-namespace accessor | `grep -n programmableLimitTable source/counters/system.cpp` at the base `858c55d9`; this lane deletes that accessor |
| Its one write, its one read | the commit block of `registerProvider`, and `Object::programmableCounters()` | same grep, then `sed` of `system.cpp:292-294` and `:436-438` |
| The two node creations | the `machine` root, and one node per accepted seed | `grep -n 'make_unique<TreeNode>' source/counters/system.cpp` (`:184`, `:264`) |
| `System::Impl` today | providers, objects, handles, handlesLock, aliases, open, and now `limits` | `core.hpp:196-256` |
| `TreeNode` today | kind, path, alias, description, leaves, and now the `owner` pointer of D-4 | `core.hpp:42-50` |
| The reachability gap at the base | the constructor pair stood private, and `local()` was the one producer | `counters_system.hpp` at the base `858c55d9`; `local()` stands at `system.cpp:194-198` |
| Two `System` objects at the base | a first `System` scripted 4, a second scripted 2, then both answer 2 | `/tmp/sg050/probe5.cpp` section A1, recorded in `/tmp/050-clarify-round1.md` |
| An unscripted path at the base | the reading `System` answers 4 for a path it declared and never scripted | `probe_unscripted.cpp`, recorded in `/tmp/050-clarify-round2.md` |
| An ended `System` at the base | a fresh `System` that scripts nothing answers 8 | `probe_ended.cpp`, same note |
| The two walk reads at the base | `parent()` answers null and `children()` answers 0, where the owning tree answers `cpu` and 1 | `probe_walks.cpp`, same note |
| The probe verdict table at the base | `targets=2 avail=0` for a later `System` that wrote nothing | `probe_verdict.cpp`, same note; `kTargetCpuBit` reads 2 at `include/speedgun-ng/counters_core.hpp:113` |
| Gated interfaces at the base | 347 interfaces, 0 gaps, and the private constructor pair held no row | `cmake --build build/dev --target dbc-gate`, recorded in `/tmp/050-dbc-notes.md` |
| Version and the two archive numbers | 0.33.0, counters 8, harness 21 | `grep -n 'VERSION 0.33.0\|SOVERSION ' CMakeLists.txt` (`:78`, `:168`, `:444`) |

The four probe rows carry the four answers SC-001 must beat. `plan.md`, the FR
coverage map, names the same four figures beside the four requirements.

## Key entities and what this lane adds

`spec.md`, Key Entities, names six entities. One gains a field here, one gains
a home, and four stand where they stand today.

| Entity | Field this lane adds | Where it lands |
| --- | --- | --- |
| Limit table | the member itself | E-01, E-02 |
| Owning `System` | the table it holds, and nothing else | E-02 (`core.hpp:196-256`) |
| Tree node | `owner` | E-03 (`core.hpp:42-50`) |
| Handle (`Object`) | none | E-04 (`counters_system.hpp:194`) |
| Open boundary | none | E-04 (`core.hpp:220`) |
| Scripted limit | none | E-04 (`counters_fake.hpp:169-170`) |

## E-01 The limit table

One `System`'s record set: the canonical path of each object whose event source
answered, mapped to the count that source published.

| Part | Carrier | Type | Rule |
| --- | --- | --- | --- |
| key | `System::Impl::limits`, named in `plan.md`, Physical view | `std::string` | the object's canonical path, the spelling `Impl::objects` already keys on (`core.hpp:199`) |
| value | the same member | `std::size_t` | how many programmable counters one context can hold open at once on that object's event source (FR-002) |
| a record | one entry of that map | pair | one object whose source answered at `enumerate`; a source that answered nothing leaves no entry (`system.cpp:253-255`) |
| the map | `std::map<std::string, std::size_t>` | member | the type the base accessor held, kept by the member (`core.hpp:214`), so the shape stands and only the owner moves |

### Which objects carry a record

| Object | Record | Why |
| --- | --- | --- |
| a scripted fake object | the scripted count | `setProgrammableCounters` stores it (`fake_provider.cpp:207`) and `enumerate` publishes it (`:318`) |
| a `linux_pmu` device | the decoded count, or nothing | the seed carries `detail::usableCounters(...)` (`linux_pmu/provider.cpp:858-860`), and a version-0 or count-0 read publishes nothing (specs/024 contract `:70`) |
| an object nobody scripted | none | the seed member defaults to no value (`counters_provider.hpp:78`), so the batch stages nothing (`system.cpp:253-255`) |
| the `machine` clock root | none | the clock provider seeds the root with the member left out (`clock_provider.cpp:302-308`), because a clock consumes no programmable counter (specs/024 contract `:71`) |
| the root through the fake | none | `addObject` refuses `machine` (`fake_provider.cpp:169-171`), so no script names the root |

The write loop tests no path against the root name: a seed that named `machine`
and carried a value would write a record (`system.cpp:253-255`, `:292-294`). No
shipped source does that, so the root carries none, the answer the Edge case
"The machine root answers nothing" states.

### Fill then read

Registration is the only writer, and its window closes at the open.
`registerProvider` refuses a call once that `System` stands open
(`system.cpp:203-207`), the first resolution that needs the catalog sets the
flag through a compare-and-exchange (`core.hpp:215-230`), and `System::tsc()`
resolves without setting it (`system.cpp:317-323`). A read needs a handle, and
every handle comes from `System::object` or `System::objects`, each of which
opens the boundary first (`system.cpp:302`, `:314`, `:374`, `:408`). Within one
`System` no read therefore races a write, and two `System` objects share no
mutable object (D-6).

## E-02 `System::Impl` (gains the member)

| Member | Type | Line today | After this lane |
| --- | --- | --- | --- |
| `providers` | `std::vector<std::unique_ptr<ProviderIface>>` | `core.hpp:198` | unchanged |
| `objects` | `std::map<std::string, std::unique_ptr<TreeNode>>` | `:199` | unchanged |
| `handles` | `std::map<std::string, std::unique_ptr<Object>>` | `:200` | unchanged |
| `handlesLock` | `std::mutex` | `:206` | unchanged |
| `aliases` | `std::map<std::string, std::string>` | `:207` | unchanged |
| `open` | `std::atomic<bool>` | `:220` | unchanged |
| `limits` | `std::map<std::string, std::size_t>` | none | new, D-3 |

`System::Impl` stands private to `System` (`counters_system.hpp:382`) and is
defined only in `core.hpp:196-256`. `m_impl` reaches it (`include/speedgun-ng/counters_system.hpp:383`), so the
owning `System` named by a node answers both reads of that node: the limit
read takes `owner->m_impl->limits`, and the two walk reads take
`System::handleFor`, which a member of `Object` may call through
`friend class Object` at `include/speedgun-ng/counters_system.hpp:369` (`plan.md`, the note below the Decision index).

## E-03 `TreeNode` (gains the owner)

| Field | Type | Line today | Rule after this lane |
| --- | --- | --- | --- |
| `kind`, `path`, `alias`, `description`, `leaves` | `std::string`, `std::vector<LeafRecord>` | `core.hpp:44-48` | unchanged |
| `owner` | `System*` | none | the `System` whose tree holds this node (FR-007, D-4) |

| Fact | Reading | Line |
| --- | --- | --- |
| who sets it | `System::System()` for the `machine` root, and `System::registerProvider` for every accepted seed: the only two sites that make a node | `system.cpp:184`, `:264` |
| when | before the node enters the tree, so no node a read can reach stands without an owner | `system.cpp:189`, `:287` |
| what one field answers | the limit read and the two walk reads, because the pointer names a `System` and its `handleFor` | `system.cpp:434-443`, `:456`, `:459`, `:508`, `:522` |
| what it may never be | a pointer to a `System` other than the one whose tree holds the node, and a pointer a read follows after that `System` ends | Edge case "A handle that outlives its `System`" |

A node lives inside `Impl::objects`, so it ends together with the `System` that
owns it. The documented rule for a handle stands as it is today: a handle names
a node inside the tree of its `System` and stops being valid when that `System`
ends (Edge case "A handle that outlives its `System`",
`counters_system.hpp:31-32`). This lane adds no lifetime guarantee and removes
none, so a use of a handle after its `System` ends stands outside this model.

## E-04 Handle, open boundary, scripted limit

| Entity | Carrier | Line | Answer it keeps |
| --- | --- | --- | --- |
| Handle (`Object`) | one `void*` member naming a tree node | `counters_system.hpp:194`; built at `system.cpp:363-366` | one member, so no exported layout moves (D-2) |
| Open boundary | `Impl::open` with `ensureOpen()` and `isOpen()` | `core.hpp:220`, `:225-230`, `:232-235` | the first resolution closes registration (specs/007 FR-009) |
| Scripted limit | `FakeProvider::setProgrammableCounters` | `counters_fake.hpp:169-170`; stored at `fake_provider.cpp:207` | the fake's per-path script, and a trap for a path the fake owns no object for (`:203-205`) |

## The route each read takes

| Read | Route today | Route after this lane | Lines |
| --- | --- | --- | --- |
| `Object::programmableCounters()` | `m_node` then the one process-wide table | `m_node`, then `owner`, then `owner->m_impl->limits` | `system.cpp:434-443`, the lookup at `:436-438` |
| `Object::parent()` | `System::local()`, then its handle map | `node->owner`, then that `System`'s map | `system.cpp:451`, `:456`, `:459` |
| `Object::children()` | `System::local()`, then its object map | `node->owner`, then that `System`'s map | `system.cpp:506`, `:508`, `:522` |

A hit answers its count and a miss answers no value: the two answers the base
gives for the one `System` a program holds (FR-002). `System::local()` keeps
its meaning, the shared handle of the process (`system.cpp:194-198`, FR-008).

## States of one table

```text
build a System        -> empty table; the tree holds the machine root alone   (system.cpp:181-190)
provider accepted     -> every answered limit of that provider enters          (system.cpp:292-294)
provider refused      -> nothing enters; the commit block never runs           (system.cpp:230-283)
second provider       -> its limits join the same table                        (system.cpp:292-294)
first resolution      -> the open closes registration                          (core.hpp:225-230, system.cpp:203-207)
read                  -> a hit answers its count, a miss answers no value      (system.cpp:436-442)
~System               -> the table ends with Impl                              (system.cpp:192)
```

No state returns to an earlier one. A registration after the open answers a
refusal and writes nothing (`system.cpp:203-207`). A later `System` starts at
the first state, because the constructor builds a fresh `Impl` and an empty
map (`system.cpp:181-190`). `~System()` is defined `= default`, so the
`std::unique_ptr` member releases the tree and the table together
(`system.cpp:192`, `core.hpp:196-256`).

## Two `System` objects in one process

| Each `System` holds | Line |
| --- | --- |
| one tree of nodes, and one limit table (new) | `core.hpp:199`, `plan.md`, Physical view |
| one provider list | `core.hpp:198` |
| one handle map and its own lock | `core.hpp:200`, `:206` |
| one alias map | `core.hpp:207` |
| one open flag | `core.hpp:220` |

| The two share | Reading | Source |
| --- | --- | --- |
| the path space | `cpu` spells one canonical string, so each table holds its own record for it and each read answers its own table | Edge case "Two objects of two `System` objects share a path" |
| no mutable object | each table takes its writes before its own open and its reads after it, so no lock spans the two | Edge case "Two `System` objects open in two threads", D-6 |
| the refused-provider rule | the staging lists belong to the one `System` whose call they serve, so a refusal touches one table and it touches nothing | Edge case "A refused provider writes nothing", `system.cpp:221-228` |
| two process-wide catalog facts | the probe verdict table and the resolved metric table keep their lifetime, so a fact one `System` wrote still answers a later one | D-8, `probe_verdict.cpp` in `/tmp/050-clarify-round2.md` |
| the harness registrations | the harness binds `System::local()` and registers its providers there, and names it at ten measured call sites, so its reads answer the local `System`'s table as they do today | Assumptions, `cli.cpp:666`, `:679`, `:700` |

## What does not move

| Item | Line | Reason it stays |
| --- | --- | --- |
| `probedKindTable()` | `system.cpp:173-177` | D-8: the request names the limit table, and the probe table's writer sits in another translation unit (`linux_pmu/provider.cpp:839`) |
| the two `detail` hooks over that table | `system.cpp:533-536`, `:538-543` | a move would move the hook signature `pmu.hpp:286` declares |
| the resolved metric table | `metric_formula.cpp:1497-1516` | Follow-on work: `metricHostCurrent()` caches `&System::local()` once per process at `:1619`, and `topdown_select.cpp:186-189` binds the same handle |
| the handle map | `core.hpp:200` | FR-003: the change adds no map and reaches no new one |
| the handle map's lock | `core.hpp:201-206` | D-6 and specs/012 FR-010: no new lock joins the tree, and `handleFor` keeps its one guard (`system.cpp:356`) |
| the alias map | `core.hpp:207` | the table keys on canonical paths, so an alias reaches it through `canonicalize` as it does today (`core.hpp:237-244`) |
| the open flag | `core.hpp:215-220` | D-6: the boundary keeps the compare-and-exchange it has |
| `System::local()` | `system.cpp:194-198` | FR-008: it keeps its meaning, the shared handle of the process |

## Validation rules

| Rule | Consequence | Stated by |
| --- | --- | --- |
| A source that answers nothing publishes nothing | its object carries no record, and the read answers no value | FR-006, `specs/024-multipass-collection/contracts/programmable-counters-fact.md:91` |
| A refused provider writes nothing | the table of its own `System` stands as it was, and no other table changes | Edge case "A refused provider writes nothing", `system.cpp:230-296` |
| A write runs before the open of its own `System` | no read races a write, and two `System` objects need no shared lock | D-6, FR-003 |
| A read follows its node's owner | a handle of any `System` answers that `System`'s table and its tree | FR-007, FR-009 |
| The table ends with its `System` | no entry of a finished table answers a later read | FR-005 |

## Invariants

| # | Invariant | Held by |
| --- | --- | --- |
| I-1 | one table stands in one `System::Impl`, and one `System` holds one table | FR-001, D-3 |
| I-2 | every node names the `System` whose tree holds it, set before the node enters `objects` | FR-007, `system.cpp:184`, `:189`, `:264`, `:287` |
| I-3 | the table holds one record per object whose source answered, and no record for the rest | FR-002, `system.cpp:253-255` |
| I-4 | every write runs before the open of its own `System`, and every read after that open | D-6 |
| I-5 | no new global mutable state, no new lock and no new atomic joins the tree | FR-003, D-6 |
| I-6 | `Object` keeps one member and `System` keeps one member | D-2, `counters_system.hpp:194`, `:383` |
| I-7 | the two sibling tables keep their shape, their writer and their lifetime | D-8 |

## Carrier changes

| Carrier | Change | Exported | Effect |
| --- | --- | --- | --- |
| `System::Impl` (`core.hpp:196-256`) | gains `limits` | no: a private header | no `SOVERSION` move (D-2) |
| `TreeNode` (`core.hpp:42-50`) | gains `owner` | no: a private type | no `SOVERSION` move (D-2) |
| `System()` and `~System()` (`counters_system.hpp:269` and `:278`) | private to public, each with its doxygen block | access only: both symbols already ship | the counters archive moves to `SOVERSION 8` (`CMakeLists.txt:168`; hard rule 12, D-2, FR-010) |
| the version line (`CMakeLists.txt:78`) | 0.32.0 to 0.33.0 | yes | FR-012 names nine lines across five files, and all nine take the minor number; the specs/046 record at `CMakeLists.txt:61` keeps its number |

## Deviations

- Two walk reads first carried spans that missed their opening line: the bodies
  run `system.cpp:445-460` and `:503-528`, and the `System::local()` leg of
  `children()` stands at `:506`.
