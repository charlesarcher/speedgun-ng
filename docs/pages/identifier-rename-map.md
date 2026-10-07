# Rename map

The old spelling and the new spelling for every name a shipped header
declared that the rename changed, including the `detail` namespaces.
FR-013 and FR-015 scope this map to `include/`, `source/`, `test/`,
`example/`, and `tools/`. A name that appears in no shipped header is
a file-local helper and is out of this map's scope. A test-only name
stays out. Each entry carries the kind, and a mark records a name in
a `detail` namespace.

The rename spans these commits:

| Commit | Scope |
| --- | --- |
| `f7591cc` | `dbc.hpp` |
| `13d8e2f` | `counters_core.hpp` |
| `c3908b4` | `counters_provider.hpp` |
| `7318450` | `counters_measurement.hpp` |
| `4789921` | `counters_clock.hpp` |
| `bcf6c9b` | `counters_pmu.hpp` and the PMU provider |
| `5c110ca` | `counters_push.hpp` and the push provider |
| `8a7b1a5` | `counters_fake.hpp` and the fake provider |
| `24ea24f` | `counters_system.hpp`, `simulation.hpp`, and the file-local helpers |
| `fb7ba67` | test-only names; none of them enters this map |

## Names by header

### `include/speedgun-ng/counters.hpp`

| Old | New | Kind | Detail | Rule |
| --- | --- | --- | --- | --- |
| `SPEEDGUN_NG_COUNTERS_HPP` | `SG_COUNTERS_HPP` | macro |  | N-3 |

### `include/speedgun-ng/counters_clock.hpp`

| Old | New | Kind | Detail | Rule |
| --- | --- | --- | --- | --- |
| `SPEEDGUN_NG_COUNTERS_CLOCK_HPP` | `SG_COUNTERS_CLOCK_HPP` | macro |  | N-3 |
| `clock_provider` | `ClockProvider` | type |  | N-1 |
| `clock_window` | `ClockWindow` | type | yes | N-1 |

### `include/speedgun-ng/counters_core.hpp`

| Old | New | Kind | Detail | Rule |
| --- | --- | --- | --- | --- |
| `SPEEDGUN_NG_COUNTERS_CORE_HPP` | `SG_COUNTERS_CORE_HPP` | macro |  | N-3 |
| `dim_same` | `kDimSame` | constant |  | N-8 |
| `events_exponent` | `kEventsExponent` | constant |  | N-8 |
| `target_cpu_bit` | `kTargetCpuBit` | constant |  | N-8 |
| `target_thread_bit` | `kTargetThreadBit` | constant |  | N-8 |
| `time_exponent` | `kTimeExponent` | constant |  | N-8 |
| `absent` | `ABSENT` | enumerator |  | N-4 |
| `bytes` | `BYTES` | enumerator |  | N-4 |
| `countable` | `COUNTABLE` | enumerator |  | N-4 |
| `fast_rdpmc` | `FAST_RDPMC` | enumerator |  | N-4 |
| `fast_tsc` | `FAST_TSC` | enumerator |  | N-4 |
| `gap` | `GAP` | enumerator |  | N-4 |
| `nanoseconds` | `NANOSECONDS` | enumerator |  | N-4 |
| `none` | `NONE` | enumerator |  | N-4 |
| `not_encodable` | `NOT_ENCODABLE` | enumerator |  | N-4 |
| `ops` | `OPS` | enumerator |  | N-4 |
| `permission_blocked` | `PERMISSION_BLOCKED` | enumerator |  | N-4 |
| `push_load` | `PUSH_LOAD` | enumerator |  | N-4 |
| `scope_refused` | `SCOPE_REFUSED` | enumerator |  | N-4 |
| `seconds` | `SECONDS` | enumerator |  | N-4 |
| `syscall` | `SYSCALL` | enumerator |  | N-4 |
| `dimension_of` | `dimensionOf` | function |  | N-2 |
| `unit_from_token` | `unitFromToken` | function |  | N-2 |
| `unit_name` | `unitName` | function |  | N-2 |
| `availability` | `Availability` | type |  | N-1 |
| `catalog_entry` | `CatalogEntry` | type |  | N-1 |
| `dim` | `Dim` | type |  | N-1 |
| `dim_quotient` | `DimQuotient` | type |  | N-1 |
| `dimension` | `Dimension` | type |  | N-1 |
| `error` | `Error` | type |  | N-1 |
| `metric_result` | `MetricResult` | type |  | N-1 |
| `read_mode` | `ReadMode` | type |  | N-1 |
| `target_mask` | `TargetMask` | type |  | N-1 |
| `unit` | `Unit` | type |  | N-1 |
| `frequency_hz` | `frequencyHz` | variable |  | N-5 |
| `running_ratio` | `runningRatio` | variable |  | N-5 |

