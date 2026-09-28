# citations: corrected anchors and superseded journal sentences

Created by T259 and T260 on 2026-09-28 for the
`007-counters-and-timers` feature. It is the record T253's debt
paragraph names and that no artifact carried. The closed task lines and
the dated phase preambles in `specs/007-counters-and-timers/tasks.md`
keep their bytes, and the corrections a reader needs to reach the code
from a closed task body live here. Three families are recorded.
The first is a `file:line` anchor that no longer lands on the text the
sentence naming it describes. The second is a sentence of the closed
journal `specs/007-counters-and-timers/sg_counters.md` that a later
requirement withdrew. The third is a clause of a closed task line that
a later decision amended, where the amendment landed in the code or in
a live artifact and the closed line kept its bytes.

## Precedence over the closed journal

Where the closed journal's resolved scope statement names a boundary, a
gate, a mechanism, or a manifest that a later requirement in
`specs/007-counters-and-timers/spec.md` withdrew, the requirement in
`spec.md` governs. The five sentences that rule settles are listed under
the second family below. T259 places this rule in the Assumptions
paragraph at `specs/007-counters-and-timers/spec.md:321`, in one
sentence beside the existing sentence that names the journal the
authoritative design record. T264 wrote that sentence into the bullet on
that line, so the rule now stands in the spec itself as well as here.
T264 appended it to the bullet, which keeps every anchor into `spec.md`
at or below line 321 unchanged.

## Method and population

The counting rule every anchor total in this feature's records depends
on, so a reader can reproduce any of them: the unit is one occurrence
of the regular expression
`(?:[\w./-]+/)*[\w.-]+\.[A-Za-z0-9]+:\d+(-\d+)?` under CPython 3.14.7
`re.finditer`, each match taken whole as `m.group(0)`. A distinct token
is the deduplicated form of that unit, so a token written on two lines
counts twice as occurrences and once as a distinct token. A bare
continuation is one occurrence of `(?<![\w./-]):\d+\b` under the same
engine and basis. T265 added this paragraph, because before it the
record named no rule and its figures reproduced under none.

The population is every anchor the Phase 1 through Phase 33 record in
`specs/007-counters-and-timers/tasks.md` places, which is lines 1
through 2422 of that file, the line before the Phase 34 heading: 702
occurrences and 467 distinct tokens of the path form, plus 420 bare
`:NN` continuations. Every token is attributed to the file it belongs to
by hand, because a line naming two files attaches a continuation to the
wrong one under an automated pass and a short path form matches several
tracked files.

Two other readings of the same rule are in circulation, and the
difference between them is the Phase 34 section. Lines 1 through 2578
of the file, the whole file as it stood when that section was
appended, carry 724 occurrences, 476 distinct tokens, and 449
continuations, so the Phase 34 section contributes 22 occurrences, 9
distinct tokens, and 29 continuations. The nine live artifacts named
below, `spec.md`, `plan.md`, `research.md`, `data-model.md`,
`quickstart.md`, `sg_counters.md`, and the three files under
`contracts/`, carry 40 occurrences over their whole length at
`f1d3023`, distributed `spec.md` 28, `quickstart.md` 4, `plan.md` 3,
`research.md` 3, `contracts/system-contract.md` 2, and none in
`data-model.md`, `sg_counters.md`, `contracts/provider-contract.md`, or
`contracts/measurement-contract.md`. At `30f6361`, the head the Phase 34
and Phase 35 sweeps ran at, the same rule yields 38 with `research.md` at
1, and `f1d3023` added the two anchors
`source/counters/detail/core.hpp:92-99` and
`source/counters/plan.cpp:593` to
`specs/007-counters-and-timers/research.md:43` and `:45`. The 740 at
`specs/007-counters-and-timers/tasks.md:2530` is that 38 beside the 702
of lines 1 through 2422, and the 762 at `:2536` is the same 38 beside
the 724 of lines 1 through 2578.

The distinct-token figures the dated preambles carry reproduce under the
rule stated above, one figure per population. The 467 for lines 1 through
2422, which the Phase 34 preamble at
`specs/007-counters-and-timers/tasks.md:2872` and the Phase 35 preamble
at `:2642` carry, the 476 for lines 1 through 2578, which the Phase 35
preamble at `:2644` carries, and the 518 for lines 1 through 2905, which
the Phase 36 preamble at `:2964` carries, are each the deduplicated form
of the unit that rule names. A deduplication of the parenthesised group
`(-\d+)?` alone yields 163, 168, and 183 for the same three populations,
and those figures count distinct line-range suffixes, which hold no path
and no line number. The occurrence totals and the bare-continuation
totals those preambles carry reproduce exactly, at 702, 724, and 776
occurrences and 420, 449, and 472 continuations.

Three criteria decide drift, and every row below was read against the
tree as it stands on 2026-09-28:

1. The cited line lies beyond the last line of the file.
2. The cited line is blank.
3. The cited line holds text other than the claim the sentence makes.

The first two criteria are mechanical and complete over the population.
The third is found by reading the landing, and the candidates it was
applied to are the anchors whose surrounding sentence names a
requirement key, a task key, or an identifier. An anchor quoted inside a
finding as the subject of that finding is outside the population,
because the sentence reports the anchor as wrong. A sentence describing
a past revision's line numbering is outside it for the same reason.
Rows numbered T253 already named are marked in the `Named by` column;
the remaining rows were found by this pass.

## Corrected anchors in closed task lines and phase preambles

