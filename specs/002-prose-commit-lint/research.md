# Phase 0 Research: Prose and Commit-Message Lint Gate

**Feature**: `002-prose-commit-lint` | **Plan**: [plan.md](./plan.md) | **Date**: 2026-09-11

Every decision below states what was chosen, why, and what was rejected.
Evidence is cited as measured output from this machine, `path:line`
references into tracked files, or upstream documentation quoted verbatim.
Section R-00 records where a prior research lane was lost and how its
questions were answered instead.

## R-00 Evidence provenance

Three research agents were dispatched. The internal-conventions lane
returned a complete inventory, used throughout. The two external lanes
(prior art, GitHub Actions range mechanics) completed but their session
transcripts became unreadable: `background_output`, `session_read`, and
`session_info` all fail for those session IDs. Their questions were
answered a second way, and the second way is stronger where it matters:
git and regex behavior was measured on this repository rather than
asserted from prose, and the one upstream behavior that could not be
measured was quoted from the tool's own README. Where a claim rests on
documentation alone it is labeled doc.

## R-01 Build the gate in-repo rather than adopt an existing linter

**Decision**: Implement one Python 3 gate script under `tools/prose/`,
stdlib plus PyYAML, structured as a twin of the existing `tools/dbc/`
gates. One script, two checks (prose, commit), one shared rule file.

**Rationale**: The repository already owns this shape. `tools/dbc/`
carries five gate scripts and a shared helper `dbc_gate_common.py`
defining the exit contract 0 pass, 1 findings, 2 usage
(`tools/dbc/dbc_gate_common.py:10-14`, `72-74`). Diagnostics go to
stderr, one summary line to stdout
(`tools/dbc/dbc_doc_gate.py:572-575`, `print(f"doc-gate: {n_ifaces}
interfaces, 0 gaps ({out_path})", flush=True)`). A new gate that follows
it costs a reviewer nothing to learn and needs no new dependency: FR-021
and SC-006 are satisfied by construction. Four facts from the inventory
decide the matter. No linter is configured anywhere: no
`.pre-commit-config.yaml`, no `commit-msg` hook (`.git/hooks` holds
samples only), no commitlint, no markdownlint, no `pyproject.toml`, no
mypy or ruff config, verified filesystem-wide including untracked files.
Nothing reads `git log`, commit messages, or trailers. `codespell` via
`.codespellrc` is the only prose-adjacent tool and checks spelling alone.
The feature is greenfield, and the half of it that is hardest, commit
messages with an `Approved-by:` trailer and a changed-line triviality
rule, has no off-the-shelf expression.

**Alternatives considered**:

- **gitlint** (Python). Configurable through a rules file, with
  `title-max-length`, `title-matches-regex`, `body-min-length`, and
  line-length rules, and range selection through `--commits`, `--stdin`,
  or `--commit-range`. A section prefix is expressible as a title regex.
  Rejected: it is a new pip dependency for the half the feature it can
  cover. The `Approved-by:` footer, triviality by changed-line count, the
  vague-title list, the merge-commit body carve-out, and every prose rule
  over Markdown and code comments would need custom rules or a second
  tool, so adoption buys half a feature at the cost of a dependency, a
  config vocabulary to reconcile with the constitution, and a wrapper
  anyway. FR-021 forbids it.
- **commitlint** (Node). Rich rule set for conventional commits
  (`header-maxlength`, `type-enum`, `subject-case`, `body-leading-blank`,
  `footer-leading-blank`), a close match for the template shape.
  Rejected: CI installs no Node runtime in any of the eight jobs, so it
  is a toolchain addition, not a dependency addition.
- **Vale** (Go binary). A serious prose linter with YAML rule definitions
  and a diff mode. Rejected for this feature: it does not lint commit
  messages at all, so the commit half stays custom; it needs an install
  step; and its rule vocabulary (`existence`, `substitution`, `accept`)
  would become a second home for rules the constitution already
  enumerates, which is drift risk against the single-canonical-home rule
  the constitution states for Principle XI. It is the right tool to
  revisit if the rule set grows into style guidance rather than literal
  patterns.
- **proselint** (Python). Vocabulary rules overlap Principle XI.5
  partially. Rejected: pip dependency, no commit support, and no
  per-line exemption carrying a mandatory reason, which FR-005 requires.
