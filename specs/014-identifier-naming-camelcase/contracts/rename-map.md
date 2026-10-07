# Contract: rename map

FR-015 defines the map. This command does not fill it.

## Files

- `specs/014-identifier-naming-camelcase/rename-map.md`
- A copy under `docs/` that carries the same entries

`specs/007-counters-and-timers/citations-log.md` gains one entry that
points at the feature-directory map.

## Entry

Each entry has four fields:

- Old spelling.
- New spelling.
- Shipped header that declares the name.
- Kind (type, function, enumerator, variable, member, constant, macro,
  or tag).

A fifth mark records a detail-namespace name. A detail name counts.

## Inclusion

A shipped header is a header under `include/` that the package installs.
The map lists every changed name declared in such a header.

The map excludes:

- A test-only name.
- A file-local helper.
- A name that did not change.
- An exception-list spelling.

FR-013 still renames the excluded owned names. They stay out of the map.

## Proof search

The search covers `include/`, `source/`, `test/`, `example/`, `tools/`,
`docs/`, `.github/workflows/`, `README.md`, and `AGENTS.md`.

An old shipped-header spelling appears only in the map and the exception
list. A hit outside those two sites fails the search.

Closed spec directories stay out of the search, except the successor-log
entry and the logged wording correction in FR-020. Historical mentions
in other closed specs stay.
