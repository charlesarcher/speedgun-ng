# citations: corrected anchors and superseded journal sentences

Created by T259 and T260 on 2026-09-28 for the
`007-counters-and-timers` feature. It is the record T253's debt
paragraph names and that no artifact carried. The closed task lines and
the dated phase preambles in `specs/007-counters-and-timers/tasks.md`
keep their bytes, and the corrections a reader needs to reach the code
from a closed task body live here. Two families are recorded.
The first is a `file:line` anchor that no longer lands on the text the
sentence naming it describes. The second is a sentence of the closed
journal `specs/007-counters-and-timers/sg_counters.md` that a later
requirement withdrew.

## Precedence over the closed journal

Where the closed journal's resolved scope statement names a boundary, a
gate, a mechanism, or a manifest that a later requirement in
`specs/007-counters-and-timers/spec.md` withdrew, the requirement in
`spec.md` governs. The five sentences that rule settles are listed under
the second family below. T259 places this rule in the Assumptions
paragraph at `specs/007-counters-and-timers/spec.md:321`, in one
sentence beside the existing sentence that names the journal the
authoritative design record. This pass left `spec.md` byte for byte as
written, so the rule is stated here and the sentence is owed there.

## Method and population

The population is every anchor the Phase 1 through Phase 33 record in
`specs/007-counters-and-timers/tasks.md` places, which is the 724
explicit `path:line` and `path:NN-MM` tokens the file carries, each
resolved to an existing tracked file, plus the 449 bare `:NN`
continuations, each attributed to the path it continues. A continuation
is attributed by hand because a line naming two files attaches the
continuation to the wrong one under an automated pass. Three criteria
decide drift, and every row below was read against the tree as it
stands on 2026-09-28:

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
| T077 | 364 | `source/counters/linux_pmu/fast_read.cpp:283` | `:281`, the `_rdpmc` the hoisted capability gate now precedes | FR-040 | this pass |
| T077 | 364 | `source/counters/linux_pmu/fast_read.cpp:40-42` and `source/counters/detail/pmu.hpp:165-166` | `:41-43` and `pmu.hpp:166-168` | FR-040 | this pass |
| T078 | 366 | `source/counters/linux_pmu/fast_read.cpp:122` | `:267`, where the lock snapshot is read | FR-040 | this pass |
| T078 | 366 | `source/counters/linux_pmu/fast_read.cpp:271` and `:287` | the index compare stands at `:278-281`; `:271` holds the width and `:287` is blank | FR-040 | this pass |
| T079 | 367 | `specs/007-counters-and-timers/plan.md:43` and `:365` | the P2 cast row was withdrawn with the mirror; the protocol stands at `plan.md:363-365` | Constitution I, P2 | this pass |
| T079 | 367 | `source/counters/linux_pmu/fast_read.cpp:191` and `:264-265` | both hold comment text; the page fields are read at `:269-272` | FR-040 | this pass |
| T080 | 368 | `specs/007-counters-and-timers/quickstart.md:108`, named twice | `:109`, where the binary pass check is stated | SC-004 | T253 |
| T080 | 368 | `specs/007-counters-and-timers/spec.md:301` | `:311`, which carries SC-004 | SC-004 | this pass |
| T080 | 368 | `docs/pages/counters-overhead.md:63-94` | the measurement tables stand at `:158-173` | SC-004 | this pass |
| T100 | 391 | `include/speedgun-ng/counters_measurement.hpp:264` and `:317` | `:275` and `:328` | FR-035 | T253 |
| T100 | 391 | `source/counters/push_provider.cpp:31` | `:45`, the plain load that never performs an atomic read-modify-write | FR-035 | T253 |
| T101 | 391 | `source/counters/system.cpp:377` | `:163`, where the seed's flag reaches the catalog entry | US4 scenario 5 | this pass |
| T101 | 391 | `source/counters/clock_provider.cpp:236` | `:232` | FR-034 | this pass |
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
| Phase 22 preamble | 1180 | `.specify/memory/constitution.md:568-570` | `:571-573`, the machine-local `CMakeUserPresets.json` sentence | Constitution IX | this pass |
| T223 | 1805 | `specs/007-counters-and-timers/contracts/measurement-contract.md:107` | `:110`, the single-target sentence | FR-024 | T253 |

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
