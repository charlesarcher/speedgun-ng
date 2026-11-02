# Contract: the `--topdown` option, its two forms and its refusals

**Feature**: specs/042-topdown | **Files**: `source/harness/detail/options.hpp`
with `source/harness/options.cpp` | **Audit point**: `65c48be9`
**Spec**: FR-019, FR-050, D-5

## The row and its constant

`OptionRow` holds `name`, `value`, `argument`, `fromEnvironment`, `help` and
`owner` (`source/harness/detail/options.hpp:132-140`). The constant stands
beside the last lane constant, `kGroupLayoutOptionValue = 340`
(`options.hpp:97`):

```cpp
// specs/042-topdown FR-019, D-5: the top-down lane holds the block 350 to 359.
inline constexpr int kTopdownOptionValue = 350;
```

The row closes the table, after `hunt-baseline`
(`source/harness/options.cpp:765-770`) and inside the array that ends at
`:771`, so every earlier name keeps its place
(`specs/028-full-command-line/contracts/option-table.md`).

```cpp
      // specs/042-topdown FR-019, FR-050, D-5: `getopt_long` reads an optional
      // argument in the attached form only, so `--topdown=2` asks for level 2
      // and `--topdown 2` asks for level 1 and leaves `2` positional. The
      // command line owns the row: no environment name, no option file.
      {"topdown",
       kTopdownOptionValue,
       OptionArgument::OPTIONAL,
       false,
       "Prints the top-down tree of the running cpu at that level",
       "specs/042"},
```

`fromEnvironment = false` means no `SPEEDGUN_TOPDOWN` exists: the gather walks
the table and skips every row without the flag
(`source/harness/options.cpp:2189-2192`).

## The two forms

| Typed | What the run asks | Why |
|---|---|---|
| `--topdown` | level 1 (FR-019) | the row's own scan arm sees `optarg == nullptr` |
| `--topdown=3` | level 3 | the same arm reads the attached text |
| `--topdown 3` | level 1, and `3` stays positional | an optional argument attaches only, the rule the table records for `--hunt` at `options.cpp:694-697` |
| `topdown` on an option file line | a refusal | the arm below |

The scan's `default:` arm turns an absent optional value into the seed text `0`
(`options.cpp:1003-1006`), so the bare word and `--topdown=0` arrive there as
one text and FR-019 cannot hold. The row therefore takes its own arm in the
scan switch, beside `case kHuntOption:` (`options.cpp:920-923`), where a null
`optarg` is the bare word and any other text is a level. `hunt` holds this
shape already (`:698-703`). The three optional rows of today keep answering as
they do now: `interleave` (`:630`), `graph` (`:674`) and `hunt` (`:698`); spec
D-5 numbers them `:632`, `:676` and `:700`, two lines late, and opens its seed
range `1001-1006` two lines early. The arm writes one member of `RunOptions`:
an engaged `std::optional<std::uint64_t>` is the option standing, and its value
is the level asked. No presence flag joins the record.

## Every refusal a caller can read

One line on standard error and exit status 2, before any benchmark runs. The
shape is the range reader's: `"%s: %s, got '%s'"` then `std::exit(2)`
(`source/harness/hunt_cli.cpp:43-49`).

| Typed | The one line |
|---|---|
| `--topdown=0` | `--topdown: expected 1 to 6, got '0'` |
| `--topdown=7` | `--topdown: expected 1 to 6, got '7'` |
| `--topdown=x` | `--topdown: expected 1 to 6, got 'x'` |
| `--topdown=` | `--topdown: expected 1 to 6, got ''` |
| `topdown` in an option file | `<file>:<line>: the option 'topdown' cannot stand in an option file` |

Measured on the nearest rows today: `--hunt-top=0 --list` answers `--hunt-top:
expected 1 to 10000, got '0'` at status 2, and a file line `hunt=1` answers
`/tmp/n42/opts_hunt.txt:1: the option 'hunt' cannot stand in an option file` at
status 2. The row takes no arm in `applySourceValue`, so a file line lands in
the `default:` arm that builds that text (`options.cpp:1517-1528`), and `refuse`
puts the origin in front of it (`:1494-1495`).

## The help line and the table oracle

`printHelp` walks the table and prints `"  --%s%s  %s\n"` with the row's
argument text (`options.cpp:1039-1044`), and an optional row's argument text is
`" [<value>]"` (`options.cpp:460-466`). `--help` measures 42 lines today, and
the new line reads:

```text
  --topdown [<value>]  Prints the top-down tree of the running cpu at that level
```

The spelling `--topdown[=<level>]` belongs to the README option row. The help
line holds the shape above, and `test/source/harness_cli_table_test.cpp` moves:

- `kContract` (`:244`, closed at `:287`) holds 43 rows and ends
  `{"topdown", kOptional, 350}`; the count case `theTableHoldsFortyTwoRows`
  (`:735`) and its message `the table holds the 42 rows of the contract`
  (`:740`) read 43.
- `inNewBlock` (`:817-824`) gains the block 350 to 359 beside the 340 bounds
  (`:318-319`), and a case `theTopdownRowStandsInItsBlock` joins, modeled on
  `theLayoutRowStandsInItsBlock` (`:1101-1118`).
- The partition loop (`:938-957`) reads every index at or past
  `kRowsBeforeHunt` (`:295`) as a hunt row inside 270 to 289, so the tail row
  must leave that predicate and answer `inNewBlock` instead.

## What a test must pin

Each case in `test/source/harness_topdown_option_test.cpp` runs the entry point
once and checks the line, the stream, the status, and that no benchmark ran.

| Fact | Case |
|---|---|
| `--topdown` asks level 1; `--topdown=4` asks level 4 | `theBareWordAsksLevelOne` (FR-019) |
| `--topdown 4` asks level 1 and the word stays positional | `aSpaceSeparateWordIsNoLevel` (D-5) |
| The five lines above, verbatim, at status 2 | `aBadLevelAnswersOneLine` (FR-050) |
| No `SPEEDGUN_TOPDOWN` name reaches the run | `noEnvironmentNameExists` (FR-019) |
| The option file line answers the refused text with its origin | `aFileLineRefusesTheRow` (FR-031) |
| The help answer carries the row's line, and the table holds 43 rows | `harness_cli_table_test.cpp`, `theHelpAnswerPrintsOneLinePerRow` (`:1397`) |

## Shared file

`source/harness/detail/options.hpp` carries the constant and
`source/harness/options.cpp` the row and the arm; both are harness-internal, so
the option moves no exported layout. The `SOVERSION` moves a record member
prices stand in `topdown-report-key.md`; the README option row the drift gate
asks for stands in `topdown-page.md`.
