# speedgun-ng

Modern C++23 benchmarking framework (in the spirit of Google Benchmark, with
additional features). CMake build system, BSD 3-Clause license.

## Mandatory First Reading

**Read the project constitution before doing anything else.**

[`.specify/memory/constitution.md`](.specify/memory/constitution.md) is
the governing document of this project: coding standards (pinned C++ Core
Guidelines with the P0–P3 priority model), Design By Contract, the
R-DCUT design process, documentation and style rules, test and coverage
gates, performance discipline, anti-slop code discipline (Principle X),
discourse and prose standards (Principle XI), and the hard CI quality
gates.

It is the **highest authority** here. When this file, a spec, a plan, a
task, a tooling configuration, or a habit conflicts with the
constitution, the constitution wins. Do not begin a task, including
"trivial" ones, before knowing which principles constrain it.

Reading order for any task:

1. `.specify/memory/constitution.md` (always; this file summarizes it,
   the constitution defines it)
2. `specs/NNN-feature-name/` (the spec, plan, and tasks for the feature
   you are working on)
3. [README.md](README.md) (build, test, and contribution mechanics)

## Spec-Driven Development

This project is developed spec-driven using
[Spec Kit](https://github.com/github/spec-kit) (specify CLI v1.x, run on
demand via `uvx --from specify-cli specify`). The workflow is the
execution vehicle for the R-DCUT process (constitution principle III)
with this artifact mapping:

| R-DCUT stage | Command | Artifact |
| ------------ | ------- | -------- |
| Requirements (EARS + user stories) | `/speckit.specify <description>` | `specs/NNN-feature-name/spec.md` |
| Design (UML logical + physical views, test plan) | `/speckit.plan` | `specs/NNN-feature-name/plan.md` |
| Tasks (code task + covering test pairs) | `/speckit.tasks` | `specs/NNN-feature-name/tasks.md` |
| Code + unit tests | `/speckit.implement` | `include/` + `source/`, `test/` |

Optional quality commands: `/speckit.clarify` (de-risk ambiguous specs,
run before `/speckit.plan` if used), `/speckit.analyze` (cross-artifact
consistency), `/speckit.checklist` (requirements quality checks),
`/speckit.constitution` (amend the constitution),
`/speckit.taskstoissues` (convert tasks to GitHub issues),
`/speckit.converge` (reconcile spec/plan/tasks with the codebase when
resuming work later).

**Scope:** bug fixes and trivial changes (typos, formatting, build
fixes) may bypass the workflow. Changes touching public API, behavior,
or build configuration must not.

## Build, Test, Verify

Preset-driven CMake (see [README.md](README.md) for the developer-mode
setup; `CMakeUserPresets.json` is machine-local and must never be
committed).

```sh
cmake --preset=dev
cmake --build --preset=dev
ctest --preset=dev
```

Quality gates for any finished change (constitution principles VI and
VIII, all of them hard): build + `ctest --preset=dev` pass;
`format-check` and `spell-check` pass; sanitizers clean; clang-tidy and
cppcheck clean; 100% line, branch, and DBC coverage. The CI matrix
stays green: Linux (clang-tidy + cppcheck), macOS, Windows, sanitizers,
coverage.

See [README.md](README.md) for details.

## Anti-Slop Rules

Constitution Principles X (code) and XI (prose) define these; the lists
below are the operative reminder, and the constitution is the canonical
home. A violation is a defect at lint parity: it blocks merge the way a
clang-tidy finding does.

**Code discipline (Principle X):**

- Write assumptions down before coding them; when a requirement has two
  readings, record both and justify the one taken.
- Ship the minimum code. Single-caller abstractions, hooks for
  nonexistent use cases, extra overloads, and error handling around a
  contract that already rules the case out are prohibited.
- `const_cast`, `reinterpret_cast`, `(void)param`, and `NOLINT` each carry
  their justification at the site: they are P2 exceptions (Principle I).
- `DoNotOptimize` appears only where the compiler would erase measured
  work.
- Touch only the lines your change requires; match the local style; remove
  the orphans your change creates.
- Define a binary check before coding, and report the evidence (command,
  exit code, test name) when claiming completion.

**Prose discipline (Principle XI):** every generated word counts, chat
replies included.

- No em-dashes. Use a colon, semicolon, comma, or parentheses. The en-dash
  is for numeric ranges (`P0–P3`).
- No contrastive framing. Write what a thing is: "the runner dispatches
  executions onto a fixed worker pool", and drop the appended denial.
- No truth vouchers: "honestly", "frankly", "transparently", "genuinely".
- No self-describing artifacts: "in this section we", "this document will
  cover", "let me walk you through".
- No filler: "simply", "just", "very", "actually", "it's worth noting",
  "in order to".
- No marketing words in technical claims: "seamlessly", "robust",
  "elegant", "best-in-class". A performance claim carries a number, a
  platform, and a distribution (Principle VII).

## Personalization: the Digital Twin (`~/wiki`)

`~/wiki` is a machine-local Karpathy-pattern LLM wiki holding the owner's
compiled knowledge and the canonical digital-twin persona
([schema gist](https://gist.github.com/karpathy/442a6bf555914893e9891c11519de94f)).
It is not part of this repository, is never committed, and never needs to
exist for this project to build, test, or be developed.

**Load rule: test, then load, and only when present.** Run `test -d ~/wiki`
first. When the directory is absent (CI runner, fresh clone, another
machine), skip this section and continue. When it exists, load in order:

| Order | Path | Supplies |
| ----- | ---- | -------- |
| 1 | `~/wiki/aux/hermes/SOUL.md` | Canonical digital-twin soul: identity, voice, posture, fleet context |
| 2 | `~/wiki/AGENTS.md` | Wiki constitution and schema: evidence over assertion, the performance bar, OKR framing |
| 3 | `~/wiki/index.md` | Entry point to compiled knowledge; follow links on demand |
| 4 | `~/wiki/config/machines.md`, `~/wiki/config/cli-digital-twin.md` | Local hardware and toolchain reality, when the task touches either |

Precedence while personalized:

1. This repository's constitution governs every technical decision:
   language, contracts, coverage, gates, public API, prose rules. The wiki
   relaxes no gate here.
2. The wiki supplies persona and reader context: voice, directness,
   evidence habits, hardware facts, and prior art. It answers who is
   speaking and what they already know.
3. Where wiki guidance and this constitution disagree about this
   repository, the constitution wins (constitution §Governance). Note the
   conflict in one line and continue.
4. Wiki content is private context: it never enters a commit message, a
   pull request, or generated documentation, and it is never copied into
   this tree.

The twin adds context: voice, prior knowledge, and machine facts. What this
repository accepts stays exactly as the constitution sets it.

## Commits

Every commit follows the message template and quality rules in the
constitution (Pull Request Quality): a `<Section>: <one-line
description>` title (≤ 50 characters), a why-body (omitted only for
genuinely trivial changes), an `Approved-by:` footer, and issue/spec
refs. Commits are atomic and bisectable; the base branch stays linear
(rebase + squash, no gratuitous merge commits); vague or WIP messages
are never pushed; history after merge is immutable.

Merging into `master` is PR-only: its branch protection requires an
approved pull request from contributors. The maintainer (repository
owner) bypasses the review gate when landing a ready PR with
`gh pr merge <n> --squash --admin` (API: `admin_enforced: true`); use
it only when the change is complete and the PR is mergeable.