### `include/speedgun-ng/counters_fake.hpp`

| Old | New | Kind | Detail | Rule |
| --- | --- | --- | --- | --- |
| `SPEEDGUN_NG_COUNTERS_FAKE_HPP` | `SG_COUNTERS_FAKE_HPP` | macro |  | N-3 |
| `add_counter` | `addCounter` | function |  | N-2 |
| `add_object` | `addObject` | function |  | N-2 |
| `gaps_at` | `gapsAt` | function |  | N-2 |
| `read_actions` | `readActions` | function |  | N-2 |
| `set_gap_actions` | `setGapActions` | function |  | N-2 |
| `set_points` | `setPoints` | function |  | N-2 |
| `m_read_actions` | `m_readActions` | member |  | N-10 |
| `fake_counter_data` | `FakeCounterData` | type |  | N-1 |
| `fake_provider` | `FakeProvider` | type |  | N-1 |
| `fake_script` | `FakeScript` | type |  | N-1 |
| `fake_window` | `FakeWindow` | type | yes | N-1 |
| `object_seed_data` | `ObjectSeedData` | type |  | N-1 |
| `delta_seed` | `deltaSeed` | variable |  | N-5 |
| `delta_state` | `deltaState` | variable |  | N-5 |
| `object_path` | `objectPath` | variable |  | N-5 |
| `ratio_pair` | `ratioPair` | variable |  | N-5 |
| `tail_delta` | `tailDelta` | variable |  | N-5 |

### `include/speedgun-ng/counters_measurement.hpp`

| Old | New | Kind | Detail | Rule |
| --- | --- | --- | --- | --- |
| `SPEEDGUN_NG_COUNTERS_MEASUREMENT_HPP` | `SG_COUNTERS_MEASUREMENT_HPP` | macro |  | N-3 |
| `add_leaf` | `addLeaf` | function | yes | N-2 |
| `add_node` | `addNode` | function | yes | N-2 |
| `compile_core` | `compileCore` | function | yes | N-2 |
| `compile_fanout_core` | `compileFanoutCore` | function | yes | N-2 |
| `fanout_fold_core` | `fanoutFoldCore` | function | yes | N-2 |
| `fold_core` | `foldCore` | function | yes | N-2 |
| `fold_pairs` | `foldPairs` | function |  | N-2 |
| `fold_pairs_core` | `foldPairsCore` | function | yes | N-2 |
| `hard_stop_sample_core` | `hardStopSampleCore` | function | yes | N-2 |
| `metric_core` | `metricCore` | function | yes | N-2 |
| `object_paths` | `objectPaths` | function |  | N-2 |
| `raw_core` | `rawCore` | function | yes | N-2 |
| `resolve_leaf_core` | `resolveLeafCore` | function | yes | N-2 |
| `ring_sample_core` | `ringSampleCore` | function | yes | N-2 |
| `sample_overhead_ns_max` | `sampleOverheadNsMax` | function |  | N-2 |
| `sample_overhead_ns_median` | `sampleOverheadNsMedian` | function |  | N-2 |
| `sample_overhead_ns_min` | `sampleOverheadNsMin` | function |  | N-2 |
| `scale_all` | `scaleAll` | function | yes | N-2 |
| `unit_token` | `unitToken` | function |  | N-2 |
| `m_capacity` | `mCapacity` | member |  | N-5 |
| `m_columns` | `mColumns` | member |  | N-5 |
| `m_dropped` | `mDropped` | member |  | N-5 |
| `m_head` | `mHead` | member |  | N-5 |
| `m_impl` | `mImpl` | member |  | N-5 |
| `m_wrapped` | `mWrapped` | member |  | N-5 |
| `hard_stop` | `hardStop` | tag |  | N-11 |
| `counter` | `Counter` | type |  | N-1 |
| `dimension_tag` | `DimensionTag` | type |  | N-1 |
| `expr_core` | `ExprCore` | type | yes | N-1 |
| `expr_node` | `ExprNode` | type | yes | N-1 |
| `expression` | `Expression` | type |  | N-1 |
| `fanout_plan` | `FanoutPlan` | type |  | N-1 |
| `fanout_result` | `FanoutResult` | type |  | N-1 |
| `hard_stop_t` | `HardStop` | type |  | N-1 |
| `is_specialization_of` | `IsSpecializationOf` | type | yes | N-1 |
| `leaf_core` | `LeafCore` | type | yes | N-1 |
| `object` | `Object` | type |  | N-1 |
| `plan` | `Plan` | type |  | N-1 |
| `points_view` | `PointsView` | type |  | N-1 |
| `push_counter` | `PushCounter` | type |  | N-1 |
| `recorder_api` | `RecorderApi` | type |  | N-1 |
| `recorder_handle` | `RecorderHandle` | type |  | N-1 |
| `ring_t` | `Ring` | type |  | N-1 |
| `scope` | `Scope` | type |  | N-1 |
| `frequency_hz` | `frequencyHz` | variable | yes | N-5 |
| `leaf_index` | `leafIndex` | variable |  | N-5 |
| `leaf_name` | `leafName` | variable | yes | N-5 |
| `leaf_remap` | `leafRemap` | variable | yes | N-5 |
| `metric_result` | `MetricResult` | variable | yes | N-5 |
| `node_base` | `nodeBase` | variable | yes | N-5 |
| `object_path` | `objectPath` | variable |  | N-5 |
| `scope_obj` | `scopeObj` | variable | yes | N-5 |

