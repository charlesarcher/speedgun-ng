# The optimization barrier rule (015 FR-029, FR-030)

Constitution Principle X.2 states the rule: a `DoNotOptimize`-style
barrier appears only where the compiler would otherwise eliminate the
measured work, and a barrier with nothing to defeat is noise. In a
benchmarking framework that noise is a defect, because the number on the
console then describes work the binary does not contain.

## What the header provides

`include/speedgun-ng/barrier.hpp` holds two functions in namespace `sg`.

- `sg::doNotOptimize(value)` takes any non-cv-qualified, non-reference
  type by universal reference and emits one empty extended-assembly
  statement that names the value as an input and output operand with a
  register-or-memory constraint and a `memory` clobber. The compiler can
  neither discard a value the asm reads nor move a store across the
  clobber.
- `sg::clobberMemory()` is
  `std::atomic_signal_fence(std::memory_order_acq_rel)`: stores the
  compiler has held pending become visible to its analysis at that
  point.

Both carry `\pre none` and `\post none`, and the contract deviation sits
in the recorded note below.

## When a barrier is justified

The measured work has to keep a result the harness can see. A loop whose
accumulator reaches nothing else is the case: at `-O2` the optimizer
proves the accumulator dead and removes the loop. The barrier goes after
the loop, on the accumulator.

No barrier is justified where the result already reaches the harness. A
store into a buffer the benchmark owns, a call the linker cannot see
through, and a value the loop feeds to its own next iteration each keep
the work alive on their own. A barrier there costs an instruction the
measurement pays for and defeats nothing.

## How the rule is enforced

`test/barrier_shape.sh` compiles one dead-by-construction chain at `-O2`
three ways and counts the instructions the shape function holds. The
columns are the counts the gate printed on the reference host.

| Build | g++ | clang++ |
| --- | --- | --- |
| with `sg::doNotOptimize` | 12 | 52 |
| with no barrier at all | 1 | 1 |
| with a planted no-op barrier | 1 | 1 |

The gate fails where the barrier build loses the chain, where the bare
build keeps it, or where the planted no-op barrier passes the barrier
check. That last case is the liveness probe: a clean run cannot come from
a detector that counts nothing. A missing compiler is skipped by name,
and the gate fails only where neither g++ nor clang++ is present.

## The recorded contract deviation

Principle II asks that a header document a contract and the source
enforce it. The barriers carry doxygen `\pre none` and `\post none` with
no runtime `SG_ENSURE`, recorded in the 015 plan's Complexity Tracking
table on the 011 precedent. Their postcondition is a property of the
generated code, which no runtime assertion can observe, and a counter
inside a barrier would defeat the barrier it is meant to check. The
codegen gate above is the enforcement.
