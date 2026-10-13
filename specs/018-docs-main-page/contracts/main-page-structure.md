# Contract: Main-Page Structure

Normative shape of the rewritten root `README.md` and of the
quick-reference tables. FR-001 to FR-024, FR-038, SC-005, SC-009, SC-011.
Consumers: the rewrite task, the forbidden-token scan, the quick-reference
name scan, and `/speckit.analyze`.

## C-A: Part order and headings

| # | Part | Heading | Level |
| --- | --- | --- | --- |
| 1 | Opening | `What speedgun does` | H2 |
| 2 | Quick start | `Quick start` | H2 |
| 3 | Feature sections | `How you do it` | H2, one H3 per capability |
| 4 | Quick reference | `Quick reference` | H2, two tables |
| 5 | Planned work | `Planned work` | H2 |
| 6 | Tail | none | one build-page link, then the one-sentence name note |

Rules:

1. The first screen answers three questions unaided: what the library is,
   where it runs, where the numbers come from (FR-002, SC-001).
2. The quick start states the platform, the C++23 requirement, and the
   CMake minimum, and names no compiler version (FR-003; clarify session).
3. The link line names `speedgun-ng::speedgun-ng` and `speedgun-ng::harness`
   (FR-004).
4. The first benchmark program block is a complete program: includes, body,
   registration, `main` (FR-005).
5. The quick start's command blocks (configure, build, install, the
   consumer configure and build, the run command) and its `find_package`
   block are marked blocks too (FR-031 binds every code block): their
   regions live in `example/quickstart.sh` and
   `example/quickstart/CMakeLists.txt` (contract `example-drift.md`
   R-1 rule 6).
6. Every feature section keeps the H3 level and the three parts in order:
   purpose sentence, snippet, result sentences (FR-010).
7. Every API name appears in code font with its header spelling (FR-012).
8. The page teaches no scaffolding macro (FR-016).
9. The opening part states all six FR-002 facts, in the reading of
   research D-10: the language standard, the supported platform, the
   Google Benchmark lineage, the counters library as the source of every
   time and counter value, hardware counter metrics that attach to a
   benchmark, and contract checks that guard the API. Each fact is
   checkable text on the first screen (FR-002).

## C-B: Forbidden-token scan

The scan reads main-page prose text only: fenced block bodies and
inline code spans are exempt (the `tools/prose/prose_gate.py` exemption
mechanism), because the page's required program, CMake, and command
blocks legitimately contain header spellings and option strings.
A scan of the main page fails on any token in these classes (FR-038,
SC-005): commit hash (7 to 40 hex characters naming a commit), kernel ref,
pinned version string, compiler version, spec identifier of the form
`NNN-kebab-name`, requirement identifier `FR-nnn`, success-criterion
identifier `SC-nnn`, task identifier `Tnnn`, owner directive, constitution
principle number, CI job name from the twelve names in
`.github/workflows/ci.yml`, re-pinning step, and any source path: a
prose path naming a file under `include/`, `source/`, `test/`, `tools/`,
`external/`, or a build file such as `CMakeLists.txt`. Paths under
`example/` are exempt. Relative documentation links
(`docs/pages/...`) are not source paths: the build-page link of C-G is
legal prose, pinned by the documentation-link fixture case in
`example-drift.md` R-8.

The scan reads source text. Example program paths are the only code
paths the page may carry.

## C-C: Quick-reference API table

Column shape: `| Name | What it does | Teaches |` where Teaches links the
feature section. The row set is closed to the harness surface (clarify
session 2026-10-13):

- Macros: `SG_BENCHMARK`, `SG_BENCHMARK_CAPTURE`, `SG_BENCHMARK_TEMPLATE`,
  `SG_BENCHMARK_F`, `SG_BENCHMARK_DEFINE_F`, `SG_BENCHMARK_REGISTER_F`,
  `SG_BENCHMARK_TEMPLATE_F`, `SG_BENCHMARK_TEMPLATE_DEFINE_F`,
  `SG_BENCHMARK_TEMPLATE_METHOD_F`, `SG_BENCHMARK_TEMPLATE_INSTANTIATE_F`.
- Types: `State`, `BenchmarkHandle`, `Fixture`, `ScopedPauseTiming`,
  `BenchmarkResult`.
- Functions: `setDefaultTimeUnit`, `speedgunMain`, `doNotOptimize`,
  `clobberMemory`.

Names of the counters facility and of the DBC facility stay out. Names the
build or the headers declare as scaffolding (`SG_BENCHMARK_REGISTRAR` and
the family listed in PC-4) stay out.

Name scan (SC-009): every row name appears in
`include/speedgun-ng/benchmark.hpp` or `include/speedgun-ng/barrier.hpp`
at the audit point. The scan is part of the drift gate tool (see
`example-drift.md`, rule R-7).

## C-D: Option table

Column shape: `| Option | What it does |`. The row set equals the
long-option table of `speedgunMain` in `source/harness/cli.cpp` at the
audit point, nine rows: `--filter`, `--list`, `--repetitions`,
`--min-time`, `--iterations`, `--warmup-time`, `--dry-run`, `--counter`,
`--catalog`. The prose states the accepted form (long form; the short
letters are inert because the short-option string is empty) and that a
usage error reports one line and exits with status 2 (FR-014).

## C-E: Planned-work list

One short list, by name, with no spec number and no date (FR-021): user
counters on the state, bytes and items throughput, labels, custom
statistics, aggregate-only report output, complexity fitting, structured
machine-readable output, histogram output.

## C-F: Budget and prose

1. SC-011's "about 400 lines" (SC-011) lands as the binary cap 400
   total lines including the quick reference (research D-14); the
   acceptance check is an executable `wc -l` assertion.
2. SC-011's "about 25 lines" per feature section (SC-011) lands as the
   binary cap 25 lines counting from the H3 heading through the line
   before the next heading (research D-14); the acceptance check is the
   section pipeline of quickstart scenario 10.
3. The prose gate reports zero findings on the page and on every file the
   feature changes (FR-040, SC-010). Code fences, inline code spans, and
   blockquote lines are auto-exempt by `tools/prose/prose_gate.py`, so
   fence-info tokens and API spellings cost no findings.

## C-G: Build-page route

The main page links the build page exactly once, near the end (FR-024).
The link target is `docs/pages/building.md` (spec decision Q-3). The link
is relative so it resolves on GitHub and inside the Doxygen output. The
forbidden-path class of C-B excludes documentation links; the R-8
fixture case pins that reading.
