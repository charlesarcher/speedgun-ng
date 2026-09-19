# Data Model: Prose and Commit-Message Lint Gate

**Feature**: `002-prose-commit-lint` | **Plan**: [plan.md](./plan.md) | **Date**: 2026-09-11

Logical view required by constitution Principle III: what the feature is
and how it behaves. The physical view is in the plan's Project Structure.
Rule identifiers are stable: a finding names one, and renaming one is a
breaking change to the rule data.

## Entity Overview

```text
RuleDataFile 1 ──── * Rule          (7 prose rules across 6 families, 8 commit rules)
             │
             ├───── 1 Thresholds    (title_max, body_wrap, trivial_max_changed_lines)
             ├───── * SectionToken  (allowed section prefixes)
             ├───── * VagueTitle
             ├───── * NonImperativeShape
             └───── * Exclusion     (path prefixes out of scope)

ExemptionMarker 1 ── 1 CheckedUnit  (at most one marker line may exempt its own line)

CheckedUnit   * ──── 1 ProseSource  (a file's comment-extracted prose, or one commit field)
Verdict       1 ──── * Finding      * ──── 1 Rule
CommitRecord  1 ──── * Finding      (via CheckedUnit references)
```

## Entity: Rule

One banned pattern or one exemption, identified by a stable identifier.
The spec defines it in Key Entities; this is its field-level shape.

| Field | Type | Validation | Source requirement |
| --- | --- | --- | --- |
| `id` | string | Unique within the file, `^[A-Z0-9][A-Z0-9._-]*$`, stable across releases | FR-017 |
| `family` | string | One of `XI.1`, `XI.2`, `XI.3`, `XI.4`, `XI.5`, `commit` | FR-001, FR-017 |
| `constitution` | string | Non-empty reference such as `Principle XI.1`, so a reviewer can read the rule | Key Entities |
| `kind` | enum | `codepoint`, `regex`, `vocabulary`, `list-membership` | R-03, R-04 |
| `tokens` | list of strings | For `vocabulary`: non-empty, whole-word, no entry containing a space-only or wildcard-only form | FR-003 |
| `pattern` | string | For `regex` and `codepoint`: compiles under Python `re`; must contain no `\w*`, `\W*`, `.*`, or `.+` | R-03 |
| `scope` | set | Subset of `{markdown, c-comment, shell-comment, cmake-comment, commit-title, commit-body}` | FR-006, FR-015 |
| `message` | string | Non-empty, states what to do instead | FR-017 |

**Prose rule identifiers**, seven rules across the six families FR-001
names:

| `id` | Family | Constitution | Kind | Scope |
| --- | --- | --- | --- | --- |
| `XI1.EMDASH` | XI.1 | No em-dashes | codepoint U+2014 | all prose scopes |
| `XI1.DOUBLE-HYPHEN` | XI.1 | No em-dashes | regex `--{1,2}` bounded to word edges | all prose scopes |
| `XI2.CONTRASTIVE` | XI.2 | No contrastive framing | regex, three alternations | all prose scopes |
| `XI3.VOUCHER` | XI.3 | Never vouch for truthfulness | vocabulary | all prose scopes |
| `XI4.META-EDITORIALIZING` | XI.4 | No meta-editorializing | vocabulary of phrases | all prose scopes |
| `XI5.FILLER` | XI.5 | No filler or hedge | vocabulary of words and phrases | all prose scopes |
| `XI5.MARKETING` | XI.5 | No marketing vocabulary | vocabulary | all prose scopes |

The `EARS` weak-language ban of Principle XI.5 stays out of the gate: it
binds EARS statements specifically, deciding that a sentence is an EARS
statement is a grammatical judgment, and FR-001 lists six families and does
not name it.

**Commit rule identifiers**, eight rules:

| `id` | Rule | Threshold source |
| --- | --- | --- |
| `CM.TITLE-FORMAT` | `<Section>: <Imperative description>`, single space after the colon, no trailing period | FR-009 |
| `CM.TITLE-LENGTH` | Title at most `thresholds.title_max`, default 50 | FR-009, R-10 |
| `CM.SECTION-UNKNOWN` | Section token present in `sections` | FR-010, R-11 |
| `CM.NON-IMPERATIVE` | The title's description (after the `<Section>: ` prefix) begins with an entry in `non_imperative_shapes`; matching is at the head of the description, so a passive phrase deeper in an otherwise imperative title stays silent | FR-011, R-11 |
| `CM.VAGUE-TITLE` | Title equals, case-insensitively, an entry in `vague_titles` | FR-012 |
| `CM.BODY-REQUIRED` | A body follows after one blank line unless changed lines at most `thresholds.trivial_max_changed_lines`, default 5 | FR-013, R-10 |
| `CM.BODY-WRAP` | No body line wider than `thresholds.body_wrap`, default 72 columns, measured with any exemption marker substring stripped (clarification 2026-09-16) | FR-013 |
| `CM.FOOTER-APPROVAL` | An `Approved-by:` footer is present | FR-014 |

Merge commits carry one state-dependent exception: `CM.BODY-REQUIRED` does
not apply, while every other commit rule does (R-09).

## Entity: RuleDataFile

The single editable source named by the spec: `tools/prose/prose_rules.yaml`,
holding the rules, the section list, the vague-title list, the
non-imperative shape list, the exclusions, and the thresholds.

| Field | Type | Validation | Source requirement |
| --- | --- | --- | --- |
| `version` | integer | Must be 1; unknown versions exit 2 | Data evolution |
| `rules` | list of Rule | Non-empty, ids unique, all seven canonical prose rule ids present, checked by id so dropping one of the two XI.5 rules fails load | FR-001, FR-023 |
| `sections` | list of strings | Non-empty, case-sensitive, seeded with nine tokens, the eight observed plus `runner` named by the constitution's example section areas (`constitution.md:443`): `CMake`, `CI`, `Constitution`, `Docs`, `Meta`, `dbc`, `deploy`, `runner`, `test` | FR-010, R-11, D4 |
| `vague_titles` | list of strings | Non-empty, matched case-insensitively against the whole title | FR-012 |
| `non_imperative_shapes` | list of strings | Non-empty; entries are word forms such as `Added`, `Adds`, `Fixing`, `Fixes`, `Update of`, `was added` | FR-011, R-11 |
| `exclusions` | list of path prefixes | Non-empty; seeded with `.opencode/`, `.specify/scripts/`, `.specify/templates/`, `build/`, `docs/images/` | FR-006, R-05 |
| `auto_exempts` | list of constructs | Fixed set: fenced block, inline code span, indented code, URL, path, shell command line, blockquote line. Present as documentation of gate behavior and for fixture assertions | FR-004, R-06 |
| `marker` | string | The literal `prose-lint: allow`, used to detect markers | FR-005, R-06 |
| `thresholds.title_max` | integer | 1 to 100, default 50 | FR-009 |
| `thresholds.body_wrap` | integer | 40 to 100, default 72 | FR-013 |
| `thresholds.trivial_max_changed_lines` | integer | 0 to 50, default 5 | FR-013, R-10 |

**Load-time validation**, run before any check. A violation exits 2 with a
message naming the offending field, because a gate reading a broken rule
file cannot claim to have checked anything. Three validations are the
substantive ones:

1. **Wildcard prohibition**: any `pattern` containing `\w*`, `\W*`, `.*`, or
   `.+` is rejected. Measured reason: `\bcandid\w*\b` matches `candidate`,
   which violates FR-003 and the `spec.md:53` acceptance scenario, while the
   enumerated alternation `(?:candid|candidly)` matches `candidly` and stays
   silent on `candidate`. See R-03.