- **Extending `codespell`**. It is already installed in the `lint` job
  (`ci.yml:23`). Rejected: it detects misspellings, has no code-point or
  multi-word phrase semantics, no added-lines scoping, and the spec keeps
  spell-check separate by Assumption.

## R-02 Rule data is one YAML file beside the gate

**Decision**: `tools/prose/prose_rules.yaml`, one file holding the six
rule families, the section-token list, the vague-title list, the
non-imperative reject list, the exclusions list, and the numeric
thresholds. It is the single editable source named by the spec's
Rule Data File entity.

**Rationale**: The repository's machine-readable gate data is YAML:
`tools/dbc/macros.yaml`, loaded by `load_registry()` and read/written by
`write_matrix`/`read_matrix`, which dispatch on file suffix between YAML
and JSON (`tools/dbc/dbc_gate_common.py`). PyYAML reaches the runners
through apt `python3-yaml` (`ci.yml:48`, `81`, `110`, `156`, `247`) and
dnf `python3-pyyaml` in the Rocky container (`ci.yml:133`), never pip. A
contributor who can run the DBC gates already has the package. YAML also
carries comments, so the exemption rationale for a rule can live next to
the rule.

**Alternatives considered**: JSON, because `json` is stdlib and would let
the gate run where PyYAML is absent. Rejected: it forks the convention
for gate data for no functional gain, since the dependency already
exists in every job that runs Python gate scripts and the import guard
already tells a local user to install `python3-yaml`
(`tools/dbc/dbc_gate_common.py:65-70`). TOML was not considered: the
standard library `tomllib` is read-only and Python 3.11+, and the repo
has no TOML precedent.

## R-03 Whole-word matching by enumerated inflection, wildcards prohibited in rule data

**Decision**: Vocabulary rules match `\b(?:tok1|tok2|...)\b` with
`re.IGNORECASE`, where the alternation lists every inflection
explicitly. The gate validates its own rule data at load time and
rejects any pattern containing a suffix wildcard such as `\w*` or `.*`.

**Rationale**: Measured on this machine with CPython `re`:

```
'candidate'      \bcandid\b           -> False
'adjust'         \bjust\b             -> False
'candidly'       \bcandid\w*\b        -> True
'candidate'      \bcandid\w*\b        -> True      # violates FR-003
'candidate'      \b(?:candid|candidly)\b -> False
'candidly'       \b(?:candid|candidly)\b -> True
'a candid note'  \b(?:candid|candidly)\b -> True
```

A suffix wildcard turns the voucher stem `candid` into a hit on
`candidate`, which the spec names as a must-stay-silent case
(`spec.md:53`, `spec.md:103`). Enumerated alternation satisfies both
directions, and it is the mechanical projection of exactly what
Principle XI.3 and XI.5 enumerate, so the rule file and the constitution
cannot drift in vocabulary. Prohibiting wildcards in rule data at load
time makes the mistake structurally impossible rather than relying on a
reviewer noticing a pattern.

**Alternatives considered**: `re.ASCII` flags, `\b` with a lookaround
allow-list, and a token-set membership pass (split on non-word
characters, then set lookup). The lookaround form
`\bcandid(?!ate)\w*\b` does silence `candidate` (measured: `False`) while
still catching `candidly`, but it requires maintaining an exception list
per stem, which is more state than the enumeration it replaces. Set
membership is equivalent for pure vocabulary and slightly cheaper, and it
cannot express the multi-word phrase rules of XI.2 and XI.4, so a regex
engine stays necessary; the alternation was measured fast enough that a
second matching engine would be dead weight (R-04 performance note).

## R-04 Dash handling, and the whole-tree performance headroom

**Decision**: U+2014 is a finding wherever it appears in prose. U+2013 is
legal when both neighbors are digits or alphanumeric range endpoints, and
out of scope otherwise, per FR-002. ASCII `--` and `---` are findings in
prose, with the exemption machinery of R-06 covering commands, URLs, and
paths. Look-alikes outside the named set (U+2010, U+2212) are out of
scope until the rule file adds them, which is the spec's explicit
position (`spec.md:115`).

**Rationale**: Measured, the mixed edge case at `spec.md:104` counts
cleanly: a line reading `Ports 0–3 carry — in the example — traffic`
yields 2 em-dashes and 1 en-dash, so the en-dash allowance and the
em-dash ban coexist on one line with no special casing beyond the
neighbor test. Performance measured on this checkout: scanning 81 in-scope
files (30 Markdown, 51 source) with a compiled alternation over the full
banned vocabulary, line by line, took **0.062 s**. SC-003's bound is 10 s
on a developer machine, so the design has roughly two orders of magnitude
of headroom and no need for caching, parallelism, or an incremental index.
That is the X.2 argument against a fancier matcher.

