# The CI run of the pushed head (T042)

The run `40912750824` of the Continuous Integration workflow read the
pushed head `c6ae33e3` end to end on pull request #58, every job
green: `lint`, `test` (145 of 145), `test-rocky` (145 of 145, the
root runner), `sanitize`, `tsan`, the three `static-analysis` jobs,
`coverage`, `dbc-gate`, `prose-lint`, `shared-audit`,
`downstream-consumer` and `consumer-release`. The `coverage` job
reads 100.0% lines (13300 of 13300) and 100.0% branches on its own
counter, with a branch shortfall of 18 rows against the D-14 recorded
allowance of 19. The `docs` job skips on a pull request by design:
it deploys the gh-pages site from a master push alone.

The run's own fixes, recorded for the reader who diffs the branch:
`9843f0b4` root-proofs the tracer write denials the Rocky root
runner exposed; `d471f620` covers the branch rows the lane owns;
`c6ae33e3` walks the setup's write-failure orders the root-proof
denials left, so `noise_tracer.cpp` reads 100% lines and branches.

Run reference: https://github.com/charlesarcher/speedgun-ng/actions/runs/40912750824
