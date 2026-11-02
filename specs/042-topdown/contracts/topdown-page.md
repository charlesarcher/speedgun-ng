# Contract: the main-page section, the example it quotes, and the guide page

**Feature**: specs/042-topdown | **Files**: `README.md` with
`example/docs/topdown.cpp` and `docs/pages/harness.md` | **Audit point**: `65c48be9`
**Spec**: FR-044 to FR-046, FR-058, D-15

## What this contract covers

The 34th capability section of the main page, the example whose region it
quotes, the quick-reference and option rows, the guide page, and the line
budget that pays for all of it.

## The section on the main page

The block stands under `## How you do it` (`README.md:97`), between
`### Multi-pass collection` (`README.md:326`, span 326-349) and `### Skips`
(`README.md:350`). Page side, verbatim:

````markdown
### Top-down analysis
One call reads the level-1 split of the running CPU's own table.
<!-- sg-example: example/docs/topdown.cpp#topdown -->
```cpp
<the region body, 15 lines>
```
Each node prints with its value, and a host with no source says so once.
````

A section costs six lines plus its body. `### Hardware counter metrics`
(`README.md:301-325`, span 25) is heading 301, purpose 302, marker 303, fence
304, body 305-323, fence 324, result 325, and no blank separates it from the
next heading. A 15-line body gives a 21-line section, four inside
`SECTION_LINE_CAP = 25`
(`tools/docs/example_drift.py:241`).

## The budget: 24 lines in, 24 lines out

`README.md` measures 1004 lines and `PAGE_LINE_CAP = 1004`
(`example_drift.py:240`), so the cap does not move and 24 added lines need 24
trimmed ones (D-15). The check reads the main page alone (`:764-765`) and answers
`README.md: N lines, the cap is 1004` (`:656-657`).

| Adds | Lines | Trims | Lines |
|---|---|---|---|
| heading, purpose, marker, two fences, result | 6 | the blank before the next heading at 410, 428, 446, 468, 492, 513, 537, 562, 578, 601, 620, 635, 657, 675, 693, 740, 763, 786, 811, 834 | 20 |
| the `topdown` region body | 15 | the blank between purpose and marker at 390 | 1 |
| two name rows above `README.md:946` | 2 | that split blank at 946 | 1 |
| the `--topdown` row of the option table | 1 | the two `## Planned work` rows at 1001 and 1002 | 2 |

Each trim line reads empty today, and the shape each one leaves already stands
on the page: 12 of the 38 headings there stand directly after a sentence
(`README.md:114-115`, `:300-301`, `:325-326`), 2 directly after a closing fence
(`:152-153`, `:217-218`), and `README.md:1001` and `:1002` name output that
ships.

## The three lists the drift tool reads

- `SECTIONS` (`example_drift.py:33-68`, 33 titles, `"Report views"` last at
  `:67`) gains `"Top-down analysis"`; a section the tuple lacks answers
  `section 'Top-down analysis' is not one of the 33 capability sections`
  (`:688-693`).
- `SECTION_OWNERS` (`:73-108`, 33 keys) gains `"Top-down analysis":
  ("topdown.cpp",)`; a missing key raises `KeyError` at `:683`, and a region
  whose file another section owns answers `belongs to another section` (`:684-687`).
- `NAME_SCAN_HEADERS` (`:530-538`, seven headers) gains
  `"counters_topdown.hpp"`, or the name rows answer `quick-reference name '<x>'
  is not in ...` (`:772-780`).

A section that holds no marked block answers `holds no marked block`
(`:674-678`), and one line over the cap answers `runs 26 lines, the cap is 25`
(`:669-673`).

## The two tables and the option row

`table_rows` walks rows until the first line that does not start with a bar
(`example_drift.py:519-523`), so the blank at `README.md:946` splits the name
table: the scan sees 107 rows today and ends at `renderText`. Deleting that
blank pulls `README.md:947-952` in, and all six of those names stand in the
seven scanned headers, measured with the tool's own `header_names`.

The option table at `README.md:954` holds 42 rows (`:956-997`) and the tool
reads 42 long options from the table today, measured. `--topdown` needs its row
or the gate answers `long option '--topdown' has no option row` (`:791-793`).
The guide page moves with it in both directions:
`thePageAndTheTableAgreeInBothDirections`
(`test/source/harness_cli_table_test.cpp:1710`) compares the page table at
`docs/pages/harness.md:666` against the live row, help text included.

## The example and the run gate

`example/docs/topdown.cpp` joins the list at `example/CMakeLists.txt:50-82`
with `add_docs_example(topdown)` after `report_views` (`:82`); the function at
`:40-48` builds `docs_topdown` and hangs `run_docs_topdown` on `run-examples`.
Markers `// sg-example:begin topdown` and `// sg-example:end topdown` bound the
region (grammar at `example_drift.py:18-24`), and the page body must equal the
region line for line.

`example_run` (`test/CMakeLists.txt:1963-1969`, `TIMEOUT 300`) demands, per
example source: the built binary `docs_topdown` (`test/example_run.sh:42`), a
non-empty `--list` (`:48-53`), a run at the fixed `--min-time=0.01` (`:57`), no
line holding `FAILED` in either stream (`:63-68`), exit status 0 (`:69-74`),
and one stdout line starting at column 1 for each registered benchmark (`:86`).
A `SKIPPED:` row needs its `// example-run: expect-skip` marker, and a declared
skip that never prints fails (`:76-101`, `:111`). The unsupported path
therefore prints its one message, holds `FAILED` out of it, and exits 0
(FR-041). The snippet gate wraps each main-page cpp fence in a four-line
prelude and syntax-compiles it (`test/snippet-compile/run.sh:84-92`), so the
quoted body compiles alone.

## The guide page

`## Top-down analysis` goes after `## Multi-pass collection`
(`docs/pages/harness.md:1549`, which ends before `## Jitter probe` at `:1673`);
`## Hardware counter metrics` stands at `:1280`. The page runs 2253 lines and
carries no cap, since the cap check reads the main page alone
(`example_drift.py:764-765`). Four edits ride with the section: the option row
at `:666`, the `<value>` line of the rule grammar fence (`:1460-1461`), the
null rule (`:1403-1410`), and the stamp sentence at `:1399`, which reads `A run
writes 1.10` while `source/harness/json_report.cpp:43` stamps `1.11` today and
`1.12` after this change (D-17).

## Naming and the gate

The main page runs under `MAIN_CLASSES` (`example_drift.py:450-461`), which
bars spec ids (`:455`), requirement ids (`:456`) and any path under `include`,
`source`, `test`, `tools` or `external` (`:460`); guide pages run under
`DOCS_CLASSES` (`:475-487`), which adds task ids, feature numbers and contract
labels. `python3 tools/docs/example_drift.py .` exits 0 at the audit point and
must again, with `example_run`, `snippet_compile` and `report_shape` green in
CTest.