Amendment (002 review): the in-scope file count moves with the tree. The
canonical command recorded in plan.md recomputes it to 60 files (15
Markdown, 45 source) on 2026-09-16. The 0.062 s wall-clock stands as the
SC-003 bound evidence: the scanned set is nowhere near the 10 s bound at
either count.

**Alternatives considered**: A Unicode category or `unicodedata.name`
test for connector dashes instead of the neighbor heuristic; rejected as
over-precise for a rule the constitution states in terms of numeric
ranges. Scanning with a single regex over whole files instead of lines;
rejected because findings must carry line numbers, and a per-line loop is
already 160 times inside budget.

## R-05 File discovery from `git ls-files`, with a vendored exclusions list

**Decision**: Enumerate in-scope files through `git ls-files`, filter by
extension to Markdown (`.md`) plus C, C++, CMake, and shell sources, keep
the surfaces FR-006 names (repository root Markdown, `.specify/memory/`,
`specs/`, `docs/`, comments in the named source kinds), and drop
everything matching an exclusions list held in the rule file. Initial
exclusions: `.opencode/`, `.specify/scripts/`, `.specify/templates/`,
`build/`, `docs/images/`. A file that fails UTF-8 decode, or that carries
a byte-order mark, is reported as skipped with a reason and never crashes
the run (`spec.md:111`). Comment extraction is per-language: `//` and
`/* */` for C and C++, `#` for CMake and shell.

**Rationale**: `git ls-files` gives the tracked set for free, which is
what makes CI and local verdicts identical (FR-019, US3 scenario 3)
without a second ignore format. `.codespellrc` already establishes both
habits: a `skip` list for vendored and generated paths, and a documented
note that Spec Kit's vendored shell scripts are out of scope
(`.codespellrc`, the `ignore-words-list` comment naming
`.specify/scripts/bash/common.sh`). Measured em-dash counts justify the
initial exclusions concretely: `.specify/scripts/bash/common.sh` carries
12 occurrences and the three `.opencode/commands/speckit.*.md` templates
carry 20, all of them vendored text this repository does not own and
cannot fix upstream.

**Alternatives considered**: A `.gitignore`-style prose-specific ignore
file (rejected: a second ignore dialect to learn, and git already owns
the tracked-file question); scanning the working tree with `os.walk`
(rejected: it would lint ignored and generated output, which FR-006
excludes); reusing `FORMAT_PATTERNS` from `cmake/lint.cmake` (rejected:
that list covers `source/`, `include/`, `test/`, `example/` C++ only,
`cmake/lint.cmake:10-16`, and holds no Markdown at all).

## R-06 One exemption marker, line-scoped, reason mandatory

**Decision**: `prose-lint: allow reason="..."` on the line it exempts.
The reason must be non-empty or the marker itself becomes a finding
(FR-005, `spec.md:55`). There is no block form, no `off` and `on` pair,
and no file-level exemption. Auto-exemptions for fenced blocks, inline
code spans, indented code, URLs, paths, and shell command lines are
implemented in the extractor, not expressed as markers (FR-004).

**Rationale**: The constitution requires a justification written at the
site for every suppression, which is Principle X.2's rule for casts and
`NOLINT`. Line scope plus mandatory reason matches that rule exactly and
keeps a suppression's blast radius to one line. Prior art splits on block
toggles, and the toggle is precisely the failure mode the repository's
anti-skip doctrine refuses: Vale's `<!-- vale off -->` disables a region
until a matching `on`, so a forgotten `on` silently stops checking the
rest of the file. `cmake/coverage.cmake:6-13` states the house position
in terms: a missing tool is a configure-time `FATAL_ERROR`, never a skip.
The marker text is valid inside every comment style in scope, so the same
string works in Markdown, C, shell, and CMake without per-language
syntax.

**Alternatives considered**: clang-tidy's `NOLINT` and
`NOLINTNEXTLINE`, adopted as the reason-in-the-same-comment habit,
rejected as a spelling because `NOLINT` carries a strong existing meaning
for C++ diagnostics; flake8 `# noqa: CODE`, which requires a code and so
matches the mandatory-reason shape but not the reason text; a file-level
`prose-lint: file-allow` for the vendored tree, rejected because the
exclusions list in R-05 already serves that need with less force.

