# Quickstart: Hunter Budgets and Promotion (033)

Validation guide for the feature; the artifacts it proves against:
[spec.md](./spec.md), [contracts/](./contracts/),
[data-model.md](./data-model.md).

## Prerequisites

Linux with GCC or Clang and CMake ≥ 3.20 (the supported platform).
No PMU access is needed for any step below except the acceptance
table: the tests run on the scripted `FakeProvider` host.

## Build and run the suite

```sh
cmake --preset=ci-rocky
cmake --build build
ctest --test-dir build --output-on-failure      # the full suite
ctest --test-dir build -R hunt --output-on-failure   # the hunt areas
```

Expected: every test passes; the hunt areas include
`harness_hunt_cli_test`, `harness_hunt_plan_test`,
`harness_hunt_json_test`, `harness_hunt_report_test` and
`harness_cli_table_test`.

## Try the budgets (scripted, any host)

The grammar and the plan decisions are pinned in
`harness_hunt_cli_test` and `harness_hunt_plan_test`; run them by
name:

```sh
ctest --test-dir build -R "hunt_cli_test|hunt_plan_test" -V
```

Expected shapes (contracts/budget-grammar.md):

- `--hunt-budget=120s,0.5e,32g` settles all three budgets;
- `120s` and `8p` parse exactly as before;
- `0.5e,2e`, `120s,,32g`, `120x` each print one line naming the
  value and exit 2.

## Try the round trip (scripted, criterion 12.7-6)

`harness_hunt_cli_test` carries the pair on the scripted host: the
hunt writes the emitted file, the re-read run reports one column per
address, none `ABSENT`.

```sh
ctest --test-dir build -R hunt_cli_test -V
```

On a real host the same pair runs as:

```sh
./build/example/docs_throughput --filter=<one instance> \
    --hunt=all --hunt-budget=120s,0.5e,32g --hunt-emit-set=culprits.txt
./build/example/docs_throughput --filter=<one instance> --counter=@culprits.txt
```

## Zen 5 acceptance (gary, FR-045)

Record the table into `specs/033-hunter-budgets/results/` in the
spec 034 shape (scenario, command, exit, observation):

| Scenario | Command | Exit | Observation |
| --- | --- | --- | --- |
| Budgeted hunt | `./build/example/docs_throughput --filter=<one> --hunt=all --hunt-budget=120s,0.5e,32g ...` | 0 | plan line names `budget=`, `sample=`, and `mux=` (the standing clause texts); the groups disclosure stands absent (gary publishes limits, the settlement is `PASSES`; SC-003's seven groups stand in the scripted fixture) |
| Emitted set | same run, `--hunt-emit-set=culprits.txt` | 0 | file holds the ranked addresses, best first |
| Read back | `./build/example/docs_throughput --filter=<one> --counter=@culprits.txt ...` | 0 | one counter column per address, none `ABSENT` |
| Conflict | `./build/example/docs_throughput --hunt-emit-set=x.txt` | 2 | one line naming the value |

No Intel run stands recorded (corsairone down; the record says so).

## Gates

Once per feature build `ci-ubuntu` (clang-tidy, cppcheck) and keep
the hard gates green: coverage 100/100 with no marker, sanitizers,
tsan, `format-check`, `spell-check`, `prose-lint`, the docs gates
with the extended `hunt_rank` example, and the suite under
`taskset -c 0,1`.
