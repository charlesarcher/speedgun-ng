# Data Model: Names and User Paths Out of Old Records (T-NAMES)

## Record line

One physical line of a tracked text file under `specs/` or `docs/`.
States: its file, its line number, its token multiset. The lane edits a
line only when a token match sits on it.

## Person-identity token

A token naming a person: the system user name inside a home path, the
repository login inside URLs, endpoints, slugs, expressions, or prose,
or a personal email address. Every in-scope occurrence maps to exactly
one replacement from the plan's decision table.

## Kept line

An in-scope line the ruling exempts, each with its reason in the spec.
The in-scope kept list is empty (D-6). Out-of-scope matches sit in the
FR-005 kept files (`test/prose-gate-fixture`, `.github/`,
`CMakeLists.txt`, `CODEOWNERS`, `LICENSE`): the 30 tree-wide matches
the lane reports and does not edit.

## Evidence identifier

A run id, pull request number, commit sha, job name, file path that
names no user, or measured figure on a record line. The lane preserves
the multiset of these tokens over the whole change (FR-004); the
identifier census of the test plan is the check.

## Rewriter

A throwaway Python program in `/tmp`. Inputs: a file list and the
decision table. Outputs: rewritten files plus a per-file count of
replacements. It holds no state between runs and enters no tracked
path.