**Consequence to record in the rule file**: Principle XI quotes its own
banned vocabulary (`constitution.md:361-388`), and the commit template
block quotes `genuinely trivial` (`constitution.md`, Pull Request
Quality). Those lines are exempt today through XI.6's literal clause and
through the fenced-block auto-exemption, implicitly. The rule file shall
carry them as documented auto-exempt constructs plus markers on the
self-quoting prose lines, so the governing document does not read as
failing its own gate. The spec notes the same requirement at
`spec.md:107`.

## R-07 Range resolution: full fetch, explicit base, fail loudly

**Decision**: The `prose-lint` CI job checks out with
`fetch-depth: 0`. Pull-request mode takes base from
`github.event.pull_request.base.sha` and head from
`github.event.pull_request.head.sha`, computes `git merge-base BASE HEAD`,
and diffs `BASE...HEAD`. Push mode on the default branch takes
`github.event.before` and `github.event.after`; when `before` is the
all-zeros SHA, unresolvable, or not an ancestor (force-push, new branch),
the job falls back to the commits the event enumerates, and if that is
unusable it exits 2 with a message. Local mode takes
`--range BASE..HEAD`; the default base is the merge-base with
`origin/master` when resolvable.

**Rationale**: The default checkout gives one commit, quoted from the
tool's own README (doc): "Number of commits to fetch. 0 indicates all
history for all branches and tags. Default: 1" and "Only a single commit
is fetched by default, for the ref/SHA that triggered the workflow. Set
`fetch-depth: 0` to fetch all history for all branches and tags."
Measured on this repository, a `--depth 1` clone of the local repo reports
`git rev-list --count HEAD` as 1 and fails `git merge-base HEAD HEAD~1`
with `fatal: Not a valid object name HEAD~1`. So the shallow default
cannot support a range check at all. Fail-loudly on an unresolvable range
is the same anti-skip posture as `coverage.cmake`, and it closes the
silently-vacuous-pass hole: an unresolved range must never be treated as
an empty range that passes.

**Alternatives considered**: `fetch-depth` with a fixed depth such as 50,
cheaper than a full fetch (rejected: PR ranges are unbounded in length
and a too-shallow fetch reproduces the vacuous pass in a way that only
appears occasionally); fetching only the base branch plus head with
`--deepen` loops (rejected: more moving parts than `fetch-depth: 0` for a
repository of this size, roughly 30 Markdown and 45 source files in
scope); trusting `github.event.pull_request.commits` as the sole source
(rejected as primary because it can lag a force-push, retained as the
push-mode fallback only); three-dot diff of refs instead of SHAs
(rejected: refs move during a review, SHAs do not, and the event payload
gives SHAs).

Amendment (002 review): commit enumeration is pinned to two-dot `git rev-list BASE..HEAD`; the prose diff stays three-dot `BASE...HEAD`.

## R-08 Added-line attribution from `git diff -U0`

**Decision**: Parse `git diff -U0 --no-color BASE...HEAD --` per file. For
each hunk header `@@ -a,b +c,d @@`, the `+` lines occupy new-file lines
`c` through `c+d-1`, with `d` omitted meaning 1. Report findings only on
those new line numbers. Suppress renames with `--no-renames` so a rename
still yields content lines, and never report a line present only on the
`-` side.

Amendment (002 review): the context flag is pinned to `-U1`. `-U0` emits no
context lines at all, which makes the `modified` state unreachable and
FR-007 unimplementable. With `-U1` the ±1 window supplies `modified` lines,
a line one below an added line can fire, and a line two below stays
grandfathered.

**Rationale**: Measured on a real commit of this repository
(`git diff -U0 56aa788 9db5c6e -- AGENTS.md`), the header shape and its
two traps both appear:

```
@@ -3,2 +3,2 @@
-Modern C++23 benchmarking framework (in the spirit of Google Benchmark, with
+C++23 benchmarking framework in the spirit of Google Benchmark, with
@@ -6,3 +6 @@ additional features). CMake build system, BSD 3-Clause license.
-## Mandatory First Reading
```