| Site | Line in `tasks.md` | Anchor as written | Line now holding the claim | Governing | Named by |
| --- | --- | --- | --- | --- | --- |
| T072 | 357 | `include/speedgun-ng/counters_measurement.hpp:610` | `:634` | Constitution X.2 | T253, T260 |
| T072 | 357 | `include/speedgun-ng/counters_core.hpp:216` and `:234` | the header carries no `NOLINTNEXTLINE`; `:216` is a comment terminator and `:234` the `metric_result` declaration | Constitution X.2 | this pass |
| T072 | 357 | `include/speedgun-ng/counters_provider.hpp:37`, `:62`, `:125`, `:135`, `:150` | the header carries no `NOLINTNEXTLINE`; `:135` is blank and the rest are comment lines | Constitution X.2 | this pass |
| T076 | 364 | `source/counters/linux_pmu/provider.cpp:193` | `:505`, where the probe stamps the mode onto every countable entry | FR-023 | this pass |
| T076 | 364 | `source/counters/clock_provider.cpp:234` | `:232`, where the scaling flag is set | FR-034 | this pass |
| T076 | 364 | `source/counters/linux_pmu/fast_read.cpp:91-92` and `:120-121` | the mirrored page's `version` and `compat_version` fields were removed with the mirror; `fast_context_read` stands at `:255-286` | FR-040 | this pass |
| T077 | 365 | `source/counters/linux_pmu/fast_read.cpp:283` | the hoist is gone with commit `2889608`: `:281` issues the instruction under the index test at `:278`, and the capability gate runs at `:86-88` inside `fast_decode`, which `:284` calls | FR-040 | this pass |
| T077 | 365 | `source/counters/linux_pmu/fast_read.cpp:40-42` and `source/counters/detail/pmu.hpp:165-166` | `:41-43` holds the `fast_index_valid` comment and `pmu.hpp:166-168` the `pmu_device` comment; the protocol order is stated at `fast_read.cpp:82-85` and `pmu.hpp:249-254` | FR-040 | this pass |
| T078 | 366 | `source/counters/linux_pmu/fast_read.cpp:122` | `:267`, where the lock snapshot is read | FR-040 | this pass |
| T078 | 366 | `source/counters/linux_pmu/fast_read.cpp:271` and `:287` | the index compare stands at `:278-281`; `:271` holds the width and `:287` is blank | FR-040 | this pass |
| T079 | 367 | `specs/007-counters-and-timers/plan.md:43` and `:365` | the P2 cast row was withdrawn with the mirror; the protocol stands at `plan.md:363-365` | Constitution I, P2 | this pass |
| T079 | 367 | `source/counters/linux_pmu/fast_read.cpp:191` and `:264-265` | both hold comment text; the page fields are read at `:269-272` | FR-040 | this pass |
| T080 | 368 | `specs/007-counters-and-timers/quickstart.md:108`, named twice | `:109`, where the binary pass check is stated | SC-004 | T253 |
| T080 | 368 | `specs/007-counters-and-timers/spec.md:301` | `:311`, which carries SC-004 | SC-004 | this pass |
| T080 | 368 | `docs/pages/counters-overhead.md:63-94` | the measurement tables stand at `:158-173` | SC-004 | this pass |
| T100 | 391 | `include/speedgun-ng/counters_measurement.hpp:264` and `:317` | `:275` and `:328` | FR-035 | T253 |
| T100 | 391 | `source/counters/push_provider.cpp:31` | `:45`, the plain load that never performs an atomic read-modify-write | FR-035 | T253 |
| T101 | 392 | `source/counters/system.cpp:377` | `:163`, where the seed's flag reaches the catalog entry | US4 scenario 5 | this pass |
| T101 | 392 | `source/counters/clock_provider.cpp:236` | `:232` | FR-034 | this pass |
| T120 | 414 | `specs/007-counters-and-timers/spec.md:276` | `:286`, which carries FR-049 | FR-049 | T253, T260 |
| T126 | 420 | `specs/007-counters-and-timers/spec.md:247` | `:257`, which carries FR-029 | FR-029 | T253, T260 |
| T129 | 423 | `source/counters/detail/core.hpp:98` | `:105`, where `bound_thread` is declared | FR-031 | this pass |
| T129 | 423 | `source/counters/plan.cpp:301`, `:326`, `:340` | the one surviving query stands at `:61`; the other two lines are blank | FR-031 | this pass |
| T131 | 447 | `source/counters/linux_pmu/fast_read.cpp:346` and `:377` | both lie beyond the file's 331 lines; the shift band and the sysfs read were removed, and the width and capability are read at `:271-272` | FR-040 | this pass |
| T139 | 451 | `source/counters/linux_pmu/fast_read.cpp:309-444` | `fast_context_open` stands at `:203` and `fast_context_read` at `:255` | FR-040 | this pass |
| T142 | 487 | `test/source/counters_fake_test.cpp:856-859` | `:504-512`, the scalar-multiply check | FR-015 | this pass |
| T145 | 493 | `source/counters/system.cpp:334` and `:348` | `:344` and `:350` | FR-001, US3 scenario 6 | this pass |
| T147 | 495 | `specs/007-counters-and-timers/spec.md:249` | `:252`, which carries FR-024 | FR-024 | T253, T260 |
| T152 | 503 | `test/source/counters_fake_test.cpp:710-740` | `:186-189` names the multiplex-ratio product scenario, whose fixture follows in the same function | FR-019 | this pass |
| T156 | 507 | `test/source/counters_linux_pmu_seam_test.cpp:352-355` | `:350-352` | FR-040 | this pass |
| T156 | 507 | `test/source/counters_linux_pmu_seam_test.cpp:381-382` and `:394` | `:384` and `:394-397`; the count comparison no longer stands there | FR-038 | this pass |
| T156 | 507 | `test/source/counters_linux_pmu_seam_test.cpp:976-977` | `:974-979`, the group-read monotonicity check | FR-040 | this pass |
| T166 | 581 | `source/counters/plan.cpp:409` | `:417`, the zero-leaf refusal, beside `:89` and `:536` | FR-046 | T260 |
| T170 | 588 | `docs/pages/counters-overhead.md:127` | the correctness-build group maximum stands at `:171` | FR-048 | T253 |
| T171 | 589 | `source/counters/fold.cpp:285` | `:289`, the `SG_REQUIRE` refusing a window that is not closed | FR-046 | this pass |
| T171 | 589 | `include/speedgun-ng/counters_measurement.hpp:1085` and `:1089-1095` | the target-taking declaration stands at `:1098-1100` and its brief starts at `:1088` | FR-021, FR-022 | T253 |
| T171 | 589 | `source/counters/clock_provider.cpp:206-235` | the constructor spans `:206-236` | FR-034 | this pass |
| T171 | 589 | `docs/pages/counters-overhead.md:219` and `:231-232` | `:321` carries the level-2 row and `:333-336` the level-3 refusal | SC-002 | this pass |
| T178 | 673 | `docs/pages/counters-overhead.md:301-304` and `:315-316` | `:319-322` carries the privilege table with its level-2 row at `:321`, and `:333-336` carries the level-3 refusal | SC-002 | this pass |
| T179 | 674 | `include/speedgun-ng/counters_measurement.hpp:1089-1095` | `:1098-1100` | FR-021, FR-022 | T253 |
| T185 | 880 | `docs/pages/counters-overhead.md:314` and `:326-327` | `:321` and `:333-336` | SC-002 | this pass |
| T189 | 976 | `specs/007-counters-and-timers/tasks.md:770` and `:829` | `:773` carries T184's body and `:831-832` carries the Phase 18 preamble sentence about its citation | Constitution IX | T253 |
| Phase 20 preamble | 1034 | `docs/pages/counters-overhead.md:34` | the line is blank; the release rows stand at `:160-161` | SC-004 | T253 |
| Phase 20 preamble | 1037 | `include/speedgun-ng/counters_measurement.hpp:1089-1095` | `:1098-1100` | FR-022 | T253, T260 |
| Phase 20 preamble | 1042 | `docs/pages/counters-overhead.md:314` and `:326-327` | `:321` and `:333-336` | SC-002 | T253 |
| Phase 21 preamble | 1115 | `include/speedgun-ng/counters_measurement.hpp:1089-1095` | `:1098-1100` | FR-022 | T253, T260 |
| Phase 22 preamble | 1178 | `source/counters/linux_pmu/fast_read.cpp:309-444` | `:203` and `:255` | FR-040 | T260 |
| Phase 22 preamble | 1180 | `.specify/memory/constitution.md:568-570` | the range already holds the machine-local `CMakeUserPresets.json` sentence at `:569-570`, so the anchor needs no correction; T263 recorded that the row named `:571-573` as the landing, and those three lines carry the Licensing bullet and one empty line | Constitution IX | T263 |
| T223 | 1805 | `specs/007-counters-and-timers/contracts/measurement-contract.md:107` | `:110`, the single-target sentence | FR-024 | T253 |
| T091 | 379 | `source/counters/detail/core.hpp:76-109` and `source/counters/fold.cpp:42` and `:165` | `plan_impl` spans `:76-116`; the two re-resolutions are the `by_address.at` at `source/counters/fold.cpp:43` and the `by_address.end()` guard at `:162` | FR-021, FR-022 | T262 |
| T195 | 1066 | `specs/007-counters-and-timers/plan.md:298` | the line is blank; the Files-and-duties row naming nine public headers stands at `:301`; at `30f6361`, the parent of the one-line insert `1827d76` made, the cited line held the table header and the row stood at `:300`, two lines low, so the third criterion at `:93` applied there and the second at `:92` applies at this head | plan: Files and their duties | T276 |
| T195 | 1066 | `specs/007-counters-and-timers/plan.md:325` | the line is blank; the sentence naming the nine public headers and listing them stands at `:328`; at `30f6361` the cited line held the `### Public API surface added` heading and the sentence stood at `:327`, two lines low, and `1827d76` added the one line at `plan.md:73` that accounts for the third | plan: Public API surface added | T276 |
| Phase 23 preamble | 1298-1299 | `plan.md:298` and `:325` | both cited lines are blank, the two landings `T195` names | plan: Files and their duties | T276 |
| Phase 23 preamble | 1312 | `plan.md:147-164` | the range's first line is blank, the block heading stands at `:148`, and the eleven sources span `:149-164` | Constitution IX | T276 |
| T231 | 1828 | `plan.md:147-164` | the range's first line is blank and the eleven sources span `:149-164` | Constitution IX | T276 |
| T236 | 1968 | `plan.md:282` | the line is blank; the scope-misuse sentence stands at `:283` | FR-046 | T276 |
| T255 | 2355 | `plan.md:282` | the line is blank; the sentence `T236` restated stands at `:283` | FR-046 | T276 |
| T256 | 2365 | `plan.md:323` | the line is blank; the Key-properties sentence stands at `:324` | FR-049 | T276 |

## Closed task clauses a later decision amended

Neither of the two shapes T091 offered ever landed, and the task was
closed. The requirement and the eight live sites that named the first
shape now state the second one, which is what the code has always done.
The decision, the competing reading, and the reason are recorded at
`source/counters/detail/core.hpp:92-99`, and the reading R-005 carries
is in `specs/007-counters-and-timers/research.md:41`.

| Site | Line in `tasks.md` | Clause as written | Amendment that governs it | Where the decision lives | Named by |
| --- | --- | --- | --- | --- | --- |
| T091 | 379 | "Store a per-composite fold program of slot references in `plan_impl`"; or "amend T017's 'fold program per composite (column references with algebraic exponents and ops)'" | the second branch, and neither shape reached the code while the task was checked | `source/counters/detail/core.hpp:92-99` | T262 |
| T017 | 66 | "fold program per composite (column references with algebraic exponents and ops)" | a plan holds leaf slots, a grouping layout, and the column geometry; the fold spine is a flat node array the caller owns, so the plan holds no per-composite program | `source/counters/detail/core.hpp:92-99`; FR-022 at `specs/007-counters-and-timers/spec.md:250` | T262 |
| T032 | 109 | "per-instance provider group reads (US3 scenario 5)" | one read group per provider, carrying every leaf that provider owns, so a fan-out over many objects is one `open` and one sampling action | `source/counters/plan.cpp:470-477`; `test/source/counters_objects_test.cpp:410-413` reconciles the per-core deltas against the shared total, so US3 scenario 5 holds on the shipped shape | T266 |
| T098 | 389 | "Split plan read groups per provider instance as T032's 'per-instance provider group reads' states, or amend the task; grouping today is per provider at `source/counters/plan.cpp:419-454`" | the second branch; the per-provider loop stands at `source/counters/plan.cpp:477-510` | `source/counters/plan.cpp:470-477` | T266 |