2. **Vocabulary whole-word guarantee**: every `vocabulary` rule compiles to
   `\b(?:...)\b` with `re.IGNORECASE` and is tested against a built-in
   probe pair, one hit and one longer-word miss, at load time. A rule whose
   probe fails exits 2.
3. **Coverage completeness**: all seven canonical prose rule ids are
   present, checked id by id, so a rule file that silently drops a rule,
   `XI5.FILLER` alongside `XI5.MARKETING` included, fails to load.
   Quietly checking less is the failure FR-023 exists to stop.

## Entity: ExemptionMarker

A per-line annotation that suppresses prose-rule findings for its own line.

| Field | Type | Validation |
| --- | --- | --- |
| `raw` | string | Contains the literal `marker` value |
| `reason` | string | Non-empty after trimming; empty raises a finding of its own |
| `line` | integer | The line it appears on, and the only line it affects |

**States and transitions**: `absent` → `present-valid` → `present-invalid`.
A marker with an empty reason transitions to `present-invalid` and produces
finding `MARKER.NO-REASON` on that line, which is the `spec.md:55`
acceptance scenario. There is no `disabled-region` state by design: block
toggles let a forgotten end marker silently stop checking a file, and the
repository treats a silent skip as a defect (`cmake/coverage.cmake:6-13`,
R-06).

`MARKER.NO-REASON` is a built-in meta-finding: it reports
with `family=XI.6` and constitution reference `Principle XI.6`, and its
source requirement is FR-005, so the FR-017 report shape (rule id, family,
constitution reference) holds for it. The full data-rule namespace is
enumerated in [contracts/rule-data.md](contracts/rule-data.md).

**Where markers live**: inside the comment or the Markdown line they exempt,
so the same literal string works in Markdown, C, C++, shell, and CMake. The
constitution's self-quoting lines divide between the two mechanisms: fenced
and inline-code forms fall under `auto_exempts`, and every double-quoted
self-quoting line of Principle XI carries its own marker with a reason. The
canonical inventory of those lines lives in
[contracts/rule-data.md](contracts/rule-data.md), section "Constitution
self-exemption". This closes the gap the spec names at `spec.md:107`.

## Entity: ProseSource and CheckedUnit

`ProseSource` is one unit of text the prose check reads: a Markdown file, or
the comment text extracted from one C, C++, CMake, or shell file, or one
commit title or body.

`CheckedUnit` is one candidate line together with its authorship status.

| Field | Type | Validation |
| --- | --- | --- |
| `source` | ref | Owner `ProseSource` |
| `line` | integer | 1-based, in new-file coordinates |
| `text` | string | May be empty after extraction, which skips the unit; a trailing `\r` of a CRLF line ending is stripped before matching and column counting |
| `authorship` | enum | `new`, `modified`, `grandfathered` |
| `exempt` | enum | `none`, `auto`, `marker`, `marker-invalid` |

**Authorship is set by mode**, and it is the whole mechanism behind the
grandfathering clause of Principle XI.1:

| Mode | `authorship` assignment |
| --- | --- |
| `range` (pull request) | Lines reported by `git diff -U1 --no-color --no-renames BASE...HEAD` as `+` become `new`; the emitted context lines (the leading-space lines inside the ±1 window of a hunk that also has `+` lines) become `modified`; every other line becomes `grandfathered`. `-U1` is chosen because `-U0` emits no context lines at all, which would make `modified` unreachable and FR-007 unimplementable (R-08 amendment). A fixture must exist whose only passing condition is a `modified` firing on a line one below an added line, with a line two below staying grandfathered |
| `tree` (whole file) | Every line becomes `new`, which is what makes whole-file mode report the 225 pre-existing em-dashes |
| `commit` | Title and body lines are `new` for the commit being checked |

**Reporting rule**: a finding is raised when `authorship` is `new` or
`modified`, `exempt` is `none`, and a rule matches. `grandfathered` or any
`exempt` value suppresses, and `marker-invalid` reports the marker instead of
the underlying rule. This is FR-007, FR-008, and the `spec.md:57` and
`spec.md:114` scenarios. The precedence among exempt constructs is the
canonical total order in [contracts/rule-data.md](contracts/rule-data.md),
section "Exemption precedence".