The second header shows a shrink from 3 old lines to 1 new line, so the
old and new counts differ and only the `+c,d` pair yields line numbers.
Its trailing text is context echoed after the second `@@`, which a naive
split on `@@` would read as content. Both are handled by anchoring the
parse on `^@@ -\d+(,\d+)? \+\d+(,\d+)? @@` and reading `+` lines
exclusively. A mode-only change or a pure rename produces no `+` lines
and therefore no findings, which is correct: nothing was written.

**Alternatives considered**: `git blame` per line to decide authorship
(rejected: O(lines) git invocations and it misattributes lines that moved
within a file); `git diff --word-diff` (rejected: word granularity does
not map to the line-level report FR-017 requires); checking the whole
file whenever any line of it changed (rejected: it breaks the
grandfathering clause of Principle XI.1, which US1 scenario 8 tests
directly).

## R-09 Commit enumeration includes merges, with the body rule relaxed for them

**Decision**: Enumerate every commit in the resolved range with
`git rev-list`, merges included, and check each against the template. A
merge commit is exempt from the body rule only; title, section, prose, and
footer rules still apply. An empty range is a pass (`spec.md:110`).

**Rationale**: The spec requires exactly that asymmetry
(`spec.md:112`). Measured on this repository, `master` contains 0 merge
commits and its history is linear, so `git log --no-merges` and
`git log --first-parent` currently return identical sets, and the
squash-merged landing commits appear in both:

```
9db5c6e Docs: Compress constitution and agent guide
56aa788 Meta: Ignore Python bytecode from DBC helpers
5756539 Docs: Add feature 002 prose and commit lint spec
```

That equivalence is incidental and will not survive a contributor branch
containing merges, which is why the gate enumerates by range and applies
the carve-out per commit type rather than filtering merges away. FR-016's
requirement to check a squash landing commit separately from the commits
it collapses follows from range enumeration: on the push to the default
branch the landing commit is the only commit in range, and the collapsed
commits were checked in the pull-request range.

**Alternatives considered**: `--first-parent` as the enumeration (rejected
for the contributor range case, where first-parent hides the commits that
need checking); `--no-merges` as the enumeration (rejected for the same
reason: it would let a merge commit inside a contributor range skip the
title and footer rules entirely); skipping the body rule for every
two-parent commit found on the default branch (rejected: too broad, and
the carve-out is about merge commits, not about branch position).

## R-10 Triviality of a body-less commit is a changed-line count

**Decision**: A commit may omit the body when its diff changes at most
`trivial_max_changed_lines` lines, default 5, counting added plus removed
lines from `git show --numstat` across all parents for non-merges. The
threshold lives in the rule file.

Amendment (002 review): the command is `git show --numstat -M`. Rename
detection keeps a pure rename from inflating the count, a binary file's
`-\t-\t` counts as 0, and a merge commit prints no numstat lines and so
sums to 0, deliberate given the R-09 body exemption for merges.

**Rationale**: The template omits the body only for a `genuinely trivial`
change, quoted from `constitution.md` under Pull Request Quality, and the
spec settled the
mechanical stand-in plus its default (`spec.md:20`, `spec.md:138`,
`spec.md:176`): 40 changed lines without a body fails, 3 passes
(`spec.md:75-76`). `--numstat` gives per-file added and removed counts in
one git call per commit, which is both cheap and stable to parse.

**Alternatives considered**: Byte-size of the diff (rejected: whitespace
refactors inflate it while real changes hide in it); file count (rejected:
one changed line in a generated header is not the same act as 40 lines in
three sources); an explicit `Skip-body:` footer in the template, which is
the spec's rejected alternative because it changes the commit template and
would need a constitution amendment.

## R-11 Imperative mood by reject list, section tokens as data seeded from history

**Decision**: Two data-driven rules. First, the section token before the
colon must be in `sections`; seeded from observed history. Second, a title
is rejected when its first word or a passive shape matches an entry in
`non_imperative_shapes`, and separately when the whole title matches
`vague_titles`. Both lists are editable data (FR-010, FR-011, FR-012).

Amendment (002 review): `sections` carries nine tokens, the eight observed
plus `runner`, a token the constitution's own example of section areas
names (`constitution.md:443`), per decision D4.

**Rationale**: Full grammatical detection is out of scope by spec
(`spec.md:21`, `spec.md:177`), and the observed failure shapes are few.
Measured section tokens across all history:

```
17 Docs       15 dbc      6 deploy   5 Constitution
 4 test        4 CMake     4 CI      3 Meta
```

