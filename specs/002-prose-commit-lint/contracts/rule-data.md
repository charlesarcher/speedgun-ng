# Contract: Rule Data File

**Feature**: `002-prose-commit-lint` | [plan.md](../plan.md) | [data-model.md](../data-model.md)

`tools/prose/prose_rules.yaml` is the single editable source the spec names
Rule Data File. The constitution is normative and this file is its
mechanical projection: on disagreement the constitution wins and the fix
lands in both in one change (`.specify/memory/constitution.md`, Governance;
spec Assumption).

## Shape

```yaml
version: 1

thresholds:
  title_max: 50                    # FR-009
  body_wrap: 72                    # FR-013
  trivial_max_changed_lines: 5     # FR-013, R-10

marker: "prose-lint: allow"        # FR-005, R-06

sections:                          # FR-010, seeded from history plus the
                                   # constitution example token `runner`
                                   # (constitution.md:443), R-11, D4
  - CMake
  - CI
  - Constitution
  - Docs
  - Meta
  - dbc
  - deploy
  - runner
  - test

vague_titles:                      # FR-012
  - wip
  - fix stuff
  - fixes
  - updates
  - update
  - cleanup
  - misc

non_imperative_shapes:             # FR-011, R-11
  - Added
  - Adds
  - Fixing
  - Fixes
  - Updating
  - Update of
  - was added
  - were added

exclusions:                        # FR-006, R-05
  - .opencode/
  - .specify/scripts/
  - .specify/templates/
  - build/
  - docs/images/
  - test/prose-gate-fixture/       # fixture files carry deliberate
                                   # violations and are checked only by the
                                   # harness that owns them

auto_exempts:                      # FR-004, R-06, D2. Documents gate behavior.
  - fenced-block
  - inline-code-span
  - indented-code
  - url
  - path
  - shell-command-line
  - blockquote-line                # a line whose stripped form begins with '>'
  - markdown-structure             # thematic breaks, table delimiter rows,
                                   # bare HTML comment delimiters (D5, FR-004)

rules:
  - id: XI1.EMDASH
    family: XI.1
    constitution: Principle XI.1
    kind: codepoint
    pattern: "\u2014"
    scope: [markdown, c-comment, shell-comment, cmake-comment,
            commit-title, commit-body]
    message: use a colon, semicolon, comma, or parentheses

  - id: XI5.MARKETING
    family: XI.5
    constitution: Principle XI.5
    kind: vocabulary
    tokens_source: constitution    # tokens copied verbatim at implementation
    tokens: [seamlessly, cutting-edge, leverages, world-class, best-in-class,
             industry-leading, robust, blazing-fast, elegant, powerful]
    scope: [markdown, c-comment, shell-comment, cmake-comment,
            commit-title, commit-body]
    message: a performance claim carries a number, a platform, and a distribution
```

Only two rules are shown; the file carries seven prose rules and drives the
eight commit rules through `thresholds`, `sections`, `vague_titles`, and
`non_imperative_shapes`.

## Provenance of every vocabulary

| Data field | Normative source | Rule of copying |
| --- | --- | --- |
| `XI3.VOUCHER` tokens | Principle XI.3 ban list | Verbatim, including multi-word entries such as `to be honest` and `straight answer` |
| `XI5.FILLER` tokens | Principle XI.5 filler and hedge list | Verbatim, including phrases such as `it's worth noting` and `in order to` |
| `XI5.MARKETING` tokens | Principle XI.5 marketing list | Verbatim |
| `XI2.CONTRASTIVE` pattern | Principle XI.2 named constructions | Three alternations: `, not `, ` rather than `, ` instead of `, plus the `does this, not that` shape |
| `XI4.META-EDITORIALIZING` tokens | Principle XI.4 banned patterns | Verbatim phrases |
| `XI1.*` | Principle XI.1 | U+2014 for `XI1.EMDASH`. `XI1.DOUBLE-HYHEN` uses the pinned regex `(?<!-)-{2,3}(?!-)`: a run of exactly two or three hyphens, so `foo--bar` and ` -- ` fire, single hyphens like `whole-file` stay silent, and the two rejected readings ("word-edge adjacent" versus "word-edge rejecting") collapse into one. Coverage recorded (D5): Markdown thematic-break lines (`---`), table delimiter rows (`| --- |`), and bare HTML comment delimiters (`<!--`, `-->`) match that regex as markup structure, so precedence row 8, `markdown-structure`, exempts lines composed solely of `-`, `:`, `|`, and space with a hyphen run inside, plus lines that are exactly one comment delimiter. En-dash U+2013 is legal between numeric or alphanumeric endpoints (FR-002). No other dash code point is in scope until added here |
| `sections` | Pull Request Quality, Title | Observed history via `git log --all --pretty=%s`, case preserved, counted 2026-09-16: 21 `Docs`, 15 `dbc`, 6 `deploy`, 5 `Constitution`, 4 each `test`, `CMake`, `CI`, `Meta`; `runner` is the D4 seed, not observed |
| `thresholds.title_max`, `body_wrap` | Pull Request Quality, Title and Body | 50 characters, 72 columns |
| `thresholds.trivial_max_changed_lines` | spec Assumption, R-10 | Mechanical stand-in for the trivial-change body exemption |