### `include/speedgun-ng/counters_pmu.hpp`

| Old | New | Kind | Detail | Rule |
| --- | --- | --- | --- | --- |
| `SPEEDGUN_NG_COUNTERS_PMU_HPP` | `SG_COUNTERS_PMU_HPP` | macro |  | N-3 |
| `pmu_provider` | `PmuProvider` | type |  | N-1 |
| `pmu_state` | `PmuState` | type | yes | N-1 |

### `include/speedgun-ng/counters_provider.hpp`

| Old | New | Kind | Detail | Rule |
| --- | --- | --- | --- | --- |
| `SPEEDGUN_NG_COUNTERS_PROVIDER_HPP` | `SG_COUNTERS_PROVIDER_HPP` | macro |  | N-3 |
| `no_disclosure_column` | `kNoDisclosureColumn` | constant |  | N-8 |
| `cpu` | `CPU` | enumerator |  | N-4 |
| `thread` | `THREAD` | enumerator |  | N-4 |
| `add_object` | `addObject` | function |  | N-2 |
| `check_action` | `checkAction` | function |  | N-2 |
| `check_thunk` | `checkThunk` | function |  | N-2 |
| `default_thunk` | `defaultThunk` | function |  | N-2 |
| `put_disclosure` | `putDisclosure` | function |  | N-2 |
| `read_points` | `readPoints` | function |  | N-2 |
| `resolve_thunk` | `resolveThunk` | function |  | N-2 |
| `set_thunk` | `setThunk` | function |  | N-2 |
| `m_column_count` | `m_columnCount` | member |  | N-10 |
| `m_disclosure_written` | `m_disclosureWritten` | member |  | N-10 |
| `m_leaf_count` | `m_leafCount` | member |  | N-10 |
| `catalog_seed` | `CatalogSeed` | type |  | N-1 |
| `leaf_set` | `LeafSet` | type |  | N-1 |
| `object_seed` | `ObjectSeed` | type |  | N-1 |
| `object_sink` | `ObjectSink` | type |  | N-1 |
| `point_sink` | `PointSink` | type |  | N-1 |
| `provider_iface` | `ProviderIface` | type |  | N-1 |
| `read_thunk` | `ReadThunk` | type |  | N-1 |
| `target` | `Target` | type |  | N-1 |
| `target_kind` | `TargetKind` | type |  | N-1 |
| `window_reader` | `WindowReader` | type |  | N-1 |
| `column_count` | `columnCount` | variable |  | N-5 |
| `disclosure_column` | `disclosureColumn` | variable |  | N-5 |
| `frequency_hz` | `frequencyHz` | variable |  | N-5 |
| `has_ratio_pair` | `hasRatioPair` | variable |  | N-5 |
| `leaf_count` | `leafCount` | variable |  | N-5 |