## Journal sentences a later requirement withdrew

The journal's own precedence rule at
`specs/007-counters-and-timers/sg_counters.md:9-12` settles every site
before line 855, because those sites sit in the older axis headers the
rule subordinates to the resolved scope statement. No rule reaches a
sentence inside the scope statement itself, which is why these five
survive three waves of restating the same claims elsewhere. The journal
text keeps its bytes, and the precedence rule above settles each site.

| Site in `sg_counters.md` | Claim the sentence carries | Settled amendment that withdrew it | Requirement or code line now governing |
| --- | --- | --- | --- |
| `:959` | the fast `tsc` leaf's frequency is calibrated at system-open from sysfs and CPUID | T153 and T258 place the calibration at provider construction | FR-034 at `specs/007-counters-and-timers/spec.md:265`; the constructor runs at `source/counters/clock_provider.cpp:206-236` |
| `:981-982` | the `perf_user_access` sysctl gates the fast read and is verified per kernel | T137 through T139 and T246 removed the sysctl from the read | FR-040 at `specs/007-counters-and-timers/spec.md:271`; the assumption at `:326` records that no sysctl takes part; the capability bit and the width come from the event page at `source/counters/linux_pmu/fast_read.cpp:271-272`; `docs/pages/counters-overhead.md:303` records the sysctl absent on this kernel |
| `:983` | pinning and index constraints are enforced at plan compile | T232 places the read-mode probe in provider enumeration | FR-023 at `specs/007-counters-and-timers/spec.md:251`; the index gate runs in `fast_context_read` at `source/counters/linux_pmu/fast_read.cpp:278-281` |
| `:1010-1011` | registering a composite into a started scope is a tier-3 contract violation | T226 and T255 name the three enforceable sequences | the edge case at `specs/007-counters-and-timers/spec.md:199` and the clarification at `:36`; `scope` exposes no registration entry point at `include/speedgun-ng/counters_measurement.hpp:992-1064`; `source/counters/fold.cpp:289-290` refuses `metric` on a window that is not closed |
| `:1049` | the standalone example's link manifest shows only speedgun-ng | T120 and T256 state that the target is a static archive | FR-049 at `specs/007-counters-and-timers/spec.md:286`; `readelf -d build/dev/example/counters_standalone_example` names `libstdc++.so.6`, `libgcc_s.so.1`, and `libc.so.6` |
| `:252` | "A composite compiles once into a flat read plan: leaf slots + fold sequence; no tree, no vtable, no closures, no lookups at read time" | T091 offered a per-composite program in the plan and the second branch amended the requirement to the shape the code has | FR-022 at `specs/007-counters-and-timers/spec.md:250`; the spine is a flat node array the caller owns at `include/speedgun-ng/counters_measurement.hpp:102-115`; the decision and its reason sit at `source/counters/detail/core.hpp:92-99` |

## Em-dash debt no machine check reports

T261 records what the shipped prose gate is structurally unable to
report. The journal carries 58 lines holding the em-dash code point
U+2014, at lines 47, 96, 154, 166, 175, 182, 187, 191, 245, 259, 278,
294, 311, 313, 342, 361, 405, 410, 481, 485, 486, 489, 504, 509, 513,
517, 521, 543, 584, 603, 606, 613, 620, 626, 631, 655, 658, 666, 673,
679, 682, 697, 778, 830, 855, 881, 912, 919, 925, 934, 936, 947, 950,
958, 963, 989, 1022, and 1035. Every one of the 58 leaves the gate
before any rule matcher runs, and one precedence row accounts for all
of them: `tools/prose/prose_gate.py:876-883` returns on a code span, a
URL, a path-like token, a shell command, or a blockquote, and 40 of the
58 carry a code span while 23 carry a path-like token. The row at
`:874-875` returns on a four-or-more-space markdown continuation and
catches 8 of the 58, at lines 361, 912, 919, 925, 934, 936, 947, and
950. The two rows overlap, so their counts do not partition the 58.

The 58 lines are pre-existing text in a dated closed record, and the
Principle XI.1 scope paragraph at
`.specify/memory/constitution.md:405-409` binds output generated after
the 2026-09-10 amendment, so they keep their bytes. The remedy that
paragraph names is a tree-wide sweep carried as a formatting-only
change under Principle V, scheduled on its own.

One line of a live artifact carried the same code point, and T261 fixed
it in place. The T116 text at
`specs/007-counters-and-timers/tasks.md:407` held two of them, one
inside each clause that named one sampling point, and the text now
carries the clauses inside one pair of parentheses.

The blind spot is the precedence row, and the range is not a second
one. T261 held that the line lay outside the range CI reads, on the
ground that the first commit touching `specs/007-counters-and-timers/`
is not an ancestor of the merge base. The gate's own range mode settles
the question: it reads `git diff -U1` over the merge base with
`origin/master` and the head, and its authorship map lists
`specs/007-counters-and-timers/tasks.md:407` as a new line, along with
2577 of the file's other lines and all 1206 lines of
`specs/007-counters-and-timers/sg_counters.md`, which every one of the
58 sits in. A file predating the range's left edge is what leaves a
line unexamined, and both of these files postdate it.

## A superseded figure in the tip commit's body

The Immutability clause of Pull Request Quality makes a landed message
immutable and corrected only by new commits, and the same clause permits
a pre-merge rewrite. The body of the tip commit `f1d3023` was composed
before that commit existed, so the range the gate resolved while it was
composed ended at its parent `1827d76`. One figure in the body's last
evidence paragraph belongs to that range.

The sentence reading that the gate over the whole repository
`exits 0 at 128 sources and 10250 units, 0 findings, 1 skipped` beside
the command `python3 tools/prose/prose_gate.py --check all` records the
head that command resolved, which is `1827d76`. Measured in a scratch
clone of this repository on 2026-09-28, that command exits 0 and
reports `128 sources, 10250 units examined, 0 findings, 1 skipped`
at that head, and exits 0 and reports
`129 sources, 10737 units examined, 0 findings, 1 skipped` at `f1d3023`,
the tree the commit leaves. The form both readings use resolves the merge
base with `origin/master`, at `65beada`, to the head, so the pair
reproduces at one head only. The whole-repository claim is carried here
by its exit code, and the form that reads the whole repository is
`python3 tools/prose/prose_gate.py --check prose --mode tree`.

The same body's paragraph on `T265` reports that a distinct-token figure
of 476 does not reproduce under the rule the preamble states, which
yields 168. The measurement recorded above finds 476 reproducing and 168
counting distinct line-range suffixes, so the three preambles carrying
467, 476, and 518 agree with the rule the record states.

The body of commit `6121416` carries the same class of claim and was
named by `T222` at
`specs/007-counters-and-timers/tasks.md:1801`, which the tree does not
show as rewritten: the message still reads that the prose gate run at
the branch tip exits 0 over 118 sources and 8862 units with 0 findings
and 1 skipped, naming no commit. Both the task text and the message
keep their bytes here, and the anchoring the four earlier passes applied
is recorded above.

## Re-anchors inside this record

T261, T262, T263, T264, T265, T266, and T267 through T278 grew this
record, so the line numbers the texts that cite it name have moved. The
table gives the number each such text wrote and the line holding the
same material now. The closed task lines, the dated preambles, and the
Phase 35 task texts that carry the old numbers keep their bytes, per
Principle VIII and Pull Request Quality: Immutability.