Seed the list with those eight, keeping the case each uses: capitalized
`CMake`, `CI`, `Docs`, `Meta`, `Constitution`, lowercase `dbc`, `deploy`,
`test`. FR-010's stated workflow, a one-line data change in the pull
request that first uses a new section, works because the file is beside
the gate. Two historical titles exceed the 50-character limit and one has
no section at all; all predate the gate and history after merge is
immutable (`constitution.md`, Pull Request Quality, Immutability), so the
gate applies forward from its landing commit. That asymmetry is the
commit-mode equivalent of XI.1 grandfathering, and it is why commit mode
runs on ranges rather than on the whole repository.

**Alternatives considered**: A dependency on a POS tagger to detect
imperative mood (rejected: a new dependency plus non-deterministic
results on short titles); a whitelist of accepted leading verbs (rejected:
unbounded, and it would reject correct new verbs while the reject list
rejects the specific observed wrong ones); case-insensitive section
matching (rejected: `dbc` and `Docs` both appear, and case-insensitivity
would let `DBC:` and `docs:` enter the log, costing the history its
uniformity).

## R-12 Report format, exit codes, and CI annotations

**Decision**: Findings to stderr, one summary line to stdout, exit 0 for
clean, 1 for findings, 2 for usage or an unusable environment
(`tools/dbc/dbc_gate_common.py:10-14`). Line findings read
`path:line: RULE-ID family=XI.n 'token': message`. Commit findings read
`commit <short-hash>: RULE-ID: message`. When the environment sets
`GITHUB_ACTIONS=true`, each finding additionally emits a workflow command
`::error file=<path>,line=<n>::message` so it annotates the diff. The
summary reports units examined and findings raised, which is what US1
scenario 7 asks for.

**Rationale**: stderr findings plus stdout summary is the existing gate
convention (`tools/dbc/dbc_doc_gate.py:562-576`). Machine-parsability
(FR-017) follows from one finding per line with a fixed prefix. Annotations
are opt-in on the environment variable so local output stays a plain list
and the same binary produces the same text a developer sees.

**Alternatives considered**: JSON findings behind a `--format=json` flag
(rejected for now as unused by any consumer; the line form is parseable,
and the flag is a one-line addition when something needs it, which is X.2
rather than a gap); the GitHub PR review-comment API (rejected: it needs a
token and turns a lint into a write action against the pull request);
writing a matrix artifact as the DBC gates do (rejected: no downstream
consumer reads it here, so it would be noise).

## R-13 Fixtures run inside the gate, red-first, registered with CTest

**Decision**: `test/prose-gate-fixture/` holds prose fixture files (one
Markdown fixture, one each for C, shell, and CMake comments, and a
fixture rule file), and `run_prose_gate_fixtures.py` builds a throwaway
commit fixture repository in a temp directory so commit-mode assertions
never depend on this repository's immutable history. The harness asserts,
for every rule family, one case that must be reported and one that must
stay silent (FR-023, SC-001, SC-002, SC-004). Register it as
`add_test(NAME prose_gate_fixtures ...)` in `test/CMakeLists.txt`,
mirroring `test/CMakeLists.txt:90-99`, and run it in the CI job with
`ctest -R prose_gate_fixtures --output-on-failure --no-tests=error`.
Land the harness before the gate so the first commit is red, following the
convention recorded at `test/CMakeLists.txt:88-89`: while the DBC gate
scripts were missing, "the harness exits with 'gate scripts not present
(TDD red)'".

Amendment (002 review): the block being mirrored spans
`test/CMakeLists.txt:89-99`, and the convention sentence quoted above sits
at `test/CMakeLists.txt:88`.

**Rationale**: This is how the repository already proves a gate fails the
way it claims, rather than trusting the gate to be correct because it
runs. The dbc harness takes `--gate`, `--registry`, `--out`, and a fixture
directory (`test/CMakeLists.txt:91-99`); mirroring the shape keeps one
mental model. Building the commit fixtures in a temp repository is
necessary because the constitution makes merged history immutable, so
malformed fixture commits cannot be planted in the real history.