`tokens_source: constitution` is documentation for a future reader, and the
coverage probe below is what keeps the two in step.

## Load-time validation contract

Every failure exits 2 and prints the offending field path.

| Check | Failure condition | Why it exists |
| --- | --- | --- |
| Parse | YAML does not load | A gate that cannot read its rules has checked nothing |
| Version | `version` absent or any value other than 1 (single predicate, matching `data-model.md`) | Forward compatibility must be a decision |
| Id uniqueness | Two rules share an id | FR-017 requires a stable identifier per finding |
| Canonical id coverage | Any of the seven canonical prose rule ids of the namespace table below is absent (checked id by id, so a file dropping `XI5.FILLER` while keeping `XI5.MARKETING` fails), or `vague_titles`, `non_imperative_shapes`, `sections` is empty | Stops a silent deletion from quietly weakening the gate (FR-023) |
| Pattern compiles | Any `pattern` fails Python `re.compile` | Fail at load, mid-check is too late |
| Wildcard prohibition | Any `pattern` contains `\w*`, `\W*`, `.*`, or `.+` | Measured: `\bcandid\w*\b` matches `candidate`, violating FR-003 and the `spec.md:53` scenario (R-03) |
| Vocabulary probe | A `vocabulary` rule fails its built-in probe pair | Catches the wildcard mistake in its other spelling, an over-broad token such as `candid` written bare to mean `candidly` |
| Scope validity | A `scope` value outside the seven defined scopes | Prevents a rule from silently matching nothing |
| Threshold bounds | `title_max` outside 1 to 100, `body_wrap` outside 40 to 100, `trivial_max_changed_lines` outside 0 to 50 | A typo such as 500 would silently pass everything |

**The probe pair** is defined per rule kind and evaluated at load:

| Rule kind | Probe hit | Probe miss |
| --- | --- | --- |
| `vocabulary` | the first token, alone in a sentence | the same token with two letters appended, for example `robustness` style construction, which must stay silent |
| `codepoint` | a sentence containing the code point | a sentence containing only the en-dash between digits, which must stay silent |
| `regex` | one literal constructed from the pattern's own literal parts | one word-boundary neighbor case |

A rule whose probe fails is a rule that does not mean what it says, and the
gate refuses to run with it.

## Exemption precedence (canonical home)

Total order when constructs overlap on one line. The first row that applies
wins, and no later row runs on that line:

1. `fenced-block`
2. `indented-code`
3. `inline-code-span`
4. `url`
5. `path`
6. `shell-command-line`
7. `blockquote-line`
8. `markdown-structure` (markdown sources only; thematic breaks, table
   delimiter rows, bare HTML comment delimiters)
9. valid marker (suppresses rule findings on the line)
10. invalid marker (`MARKER.NO-REASON`, raised only when rows 1 to 8 do not
    already exempt the line)
11. rule matching, in `rules` order, on the remaining text

Three consequences follow the order and are part of the contract:

- An invalid marker inside a fenced block stays silent: row 1 wins over
  row 9.
- A valid marker's own text is never scanned for vocabulary: row 8 wins
  over row 10, so the marker string itself cannot fire a rule.
- Auto-exemption binds before markers are consulted: a line inside a code
  span that also carries an invalid marker is exempt, unreported.

## Marker grammar (canonical home)

- Detection: the substring `prose-lint: allow reason="<text>"` anywhere on
  the line, where `<text>` opens with a straight double quote and closes at
  the next straight double quote.
- The HTML comment wrapper `<!-- prose-lint: allow reason="..." -->` is
  accepted, and so is the bare substring outside any comment: the grammar
  inspects the substring and requires no comment wrapper.
- Valid marker: `<text>` is non-empty after leading and trailing
  whitespace removal.
- Invalid marker: the string `prose-lint: allow` appears while the
  `reason="..."` part is absent, unbalanced, or empty after stripping; the
  finding is `MARKER.NO-REASON`.
- Reason text is checked for non-emptiness and nothing more. The reason
  serves the human reviewer and is visible in the diff; an accepted
  per-line limit.
- A commit body line's width is measured for `CM.BODY-WRAP` with the
  marker substring stripped: the marker is annotation, and the quoted
  text itself stays inside `thresholds.body_wrap` (FR-013).

## Full rule identifier namespace (canonical home)

The seven prose rule ids in `rules`, the canonical-id coverage probe's
checklist:

| id | family | constitution reference |
| --- | --- | --- |
| `XI1.EMDASH` | XI.1 | Principle XI.1 |
| `XI1.DOUBLE-HYHEN` | XI.1 | Principle XI.1 |
| `XI2.CONTRASTIVE` | XI.2 | Principle XI.2 |
| `XI3.VOUCHER` | XI.3 | Principle XI.3 |
| `XI4.META-EDITORIALIZING` | XI.4 | Principle XI.4 |
| `XI5.FILLER` | XI.5 | Principle XI.5 |
| `XI5.MARKETING` | XI.5 | Principle XI.5 |

The eight commit-template rules, derived from `thresholds`, `sections`,
`vague_titles`, and `non_imperative_shapes`, asserted by the commit
fixtures:

| id | source requirement |
| --- | --- |
| `CM.TITLE-FORMAT` | FR-009 |
| `CM.TITLE-LENGTH` | FR-009 |
| `CM.SECTION-UNKNOWN` | FR-010 |
| `CM.NON-IMPERATIVE` | FR-011 |
| `CM.VAGUE-TITLE` | FR-012 |
| `CM.BODY-REQUIRED` | FR-013 |
| `CM.BODY-WRAP` | FR-013 |
| `CM.FOOTER-APPROVAL` | FR-014 |

One built-in meta-finding sits outside the data: rule files carry no such
rule, and rule-data editing cannot remove it:

| id | family | constitution reference | source requirement |
| --- | --- | --- | --- |
| `MARKER.NO-REASON` | XI.6 | Principle XI.6 | FR-005 |

## Editing contract

| Task | Edit | Guard |
| --- | --- | --- |
| Add a section token, FR-010 | one line under `sections` in the pull request that first uses it | Canonical id coverage and case-sensitivity checks still pass |
| Add a filler or voucher word | one entry under the relevant rule's `tokens` | Vocabulary probe passes, and the constitution's XI.3 or XI.5 list gains the same word in the same change |
| Add a non-imperative shape | one entry under `non_imperative_shapes` | No wildcard; entries are word forms |
| Exempt a new path | one prefix under `exclusions` | Must be vendored, generated, or checker input carrying deliberate violations (the FR-006 category), never ordinary project prose |
| Add a dash code point, for example U+2010 | one `codepoint` rule | Only with a constitution change naming it: XI.1 currently names U+2014, `--`, and `---`, and `spec.md:115` keeps every other character out of scope |
| Exempt one quoted line | the marker with a non-empty reason on that line | An empty reason raises `MARKER.NO-REASON`, FR-005 |
| Change any vocabulary, shape, or threshold value | one entry edit | The matching constitution list changes in the same pull request (the doctrine recorded at `spec.md:22`): a rule-data value diff without its constitution pairing is a review defect |

**No rule may be added from this file alone when it changes what the
constitution requires.** A new family, a new dash character, or a widened
threshold is a constitution amendment first (Governance), then a data line,
in one change.

## Constitution self-exemption, recorded here so it is explicit

Principle XI quotes its own banned vocabulary, and the commit template
quotes the trivial-change wording. The gate must not fail the document that
defines it, so:

- Fenced blocks, including the commit template block, are auto-exempt
  (`auto_exempts: fenced-block`).
- Inline code spans are auto-exempt (`auto_exempts: inline-code-span`). The
  XI.6 machine-string list (constitution lines 395-397) is backticked, so
  that exemption covers it.
- Double quotes carry no exemption. The vocabulary lists of XI.3, XI.4, and
  XI.5, and the two double-quoted claim examples of XI.2, are not auto-exempt
  and require per-line markers with reasons. The marker with a reason naming
  the quotation is the mechanism the spec requires at `spec.md:107`.

### Canonical self-quotation inventory (canonical home)

Other artifacts reference this table as the single source for the
constitution marker sweep under FR-022. Line numbers were verified against
`.specify/memory/constitution.md` on 2026-09-16.

| Principle | Lines | Verified status |
| --- | --- | --- |
| XI.1 | 338-339 | Dash examples are backticked, auto-exempt; no double-quoted self-quotation needs a marker |
| XI.2 | 350 | The three contrastive pattern placeholders are backticked, auto-exempt via inline code span |
| XI.2 | 351, 353 | The double-quoted claim example and the Wrong example carry banned constructions, marker with reason required |
| XI.3 | 362-363 | Voucher vocabulary list is double-quoted, marker with reason required per line |
| XI.4 | 371-373 | Meta-editorializing pattern list is double-quoted, marker with reason required per line |
| XI.5 filler | 378-380 | Filler and hedge list is double-quoted, marker with reason required per line |
| XI.5 marketing | 382-384 | Marketing vocabulary list is double-quoted, marker with reason required per line |
| XI.6 | 395-397 | Machine-string list is backticked, auto-exempt via inline code span |
