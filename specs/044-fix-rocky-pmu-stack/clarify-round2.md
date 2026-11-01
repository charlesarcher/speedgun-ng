# Clarify Round 2: specs/044-fix-rocky-pmu-stack

Verdict: NOT CONVERGED

The four round-1 content edits and the three minor edits stand in the spec.
The round-1 Principle XI prose fixes swapped one banned contrastive shape for
another banned shape, so three sentences still violate XI.2, and one round-1
flagged string survives. The edits below are the only remaining work.

## Verified as in the spec

- Base-commit rule and the PR #68 file exclusion: FR-013 carries `d454bcff`,
  the re-branch rule with the sha in the first commit body, and the
  `reconcile-fix-2026-10-30` exclusion.
- PR title and push protocol: user story 3, scenario 3 carries the title
  `Counters: Stop unmapping test stack`, the first push with `-u`, the
  `--force-with-lease` protocol with the last pushed sha, and the stop with a
  report on a failed lease.
- DBC contracts on the move operations: FR-001 requires the `\pre`/`\post`
  contracts with `SG_ENSURE`, the moved-from postcondition, and the green
  contract pair gate.
- Four named sites in two scenarios: FR-004 and user story 1, scenario 4 carry
  the critic's exact wording (the multiplex-window and raw-page sites in
  `multiplexWindowScenario`, the two pinned-read sites in
  `migratedThreadScenario`, one per contract build).
- SC-002 carries the 50-plus-50 split per test per preset.
- FR-009 names both load-tool options and requires the chosen tool beside the
  counts.
- FR-018 names the footer block `Refs: specs/044-fix-rocky-pmu-stack` then
  `Approved-by: OmO`.

## Verified against diagnosis-b2.md

- FR-009 records the 264-of-500 reproduction under full-cpu load and the
  `taskset -c 0` degeneracy (the affinity precondition asks for two cpus, so
  the setup yields no signal about the gate). Both match the report.
- FR-012 names the fix direction the report names: the reader-install seam
  (`installTestReader`), a counting reader over the provider's scripted
  window, and waits that end on the probe's own read count. The report's
  per-cpu attribution finding stands behind FR-012's "every probe cpu" clause.
- FR-012's "1,024 reads per cpu" is grounded: the probe's measurement pass
  takes `kPassReads = 1024` loop reads per cpu
  (`source/harness/jitter_probe.cpp:48-50`).
- No new ambiguity or contradiction appeared elsewhere in the document.

## Remaining edits (all Principle XI, exact)

The round-1 rewrite replaced `, not` with `rather than` or `instead of`. XI.2
bans all three shapes, and the prose gate's rule data (`XI2.CONTRASTIVE`)
flags all three. Each edit states both facts in their own words.

1. User story 2, Independent Test (line 37). Current text: "...the fix makes
   the gate wait end on an event the test controls `instead of` on a
   margin."
   Edit to: "...the fix makes the gate wait end on an event the test
   controls; a margin no longer ends the wait."
2. User story 3, Why this priority (line 51). Current text: "...the value is
   a mergeable, evidence-backed PR `rather than` a feature." Edit to: "...the
   value is a mergeable, evidence-backed PR; the work adds no feature."
3. Assumptions, last bullet (line 123). Current text: "...the loop's counts
   are evidence for the pull request body `rather than` a gate change." Edit
   to: "...the loop's counts are evidence for the pull request body; the
   loop changes no gate."
4. FR-009 (line 87). Current text: "...so that setup yields no signal about
   the gate". The string reads as the connective "so that" the round-1
   critic flagged. Edit to: "...so the setup yields no signal about the
   gate".

## Checked and exempt

- The em-dash in the Input quotation (line 9) sits inside the verbatim
  user description; XI.6 exempts it, and the gate's inline-code exemption
  covers the line.
- The `---` thematic breaks (lines 29, 45, 61) are Markdown structure the
  gate's `MD_STRUCTURAL_RE` exempts; they need no change.

Once edits 1 to 4 land, the spec converges; no other part needs a change.
