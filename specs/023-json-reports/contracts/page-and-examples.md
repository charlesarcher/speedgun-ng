# Contract: The Main Page, the Example and the Budget

## The main-page section

`README.md` gains one section titled `JSON reports`, placed after
`Percentiles`, and `tools/docs/example_drift.py` gains the title in its
`SECTIONS` tuple (`:30-51`) and its owner in `SECTION_OWNERS`
(`:53` onward):

```python
"JSON reports": ("json_reports.cpp",),
```

The section holds one copied region of the example, one result sentence
that names what the run wrote, and one link to
`docs/pages/harness.md#json-reports`, the shape the `Percentiles` section
at `README.md:534-549` uses.

## The example

`example/docs/json_reports.cpp` is one complete program with a region
marked `json_reports`, and `example/CMakeLists.txt` gains
`add_docs_example(json_reports)` beside the list at `:50-70`. The program
runs one family with a fixed iteration count, so its rows are stable, and
its region shows the `showPercentiles` call beside the option the page
names. `test/example_run.sh` runs it through `run_docs_json_reports`.

`example/docs/json_reports.expected` records the fields the result
sentence names, the way `percentiles.expected` does. The file may stand
unreferenced by a region, the state 021 records for its own pair.

## The quick reference

The table at `README.md:551` gains three rows beside the percentile rows
at `:648-649`:

```markdown
| `--json` | Writes one JSON report document for the run |
| `--json-raw` | Selects the chunk columns the document carries |
| `loadReport` | Reads a report document back as the run's own values |
```

## The guide page

`docs/pages/harness.md` gains a `## JSON reports` section with the
anchor `json-reports`, holding: the two options, the document's top-level
shape, the `null` rule for an unanswered percentile, the schema file path,
the `loadReport` call with its error channel, and the rule that the file
carries no date. The page quotes the same region the main page quotes, so
the drift tool sees one owner.

## The line budget

`PAGE_LINE_CAP` at `tools/docs/example_drift.py:112` holds the measured
length of the page as the feature lands. The move is computed, not
guessed: the section span plus the two quick-reference rows, added to 659.
The new section's own span must stay at or under `SECTION_LINE_CAP` (25)
unless the implementer measures a larger span and records it in
`SECTION_LINE_CAPS` with the reason, the rule that gives `Registration`
38 and `Fixtures` 43.

The gate that enforces all of it:

```text
ctest --test-dir build -R 'example_drift|report_shape'
```

## What the page must not claim

- No claim that the file is stable across builds: it is stable across
  runs of the same build, which is the claim FR-011 and SC-009 make.
- No claim that the loader validates against the schema: the schema is the
  gate's oracle, and the loader checks the fields it reads.
- No percentile or report byte quoted in prose that the example does not
  produce, the drift rule the tool enforces.
