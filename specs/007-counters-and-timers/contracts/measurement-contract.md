# Contract: Algebra, Plan, Recorder, Folds, Scope

**Feature**: `007-counters-and-timers` | Namespace `sg::counters` (R-001) | Entities: [data-model.md](../data-model.md) E-05..E-10

## Dimensions and algebra (FR-014..FR-017)

```cpp
template <int T, int C> struct dim {};      // time^T x events^C
template <dim D> class counter;             // resolved leaf
template <dim D> class expression;          // typed composition

// operator rules (all compile-time):
//   a + b, a - b   : require identical tags
//   a / b          : quotient tag subtracts exponents
//   scalar * e     : unrestricted (scalar scale)
```

| Expression | Tag rule | Example | Requirement |
|---|---|---|---|
| `a + b`, `a - b` | `D` identical or the program is ill-formed | `cycles + branches` ok; `bytes + monotonic` fails to compile | FR-015, US1 scenario 3 |
| `a / b` | exponents subtract | `instructions / cycles` is `dim<0,0>`; `bytes / monotonic` is `dim<-1,1>` | FR-015 |
| `k * e` | tag preserved | `1e9 * seconds_expr` | FR-015 |
| catalog unit mapping | closed switch, unknown unit is a resolution error | `seconds` to `time^1`; count units to `events^1` | FR-017 |
| read path | tags erased; columns are `uint64` and folds are `double` | zero read-path cost for dimensions | FR-016 |

Accepted, documented limitation: `events^1 + events^1` compiles (A10); the system catches the time-versus-count class.

## Metric result (FR-019)

```cpp
struct metric_result {
  double value;
  double running_ratio;   // enabled/running quotient when the source has that pair;
                          // sources without one disclose 1.0
  bool scaled;            // false whenever ratio disclosure is 1.0 by construction
};
```

- A composite's ratio is the product of constituent ratios each raised to its algebraic exponent (clarification 2; FR-019).
- The fields are members of the returned struct: omitting the disclosure is structurally impossible (FR-019, SC-008).

## Plan compile (FR-022, FR-024, FR-031, FR-032)

```cpp
std::expected<plan, error> compile(const system&, targets, /* expressions... */);
// post: flat leaf slots, PMU group layout (one leader per PMU), per-leaf achieved
//       read mode, fold programs, arena geometry; zero hardware reads performed
// error: zero-leaf expression, empty plan, group target/clock mismatch (FR-024,
//        spec edge case), expression over a non-countable leaf with catalog state
//        in the message (spec edge case)
class plan {
  double sample_overhead_ns_min/median/max() const;   // FR-032 calibration
};
```

- The read path contains no expression tree, no dynamic dispatch, no name lookup (FR-022, R-004).
- Hardware targeting (thread or cpu) binds at plan open; plans are per-thread; multiple plans over one system are first-class (FR-031).

## Recorder factory and handle (FR-025..FR-030)

```cpp
struct hard_stop_t {};  struct ring_t {};
inline constexpr hard_stop_t hard_stop{};  inline constexpr ring_t ring{};

class plan {
  auto recorder(std::size_t capacity) const;              // hard_stop default
  auto recorder(std::size_t capacity, ring_t) const;      // tag argument, no <>
  // post: capacity point columns allocated now; zero allocation later
  // error (ring): capacity not a power of two is a recoverable construction error
};

template <class P> class recorder_handle {                // value handle (FR-029)
  void sample() noexcept;                                 // THE critical path (FR-026)
};
```

| Clause | Rule | Requirement |
|---|---|---|
| C-MEA-1 | `sample()`: `noexcept`, zero allocation, zero lock; fast-mode leaves read in-instruction; syscall mode does one group read per PMU leader plus vDSO reads plus plain push loads, all in one sampling action | FR-026, FR-047 |
| C-MEA-2 | `hard_stop`: sample past capacity aborts through `SG_REQUIRE_ALWAYS`, present in every build configuration | FR-027, spec edge case |
| C-MEA-3 | `ring`: power-of-two enforced at construction; index masks branchlessly; wrapped flag and dropped count recorded; folds consult drops | FR-028 |
| C-MEA-4 | capacity counts stored point columns; capacity 1 holds one sample; folds require at least 2 | FR-025, clarification 1 |
| C-MEA-5 | recorder is a small trivially-copyable value handle over a plan-arena buffer; two recorders of one plan get independent buffers sharing the compiled layout | FR-029 |
| C-MEA-6 | per-thread objects; cross-thread plan/recorder/push use is a tier-3 violation | FR-031, FR-035, spec edge case |
| C-MEA-7 | documented tight-loop idiom: chunked sampling every K iterations, capacity `N/K + 1`, `fold_pairs` for the series, K=1 observer-effect cost stated from the plan calibration | FR-048 |

## Folds (FR-018, FR-020, FR-021)

```cpp
template <dim D> class expression {
  metric_result fold(const recorder_api&, std::size_t i, std::size_t j) const;
  std::vector<metric_result> fold_pairs(const recorder_api&) const;
  metric_result fold(const recorder_api&) const;         // first to last
  points_view raw(std::string_view object_path, std::string_view leaf) const;
};
```

- Deltas: `point[j] - point[i]` unsigned modular at `2^64`; a single hardware wrap subtracts out and no violation fires (FR-013, US2 scenario 4).
- Range: `i < j` within the recorded extent, tier-3 checked (FR-018; `i >= j` or out-of-extent is developer error, US2 scenario 6).
- Monotonicity: a push decrement between folded points is tier-3 (FR-035, US4 scenario 6).
- Lazy and pure: folds compute only on demand, run any number of times over any subset after measurement, and never trigger provider reads (FR-021, US1 scenario 5).
- Provenance: `raw(...)` exposes the constituent leaf's object path, name, description, unit, raw point column, point identity, and multiplex ratio (FR-020, US1 scenario 7).
- Fold windows include their endpoints' sample cost, stated from the plan calibration (FR-032, cadence contract).

## Fan-out (US3 scenario 5, SC-007)

`compile(system, expr, selection)` produces a plan with one fold program per selected object: leaf slots instantiated per object, one provider group read per instance per sampling action, all inside the shared window (FR-047). Folds over a fan-out plan yield one `metric_result` per object, keyed by canonical path; per-object instruction deltas reconcile against the shared total (SC-007). A layout conflict (duplicate instance slot, target/clock mismatch) is a recoverable construction error, the FR-024 pattern.

## Scope sugar (FR-030)

```cpp
class scope {                    // exactly a two-point recorder
  void start();                  // sample() #1
  void finish();                 // sample() #2
  metric_result metric(const expression<D>&) const;   // fold(rec, 0, 1)
};
```

One semantics, two spellings. Misuse sequences (`metric` before `finish`, `finish` without `start`, double `start`, use-after-finish, registering a composite into a started scope) are tier-3 contract violations (spec edge cases, FR-046, A6).

## Standalone embeddability (FR-049, FR-050)

- An example using public headers plus the standard library compiles, runs, and folds a metric; the link manifest names this library alone; zero benchmarking-framework code appears anywhere in 007.
- All construction fits an untimed setup region; recorder capacity is computable from a known iteration count; per-thread plans; fold results serve as per-iteration counter inputs.
