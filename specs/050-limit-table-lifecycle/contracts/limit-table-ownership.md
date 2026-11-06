# Contract: one limit table per System

**Feature**: specs/050-limit-table-lifecycle | **File**: `source/counters/system.cpp`,
`System::registerProvider`, `Object::programmableCounters`, `Object::parent`,
`Object::children`, and `source/counters/detail/core.hpp` | **Audit point**:
`e7080f36`
**Spec**: FR-001 to FR-009, D-3, D-4, D-6, D-8

## What this contract covers

Who owns the programmable-counter limit table, what one record means, who
writes it, who reads it, and when it ends. The contract this one refines
stands at
`specs/024-multipass-collection/contracts/programmable-counters-fact.md`: its
public surface, its table of sources (`:67-72`), and its rule "a source that
cannot answer publishes nothing" (`:91`). That fact and the meaning of no
value stand unchanged there, and only the owner of the storage moves. The
fields, the states and the entities stand in `data-model.md`; the modules, the
commit order and the gates stand in `plan.md`. `spec.md` and `plan.md` carry no
line number here, because the lane edits both while it plans: each citation
names a requirement, a decision or a section.

## The rule

One `System` owns one programmable-counter limit table, that table stands as
state of the `System`'s private `System::Impl`, and it ends when that `System`
ends (FR-001, FR-005, D-3).

| Item | Reading |
| --- | --- |
| the storage | the member `System::Impl::limits`, in place of one function-local static behind an anonymous-namespace accessor (`system.cpp:187-191` at the base `858c55d9`) |
| the route | the tree node's owner pointer, set where the node is made, in place of the process-wide accessor and of `System::local()` (FR-007, D-4) |
| the fact, its sources, its answers | unchanged (specs/024 contract `:67-72`, FR-002) |
| the public surface | `Object::programmableCounters()` keeps its signature, its `const noexcept` and its two answers (`include/speedgun-ng/counters_system.hpp:92-93`) |
| the two sibling tables | unchanged (D-8, the Boundary section below) |

## What no value means

A record stands for one object whose event source answered, and a source that
cannot answer leaves no record (`system.cpp:266-268`). A read of a path the
table holds no record for answers no value: an object nobody scripted, the
`machine` clock root, and every object of a `System` that scripted nothing
(FR-006, specs/024 contract `:49`, `:91`). No value carries no count, and the
pass planner limits nothing for the leaves of such an object. specs/024 keeps
that meaning, and this lane adds no third answer.

## Who publishes

| Publisher | What it publishes | Line |
| --- | --- | --- |
| `System::registerProvider` | the limit each accepted seed carried, into the table of the `System` that owns that call | `system.cpp:292-294`, staged at `:253-255` |
| `ObjectSeed::programmableCounters` | the count for one object: no value when the source cannot answer | `include/speedgun-ng/counters_provider.hpp:78` |
| the `linux_pmu` provider | the CPUID count for each device it seeds | `source/counters/linux_pmu/provider.cpp:848-852` |
| the clock provider | nothing for the `machine` root it seeds | `source/counters/clock_provider.cpp:302-308` |
| `FakeProvider::setProgrammableCounters` | the scripted count for one path, for the next `System` that opens this fake | `include/speedgun-ng/counters_fake.hpp:169-170`, stored at `source/counters/fake_provider.cpp:207` |

A provider that registration refuses publishes nothing. Registration stages
the whole provider and commits only after every seed validates
(`system.cpp:221-296`), so a duplicate path, a duplicate name, a taken alias
or a bad unit token leaves the table of that `System` untouched and touches no
other table (Edge case "A refused provider writes nothing").

## Who reads

| Reader | Route | Answer |
| --- | --- | --- |
| `Object::programmableCounters()` | `m_node`, then the node's owner, then `owner->m_impl->limits` (`system.cpp:434-443`) | the count for that node's canonical path, or no value |
| the pass planner and the harness | that interface and nowhere else: `source/harness/runner.cpp:1594`, `source/harness/hunt.cpp:165`, `source/harness/hunt_cli.cpp:732-735` | the count a set of columns splits on (specs/024 contract `:8-10`) |

