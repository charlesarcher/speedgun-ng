# Lane Commits: specs/051-raw-events

The built head for every wave 7 read. The sha stands only as
copied from the command below, read at this write.

```
git rev-parse HEAD
a3c465ac71e73f5fd84c674bfdcadaa1164fea26
```

```
git log -1 --format=%s HEAD
Counters: Take raw event codes as leaves
```

Source, test, build and docs surfaces match this head at the
reads that follow it; later commits move only files under
specs/051-raw-events/.

## The built head after the pin carriers moved

The T024 suite leg at the first built head named two standing version
pins still reading 0.34.0, and T043 moved them. The built head for
the wave 7 reads from here on, copied from:

```
git rev-parse HEAD
b0cd97e2e23f605724d3d2593490df60a71a3cd8
```

The commit it names is the version pin move, and the commits between
the two heads touch specs/051-raw-events paths and those two test
files only.

## The built head after the coverage closure

The T044 closure moves source and tests, so the wave 7 built head
moves with it. The sha, copied from a fresh read:

```
git rev-parse HEAD
9033535eb95ed0d1065360ef4c6bd9cae5c8f287
```

That commit is the test-scenario landing; the transform commit stands
one below it, named in the T044 block of tasks.md.

## The final built head

The last two counters cases move the test file again, so the built
head moves once more. The sha, copied from a fresh read:

```
git rev-parse HEAD
2f56e9fc06055d10a0ed5857623c6830a64fe731
```

The named head for the remaining legs is that commit; the closure
commits above it are bb45fa52 (transforms), 9033535e (first scenario
batch) and this one (second batch).

## The built head after the stamp revert

The revert of the stamp assert moves source again, so the built head
moves once more. The sha, copied from a fresh read:

```
git rev-parse HEAD
da32efe9e2d22bbab900b633d8adfd20e6af9b4a
```

That commit is the named built head for the remaining legs. The
earlier sections above stand as history: 2f56e9fc and 9033535e each
named a head while it stood.

## The built head after the constant-case rename

The rename moves the test file again, so the built head moves. The
sha, copied from a fresh read:

```
git rev-parse HEAD
b77ebf2ab11e02c6411b8789453f548d148c4a6e
```

Named head for every remaining leg.

## The built head after the second constant-case rename

The harness unit tables move case, so the built head moves. The sha,
copied from a fresh read:

```
git rev-parse HEAD
b2763369963f761b5138cc97dd375876dd03eb0d
```

Named head for every remaining leg.

## The squash and the amend chain

The lanes thirty-nine commits, from the spec landing through the
pull-request body draft, collapse to one commit titled `Harness: Add
raw event-code counter columns` on top of `0fce6651`. Amends move a
commits identity and kill the old one, so the chain is named here in
full, each hop with the delta that the amend carried:

```
257e89f43cb211b8190d807a401ffd6489032f3e   pre-squash lane head (kept in /tmp/051-presquash.txt)
a82144f3419809759e519ffda64b8c360c7d4c2d   the squash
f91d6b6fa4a9e70161234c311cb4ebb42d8c673c   the T040 record amend (specs paths only)
c0a73e98e159026b97740c0950956ab891e4286f   the consumer-release fix: [[maybe_unused]] on
                                           rawStateRule in provider.cpp, the house pattern
                                           for contract-only helpers at internal.hpp:1248
9a0aa410b8d481d4857ae3d0804aac32b51c184e   the converge fold (specs paths only)
17891ef77ea16711f9bcfa5e86edc6ea563b104d   the record-integrity fold: identity chain,
                                           proof restatement, body table rows
```

The table is copied at a read moment and can itself lag one hop, since
the amend that carries it moves the identity it names; the live
identity always answers the fresh reads above.

The live identity always answers a fresh read:

```
git rev-parse HEAD
git ls-remote origin refs/heads/051-raw-events
```

Gate rows name the built head at which they ran; a records-only amend
moves the identity without moving any leg, and the tree-equality hop
that lets the rows carry is copied fresh between the named pair:

```
git diff --name-only c0a73e98e159026b97740c0950956ab891e4286f HEAD | grep -vc '^specs/051-raw-events/'   # answers 0
```

The two proofs at the squash itself, copied when each read was true:

```
git diff --name-only 257e89f43cb211b8190d807a401ffd6489032f3e HEAD | wc -l   # answered 0 at the squash identity
git diff --name-only b2763369963f761b5138cc97dd375876dd03eb0d HEAD | grep -vc '^specs/051-raw-events/'   # answered 0 at the squash identity
```

At the fold identity those two reads answer six paths and one file,
and the named file is `source/counters/linux_pmu/provider.cpp`, the
consumer-release fix hop; every other difference is a record path. The
`b2763369`-named gate rows stand superseded by the wave at
`c0a73e98`, which is the built head the current rows name.
