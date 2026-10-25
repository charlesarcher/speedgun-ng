# Handoff Records

**Surface**: every file under `specs/030-ci-speedup/results/`.
**Why**: the readings FR-051 to FR-056 demand for the pull request are
written here first, one short record per reading.

DAG nodes pass readings through these files, never through a prompt. A
downstream node opens the file its upstream node is named for, so a
re-run of an upstream node needs no prompt edit. Every rule below is a
check: a record that fails one is incomplete, and its node reports
blocked.

## Path

One file per node, at
`specs/030-ci-speedup/results/<phase>/<node>.md`.

- `<phase>` is `ph2` through `ph11`, one directory per task phase.
- `<node>` is the file the DAG `files` entry names for that node, in
  kebab case: node `finding_diff` writes `ph5/finding-diff.md`.
- One node writes one file. A re-run rewrites that file in place, and no
  node appends to a file another node owns.

## The four fields

Every record carries all four, each one named on its own line or in its
own table row:

| Field | Content |
| --- | --- |
| head sha | the commit the reading was taken at, from `git rev-parse HEAD` |
| run id | `github.run_id` of the run read, or `local` for a reading taken on a lane host |
| command | the one command line that produced the numbers, pasted verbatim |
| numbers | the values with their units, one per row, with the verdict they carry |

A record that cites a workflow run states the run id behind every row,
so a reader can re-read the same table from `gh`.

## The two gates

Before its node reports done, the author of a record runs both commands
from the repository root, and both print zero findings and exit 0:

```text
python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/030-ci-speedup/results
codespell specs/030-ci-speedup/results/
```

These records are repository files, and two gates read them: the spell
check the `lint` job runs over the whole tree with
`cmake -P cmake/spell.cmake`, and the prose gate the `prose-lint` job
runs over the lines a push adds (plan X.3). A hit inside a record fails
the run that carries it. The tree command above reads every record in
the directory, so it finds an older file's hit before a later push puts
that file in range.

The gate lists tracked files, so a record that is still unstaged goes
unseen and the summary line reads `0 sources`. Read that count: a pass
over zero sources says nothing about the record it was run for.