### `include/speedgun-ng/counters_push.hpp`

| Old | New | Kind | Detail | Rule |
| --- | --- | --- | --- | --- |
| `SPEEDGUN_NG_COUNTERS_PUSH_HPP` | `SG_COUNTERS_PUSH_HPP` | macro |  | N-3 |
| `add_counter` | `addCounter` | function |  | N-2 |
| `push_point` | `PushPoint` | type |  | N-1 |
| `push_provider` | `PushProvider` | type |  | N-1 |
| `push_window` | `PushWindow` | type | yes | N-1 |

### `include/speedgun-ng/counters_system.hpp`

| Old | New | Kind | Detail | Rule |
| --- | --- | --- | --- | --- |
| `SPEEDGUN_NG_COUNTERS_SYSTEM_HPP` | `SG_COUNTERS_SYSTEM_HPP` | macro |  | N-3 |
| `handle_for` | `handleFor` | function |  | N-2 |
| `register_provider` | `registerProvider` | function |  | N-2 |
| `filter` | `Filter` | type |  | N-1 |
| `impl` | `Impl` | type |  | N-1 |
| `system` | `System` | type |  | N-1 |

### `include/speedgun-ng/dbc.hpp`

| Old | New | Kind | Detail | Rule |
| --- | --- | --- | --- | --- |
| `SPEEDGUN_NG_DBC_HPP` | `SG_DBC_HPP` | macro |  | N-3 |
| `assertion` | `ASSERTION` | enumerator |  | N-4 |
| `invariant` | `INVARIANT` | enumerator |  | N-4 |
| `postcondition` | `POSTCONDITION` | enumerator |  | N-4 |
| `precondition` | `PRECONDITION` | enumerator |  | N-4 |
| `check_assertion` | `checkAssertion` | function |  | N-2 |
| `check_assertion_always` | `checkAssertionAlways` | function |  | N-2 |
| `check_invariant` | `checkInvariant` | function |  | N-2 |
| `check_invariant_always` | `checkInvariantAlways` | function |  | N-2 |
| `check_postcondition` | `checkPostcondition` | function |  | N-2 |
| `check_postcondition_always` | `checkPostconditionAlways` | function |  | N-2 |
| `check_precondition` | `checkPrecondition` | function |  | N-2 |
| `check_precondition_always` | `checkPreconditionAlways` | function |  | N-2 |
| `default_response` | `defaultResponse` | function | yes | N-2 |
| `in_response_flag` | `inResponseFlag` | function | yes | N-2 |
| `kind_name` | `kindName` | function | yes | N-2 |
| `observer_slot` | `observerSlot` | function | yes | N-2 |
| `report_violation` | `reportViolation` | function | yes | N-2 |
| `set_observer` | `setObserver` | function |  | N-2 |
| `response_guard` | `ResponseGuard` | type | yes | N-1 |
| `violation_observer` | `ViolationObserver` | type |  | N-1 |
| `in_response` | `inResponse` | variable | yes | N-5 |
| `predicate_text` | `predicateText` | variable | yes | N-5 |

### `include/speedgun-ng/simulation.hpp`

| Old | New | Kind | Detail | Rule |
| --- | --- | --- | --- | --- |
| `SPEEDGUN_NG_SIMULATION_HPP` | `SG_SIMULATION_HPP` | macro |  | N-3 |
| `simulation_start_tag` | `kSimulationStartTag` | constant |  | N-8 |
| `simulation_start` | `simulationStart` | function |  | N-2 |

### `include/speedgun-ng/speedgun-ng.hpp`

| Old | New | Kind | Detail | Rule |
| --- | --- | --- | --- | --- |
| `exported_class` | `ExportedClass` | type |  | N-1 |

