# speedgun-ng

Modern C++20 benchmarking framework (in the spirit of Google Benchmark, with
additional features). CMake build system, BSD 3-Clause license.

## Mandatory First Reading

**Read the project constitution before doing anything else.**

[`.specify/memory/constitution.md`](.specify/memory/constitution.md) is
the governing document of this project: coding standards (pinned C++ Core
Guidelines with the P0–P3 priority model), Design By Contract, the
R-DCUT design process, documentation and style rules, test and coverage
gates, performance discipline, and the hard CI quality gates.

It is the **highest authority** here. When this file, a spec, a plan, a
task, a tooling configuration, or a habit conflicts with the
constitution, the constitution wins. Do not begin a task — including
"trivial" ones — before knowing which principles constrain it.

Reading order for any task:

1. `.specify/memory/constitution.md` — always (this file summarizes it,
   the constitution defines it)
2. `specs/NNN-feature-name/` — the spec, plan, and tasks for the feature
   you are working on
3. [HACKING.md](HACKING.md) / [BUILDING.md](BUILDING.md) /
   [CONTRIBUTING.md](CONTRIBUTING.md) — build, test, and contribution
   mechanics

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

Preset-driven CMake (see [HACKING.md](HACKING.md) for the developer-mode
setup; `CMakeUserPresets.json` is machine-local and must never be
committed).

```sh
cmake --preset=dev
cmake --build --preset=dev
ctest --preset=dev
```

Quality gates for any finished change (constitution principles VI and
VIII — all are hard): build + `ctest --preset=dev` pass;
`format-check` and `spell-check` pass; sanitizers clean; clang-tidy and
cppcheck clean; 100% line, branch, and DBC coverage. The CI matrix
stays green: Linux (clang-tidy + cppcheck), macOS, Windows, sanitizers,
coverage.

See [HACKING.md](HACKING.md), [BUILDING.md](BUILDING.md) and
[CONTRIBUTING.md](CONTRIBUTING.md) for details.