| Anchor as written | Line now holding the same material |
| --- | --- |
| `:20-24` | `:18-28`, the Precedence section |
| `:28-31` | `:32-41` for the counting rule, `:43-50` for the population, `:52-86` for the two other readings and the distinct-token result |
| `:40` | `:93`, the third drift criterion |
| `:46` | `:46`, unchanged; the line records the 467 the rule yields |
| `:55` | `:55`, unchanged; the line records the 476 the rule yields |
| `:57-58` | `:110-111` |
| `:59` | `:112` |
| `:80` | `:97`, and a second `:80` written by another text at `:133` |
| `:94` | `:147` |
| `:96` | `:149` |
| `:97` | `:150` |
| `:100` | `:153` |
| `:103` | `:156`, the row T263 corrected |
| `:145-150` | `:170-175`, the T091 clause paragraph |
| `:180-186` | `:205-211`, the 58-line journal list |
| `research.md:33` and `:37` | `research.md:43` and `:45`, the two anchors `f1d3023` added; the task text of `T270` at `specs/007-counters-and-timers/tasks.md:3123` carries the stale pair |
| `T274` `:57` | `:56`, the Phase 34 contribution of 9 distinct tokens, which `T278` at `specs/007-counters-and-timers/tasks.md:3589` names beside `T274` at `:3322` |
| `T276` `citations.md:275-282` | `:283-290`, this section |
| `citations.md:266-273` | `:274-281`, the paragraph on the body of commit `6121416`, which the Phase 37 preamble at `specs/007-counters-and-timers/tasks.md:3285` names |
| `T278` `citations.md:289-290` | `:297-298`, the two rows this table carries for `:46` and `:55`, which stay unchanged at those numbers |

## The nine live artifacts at a second head

The distribution above names `f1d3023` as the head it was measured at, and
that is the head the sentence is about, so both figures below name their
own head. At `f1d3023` the nine live artifacts carry 40 occurrences,
distributed `spec.md` 28, `quickstart.md` 4, `plan.md` 3, `research.md` 3,
and `contracts/system-contract.md` 2. At `9c5dfa5` they carry 44 with the
same distribution except `plan.md` at 7, the four added anchors being
`tools/dbc/coverage_gate.sh:25-27`,
`include/speedgun-ng/counters_measurement.hpp:619`,
`include/speedgun-ng/counters_provider.hpp:110`, and
`include/speedgun-ng/counters_measurement.hpp:1042-1045`. A pass that
applies the rule to the working tree reads 44 and names no head, which is
the reading the Phase 37 preamble at
`specs/007-counters-and-timers/tasks.md:3242` carries, and a pass that
applies it at `f1d3023` reads 40, which is the reading the paragraph above
this one records.

## A prose-gate total that names no head

The Phase 36 preamble at
`specs/007-counters-and-timers/tasks.md:2942` opens by naming three heads
for its prose-gate figures and then gives a fourth total,
`13 sources and 5278 units`, from
`python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/007-counters-and-timers docs/pages/counters-overhead.md`,
and names no head for it. That form takes its candidate set from
`git ls-files` at `tools/prose/prose_gate.py:587-590` and reads every
candidate from the working tree at `:842-860`, as `:936` shows it does
in both modes, so its total is a reading of the working tree. Measured
here, that form exits 0 at 13 sources and reports one reading with and
without `--head 9c5dfa5`, which confirms that the head argument selects
no text in that mode, and the sentence now carries its exit code alone.
`T206` applied that treatment to six earlier preambles, `T215` to the
coverage-trace freshness claim, `T268` to the tip body, and `T269` to
the two range-form sentences.

## The measured link manifest

`readelf -d build/dev/example/counters_standalone_example` and
`readelf -d build/dev/example/counters_giraffe_example` each carry 3
`NEEDED` entries at this head, `libstdc++.so.6`, `libgcc_s.so.1`, and
`libc.so.6`, and name no `libm` entry. `ldd` on either binary prints 6
lines, those three objects, `libm.so.6`, `linux-vdso.so.1`, and the loader
`ld-linux-x86-64.so.2`, so the two tools name different sets. The
clarification at `specs/007-counters-and-timers/spec.md:31` and section 2
at `specs/007-counters-and-timers/quickstart.md:25` each enumerated four
libraries including `libm`, and `T279` restated both to the measured
three. The clarification's own question keeps its claim, because both
tools name no `speedgun-ng` entry. The SC-001 row at
`specs/007-counters-and-timers/quickstart.md:137` reported
`readelf -d ... | grep -c NEEDED` at 4, and the evidence log
`.omo/evidence/007-counters-and-timers/sc-001-link-manifest.txt` shows the
`build/agent-dc` binary of that pass carrying four entries including
`[libm.so.6]`, so the row records its own pass accurately and the figure
in the table below is the one the current tree yields.

| Site | Figure as written | Measured at this head | Governing |
| --- | --- | --- | --- |
| `specs/007-counters-and-timers/quickstart.md:137` | `readelf -d ... \| grep -c NEEDED` 4, all platform runtime | 3, `libstdc++.so.6`, `libgcc_s.so.1`, and `libc.so.6` | FR-049 at `specs/007-counters-and-timers/spec.md:286`, SC-001 at `:308`; T256 |

The link-manifest row in the journal table above covers
`specs/007-counters-and-timers/sg_counters.md:1049` alone. Every other
site carrying the four-library enumeration is a closed task line or a
dated preamble in `specs/007-counters-and-timers/tasks.md`, at `:818`,
`:933`, `:2028`, and `:2370-2374`, and each keeps its bytes. The manifest
check at `.github/workflows/ci.yml:148-158` filters the `NEEDED` lines
against the four names, so it admits the measured three and states no
count.

## Anchor totals under each engine

The Phase 38 preamble at
`specs/007-counters-and-timers/tasks.md:3429-3430` states that this file
carries 101 occurrences of the path form. Under the rule at `:32-41`
applied with CPython 3.14.7 `re.finditer` over whole matches `m.group(0)`,
this file carried 114 occurrences and 101 distinct tokens over its whole
length at `9da43ac`, and 101 occurrences over 90 distinct tokens at the
parent `b82fb7e`. The figure 101 is the occurrence total at the commit
that authored the sentence and the distinct total at `9da43ac`, and the
two senses are separate counts. The sections appended after those four
raised the totals to 144 occurrences and 126 distinct tokens, which is the
reading the working tree yields. The counting rule and every figure the
record carried at `9da43ac` keep their values, and the preamble keeps its
bytes.

Over `specs/007-counters-and-timers/tasks.md` lines 1 through 2422, the
population `:43-50` names, each engine returns the following.

| Engine | Basis | Occurrences | Distinct |
| --- | --- | --- | --- |
| CPython 3.14.7 `re.finditer` | whole matches, `m.group(0)` | 702 | 467 |
| GNU grep 3.12 `grep -oP` | 467 after `sort -u` | 702 | 467 |
| CPython 3.14.7 `re.findall` | 702 items, the one capturing group `(-\d+)?`, whose values are line-range suffixes | 702 | 163 |
| GNU grep 3.12 `grep -oE` | no match, and the warnings `? at start of expression` and `stray \ before d`, because `(?:` is outside POSIX ERE | 0 | 0 |

The `T277` task text at
`specs/007-counters-and-timers/tasks.md:3568-3570` states that `grep -oP`
returns 467, which is its deduplicated count, and that `re.findall`
returns 163, which is the deduplicated form of the capturing group. The
closed task line keeps its bytes.

## The coverage-exclusion population at this head

The command the dated gate-pass row names,
`rg -c 'LCOV_EXCL' source/counters include/speedgun-ng/counters*.hpp`,
returns 301 over 11 files, and `rg -o` returns 301 tokens over 301 marker
lines, so the population is one token per line.
`specs/007-counters-and-timers/quickstart.md:169` reports 296 marker
lines, and the evidence log
`.omo/evidence/007-counters-and-timers/t066-marker-count.txt` sums to 296
over the same 11 files, so the row records that pass accurately. The
figure moved from 304 to 301 at `b270503`, which changed
`source/counters/linux_pmu/group_io.cpp` alone by 14 insertions and 14
deletions, and the parent `a8ed1d6` yields 304 for the same command. The
registered P2 justification at
`specs/007-counters-and-timers/plan.md:457` governs the population the
exception covers, so `T283` restated its leading figure to 301 and
recorded the later movement beside it.

| Site | Figure as written | Measured at this head | Governing |
| --- | --- | --- | --- |
| `specs/007-counters-and-timers/quickstart.md:169` | 296 marker lines | 301 marker lines and 301 tokens, 11 files | the P2 row at `specs/007-counters-and-timers/plan.md:457`; Constitution I, P2 |

## The whole-repository prose-gate verdict

The form the paragraph above names as the one reading the whole repository
is the command
`python3 tools/prose/prose_gate.py --check prose --mode tree`,
which exits 1 and reports
`prose-lint: 183 sources, 19055 units examined, 108 findings, 0 skipped`
on the working tree that carried the Phase 39 append. The units total is a
reading of the working tree, so it rises with every section appended here
and with every other edit to the tree, while the findings total stays at
108. The 108 findings fall outside the feature scope: 89
in `specs/001-dbc-facility/`, 9 in `test/`, 5 in `tools/dbc/`, 3 in
`docs/pages/dbc-overhead.md`, and 2 in `include/speedgun-ng/`. The branch
touches two of the files carrying them,
`specs/001-dbc-facility/tasks.md` and
`test/source/dbc_test.cpp`, and those two carry 24 of the 108. The
constitution's own gate is the range form at
`.specify/memory/constitution.md:245-248`, and the commands
`python3 tools/prose/prose_gate.py --check all` and
`cmake -P cmake/prose-lint.cmake` both exit 0, so no finding falls on a
line the branch's diff added or modified. The units figure is a reading of
the working tree and moves with every edit to it; the findings figure does
not. No gate rule, threshold, or marker moved with this record.

