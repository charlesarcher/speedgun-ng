# speedgun-ng

C++23 benchmarking framework in the spirit of Google Benchmark, with
additional features. CMake build system, BSD 3-Clause license.

## Read first: the constitution

[`.specify/memory/constitution.md`](.specify/memory/constitution.md) is
the highest authority: standard-first coding with the P0–P3 priority model
(I), Design By Contract (II), R-DCUT (III), documentation (IV), style (V),
coverage (VI), performance discipline (VII), hard CI gates (VIII),
spec-driven development (IX), anti-slop code discipline (X), discourse and
prose standards (XI). Where this file, a spec, a plan, a task, a tooling
configuration, or a habit conflicts with it, the constitution wins.

Read it before any task, trivial ones included, and know which principles
constrain that task. Principle X and XI are canonical there and this file
restates nothing: read them before writing code, comments, commits, specs,
or replies, since XI binds chat replies too. A violation of either is a
defect at lint parity and blocks merge the way a clang-tidy finding does.

Order for any task: the constitution, then `specs/NNN-feature-name/` for
the feature at hand, then [README.md](README.md) for build, test, and
contribution mechanics.

## Spec-driven development (IX)

Spec Kit via `uvx --from specify-cli specify`: `/speckit.specify` →
`specs/NNN-name/spec.md`, `/speckit.plan` → `plan.md`, `/speckit.tasks` →
`tasks.md`, `/speckit.implement` → `include/` + `source/` and `test/`.
Optional: `clarify` (de-risk an ambiguous spec, before `plan` if used),
`analyze` (cross-artifact consistency), `checklist` (requirements
quality), `constitution` (amend), `taskstoissues`, `converge` (reconcile
artifacts with the codebase when resuming).

Bug fixes and trivial changes (typos, formatting, build fixes) may bypass
the workflow. Changes touching public API, behavior, or build
configuration may not.

## Build, test, verify

`cmake --preset=dev` then `cmake --build --preset=dev` then
`ctest --preset=dev`. `CMakeUserPresets.json` is machine-local and never
committed.

A finished change clears every hard gate in Principle VIII and keeps the CI
matrix green: Linux (clang-tidy, cppcheck), macOS, Windows, sanitizers,
coverage.

## Commits and merging

Template and quality rules live in the constitution (Pull Request
Quality): `<Section>: <one-line imperative>` at 50 characters or fewer, a
why-body wrapped at 72 columns (omitted only for trivial changes), an
`Approved-by:` footer, `Fixes #n`, `Refs: specs/NNN-name`. Commits are
atomic and bisectable; the base branch stays linear through rebase and
squash.

Merging into `master` is PR-only behind branch protection requiring an
approved pull request. The repository owner lands a ready, mergeable PR
with `gh pr merge <n> --squash --admin` (`admin_enforced: true`).

## Personalization: the digital twin (`~/wiki`)

`~/wiki` is a machine-local Karpathy-pattern LLM wiki holding the owner's
compiled knowledge and the digital-twin persona
([schema gist](https://gist.github.com/karpathy/442a6bf555914893e9891c11519de94f)).
It is outside this repository, never committed, never required to build,
test, or develop.

Load rule: `test -d ~/wiki` first. Absent (CI runner, fresh clone, other
machine) means skip this section. Present means load in order:
`aux/hermes/SOUL.md` (persona: identity, voice, posture, fleet context),
`AGENTS.md` (wiki constitution and schema: evidence over assertion, the
performance bar, OKR framing), `index.md` (entry to compiled knowledge,
follow links on demand), then `config/machines.md` and
`config/cli-digital-twin.md` when the task touches hardware or toolchain.

Precedence while personalized: this repository's constitution governs every
technical decision and the wiki relaxes no gate here; the wiki supplies
persona and reader context (voice, directness, evidence habits, hardware
facts, prior art); where the two disagree about this repository the
constitution wins (Governance), noted in one line, and work continues; wiki
content stays private, out of commit messages, pull requests, generated
documentation, and this tree.