The harness binds `System::local()` (`source/harness/cli.cpp:666`) and registers
its providers on it (`:679`, `:700`), so every harness read answers the local
`System`'s table, the answer it gives today. `source/harness/runner.cpp:58`
shows the same binding inside a run.

## Two `System` objects in one process

| Case | Answer | Reading |
| --- | --- | --- |
| one path, two scripted counts | each `System` answers its own count, and the first keeps its count while both live (FR-004) | measured 4 and 2, with both answering 2 at the base: `/tmp/sg050/probe5.cpp` section A1 in `/tmp/050-clarify-round1.md` |
| a path one `System` declares with one leaf and never scripts | no value, whatever the other `System` scripted (FR-006) | measured 4 at the base: `probe_unscripted.cpp` in `/tmp/050-clarify-round2.md` |
| what the two hold apart | a tree, a provider list, a handle map with its lock, an alias map, an open flag and a limit table, one of each per `System` | `core.hpp:196-256` |
| what the two share | no mutable object: each table takes its writes before its own open and its reads after it, so no lock spans the two (D-6) | Edge case "Two `System` objects open in two threads" |
| the spelling of a path | `cpu` names one canonical string in both tables, so the two records sit apart with the same key | Edge case "Two objects of two `System` objects share a path" |

A program that builds its own `System` while the harness runs sees two
catalogs, because the harness registers on `System::local()` (`plan.md`, the
Risks bullet Two trees, one process). The rule that a handle belongs to the
`System` that made it is what keeps the two trees from answering for each
other.

## An ended `System`

The table ends with its `System`. `~System()` is defined `= default`
(`system.cpp:192`), so the `std::unique_ptr` member releases `System::Impl`,
its object map and its limit table together (`core.hpp:196-256`,
`counters_system.hpp:383`). No clear call exists, and none is needed
(FR-005).

| Case | Answer | Reading |
| --- | --- | --- |
| a `System` that scripted 8 ends, and a fresh `System` declares that path with one leaf and scripts no limit | no value | measured 8 at the base: `probe_ended.cpp` in `/tmp/050-clarify-round2.md` |
| a fresh `System` scripts 1 for that path | 1, before and after this change | `/tmp/050-clarify-round2.md`, check 3 |
| a handle of the ended `System` | it names a node inside that tree and stops being valid with it | Edge case "A handle that outlives its `System`"; this contract states no wider lifetime rule |

## The walk reads

`Object::parent()` and `Object::children()` answer the tree of the `System`
that owns the handle's node (FR-009). Each read keeps its steps and changes one
of them: the `System` it walks.

| Read | Today | After this change |
| --- | --- | --- |
| `parent()` | `System::local()` at `system.cpp:463` at the base `858c55d9`, then its tree test at `:468` and its handle at `:471` | `node->owner` in the same three steps |
| `children()` | `System::local()` at `system.cpp:518` at the base `858c55d9`, then the walk of its object map at `:520` and its handles at `:534` | `node->owner` in the same three steps |

Measured at the base: on a second `System` that holds `cpu` and `cpu/core-0`,
`parent()` answers null and `children()` answers 0, where the owning tree
answers `cpu` and one entry (`probe_walks.cpp` in
`/tmp/050-clarify-round2.md`). `System::handleFor` stays private
(`counters_system.hpp:371-372`), and `friend class Object` at `:369` lets an
`Object` member reach it, the way `system.cpp:459` and `:522` do.
`System::local()` keeps its meaning: the shared handle of the process
(`system.cpp:194-198`, FR-008).

## The thread rule

