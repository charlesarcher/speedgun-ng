# Contract: The Rule Parse Call and Its Value Types

## The public call

```cpp
// include/speedgun-ng/benchmark.hpp
/**
 * @brief Parse one rule text into one rule value.
 *
 * \pre `text` is the whole rule text of one `--compare-rule=` or of
 *      one library caller's rule
 * \post When the call answers a value, every field holds what the text
 *       spelled and `text` holds the input whole (FR-004)
 * \post When the call answers an error, the string names the field of
 *       the fault and the text read; the caller owns the exit status
 *       (FR-005)
 * \post The call throws nothing to the caller and aborts nothing.
 */
[[nodiscard]] SPEEDGUN_NG_EXPORT auto
parseCompareRule(std::string_view text)
    -> std::expected<CompareRule, std::string>;
```

The command line and library users read the same grammar through this
one call (spec D-1, FR-004). The value type `CompareRule` and its fields
stand in [data-model.md](./data-model.md) E-01.

## The grammar

```text
<rule>    ::= <value> [ @ <aggregate> ] : <direction> : <threshold> [ : <filter> ]
<value>   ::= time | real_time | cpu_time | p <point>
            | counter : <label> | metric : <label> | stat : <label>
<point>   ::= digits [ . digits ]
<label>   ::= any text; ends at the first unescaped colon or at sign
<aggregate> ::= median | mean | min | max
<direction> ::= lower | higher
<threshold> ::= <percent>% | abs = <number> | <percent>% , abs = <number>
<filter>  ::= any text; the rest of the rule text
```

The parse rules, each closing its FR:

- The fields run in the order above; the parse reads them in that
  order (FR-012, FR-021).
- A prefixed value name reads its label to the first unescaped colon,
  and the backslash escapes `\:` , `\,` , `\@` and `\\` stand inside the
  label (FR-020, FR-021, D-06, spec D-15 carried). An unescaped `@`
  after a `stat:` label ends the name where the parser reads the
  aggregate marker.
- The `@` suffix names one of the four aggregates and defaults to
  `median` for a statistics name (FR-016). A written suffix on a
  `p<point>` or `stat:` name is a fault: the `@median` default applies
  only to a value the report holds as a statistics record (FR-018,
  spec D-21).
- The direction word is required (FR-012).
- The threshold field carries a percent, an `abs=` number, or the two
  in that order (FR-022, FR-023, D-07). The percent answers a fraction
  (`5%` answers `0.05`); the absolute number stands in the stored unit
  of the value (spec D-22).
- The filter field is the rest of the text after the threshold's
  colon; empty means no filter (FR-024).
- A rule filter is a `std::regex` over the full instance name, the
  same matching as the batch filter (`source/harness/cli.cpp:453`); an
  invalid pattern is a rule fault (FR-024).
- Unknown words, a missing field, a malformed point, a malformed
  number or trailing text are faults naming the field (FR-005).

## Fault examples the tests carry

| Text | Fault named |
| --- | --- |
| `time:lower:5` | the threshold field: neither the percent form nor the `abs=` form |
| `time@mode:lower:5%` | the aggregate word |
| `p99@median:lower:10%` | a written aggregate suffix on a percentile cell (FR-018) |
| `stat:peak@mean:lower:5%` | a written aggregate suffix on a statistic (FR-018) |
| `time:faster:5%` | the direction word |
| `time@median:lower:abs=12.5,5%` | the threshold order: the percent stands first (D-07) |
| `time@median:lower:5%:(` | the filter regex |
| `time@median:lower:5%,abs=12.5` | accepted: the percent then the `abs=` marker |
| `counter:cache\:misses@mean:higher:2%` | accepted: the escaped colon inside the label |

## The value types

`Aggregate` and `Direction` are public scoped enums
([data-model.md](./data-model.md) E-05). `CompareRule` is public
because the comparison call consumes it (E-01). A caller may build a
rule in code instead of parsing; the `text` field then holds whatever
the caller sets, and the table and the document print it (D-08).

## What the parse never does

- It never consults a report: the grammar is one text in, one value or
  one fault out.
- It never defaults a direction or a threshold: only the aggregate
  carries a default, and only for a statistics name.
- It never repairs a filter regex or rewrites a label.