## The mapped-page protocol order `T077` required

`T077` at `specs/007-counters-and-timers/tasks.md:365` is closed with a
requirement the code has not carried since commit `2889608`. Its text read
that the capability gate is hoisted ahead of the instruction, and the two
anchor rows above recorded the landing as an instruction a hoisted gate
precedes. The shipped `fast_context_read` reads the capability bit from the
event page, issues the instruction under the index test alone, and applies
the capability gate as the first check of `fast_decode`, which the call
site invokes after the instruction has run
(`source/counters/linux_pmu/fast_read.cpp:272`, `:278`, `:281`, `:284`, and
`:86-88`). The decision is that the decode gate governs, and the task line
now states the ordering the code ships. The competing reading is the
instruction gate, which reinstates the hoist.

`git show 2889608 -- source/counters/linux_pmu/fast_read.cpp` is the commit
that withdrew the hoist. Its diff deletes the block reading `Capability gate ahead of the instruction (FR-040, R-011): the published protocol tests the capability before it takes the read, so a caller the kernel grants no read capability never pays for the instruction` together with the `cap_user_rdpmc` compare that followed it, and adds the comment stating that every gate is decided by the page. The reason is recorded where the
shipped ordering is documented, at
`source/counters/detail/pmu.hpp:249-254`: the caller reads the instruction
only for a nonzero index, so a gate applied in the decode costs a page load
and never an instruction. `FR-040` at
`specs/007-counters-and-timers/spec.md:271` names capability gating among
the protocol steps and orders none of them, and the decode-order comment at
`source/counters/linux_pmu/fast_read.cpp:82-85` states the order the code
follows.

The decode gate costs one page load and one compare on a read whose page
publishes no capability bit, and it leaves the capability word off the
critical path of a read that succeeds. A restored hoist moves that load and
compare ahead of the instruction, puts a branch in front of every
successful read, and withholds no instruction the index test already
withholds, because the kernel assigns a nonzero index only on a host that
grants the capability. The amendment is recorded in the shape `T262` used,
in this file and at the site that carries the decision; the repository's one
DCR-form reference is the source-ref policy at
`specs/007-counters-and-timers/plan.md:479`.

## The scaled-value clause of US6 scenario 6

The third clause of scenario 6 at
`specs/007-counters-and-timers/spec.md:150` required that the folded value
be the scaled estimate the kernel computed. A kernel group read publishes
`time_enabled` and `time_running` and computes no scaled estimate, and a
leaf node yields the raw modular delta as a double
(`source/counters/fold.cpp:45-46`). The decision is that the fold discloses
the fraction and the flag and leaves the scaling to the caller, and the
clause now states that. The competing reading scales the value by the
disclosed ratio, and it collides with the product form `FR-019` at
`specs/007-counters-and-timers/spec.md:244` and the clarification at `:25`
mandate, because a composite's ratio is the product of its constituent
ratios and a scaled value would carry the multiplex correction once per
constituent. `test/source/counters_pmu_test.cpp:644-650` states the
consequence and calls the fold right, and no assertion anywhere asks for a
scaled value.

Measured in this pass, `./build/dev/test/counters_pmu_test` exits 0 and prints `scenario 6: 64 events opened against this PMU; enabled advanced 13986033 ns, running advanced 5972952 ns, so the kernel ran them 0.427065 of the time; the composite discloses running_ratio 0.000000 with scaled 1`. A
composite of 64 oversubscribed members discloses a ratio near zero, and a
value scaled by it is a number a caller cannot use. The closed task text at
`specs/007-counters-and-timers/tasks.md:159` carries the withdrawn clause
as `ratio below 1, scaled set, value is the kernel scaled estimate` and
keeps its bytes.

## The range form's per-line default

The range form examines a line only when the range's authorship map holds an
entry for it. `tools/prose/prose_gate.py:959` reads
`status = authorship.get(path, {}).get(lineno, "grandfathered")` and `:960-961`
skips a line whose status is `grandfathered`, so a line the map holds
nothing for is left unexamined while the rest of its file is examined.
`collect_candidates` at `:587` builds that map from the `git diff -U1` at
`:597` and returns `authorship = None` in tree mode alone, where `:592`
lists the candidates with `git ls-files`, and `read_source` at `:842` reads
the examined text from the working tree in both modes, as the call at `:936`
shows. The last paragraph of the em-dash section above names one condition
that leaves a line unexamined, and the code has two.

The first is a file the range's diff does not touch, which is no candidate
at all. The range from the merge base with `origin/master` to `8a62b69` names
623 files and holds none of `docs/pages/dbc-overhead.md`,
`include/speedgun-ng/dbc.hpp`, or anything under `tools/dbc/`, which is where
the whole-repository tree form reports three, one, and five of its findings.
The second is a line inside a ranged file that the committed diff holds no
entry for, which is every uncommitted insertion there, because the map's
keys are the new-file line numbers of the committed diff while the text
examined is the working tree. A committed line the branch's own diff added
always carries an entry, so the range form's green verdict covers the
branch's added prose.

Measured on a scratch clone of this repository at `8a62b69` holding a
two-line uncommitted insertion at the end of `docs/pages/counters-overhead.md`
reading `really, basically very important, honestly`,
`python3 tools/prose/prose_gate.py --check all --head 8a62b69` exits 0 at
`135 sources, 12313 units examined, 0 findings, 1 skipped`, the figure the
Phase 40 preamble records for that head, while
`python3 tools/prose/prose_gate.py --check prose --mode tree --paths docs/pages/counters-overhead.md` exits 1 at
`1 sources, 290 units examined, 3 findings, 0 skipped` and names the three
at line 348. A range figure therefore reproduces at a head only when the
working tree equals that head. The clause in the Phase 34 preamble at
`specs/007-counters-and-timers/tasks.md:2432-2435` that reads `so an uncommitted line inside a ranged file is examined`
overstates for the insertion case, and that preamble keeps its bytes under
the Immutability clause of Pull Request Quality.

## The static-analysis figure the feature scope carries

`specs/007-counters-and-timers/tasks.md:4270-4273`, the Phase 41
preamble, records `reports 0` lines carrying `warning:` and 0 carrying
`error:` for the feature scope, and
`specs/007-counters-and-timers/tasks.md:4426-4430`, `T292`'s own text,
carries the same figure at `:4428-4429` and rests a conclusion on it at
`:4429-4430`: `so the report is clean and the clause's figure holds`.
Both texts are closed records and both keep their bytes under the
Immutability clause of Pull Request Quality. Both are wrong on the
count, and the figure `T292` concluded from is the one the branch's own
2.10.0 Sync Impact Report contradicts at
`.specify/memory/constitution.md:10-20`.

Measured at head `9d83823` over the working tree this record sits in,
`run-clang-tidy` 22.1.8 over the eleven translation units
`find source/counters -name '*.cpp'` returns, selected from
`build/dev/compile_commands.json` with the repository `.clang-tidy`,
exits 0 and reports 734 lines carrying `warning:` and 0 carrying
`error:`. The invocation:

```
run-clang-tidy -p build/dev \
  -header-filter='^/home/archerc/code/speedgun-ng/' \
  -exclude-header-filter='^/home/archerc/code/speedgun-ng/external/' \
  -source-filter='.*/source/counters/.*' \
  $(find source/counters -name '*.cpp' | sort)
```

The 734 decompose by the file the diagnostic names. The eleven
translation units contribute 358, their two provider-private headers
contribute 42, the seven public `counters` headers contribute 320,
`include/speedgun-ng/dbc.hpp` contributes 11 because the counters
sources include it, and two vendored simdjson inline headers contribute
3 despite the exclude filter, which the header filter does not reach
for an inl-header diagnostic.