| Rule | Reading | Line |
| --- | --- | --- |
| registration writes before the open | `registerProvider` refuses a call once that `System` stands open | `system.cpp:203-207` |
| the open is the first resolution that needs the catalog | one compare-and-exchange moves the flag exactly once | `core.hpp:215-230` |
| a read follows the open of its own `System` | every handle comes from `object()` or `objects()`, and each opens the boundary first | `system.cpp:302`, `:374` |
| within one `System` no read races a write | the write set closes at that open | D-6 |
| two `System` objects share no mutable object | each holds its own tree, tables, handles, aliases and flag | D-6, `core.hpp:196-256` |
| the handle map keeps its own lock | one guard covers the lookup and the insert | `core.hpp:200-205`, `system.cpp:356`; specs/012 FR-010 |
| no new lock and no new atomic join the tree | the change moves storage, it adds no synchronization | FR-003, Constraints |
| the thread sanitizer job stays the arbiter of a race | the gate stands as CI runs it | D-6, `plan.md`, Gate commands |

The doc comment of `Object::programmableCounters` carries these words, beside
the interface they govern (`counters_system.hpp:75-91`, D-6, D-7).

## Boundary with the follow-on tables

| Table | Line | Reading |
| --- | --- | --- |
| `probedKindTable()` | `system.cpp:173-177` | stays process-wide (D-8): its writer reaches it from another translation unit, `source/counters/linux_pmu/provider.cpp:839` through the hook at `source/counters/detail/pmu.hpp:286`, and a verdict one `System` wrote answered `targets=2` for a later `System` (`probe_verdict.cpp` in `/tmp/050-clarify-round2.md`) |
| the resolved metric table | `source/counters/linux_pmu/metric_formula.cpp:1497-1516` | stays process-wide: `metricHostCurrent()` caches `&System::local()` once per process at `:1619`, and `source/counters/linux_pmu/topdown_select.cpp:186-189` binds the same handle |

The rule of this contract governs limits. It says nothing about a probe verdict
or a resolved metric, and both keep the lifetime they have today (Clarification 2,
Follow-on work).

## Out of scope

| Item | Reason |
| --- | --- |
| the pass planner and every harness reader of the fact | they read through `Object::programmableCounters()`, and that surface keeps its signature, its meaning and its answers (Out of scope) |
| the JSON report, its schema, the loader and the comparison | no field moves (Out of scope) |
| the two `SOVERSION` numbers, the schema version, every option | D-2 rules both numbers: the harness stays 21, the counters archive goes to 8. No option joins the lane. |
| `Object::counters()` | it reads the probe verdict table, and D-8 leaves that table alone |
| the two doxygen blocks FR-010 asks for | the pair gate decides the shape, and `plan.md`, the Constitution Check row II, records the `\pre none` and `\post none` form |
| every CI file | Out of scope, D-11 |

## Verification

| Fact | Case |
| --- | --- |
| two `System` objects answer their own scripted counts on one path | case 1 of SC-001 (FR-004) |
| a declared, unscripted path answers no value across `System` objects | case 2 (FR-006) |
| an ended `System` leaves no entry a later read reaches | case 3 (FR-005) |
| the single-`System` answers stand for a scripted object, an unscripted object and the root | case 4, beside `test/source/counters_objects_limit_test.cpp` (FR-002) |
| the two walk reads follow the owner | case 5 on a `System` that holds `cpu` and `cpu/core-0` (FR-009) |
| the accessor and its function-local static are gone | T007 deletes the accessor and its static at `system.cpp:179-191` of the base `858c55d9`, and its `grep -c 'programmableLimitTable' source/counters/system.cpp` answers 0 where the base answers 3. T006's `grep -c 'limits;' source/counters/detail/core.hpp` answers 1, so the table stands in `System::Impl` (`tasks.md`, the two Verify lines) |
| no new global state, no new lock and no new atomic appear | the diff, and the thread sanitizer job (`plan.md`, Gate commands) |

The five cases stand in `test/source/counters_limit_lifecycle_test.cpp` with
one CTest entry beside the existing `counters_objects_limit_test` entry
(`test/CMakeLists.txt:715-722`). Each cross-`System` case declares the path it
reads on the `System` that reads it, so the read reaches a limit answer
(Clarification 1).

## Deviations

- specs/024 `data-model.md:64` names `counters_system.hpp:34` for the query
  this contract refines. At this head `:34` stands inside the class comment of
  `Object`, and the interface stands at `counters_system.hpp:92-93`. This
  contract cites the header range.
