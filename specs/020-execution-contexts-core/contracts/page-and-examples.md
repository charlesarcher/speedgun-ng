# Contract: The main page, the guide, and the examples

The docs obligations: two new main-page sections in the D1 shape,
quick-reference rows, the detail page, the owning examples, and the
page-cap rule FR-034 states (FR-029 to FR-035).

## The main page (FR-029 to FR-034)

| Rule | Requirement |
| --- | --- |
| The main page shall gain one "Execution contexts" section and one "Threads" section, each in the D1 shape of one purpose sentence, one marked snippet, and one result sentence; the first teaches a context, a policy, and the dispatch of a noise load, and the second teaches `threads(4)` and `threadIndex()`. | FR-029 |
| Each new section's snippet shall be a marked region of its own runnable file under `example/docs/`, built by a `run_docs_` target and run through an `example_run` entry, in the shape H4 set for every main-page section. | FR-030 |
| The quick reference shall gain a row for each new public name this feature ships, in the table's own columns; the option table gains a row only when a command-line option lands, and this feature adds none. | FR-031, FR-044 |
| "Reading the report" shall show a `/threads:N` row with the FR-025 disclosure legible in it. | FR-032 |
| `docs/pages/harness.md` shall gain the aggregation table of FR-020 and the lifecycle rules of FR-006, in the reader voice of the rest of that page and with no spec numbers. | FR-033 |
| The drift gate's `SECTIONS` list gains the two new titles and `SECTION_LINE_CAP` stays 25. The page cap moves by the measured size of the two new sections and their quick-reference rows, the amount the spec's budget note measures when the sections land; prose is trimmed first and no snippet is dropped to fit; no file under `specs/019-harness-user-counters-and-statistics/` is edited. | FR-034 |

## The owning examples and the gate (FR-030, FR-035)

```text
example/docs/execution_contexts.cpp   # owns the "Execution contexts" region
example/docs/threads.cpp              # owns the "Threads" region
example/docs/reading_the_report.cpp   # edited: gains a /threads:N row
example/docs/reading_the_report.expected
```

Both new files follow the shipped `add_docs_example(NAME)` shape: a
`main` calling `sg::speedgunMain`, built as `docs_<name>` with a
`run_docs_<name>` target on `run-examples`. The `example_run` gate
covers both files: an undeclared skip, a `FAILED` row, and a registered
benchmark whose row never prints each fail the gate (FR-035).

## The drift gate (FR-034)

`tools/docs/example_drift.py`: `SECTIONS` gains the two titles,
`SECTION_OWNERS` gains the two owner files, and `PAGE_LINE_CAP` moves
by the measured amount above. The shipped `SECTION_LINE_CAPS`
overrides and the 019 owner exemptions stand untouched.

## Consumers

`test/example_run.sh` runs the two new examples;
`python3 tools/docs/example_drift.py .` passes with the nineteenth and
twentieth sections; the prose gate, the spell check, and the name check
report zero findings over every new section and artifact (FR-036,
FR-048).
