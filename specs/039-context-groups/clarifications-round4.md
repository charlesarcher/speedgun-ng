# Spec 039 clarification questions, round 4

Feature: `039-context-groups`, branch `039-context-groups`. Method:
the speckit-clarify discipline, a fourth pass over the spec the three
earlier rounds amended, no spec edit. Round 1's five answers, round 2's
two and round 3's one stand in the spec's Clarifications section, carry
the weight of a Decision, and stand unasked here. Each of the three
round files carries its Reconciliation record, and the readings they
settled stand closed: the slots-only grammar with the key `cpus`
refused, the fixture pair on context 0 with a publication that crosses
no round and a teardown after the last settled body, `kNoGroup` naming
no partition, the pairing refusal beside `--hunt` and beside a resolved
preset that names a partition, a preset above `none` that names no
partition standing allowed beside the file, the machine line keeping
its one standing rule, a group name carrying the plan's own unit value,
and no group spanning a round. The further candidates rounds 1, 2 and 3
dropped stay dropped and return nowhere here.

The bar this pass holds: a question names a split that moves scope, a
user-visible byte, the public surface, or the shape of a required test.
It stands checked against the tree with a `path:line` citation, and no
clause of the spec, the plan, the contracts, the constitution or a
ruling R-1 to R-5 answers it. The pass read the spec, `plan.md`,
`tasks.md`, `data-model.md`, both files under `contracts/` and the three
earlier round files, then re-read the branch's tree, whose sources
stand unchanged from `bea6af5b`, on the topology walk, the resolver,
the option table and its test twin, the fit walk, the attempt body and
the disclosure printer. Three passes covered the surface, and this one
reads no split that clears the bar: the spec stands, and the round files
no question.

## Considered and dropped

- A `groups` key whose value reads as a mapping or a scalar, the shape
  FR-012's sequence clause breaks with no fault of its own in FR-018's
  list: FR-017 bars nesting and every key outside the grammar
  (`specs/039-context-groups/spec.md:655-656`), A-2 of
  `specs/039-context-groups/contracts/group-layout.md:12-13` binds the
  reader to that grammar and to no fault beyond it, and the row at
  `specs/039-context-groups/contracts/group-layout.md:58` carries the
  one line naming the path, the entry and the key. The refusal set
  SC-004 counts stays at 16 faults of FR-018 with the two pairings
  FR-019 names (`specs/039-context-groups/spec.md:815-817`).

- The two cpu readings the spec still carries after round 1: the Layout
  file entity names "a name and either slots or cpus"
  (`specs/039-context-groups/spec.md:789-790`) and D-8 states a fit
  refusal "when a cpu entry names no cpu of that instance's plan"
  (`specs/039-context-groups/spec.md:304-305`). D-6 keeps the grammar
  slots-only (`specs/039-context-groups/spec.md:283-293`), FR-014
  refuses the key at the grammar
  (`specs/039-context-groups/spec.md:643-646`), and the Edge Cases row
  seats that refusal "before the fit step reads the file"
  (`specs/039-context-groups/spec.md:549-550`), so FR-020's slot-count
  clause (`specs/039-context-groups/spec.md:682-685`) stands the fit
  step's whole rule. The two lines read as residue of the reading
  round 1 closed, and no refusal count moves with them.

- Where the new option row stands in the table, which is the order
  `--help` prints (`source/harness/options.cpp:971-977`): the table
  test reads a row's block from its position against `kRowsBeforeHunt`
  (`test/source/harness_cli_table_test.cpp:254`,
  `test/source/harness_cli_table_test.cpp:880-886`), so a row carrying
  the constant `299` stands in the lane block beside `jitter-probe`
  (`source/harness/options.cpp:635`) and `os-noise`
  (`source/harness/options.cpp:641`) and ahead of `hunt`
  (`source/harness/options.cpp:655`).
  `specs/039-context-groups/tasks.md:132-137` names the checks that
  move with the row, and the guide-page check pairs a page row with a
  table row by name, so the page row's place reads free
  (`test/source/harness_cli_table_test.cpp:1516-1534`,
  `specs/039-context-groups/tasks.md:160-164`).

- The count word in the plan's reader test plan, "the fifteen fault
  shapes" (`specs/039-context-groups/plan.md:218-219`): T007 binds the
  test's rows to the fault table of
  `specs/039-context-groups/contracts/group-layout.md:43-60`
  (`specs/039-context-groups/tasks.md:89-96`), so the row list reads
  off the contract and the word carries no decision.

- A layout entry whose `name` spells `l3-0`, the text a preset group
  takes: FR-015 admits those characters and that length
  (`specs/039-context-groups/spec.md:647-650`), D-4 keeps one name
  unique within the run (`specs/039-context-groups/spec.md:273-277`),
  and FR-003 fixes one source per run
  (`specs/039-context-groups/spec.md:595-596`), so the name states the
  group while FR-028's machine line states whether a preset planned
  the run (`specs/039-context-groups/spec.md:720-726`).

- `--interleave` beside a layout file: the option permutes the walk
  order of the selected instances (`source/harness/cli.cpp:1162-1166`)
  and touches no instance's context set, and D-11 with FR-034 keep the
  groups on the instance's threaded run alone
  (`specs/039-context-groups/spec.md:317-320`,
  `specs/039-context-groups/spec.md:748-750`).

- A threaded body that skips its measurements under a partition: the
  callable returns through the standing path, the body tops up its
  window and arrives at its own group's stop round as every other body
  does (`source/harness/runner.cpp:2136-2149`), and FR-024 keeps the
  standing round rules over the group rounds
  (`specs/039-context-groups/spec.md:699-703`).

- One family whose instances resolve to different presets beside one
  layout file: FR-019 reads each selected instance's resolved preset,
  so the instance that names a partition earns the refusal and the
  others stand allowed
  (`specs/039-context-groups/spec.md:674-677`), the case round 2's Q1
  and round 3's ruling settled together.
