# Contract: `simulation_start`

**Feature**: `specs/011-nanosecond-counter-ssc-mark/spec.md`
**Date**: 2026-10-03

The public surface of the trace-start marker, and the byte contract a
tracer matches. Everything here is normative for the implementation and for
the codegen gate.

## Declaration

One free function in one new public header. No macro, no run-time tag, no
second entry point.

```cpp
namespace sg {

/// Tag an attached tracer watches for. Pass it to the tracer as
/// -start_ssc_mark FACE on its command line.
inline constexpr std::uint32_t simulation_start_tag = 0xFACEU;

/// Emits the marker sequence an attached tracer watches for.
[[nodiscard]] void simulation_start() noexcept;

}  // namespace sg
```

- Return type `void`, parameter list empty, `noexcept`, exported with
  `SPEEDGUN_NG_EXPORT`.
- The header includes the generated export header and nothing from the
  platform. No clock identifier, no register name, and no platform function
  appears in it.
- The declaration carries doxygen `\pre none` and `\post none`, the
  project's spelling for a contract with nothing to assert, plus descriptive
  text carrying the three properties below.
- The tag constant is the only value a caller passes onward. The library
  neither launches a tracer nor reads tracer output.

## Emitted sequence

```text
BB CE FA 00 00 64 67 90
```

| Bytes | Instruction | Notes |
|-------|-------------|-------|
| `BB CE FA 00 00` | `mov $0xface,%ebx` | `0xBB` selects EBX through the register field of `mov r32, imm32`; the immediate is the tag, little-endian |
| `64 67 90` | `fs addr32 nop` | `0x64` FS segment override, `0x67` address-size override, `0x90` the one-byte no-operation |

Fixed properties of the sequence:

1. **Eight bytes, one window.** A tracer matches eight contiguous bytes at
   an instruction boundary. Bytes outside the window are not part of the
   marker, so a neighboring no-operation neither satisfies nor breaks the
   match.
2. **The immediate is 32 bits wide.** A tag outside the unsigned 32-bit range
   cannot be encoded, and a `static_assert` in the translation unit rejects
   it at compile time in every configuration.
3. **Register preservation is the compiler's obligation.** The statement
   names EBX in its clobber list, so the compiler saves and restores the
   callee-saved register around it. Every general-purpose register is
   bit-identical across the call.
4. **No memory clobber.** The clobber list names the register and nothing
   else. An attached tracer observes executed-instruction order directly, so a
   memory barrier would constrain the program's own ordering with no
   observable effect on collection.
5. **No barrier, no lock prefix, no fence.** The window is the whole
   observable effect.
6. **Empty where the instruction set has no marker.** Under a preprocessor
   guard for x86, the body is empty; the call compiles, links, and returns,
   and no marker appears in the emitted code. The tag stays published so a
   caller can still pass it to a tracer.

## Assembler syntax

The body is written in the syntax the project selects by default, which is
what the two supported compilers accept from a C++ translation unit. No
build option in this project switches the assembler to Intel syntax, and a
build that did would fail to assemble the body. A silently
different sequence would reach the tracer and start the trace in the wrong
place. The gate script compiles the translation unit itself, so
it sees the same syntax selection the library build sees.

## Tracer command line

```sh
sde64 -start_ssc_mark FACE:repeat -- <program> <args>
```

| Property | Value |
|----------|-------|
| Option | `-start_ssc_mark` for a collection start, `-stop_ssc_mark` for a stop. This feature ships the start form; the stop form takes a second tag a later change adds. |
| Value syntax | hexadecimal, most-significant nibble first, no `0x` prefix. `0xFACE` is written `FACE`. |
| Count suffix | `:repeat` for every occurrence, `:1` for the first occurrence only. |
| Where the event fires | at the marker instruction itself. The surrounding basic block does not decide the event. |
| Stream form | the parsed value is stored little-endian into bytes 1 through 4 of the window, which is the ordinary x86 encoding of the immediate. |

The value is bounded by the width of the immediate, so a 32-bit range is the
contract's own ceiling. No stop tag ships here, and no marker for a region
end.

## What the gate asserts

The gate compiles the marker translation unit at a release optimization
level, once per available compiler and once per contract-enforcement setting,
disassembles the object, and asserts:

1. the eight-byte window appears exactly once in the object's text, which is
   the in-function count because the unit holds exactly one function;
2. a scratch copy carrying a wrong window makes that assertion fail, so a
   passing run cannot come from a detector that inspects nothing;
3. when no compiler on `PATH` targets x86, the gate reports a skip and exits
   successfully.

The gate asserts the bytes and the count. It does not inspect a clobber list,
so register preservation is proved by the runtime test and the absent memory
barrier is proved by review. Both gaps are recorded in the plan's
Verification Matrix.

## What the runtime test asserts

1. A known value whose upper 32 bits are set is placed in the designated
   callee-saved register, the call runs, and the register is bit-identical
   afterwards. The upper half matters: an implementation that preserved only
   the low half passes a test built from small integers.
2. Repeated calls return and the program terminates normally, which is the
   no-tracer-attached case.
3. The tag constant carries the value the header documents.

## Provenance

Intel's macro is `__SSC_MARK`, an Intel C++ Compiler builtin needing no
include, mirrored verbatim in Clang's `x86gprintrin.h`. Intel publishes no
byte encoding of its own: its regions document defines the marker only as two
instructions where the first carries an identifying immediate. The byte
contract above rests on that macro and on the byte-level match in Intel's own
tracing component, which builds the window as
`{0xbb, 0, 0, 0, 0, 0x64, 0x67, 0x90}` and fills bytes 1 through 4 from the
tag. The identifier `TRACING_SSC_MARK` appears in no Intel source; it is a
local name in one third-party project whose macro body emits these bytes.
No Intel source publishes a macro under the requested name, and no Intel
option is named `-marker`.