**Skipped sources**: a file that fails UTF-8 decode, or that carries a
byte-order mark, becomes a `ProseSource` in state `skipped` with a reason,
counted in the summary and never a crash (`spec.md:111`). The skip
contributes nothing to the exit code: an encoding defect is not a prose
finding, and that is the recorded, accepted deviation from the never-skip
doctrine, made visible by the printed reason and the summary count rather
than silent.

## Entity: CommitRecord

| Field | Type | Validation |
| --- | --- | --- |
| `sha` | string | Full 40 hex |
| `short` | string | 7 hex, as reported |
| `title` | string | First line |
| `body` | string | Everything after the first blank line, empty when absent |
| `footers` | list of `Key: value` | Parsed from the trailing paragraph |
| `changed_lines` | integer | Sum of added plus removed from `git show --numstat -M` (rename detection so a pure rename does not inflate), a binary file's `-\t-\t` counted as 0, and 0 for an empty commit; a merge commit prints no numstat lines and so sums to 0, deliberate given the R-09 body exemption (R-10 amendment) |
| `parents` | list of sha | Length 2 means merge commit, which relaxes `CM.BODY-REQUIRED` only |
| `in_range` | boolean | Determined by `git rev-list BASE..HEAD`, two-dot enumeration so base-side-only commits never enter the checked set; the prose diff stays three-dot `BASE...HEAD` (R-07 amendment) |

**Transitions**: `unresolved-range` is a state of the run, and it is
terminal with exit 2 (R-07). An empty range is a successful run with zero
`CommitRecord`s (`spec.md:110`).

## Entity: Finding and Verdict

| `Finding` field | Type | Notes |
| --- | --- | --- |
| `rule` | Rule | Supplies `id`, `family`, `constitution` |
| `location` | string | `path:line` for prose, `commit <short>` for commits |
| `token` | string | The offending substring, so FR-017 can name it |
| `message` | string | From the rule |

| `Verdict` field | Type | Notes |
| --- | --- | --- |
| `findings` | ordered list of Finding | Ordered by location, then by rule id, so CI and local output are identical for identical input |
| `units_examined` | integer | US1 scenario 7 requires the count |
| `sources_skipped` | list | Path plus reason |
| `exit_code` | integer | Derived: 2 if the environment was unusable, else 1 if findings, else 0 (FR-018) |

**Ordering is a contract**: identical input must produce byte-identical
output in CI and locally, which is US3 scenario 3 and the parity claim in
the CLI contract.

## Validation Rules Summary

| Rule | Enforcement point | Outcome |
| --- | --- | --- |
| Rule data parses, ids unique, families complete | Load | exit 2 |
| No wildcard patterns in rule data | Load | exit 2 (R-03) |
| Vocabulary probe pair passes | Load | exit 2 |
| Marker requires a non-empty reason | Per line | finding `MARKER.NO-REASON` (FR-005) |
| Whole-word vocabulary, longer words silent | Per line | `XI3`, `XI5` rules only (FR-003) |
| Deleted-side lines never reported | Per diff | by construction of the `-U1` parse (R-08, `spec.md:114`) |
| Unresolvable range | Run start | exit 2, message names the inputs (R-07) |
| Merge commit | Per commit | `CM.BODY-REQUIRED` skipped, all others enforced (R-09) |
| Excluded or vendored path | Discovery | never read (FR-006, R-05) |

## Out of Model

Interactive chat replies leave no artifact to read, so Principle XI's
coverage of them stays with reviewer discipline (spec Assumption). Spell
checking stays with `spell-check`. The tree-wide em-dash sweep is a separate
formatting-only change under Principle V, and the model supports it through
`tree` mode alone, with no new state.
