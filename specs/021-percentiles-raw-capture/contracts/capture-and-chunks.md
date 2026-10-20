# Contract: Capture and Chunks

The seams the timed path and the post-run fold expose. Signatures are the
shape the implementation carries; the contracts are the ones Principle II
requires.

## The chunk plan

```cpp
// source/harness/detail/calibration.hpp
inline constexpr std::size_t kMaxChunkPointsPerLeaf = 32'768;

struct ChunkPlan
{
  std::uint64_t chunkIterations = 1;
  std::uint64_t chunkCount = 0;
  std::size_t pointCapacity = 0;
};

// \pre iterations >= 1, iterationNs >= 0, sampleOverheadNs >= 0,
//      pauseRoom <= kMaxPauseSamplesPerLeaf
// \post chunkCount >= 1, chunkIterations >= 1,
//       chunkCount <= kMaxChunkPointsPerLeaf,
//       pointCapacity == chunkCount + 1 + pauseRoom, and
//       (chunkCount - 1) * chunkIterations < iterations <=
//       chunkCount * chunkIterations
[[nodiscard]] auto chunkPlanFor(std::uint64_t iterations,
                                double iterationNs,
                                double sampleOverheadNs,
                                std::size_t pauseRoom) noexcept -> ChunkPlan;
```

`sampleOverheadNs` is `Plan::sampleOverheadNsMedian()`, the same read that
fills `ResultRow::overheadFloorNs` today.

## The recorder

```cpp
// source/harness/runner.cpp, the attempt sampler
auto attemptRecorder = plan->recorder(chunkPlan.pointCapacity);
```

The `HardStop` policy stands, so a sample past the capacity is an
`SG_REQUIRE_ALWAYS` violation in every configuration and `dropped()` stays 0
(FR-006). The reservation seam records the plan:

```cpp
lastRecorderReservation() = RecorderReservation {
    chunkPlan.chunkCount + 1, pauseRoom, chunkPlan.chunkCount};
```

## The loop cursor

```cpp
// include/speedgun-ng/benchmark.hpp, State::Cursor
std::uint64_t m_remaining = 0;      // the current chunk, the loop's one read
std::uint64_t m_runRemaining = 0;   // read only at a chunk boundary
std::uint64_t m_chunkIterations = 0;// the refill size
```

`operator!=` keeps its shape: while `m_remaining` stands above zero it
returns true; at zero it reads `m_runRemaining`, and when the run still holds
iterations it takes one sample, refills `m_remaining` with
`m_chunkIterations`, subtracts the chunk from `m_runRemaining`, and returns
true; when the run holds none it closes the window and returns false.

**Loop shape requirement**: the per-iteration path holds one decrement in
`operator++` and one compare in `operator!=`, the codegen
`test/loop_shape.sh` measures today. The gate gains a probe that plants a
sample inside the per-iteration path and requires the detector to fail, so a
loop that pays per iteration cannot pass it (FR-001, D-01).

## The chunk fold

```cpp
// source/harness/detail/percentiles.hpp
// \pre api holds at least chunkCount + 1 committed points for core
// \post one value per chunk, each the netted fold of that chunk's window
[[nodiscard]] auto buildChunkValues(const sg::counters::detail::ExprCore& core,
                                    const sg::counters::RecorderApi& api,
                                    std::size_t basePoint,
                                    std::uint64_t chunkCount,
                                    const std::vector<std::pair<
                                        std::uint64_t, std::uint64_t>>&
                                        pausePairs,
                                    std::size_t foldColumn)
    -> std::vector<double>;
```

A pause pair whose two ordinals stand inside one chunk window nets out of that
window, the rule FR-015 inherits from the whole-run netting of 019.

## The rank read

```cpp
// source/harness/detail/percentiles.hpp
// \pre none
// \post The value stands available only when the column holds at least
//       ceil(100 / (100 - p)) values; the read is then the nearest-rank
//       value at rank ceil(p * n / 100), 1-based.
[[nodiscard]] auto percentileAt(const std::vector<double>& sortedValues,
                                double p) noexcept -> PercentileValue;

// \pre 0 < p < 100
// \post The count is the least chunk count that can answer p.
[[nodiscard]] auto minSamplesFor(double p) noexcept -> std::uint64_t;
```

No interpolation, no extrapolation, no histogram (FR-011, FR-013, D-06).

## Threaded runs

Each context folds its own column on its own thread and publishes its own
chunk values. The merge concatenates the contexts' chunk values into the
merged row's column before the rank read, so the merged percentile is the
nearest-rank over the union (FR-014, D-12). No fold reads another context's
points (FR-004).