| File the diagnostic names | `warning:` lines |
| --- | --- |
| `include/speedgun-ng/counters_measurement.hpp` | 196 |
| `include/speedgun-ng/counters_provider.hpp` | 99 |
| `source/counters/linux_pmu/table_parse.cpp` | 64 |
| `source/counters/linux_pmu/group_io.cpp` | 54 |
| `source/counters/fold.cpp` | 42 |
| `source/counters/system.cpp` | 37 |
| `source/counters/plan.cpp` | 35 |
| `source/counters/linux_pmu/provider.cpp` | 33 |
| `source/counters/linux_pmu/../detail/pmu.hpp`, which is `source/counters/detail/pmu.hpp` | 30 |
| `source/counters/clock_provider.cpp` | 31 |
| `source/counters/fake_provider.cpp` | 20 |
| `source/counters/linux_pmu/encode.cpp` | 18 |
| `source/counters/linux_pmu/fast_read.cpp` | 17 |
| `include/speedgun-ng/counters_system.hpp` | 16 |
| `source/counters/detail/core.hpp` | 12 |
| `include/speedgun-ng/dbc.hpp` | 11 |
| `source/counters/push_provider.cpp` | 7 |
| `include/speedgun-ng/counters_push.hpp` | 3 |
| `external/simdjson/include/simdjson/dom/document-inl.h` | 2 |
| `include/speedgun-ng/counters_clock.hpp` | 2 |
| `include/speedgun-ng/counters_fake.hpp` | 2 |
| `include/speedgun-ng/counters_pmu.hpp` | 2 |
| `external/simdjson/include/simdjson/padded_string-inl.h` | 1 |

The single-translation-unit pair the 2.10.0 report and both sibling
commit bodies state reproduces. `clang-tidy` 22.1.8 on
`source/counters/plan.cpp` with the flags
`build/CMakeFiles/speedgun-ng_speedgun-ng.dir/flags.make` records and
the header and exclude filters `build/CMakeCache.txt` records exits 0
with 92 lines carrying `warning:` and 0 carrying `error:`, and the same
invocation under `--warnings-as-errors='*'` exits 1 with 0 carrying
`warning:` and the same 92 carrying `error:`. The run reports `20811
warnings generated.` before suppression. The 92 decompose
`include/speedgun-ng/counters_measurement.hpp` 40,
`source/counters/plan.cpp` 35,
`include/speedgun-ng/counters_provider.hpp` 9,
`include/speedgun-ng/counters_system.hpp` 4,
`source/counters/detail/core.hpp` 3, and
`include/speedgun-ng/dbc.hpp` 1. The largest check class is
`readability-identifier-length` at 43 of the 92.

The flag set is not the discriminator. The same file with the dev
preset's flags, read from
`build/dev/test/CMakeFiles/counters_trap_checked_test.dir/flags.make`
form, exits 0 with the same 92 `warning:` lines. A run whose output was
not read, or whose findings were filtered out of the log, produces the
0 the two closed texts record. The branch's two most recent code-bearing
commit bodies report lines that carry a finding, so the 0 is a
measurement error in two closed texts and in no gate.