**Alternatives considered**: `unittest` with `python3 -m unittest
discover` (rejected as the sole mechanism: it would not run under
`ctest --preset=dev`, so the CI job and the developer loop would diverge,
costing US3 its parity claim. `unittest` is still used inside the harness
for the extractor's own unit tests); shell-based golden-file comparison
(rejected: verdict text changes would produce diffs instead of named
assertions, and FR-023 asks for both directions per family, which reads
better as explicit assertions).

## R-14 How DBC, coverage, and format gates apply to Python tooling

**Decision**: The 100% line, branch, and DBC coverage gates and the
doxygen contract requirements govern the C++ library and are unaffected,
because the feature adds no C++. The gate script follows the established
Python conventions of `tools/dbc/`: `from __future__ import annotations`,
type hints, `main(argv) -> int` with `sys.exit(main())`, argparse for
preconditions, and `die()` for usage failures. This is recorded as an
interpretation, not a violation.

**Rationale**: Principle II speaks of interfaces documented with doxygen
`\pre`, `\post`, `\invariant` and of a contract facility that emits no
code in release; its enforcement path is the DBC gate, which reads
doxygen XML against `tools/dbc/macros.yaml`, and the coverage gate, which
reads gcov data. Neither instrument has a Python input, and no existing
Python tool in the repository carries doxygen markup
(`tools/dbc/dbc_doc_gate.py`, `dbc_pair_gate.py`,
`test/dbc-gate-fixture/run_gate_fixtures.py`). No Python type checker or
linter is configured anywhere, confirmed by the absence of
`pyproject.toml`, `setup.cfg`, `mypy.ini`, `.flake8`, `.pylintrc`, and
`ruff.toml`. `cmake/lint.cmake:10-16` formats only `source/`, `include/`,
`test/`, and `example/` C++, so new `.py`, `.cmake`, and `.yaml` files
fall outside `format-check` while remaining inside the global
`spell-check`.

**Alternatives considered**: Introducing mypy or ruff for the new script
(rejected under X.2 and FR-021: a new dependency and a new gate the
constitution does not ask for, and enforcing types on one file while the
five existing gate scripts are unenforced is the kind of inconsistency
Principle X.3 forbids); writing the gate in C++ to bring it inside DBC and
coverage (rejected: a build cycle to lint text, and the repository's own
gate tooling is Python for the same reason); adding doxygen-style comments
to Python to imitate Principle II (rejected: content-free comments are
forbidden by Principle IV, and markup no tool reads is exactly that).

## Resolved open questions

Every item the spec recorded as a plan decision is now decided:

| Spec item | Resolution |
| - | - |
| Interpreter and libraries (Assumption, `spec.md:174`) | Python 3.12, the runner's distribution `python3` with apt `python3-yaml`; the job does not run `actions/setup-python`, which stays with the `lint` job at `ci.yml:19-20` (R-01, R-02, D1) |
| Rule data location and format | `tools/prose/prose_rules.yaml` (R-02) |
| Exemption marker spelling | `prose-lint: allow reason="..."`, line-scoped (R-06) |
| Section token list | nine tokens: eight from history plus `runner` (R-11, D4) |
| Triviality threshold | 5 changed lines via `--numstat` (R-10) |
| Range and diff mechanics | full fetch, `BASE...HEAD`, `-U0` added-line attribution, fail loudly (R-07, R-08) |
| Merge commits | enumerated, body rule exempt (R-09) |
| Fixture mechanism | harness plus `add_test(NAME prose_gate_fixtures)`, red-first (R-13) |
| Applicability of DBC and coverage to the tooling | interpretation recorded (R-14) |

No `NEEDS CLARIFICATION` items remain.

Amendment (002 review): the interpreter and section-token rows above carry
the owner's decisions from spec.md, superseding the R-01 and R-11 research
where the two differ. D1 sets the interpreter to the runner's distribution
`python3` with apt `python3-yaml`, and `actions/setup-python@v5` at
`ci.yml:19-20` stays with the `lint` job; D4 seeds nine tokens, `runner`
included (R-11 amendment).

## Known quantities for the tree-wide sweep, out of scope here

Whole-file mode against today's tree reports **225 em-dash occurrences in
12 in-scope files** after the R-05 exclusions: 209 of them in the
`specs/001-dbc-facility/` artifacts, 2 in this feature's own `spec.md`, and
4 inside C++ and shell comments (`include/speedgun-ng/dbc.hpp`,
`tools/dbc/overhead.cpp`, `tools/dbc/asm_smoke.sh`,
`tools/dbc/coverage_gate.sh`). The local command therefore defaults to
range mode, and whole-file mode stays opt-in until the formatting-only
sweep required by Principle V and `spec.md:175` lands. Making whole-file
mode the CI default before that sweep would fail every pull request for
text nobody touched.
