# Research: Names and User Paths Out of Old Records (T-NAMES)

All readings are from the branch base `1f1f9b20`, the live
`origin/master` at lane start.

## R1. The agreed grep and its counts

`git grep -nE 'arche[r]c|char[l]es|@gmai[l]'` over `specs docs` reads
193 lines in 71 files across 18 lanes. `docs/` reads 0. Tree-wide the
same grep reads 223 lines; the 30 kept lines sit in
`test/prose-gate-fixture/commits/cases.yaml` (19),
`test/prose-gate-fixture/run_prose_gate_fixtures.py` (5),
`.github/workflows/ci.yml` (4), `CMakeLists.txt` (1), and
`CODEOWNERS` (1). `LICENSE` reads 0: its copyright line names the
holder in words the pattern does not reach.

Token families in scope: 160 lines carry a home path (159 with the
`/home/<user>/` prefix, one bare recorded search string in
`specs/048-hwlat-probe/plan.md`); 33 lines carry the repository login
inside Actions links, `gh` endpoints, bare slugs, or one quoted
workflow expression. No line carries both. No personal email address
sits in scope.

## R2. Why the uniform `$HOME/` rule (D-1)

specs/045 F1-006 set the replacement shape ("`$HOME/` or a path
relative to the build tree"). A per-line choice between the two spellings
would put judgement into 160 mechanical edits and invite worker drift.
One spelling satisfies the ruling, keeps every recorded command shape,
and is verifiable by one grep for the old prefix. The 53 lines naming
home paths outside the checkout cannot be made runnable on a new
machine by any spelling; the cleanup removes the personal name from
them, which is the ruling's goal. Of the 160 home-path lines, 106 name
the checkout, 53 name other home paths on the old lane host, and one
carries the bare recorded string.

## R3. Why bare run ids replace run links (D-2)

specs/045 records already spell CI evidence as `PR #73 run 41576309608`
with no URL. The run id alone reaches the Actions list of this
repository from any account that can see the repository. A URL with the
owner login is the only spelling that carries the person token, so
removing the link loses no evidence and keeps the trail.

## R4. Why `<owner>` for recorded endpoints (D-3)

`gh` resolves a repository from the current directory, so a recorded
endpoint without the login reads correctly wherever it runs: the reader
is inside the repository when following a record. A `<owner>` placeholder
keeps every other endpoint segment, so the recorded API call keeps its
shape. The quoted workflow expression names a secret-free context field;
the placeholder keeps the sentence true while the kept workflow file
carries the real spelling.

## R5. Why the fixture data stays (FR-005)

`test/prose-gate-fixture/` plants the login inside commit fixtures that
the prose gate itself parses; the fixtures assert gate behavior over
login-shaped trailers. Editing them would change test inputs and break
the prose-gate fixtures, and FR-006 forbids touching `test/`. specs/045
F1-006 recorded the same decision when it moved these matches to this
ticket: the ticket cleans record text, and the fixture matches are test
inputs the ruling keeps.

## R6. Work groups (base counts)

| Group | Lanes | Lines | Files | Occurrences |
|---|---|---|---|---|
| A | 030-ci-speedup | 55 | 26 | 55 |
| B | 039-context-groups | 32 | 7 | 32 |
| C | 029-hunter-core | 26 | 4 | 26 |
| D | 043-fwq-ftq | 16 | 2 | 16 |
| E | 050, 048, 041, 045 | 23 | 11 | 23 |
| F | 024, 007, 014, 037 | 33 | 15 | 38 |
| G | 040, 031, 036, 035, 034, 032 | 8 | 6 | 8 |
| Total | 18 lanes | 193 | 71 | 198 |

Groups own disjoint file sets, so they run in parallel in one worktree.

## R7. Two pull-request URLs and the census grammar

Two base lines carry a pull-request URL naming the login:
`specs/032-fix-merged-features/results/record.md:348` (bare URL) and
`specs/035-numa-placement/results/quickstart.md:76` (a link whose text
already spells `#56`). The plan's table covers both: link text survives
as plain text, and a bare pull URL becomes `PR #<n>`.

The identifier census has one grammar and two reads. One scan, the
alternation `\b[0-9a-f]{4,}\b|\b[0-9]{2,}\b` with each match position
counted once, runs over the 193 base line bodies as written for the
recorded multiset: 49 distinct tokens, 146 occurrences. The equality
read masks, on both sides, whole markdown links to URLs, bare
`https?://` spans, a case-insensitive `run <digits>` word,
`PR #<digits>`, and `#<digits>` spans, because the D-2 table rows
replace those spans by construction: the masked read measures 39
distinct tokens and 122 occurrences at base, and the same masked read
after the rewrite must return the same multiset, line by line. The
identifiers inside masked spans are proven by the T003 rule probes and
the per-line rewrite equivalence (FR-004).

## R8. Host and gate figures at base

This host reports kernel `7.3.3-1-cachyos`, `g++ (GCC) 16.2.1
20260810`, clang 23.1.4, `vendor_id AuthenticAMD`, `cpu family 26`,
`model 68`, `model name AMD Ryzen 9 9950X3D 16-Core Processor`. The
marker read `git grep -c LCOV_EXCL -- source include` at base sums 84.
Version 0.34.0, schema 1.16, and the two SOVERSION numbers stand
unchanged through the lane.