| Site | Figure as written | Measured at `9d83823` | Governing |
| --- | --- | --- | --- |
| `specs/007-counters-and-timers/tasks.md:4272-4273`, the Phase 41 preamble | 0 `warning:`, 0 `error:` over the eleven translation units | 734 `warning:`, 0 `error:` over the same eleven | Principle VIII; X.4 |
| `specs/007-counters-and-timers/tasks.md:4428-4429`, `T292` | 0 `warning:`, 0 `error:` over the same eleven | 734 `warning:`, 0 `error:` over the same eleven | Principle VIII; X.4 |
| `specs/007-counters-and-timers/tasks.md:4429-4430`, `T292`'s conclusion | `so the report is clean and the clause's figure holds` | the report carries 734 findings and the clause states no count | Principle VIII; X.4 |

A finding count and a new-finding count are different obligations, and
the clause names only the second. `.specify/memory/constitution.md:269-274`
requires that the two analyzers `report no new findings, against the
pinned Core Guidelines baseline (I) from the same configuration`. The
baseline that word names is the pinned rule set, a list of check names in
`.clang-tidy`; it carries no finding count and no tree. Nothing in the
clause, in `.clang-tidy`, or in the presets names a recorded per-tree
count, so `new` has nothing to be new against: against this tree the
pinned check set reports 734 over the feature scope's eleven
translation units and 92 over one of them, and the 2.10.0 report records
23700 of its whole-database total from the vendored trees. The clause is
therefore not measurable as written, and the record states the fact
rather than a remedy. A baseline is a governance decision under
Principle IX and belongs to the repository owner: it would take a
recorded per-tree finding count at a named head, pinned the way the Core
Guidelines revision is pinned, with the rule that a change to
`.clang-tidy` or to a vendored tree moves the baseline with it. No
analyzer call was suppressed, no warning class demoted, and no
`.clang-tidy` entry or preset value changed in reaching this section.

## The constitution re-anchored after amendment 2.10.0

Amendment 2.10.0, commit `cb5dee5`, states in its own Sync Impact
Report at `.specify/memory/constitution.md:24-28` that it shifts every
line in the file and that a citation into the constitution needs
re-anchoring by the next convergence pass. The pass named is this one.
The constitution held 612 lines at `4fd1189`, the head the 2.9.1 report
held, and holds 645 at `9d83823`.

`git diff -U0 4fd1189 cb5dee5 -- .specify/memory/constitution.md` gives
three hunks, at old line 2, old line 242, and old line 596, so the shift
map is arithmetic and a later amendment is re-derivable from it. An old
line at or below 241 moves by 29, an old line from 243 through 595 moves
by 32, and an old line at or above 596 moves by 33. Old line 242 is the
static-analysis clause itself, which the amendment replaced, and it now
occupies `:271-274`. The task text behind this section states the
pre-hunk boundary as old line 238 and names two bands; the measured
boundary is old line 241 and there are three bands, the third covering
old lines at or above 596.

Counted with the rule at `:32-41` over `git ls-files` output, the
feature places 48 path-form anchors into the constitution: 45 in
`specs/007-counters-and-timers/tasks.md` lines 1 through 4479 and 3 in
this record. Every one of the 48 was read individually: at `4fd1189`
the cited line holds the text the sentence naming it describes, and at
`9d83823` the shifted line holds that same text while the cited line
holds other text. The class of an anchor that was never right is empty
over these 48. A hand attribution of the bare `:NN` continuations
sitting on the same lines adds 17, of which 11 assert a current
position and are stale by the same map, and 6 assert a pre-amendment
position and stand as history. The 65 anchors divide into 59 stale and 6
historical.

The three rows in this record come first, because they are the ones a
reader follows. `specs/007-counters-and-timers/citations.md:156` names
`:568-570` for the machine-local `CMakeUserPresets.json` sentence,
which stands at `:600-602`. The same row records that the landing it
names, `:571-573`, held the Licensing bullet and one empty line at the
head it was written at; that range stands at `:603-605` now.
`specs/007-counters-and-timers/citations.md:222` names `:405-409` for
the Principle XI.1 scope paragraph, which stands at `:437-441`.
`specs/007-counters-and-timers/citations.md:455` names `:245-248` for
the constitution's own prose-lint gate and the range form, which stands
at `:277-280`; `:245-248` now holds Principle VII's per-platform
baselines bullet, and `T284` and the Phase 40 preamble both lean on the
row.

| Site | Anchor as written | Line holding the same text at `9d83823` | Head the number was read at | Shift |
| --- | --- | --- | --- | --- |
| `citations.md:156` | `:568-570` | `:600-602` | `4fd1189` | 32 |
| `citations.md:222` | `:405-409` | `:437-441` | `4fd1189` | 32 |
| `citations.md:455` | `:245-248` | `:277-280` | `4fd1189` | 32 |
| `tasks.md:358`, `T073` | `:461-462` | `:493-494` | `4fd1189` | 32 |
| `tasks.md:397`, `T106` | `:568-570` | `:600-602` | `4fd1189` | 32 |
| `tasks.md:751` | `:279-287` | `:311-319` | `4fd1189` | 32 |
| `tasks.md:773`, `T184` | `:279-287` | `:311-319` | `4fd1189` | 32 |
| `tasks.md:832` | `:279-287` | `:311-319` | `4fd1189` | 32 |
| `tasks.md:861` | `:59-64` | `:88-93` | `4fd1189` | 29 |
| `tasks.md:888`, `T187` | `:279-287` | `:311-319` | `4fd1189` | 32 |
| `tasks.md:888`, `T187` | `:59-64` | `:88-93` | `4fd1189` | 29 |
| `tasks.md:962` | `:424-425` | `:456-457` | `4fd1189` | 32 |
| `tasks.md:976`, `T189` | `:19-20` | `:48-49` | `4fd1189` | 29 |
| `tasks.md:976`, `T189` | `:232-237` | `:261-266` | `4fd1189` | 29 |
| `tasks.md:976`, `T189` | `:250` | `:282` | `4fd1189` | 32 |
| `tasks.md:980`, `T190` | `:515` | `:547` | `4fd1189` | 32 |
| `tasks.md:988`, `T192` | `:443` | `:475` | `4fd1189` | 32 |
| `tasks.md:988`, `T192` | `:361-388` | `:393-420` | `4fd1189` | 32 |
| `tasks.md:988`, `T192` | `:515` | `:547` | `4fd1189` | 32 |
| `tasks.md:1135`, `T196` | `:554-556` | `:586-588` | `4fd1189` | 32 |
| `tasks.md:1135`, `T196` | `:568-570` | `:600-602` | `4fd1189` | 32 |
| `tasks.md:1136`, `T197` | `:447-448` | `:479-480` | `4fd1189` | 32 |
| `tasks.md:1136`, `T197` | `:461-462` | `:493-494` | `4fd1189` | 32 |
| `tasks.md:1180` | `:568-570` | `:600-602` | `4fd1189` | 32 |
| `tasks.md:1182` | `:461-462` | `:493-494` | `4fd1189` | 32 |
| `tasks.md:1183` | `:515` | `:547` | `4fd1189` | 32 |
| `tasks.md:1209` | `:445-447` | `:477-479` | `4fd1189` | 32 |
| `tasks.md:1287` | `:515` | `:547` | `4fd1189` | 32 |
| `tasks.md:1288` | `:515` | `:547` | `4fd1189` | 32 |
| `tasks.md:1289` | `:568-570` | `:600-602` | `4fd1189` | 32 |
| `tasks.md:1290` | `:461-462` | `:493-494` | `4fd1189` | 32 |
| `tasks.md:1291` | `:445-447` | `:477-479` | `4fd1189` | 32 |
| `tasks.md:2711` | `:568-570` | `:600-602` | `4fd1189` | 32 |
| `tasks.md:2734` | `:397-403` | `:429-435` | `4fd1189` | 32 |
| `tasks.md:2752` | `:408-409` | `:440-441` | `4fd1189` | 32 |
| `tasks.md:2813` | `:568-570` | `:600-602` | `4fd1189` | 32 |
| `tasks.md:2815` | `:571` | `:603` | `4fd1189` | 32 |
| `tasks.md:2997` | `:472-474` | `:504-506` | `4fd1189` | 32 |
| `tasks.md:3147` | `:203-205` | `:232-234` | `4fd1189` | 29 |
| `tasks.md:3174` | `:190-192` | `:219-221` | `4fd1189` | 29 |
| `tasks.md:3295` | `:112-118` | `:141-147` | `4fd1189` | 29 |
| `tasks.md:3299` | `:190-192` | `:219-221` | `4fd1189` | 29 |
| `tasks.md:3724` | `:190-192` | `:219-221` | `4fd1189` | 29 |
| `tasks.md:3846` | `:245-248` | `:277-280` | `4fd1189` | 32 |
| `tasks.md:4009` | `:245-248` | `:277-280` | `4fd1189` | 32 |
| `tasks.md:4013` | `:190-192` | `:219-221` | `4fd1189` | 29 |
| `tasks.md:4413`, `T292` | `:240-242` | `:269-270` for `:240-241`, and `:271-274` for `:242`, the clause the amendment replaced | `4fd1189` | 29 |
| `tasks.md:4475`, `T294` | `:405-409` | `:437-441` | `4fd1189` | 32 |

The 11 bare continuations assert a current position and are stale by the
same map. Six further continuations on the same lines name a
pre-amendment position and stand as history, so they are listed with
their head and take no correction: `tasks.md:980`'s `:443` twice and
`:492`, `tasks.md:1135`'s `:531-533` and `:545-547`, and `tasks.md:1136`'s
`:424-425`.

| Site | Continuation as written | Line holding the same text at `9d83823` | Head the number was read at | Shift |
| --- | --- | --- | --- | --- |
| `tasks.md:962` | `:447-448` | `:479-480` | `4fd1189` | 32 |
| `tasks.md:976`, `T189` | `:211-216` | `:240-245` | `4fd1189` | 29 |
| `tasks.md:976`, `T189` | `:258-264` | `:290-296` | `4fd1189` | 32 |
| `tasks.md:976`, `T189` | `:279-287` | `:311-319` | `4fd1189` | 32 |
| `tasks.md:980`, `T190` | `:466` | `:498` | `4fd1189` | 32 |
| `tasks.md:988`, `T192` | `:387-476` | `:419-508` | `4fd1189` | 32 |
| `tasks.md:1136`, `T197` | `:411-421` | `:443-453` | `4fd1189` | 32 |
| `tasks.md:1136`, `T197` | `:442` | `:474` | `4fd1189` | 32 |
| `tasks.md:2813` | `:571-573` | `:603-605` | `4fd1189` | 32 |
| `tasks.md:4014` | `:258-260` | `:290-292` | `4fd1189` | 32 |
| `tasks.md:4414`, `T292` | `:227` | `:256` | `4fd1189` | 29 |

The closed task lines and the dated preambles keep their bytes, so this
table is the only vehicle. The record's own counting rule at `:32-41`
and every figure the record carries keep their values, and the shift
map above is what a later amendment is measured against.

## The tree form takes its candidate set from `git ls-files`

The sentence at `specs/007-counters-and-timers/tasks.md:3864-3867` is
false in its first half and true in its second. It reads `Both forms
take their candidate file set and their per-line authorship filter from
the range`, and the code takes the candidate set from the range in the
range mode alone. `tools/prose/prose_gate.py:591-593` returns the
`git ls-files` listing with `authorship = None` under `if mode == "tree"`,
the range path is `:594-600`, and `:956-957` sets every examined line's
status to `new` whenever the map is `None`, so the tree form holds no
per-line filter at all. The sentence's second half, that `read_source` at
`:842` reads the examined text from the working tree in both modes as
the call at `:936` shows, holds. The clause is at `:3868-3869`.

The same preamble carries a tree-form figure at `:3875-3885`, and a
tree-form run narrowed to this feature, so a reader applying its first
half concludes the tree form is range-limited, which would drop every
file the range does not touch. The Phase 41 sentence at
`specs/007-counters-and-timers/tasks.md:4226-4231` states the same
attribution about `collect_candidates` without the `Both forms`
quantifier, and is saved by its surrounding paragraph, which declines to
give a figure for the range holding it. Read on its own the attribution
has the same gap, and a reader who quotes it needs the paragraph with it.
The section above this one states the correct behaviour at `:526-536`
and is the place a reader reaches for it.

| Site | Clause as written | What the code does | Governing |
| --- | --- | --- | --- |
| `tasks.md:3864-3867` | `Both forms take their candidate file set and their per-line authorship filter from the range` | the range mode alone takes both; the tree mode takes `git ls-files` at `prose_gate.py:592` and no per-line filter, since `:956-957` sets every examined line to `new` | Principle X.4; XI.6 |
| `tasks.md:3868-3869` | `read_source` reads the examined text from the working tree in both modes | holds, at `prose_gate.py:842` and its call at `:936` | Principle X.4 |
| `tasks.md:4226-4231` | the same attribution about `collect_candidates`, unquantified | the tree mode returns `authorship = None` at `:593`; the sentence is saved by its paragraph's scope | Principle X.4 |

The preamble keeps its bytes, and no gate rule, threshold, vocabulary, or
marker moved with this row.

## The code-span exemption is unit-wide

`tools/prose/prose_gate.py:876-883` returns an empty finding list for a
unit when any of five triggers matches anywhere in it: `INLINE_CODE_RE`
at `:877`, `URL_RE` at `:878`, `path_like` at `:879`,
`SHELL_COMMAND_RE` at `:880`, and `BLOCKQUOTE_RE` at `:881`. The row is
a unit. `.specify/memory/constitution.md:504-506` scopes the exemption to
`a banned token inside a verbatim quotation, code span, command, file
name, or a literal that is itself the subject under discussion`, which
is a token. A banned token outside the span on a line that carries one
is therefore dropped, and the row sits above every rule matcher, so no
rule sees the line at all. The blind-spot section at `:203-243` names the
em-dash family and the split-span class and names neither this scope nor
the three instances below.

Three Principle XI violations in this feature's live artifacts rode the
row, and `T298` closed all three. Each sentence now states what the
thing is, each fact in its own sentence, and no line count moved, so
every anchor into these three files keeps its number.

| Site | Token the row dropped | Sentence now reads |
| --- | --- | --- |
| `specs/007-counters-and-timers/plan.md:393` | XI.2 `, not ` | `The kernel grants the path. The probe was the obstacle.` |
| `specs/007-counters-and-timers/quickstart.md:140` | XI.2 `, not ` | `the binary check is reported as a measurement` |
| `specs/007-counters-and-timers/research.md:57` | XI.5 `simply` | `Platforms without a usable TSC omit the leaf` |

The measured scope of the row, over `git ls-files` with the gate's own
`classify_source`, `INLINE_CODE_RE`, `URL_RE`, `path_like`,
`SHELL_COMMAND_RE`, and `BLOCKQUOTE_RE` and with the carriers masked
before the token search, is 279 lines repository-wide over 25 files that
carry a token the row drops, read at `14b8e48`, whose uncommitted append
adds none: 68 in `specs/007-counters-and-timers/tasks.md`, 61 in
`specs/007-counters-and-timers/sg_counters.md`, 43 in
`specs/001-dbc-facility/plan.md`, 33 in its `research.md`, 33 in its
`tasks.md`, 12 in its `spec.md`, 5 in `.specify/scripts/bash/common.sh`,
4 in its `contracts/api-contracts.md`, 3 in its `data-model.md`, 2 in
`specs/007-counters-and-timers/plan.md`, and 1 in each of 15 further
files. The feature's own files hold 132 of the 279 and the other 21 hold
147, and no subset of the seven prose rules yields 129.

The task text behind this section records 211 repository-wide; the
measured total is 279, and the three lines it leaves in the live
artifacts are `plan.md:347`, `plan.md:457`, and `spec.md:33`, which the
final section of this file records closed. The 68 lines in `tasks.md`
include the five the task text names at `:390`, `:487`, `:497`, `:1633`,
and `:2723`; all five keep their bytes under the Principle XI.1 scope
paragraph at `:437-441`, and the 61 lines in the closed journal keep
theirs under the same clause. The tree this pass leaves carries 276 over
23 files. No gate rule, threshold, vocabulary, or marker moves here. A
narrower row belongs to `specs/002-prose-commit-lint` and to a
constitutional reading of the exemption's scope, and that reading is the
repository owner's under Principle IX.

## The executable count the last three preambles carry

`test/CMakeLists.txt` carries 13 `add_executable(counters_` calls at this
head. The Phase 39 preamble at `specs/007-counters-and-timers/tasks.md:3684`,
the Phase 40 preamble at `:3957`, and the Phase 41 preamble at
`:4307-4308` each carry 12. Each figure was correct at the head its
preamble was written at, so the three keep their bytes under the
Immutability clause of Pull Request Quality. The movement is a test-side
addition, commit `7920853`, which added the negative-control fixture
`counters_trap_noguard_fixture` at `test/CMakeLists.txt:203-205` and
changed no library code.

| Site | Figure as written | Measured at `9d83823` | Commit that moved it | Governing |
| --- | --- | --- | --- | --- |
| `tasks.md:3684`, Phase 39 preamble | 12 `add_executable(counters_` calls | 13 | `7920853` | Principle X.4 |
| `tasks.md:3957`, Phase 40 preamble | 12 `add_executable(counters_` calls | 13 | `7920853` | Principle X.4 |
| `tasks.md:4307-4308`, Phase 41 preamble | 12 `add_executable(counters_` calls | 13 | `7920853` | Principle X.4 |

## The release-configuration static-analysis total

Principle IX requires one release-configuration build per feature, and
`.specify/memory/constitution.md:269-274` states that the static-analysis
gate reports. This is the measured total for the configuration the gate
drives, forced from clean. The build before this one was a no-op with 26
targets and 0 compile actions, which is why the Phase 39, Phase 40, and
Phase 41 preambles each state no total.

`cmake --preset=ci-ubuntu` exits 0 with `CMAKE_BUILD_TYPE:STRING=Release`
and `CMAKE_CXX_CLANG_TIDY:UNINITIALIZED=clang-tidy;--header-filter=^/home/archerc/code/speedgun-ng/;--exclude-header-filter=^/home/archerc/code/speedgun-ng/external/`
in the cache. The 38 object files under `build/CMakeFiles`,
`build/test/CMakeFiles`, and `build/example/CMakeFiles` were then
deleted, the ci-ubuntu tree's own project targets, and 0 object or gcov
files remained in those three directories. The `build/_zlib`,
`build/_simdjson`, `build/_yaml-cpp`,`build/_hdrhistogram`, and
`build/hwloc_vendor-prefix-e21a30e9` object files, 74 of them, were left
in place, and the `build/agent-*` trees were not touched.
`cmake --build build` then exits 0 over 38 compile actions and 27 built
targets, 1 m 50 s.

The log carries 2953 lines with `warning:` and 1 line with `error:`. The
single `error:` line is
`test/source/dbc_test.cpp:231:9: error: Unhandled exception thrown in
function that is an entry point. [throwInEntryPoint]`, a static-analysis
finding the launcher reports at error severity while the build succeeds,
which is the design the 2.10.0 Sync Impact Report records. `cppcheck`
contributed no line: the launcher form `cppcheck;--inline-suppr` in the
cache produced no finding on any of the 38 translation units.

| Population | `warning:` lines |
| --- | --- |
| feature scope: `source/counters/` and the seven public `counters` headers | 1467 |
| feature scope: the twelve `test/source/counters_*` and two `example/counters_*` units | 1243 |
| other project units: the feature 001 DBC tests | 185 |
| other project units: `include/speedgun-ng/dbc.hpp` and `include/speedgun-ng/speedgun-ng.hpp` | 33 |
| other project units: the five vendored-tree gate translation units under `source/` | 22 |
| `external/`: two simdjson inline headers, which the exclude header filter does not reach for an inl-header diagnostic | 3 |

The 2953 decompose between the feature scope, 2710, and everything else
in the same tree, 240, with the 3 vendored lines sitting outside both.
The vendored trees' own 74 object files were not recompiled, so a fully
from-clean release build would add their diagnostics to this total, and
the only figure recorded for them is the 23700 the 2.10.0 Sync Impact
Report states for the whole database, which this pass did not
re-measure. Head `9d83823` with one uncommitted change, the refusal
check in `test/source/counters_trap_checked_test.cpp`, which contributes
59 of the 2953 at `test/source/counters_trap_checked_test.cpp`. No
analyzer call was suppressed and no gate moved in reaching this section.

## The cppcheck half of the same gate, measured

Principle VIII's static-analysis clause names two analyzers. The section
on the feature scope's static-analysis figure above carries the
`clang-tidy` half at 734 `warning:` lines over the eleven translation
units. The `cppcheck` half is measured here, at head `9d83823`:

```
cppcheck --inline-suppr -q --force $(find source/counters -name '*.cpp') -I include
```

exits 0 and reports 0 findings. The same launcher form on
`source/counters/plan.cpp` alone exits 0 and prints three progress lines
and no finding. The `Phase 42` preamble at
`specs/007-counters-and-timers/tasks.md:4548-4550` records that the same
command `exits 0 with findings printed`, and no finding is printed at
this head. The preamble keeps its bytes, and the figure that clause
needs is 0 for `cppcheck` over the feature scope and 734 for
`clang-tidy` over the same eleven translation units, so a reader
collecting the report reads one analyzer's findings and none from the
other. No `cppcheck` call was suppressed, no `--error-exitcode` added,
and no configuration value changed in reaching this section.

## The scope the em-dash figure names

The Phase 40 preamble at
`specs/007-counters-and-timers/tasks.md:4593` reads that 0 occurrences
of the em-dash code point stand in every feature-scope file, and the
`T298` text at `:4790` reads the same. Measured at `14b8e48` over
`git ls-files`, the code scope, which is the nine
`include/speedgun-ng/counters*.hpp` headers, everything under
`source/counters/`, the `test/source/counters_*.cpp` units,
`test/compile-fail/`, `test/pmu-events-gate-fixture/`, `example/`,
`tools/pmu_events/`, `docs/pages/counters-overhead.md`, `cmake/`,
`CMakeLists.txt`, `.github/workflows/ci.yml`, and `test/counters_*.sh`,
holds 81 tracked files and 0 occurrences of U+2014. The feature's 12
tracked Markdown artifacts hold 65, 64 in the closed journal above and
1 at `specs/007-counters-and-timers/tasks.md:2731` inside a closed task
line, so a reader who takes that phrase to cover the artifacts reads 0
where 65 stand.

## The three violations `T303` closed

Three Principle XI violations in this feature's live artifacts rode the
row and stood after `T298` closed its three, and `T303` closed these.
Each sentence now states what the thing is, each fact in its own
sentence, and no line count moved in either file, so every anchor into
them keeps its number.

| Site | Token the row dropped | Sentence now reads |
| --- | --- | --- |
| `specs/007-counters-and-timers/plan.md:347` | XI.2 ` rather than ` | `are all measured, and each of the three sits inside the coverage the gate scores` |
| `specs/007-counters-and-timers/plan.md:457` | XI.2 ` rather than ` three times and ` instead of ` twice | each of the five clauses states what the thing is, in its own sentence |
| `specs/007-counters-and-timers/spec.md:33` | XI.2 ` rather than ` | `the regex is recorded where the matched text would go` |
