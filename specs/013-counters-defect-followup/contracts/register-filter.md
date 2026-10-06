# Contract: the register filter and the no-obligation keys

This contract covers I-01, both its halves. Every declaration below is a
public contract, and each is paired with the enforcement that guards it
(Principle II).

## What this feature changes

1. A row carrying a register filter encodes that filter through the format
   its register index names.
2. A row whose index names a pair encodes under the first index of the pair.
3. `Counter` and `Deprecated` join the no-obligation key list.

No `catalog_entry` field changes. No public type gains a member. The
change is inside the parser and inside what reaches the encoder.

## The authority

The kernel's own table generator,
`tools/perf/pmu-events/jevents.py` at kernel commit
`eaab2eb09dc2f86f41e8fa55243c31a274978233`, line 244 through 258:

```python
def lookup_msr(num: Optional[str]) -> Optional[str]:
  """Converts the msr number, or first in a list to the appropriate event field."""
  if not num:
    return None
  msrmap = {
      0x3F6: 'ldlat=',
      0x1A6: 'offcore_rsp=',
      0x1A7: 'offcore_rsp=',
      0x3E0: 'offcore_rsp=',
      0x3E1: 'offcore_rsp=',
      0x3E2: 'offcore_rsp=',
      0x3E3: 'offcore_rsp=',
      0x3F7: 'frontend=',
  }
  return msrmap[int(num.split(',', 1)[0], 0)]
```

Three facts follow, and this feature follows all three.

**The docstring names the rule**: "or first in a list". A pair publishes
under its first index. This feature adopts the same rule, and FR-010's
plan precondition is satisfied by this reading. No verification step
remains open.

**The parse is base zero**: `int(..., 0)` accepts `0x1a6`, `0x1A6`, and a
leading-zero spelling alike. The pinned tree uses seven distinct
spellings and every one parses.

**The map has no default arm**: `msrmap[...]` raises `KeyError` on an
unmapped index. A library cannot abort a process over one table row, so
this feature returns no format and the row publishes `not_encodable`.

## The published ranges

`arch/x86/events/intel/core.c` lines 6602, 6606, and 6608, at the same
kernel commit:

| Line | Format | Range |
| --- | --- | --- |
| 6602 | `offcore_rsp` | `config1:0-63` |
| 6606 | `ldlat` | `config1:0-15` |
| 6608 | `frontend` | `config1:0-23` |

**Contract**: the parser stores the format name. The range belongs to the
device, and `pmu_compose_config` at `encode.cpp:132-159` already resolves
a name against the device's own published range. A device publishing
`ldlat` at a different width is a different device, and the row encodes
through whatever the device publishes.

## The register filter's contract

**Input**: a table row with `MSRValue` present and non-zero, and an
`MSRIndex` present.

**Output**: the row carries a field named by the map entry for the first
index of its `MSRIndex`, holding the parsed `MSRValue`. The row's base
config fields are unchanged.

**States the row can reach**:

| Condition | State | Why |
| --- | --- | --- |
| `MSRValue` present and zero | `countable`, base event, no filter | a zero filter selects every occurrence of the base event |
| `MSRIndex` absent, `MSRValue` non-zero | `not_encodable` | a filter with no register to name it has nowhere to encode |
| index outside the map | `not_encodable` | the kernel raises; this feature publishes a state |
| format named, device publishes it | `countable`, filter encoded | the ordinary path |
| format named, device publishes no such format | `not_encodable` | D-07, and spec 012's FR-037 unchanged |
| no `MSRValue` at all | `countable`, base event, no filter | unchanged from today |

**Enforcement**: `SG_REQUIRE(parsed_index >= 0)` at the parser's entry, so
a malformed index never reaches the encoder, and `SG_ENSURE(fields
contains only formats the device publishes)` before the row is handed to
`pmu_compose_config`.

**Why `not_encodable` and not a wrong count**: 570 rows across the four
pinned Intel directories carry a non-zero register value today and publish
as `countable` with no filter. Their counts belong to a different event,
which is the defect. The alternative, publishing all 570 as
`not_encodable`, discards 448 valid offcore rows in those four
directories and 5997 across 34 pinned directories. Encoding keeps the row
and makes it correct.

## The no-obligation keys

**Contract**: a key on the list carries no encoding obligation whatever
its value. A key outside the list whose value parses as a number becomes a
required field, and the row publishes `not_encodable` when the device
publishes no format of that name.

The list grows by two and loses none. `Counter` and `Deprecated` join the
seven keys the parser names today.

**Why two and not a mapping**: the kernel's generator has no term for
either key. Its `event_fields` list at `jevents.py:389-404` maps
`AnyThread`, `PortMask`, `CounterMask`, `EdgeDetect`, `FCMask`, `Invert`,
`SampleAfterValue`, `UMask`, `NodeType`, `RdWrMask`, `EnAllCores`,
`EnAllSlices`, `SliceId`, and `ThreadMask`. Neither `Counter` nor
`Deprecated` appears. Mapping `Counter` to `cmask` would collide with
`CounterMask`, which is a different field with a different meaning.

**Enforcement**: the parser's drop arm is covered by the recounted
encodable figures, which rise by exactly the count of rows carrying either
key with a parseable value: 466 for `Counter` and 42 for `Deprecated`.

## The three new terms

D-05 and D-06 together add three format names the parser does not reach
today. `offcore_rsp` is already in `kernel_spelling` at
`table_parse.cpp:184-186`, unreachable because the key `OffcoreRsp` occurs
zero times in the pinned tree. The three that reach a row are:

| Table key | Format | Reached through |
| --- | --- | --- |
| `AnyThread` | `any` | D-06's key-to-term map |
| `PortMask` | `ch_mask` | D-06's key-to-term map |
| `FCMask` | `fc_mask` | D-06's key-to-term map |

**Enforcement**: a fixture row carrying each key, asserted to reach its
kernel format, and a fixture row carrying a key the generator maps but the
device publishes no format of, asserted `not_encodable`.

## The encodable-row figures

The synthetic device in `counters_linux_pmu_seam_test.cpp:767-795`
publishes `event`, `umask`, `cmask`, `edge`, `inv`, `config`, `config1`,
`config2`, and `offcore_rsp`. It grows `ldlat` and `frontend` under this
contract, at `config1:0-15` and `config1:0-23`.

| Directory | Pinned today | After this feature |
| --- | --- | --- |
| skylake | 576 | 581 |
| icelake | 342 | 346 |
| alderlake | 521 | 564 |
| sapphirerapids | 1685 | 2141 |
| amdzen4 | 326 | 326 |
| amdzen5 | 322 | 322 |

The register-filter correction moves no figure, because the parser drops
both keys today and such a row already encodes on its base fields. The
no-obligation correction moves each Intel figure by the count of rows
carrying either key. No AMD figure moves, because no AMD row carries a
register index or either key.

**Enforcement**: the seam test pins each figure, and a directory whose
measured figure differs from this table sends the correction back to
review.

## The reference host's list

The reference host publishes `cmask`, `edge`, `event`, `inv`, and `umask`
on its core device, and publishes no `offcore_rsp`, no `ldlat`, and no
`frontend`, because it is an AMD core. Every register-filter row therefore
publishes `not_encodable` there under D-07. That is a correct answer and
not a defect, and SC-005's pinned figures are measured against the
synthetic list for exactly this reason.
