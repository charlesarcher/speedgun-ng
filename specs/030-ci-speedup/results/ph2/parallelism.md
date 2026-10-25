# Phase 2 Parallelism Sites

**Surface**: every build and test parallelism flag in
`.github/workflows/ci.yml` (T007).
**Head sha**: b6c11cd7b4c6009d043221a9d6467edc4b60ffdf
**Run id**: local
**Command**: the command block below, each line run from the repository
root
**Numbers**: the table under Results, one row per numbered check

All twenty `-j 2` sites now carry the runner's cpu count, except the
instrumented suite of the coverage lane, which goes to `-j 1` (FR-001,
FR-002, FR-044, P-11). One file changed: `.github/workflows/ci.yml`. No
library source, preset, or test registration is touched here.

## The sites

Twenty sites, nineteen at `-j "$(nproc)"`, one at `-j 1`. Each entry is
the job, the step, and the line that carries the flag, spelled as the
file spells it. Where a step chains commands with `&&`, the chain is
shown whole, because the flag sits mid-chain.

```text
coverage / Build
  before  cmake --build build/coverage -j 2
  after   cmake --build build/coverage -j "$(nproc)"
coverage / Test
  before  ctest -j 2 --output-on-failure --no-tests=error
  after   ctest -j 1 --output-on-failure --no-tests=error
sanitize / Build
  before  cmake --build build/sanitize -j 2
  after   cmake --build build/sanitize -j "$(nproc)"
sanitize / Test
  before  ctest --output-on-failure --no-tests=error -j 2
  after   ctest --output-on-failure --no-tests=error -j "$(nproc)"
tsan / Build
  before  cmake --build build/tsan -j 2
  after   cmake --build build/tsan -j "$(nproc)"
tsan / Test
  before  ctest --output-on-failure --no-tests=error -j 2
  after   ctest --output-on-failure --no-tests=error -j "$(nproc)"
test / Build
  before  cmake --build build --config Release -j 2
  after   cmake --build build --config Release -j "$(nproc)"
test / Test
  before  ctest --output-on-failure --no-tests=error -C Release -j 2
  after   ctest --output-on-failure --no-tests=error -C Release -j "$(nproc)"
test-rocky / Build
  before  cmake --build build --config Release -j 2
  after   cmake --build build --config Release -j "$(nproc)"
test-rocky / Test
  before  ctest --output-on-failure --no-tests=error -C Release -j 2
  after   ctest --output-on-failure --no-tests=error -C Release -j "$(nproc)"
shared-audit / Build
  before  cmake --build build-shared -j 2
  after   cmake --build build-shared -j "$(nproc)"
downstream-consumer / Install a system HdrHistogram_c for the consumer premise
  before  cmake -S external/hdrhistogram_c -B sys-hdr -DHDR_HISTOGRAM_BUILD_PROGRAMS=OFF -DCMAKE_BUILD_TYPE=Release
          && cmake --build sys-hdr -j 2
          && sudo cmake --install sys-hdr --prefix /usr/local
  after   cmake -S external/hdrhistogram_c -B sys-hdr -DHDR_HISTOGRAM_BUILD_PROGRAMS=OFF -DCMAKE_BUILD_TYPE=Release
          && cmake --build sys-hdr -j "$(nproc)"
          && sudo cmake --install sys-hdr --prefix /usr/local
downstream-consumer / Install a system yaml-cpp for the consumer premise
  before  cmake -S external/yaml-cpp -B sys-yaml -DYAML_CPP_BUILD_TESTS=OFF -DYAML_CPP_BUILD_TOOLS=OFF
          && cmake --build sys-yaml -j 2
          && sudo cmake --install sys-yaml --prefix /usr/local
  after   cmake -S external/yaml-cpp -B sys-yaml -DYAML_CPP_BUILD_TESTS=OFF -DYAML_CPP_BUILD_TOOLS=OFF
          && cmake --build sys-yaml -j "$(nproc)"
          && sudo cmake --install sys-yaml --prefix /usr/local
downstream-consumer / Install a system quill for the consumer premise
  before  cmake -S external/quill -B sys-quill -DQUILL_ENABLE_INSTALL=ON -DQUILL_BUILD_EXAMPLES=OFF -DQUILL_BUILD_TESTS=OFF -DQUILL_BUILD_BENCHMARKS=OFF -DCMAKE_BUILD_TYPE=Release
          && cmake --build sys-quill -j 2
          && sudo cmake --install sys-quill --prefix /usr/local
  after   cmake -S external/quill -B sys-quill -DQUILL_ENABLE_INSTALL=ON -DQUILL_BUILD_EXAMPLES=OFF -DQUILL_BUILD_TESTS=OFF -DQUILL_BUILD_BENCHMARKS=OFF -DCMAKE_BUILD_TYPE=Release
          && cmake --build sys-quill -j "$(nproc)"
          && sudo cmake --install sys-quill --prefix /usr/local
downstream-consumer / Install library
  before  cmake --preset=ci-linux-audit -B build -D CMAKE_BUILD_TYPE=Release
          && cmake --build build -j 2
          && cmake --install build --prefix prefix
  after   cmake --preset=ci-linux-audit -B build -D CMAKE_BUILD_TYPE=Release
          && cmake --build build -j "$(nproc)"
          && cmake --install build --prefix prefix
consumer-release / Build consumer library
  before  cmake --build build/consumer-release -j 2
  after   cmake --build build/consumer-release -j "$(nproc)"
consumer-release / Build trap fixture and dbc_test
  before  cmake --build build/consumer-release-dev -j 2 --target dbc_trap_fixture counters_trap_fixture counters_trap_noguard_fixture dbc_test
  after   cmake --build build/consumer-release-dev -j "$(nproc)" --target dbc_trap_fixture counters_trap_fixture counters_trap_noguard_fixture dbc_test
consumer-release / Run counters_trap_checked under ignore
  before  cmake --build build/consumer-release-dev -j 2 \
  after   cmake --build build/consumer-release-dev -j "$(nproc)" \
dbc-gate / Build dbc-gate
  before  cmake --build build/dev -t dbc-gate -j 2
  after   cmake --build build/dev -t dbc-gate -j "$(nproc)"
dbc-gate / Test dbc gate fixtures
  before  ctest -R dbc_gate_fixtures --output-on-failure --no-tests=error -j 2
  after   ctest -R dbc_gate_fixtures --output-on-failure --no-tests=error -j "$(nproc)"
```

## The commands

```text
cd /home/archerc/code/speedgun-ng
python3 -c "import yaml; print(len(yaml.safe_load(open('.github/workflows/ci.yml'))['jobs']))"
grep -c -- '-j 2' .github/workflows/ci.yml
grep -c -- '-j "$(nproc)"' .github/workflows/ci.yml
grep -n -- '-j 1' .github/workflows/ci.yml
bash -n <each changed run block, written to a temporary file>
python3 tools/prose/prose_gate.py --check prose --mode tree --paths specs/030-ci-speedup/results
codespell specs/030-ci-speedup/results/
```

## Results

| Check | Expect | Exit | Decisive line | Verdict |
| --- | --- | --- | --- | --- |
| 1 jobs parsed | 12 | 0 | 12 | PASS |
| 2 sites still at `-j 2` | prints 0 | 1 | grep exits 1 on a zero count | PASS |
| 3 sites at `-j "$(nproc)"` | 19 | 0 | 19 | PASS |
| 4 sites at `-j 1` | one, inside `coverage` | 0 | 71: run: ctest -j 1 --output-on-failure --no-tests=error | PASS |
| 5 parsed step count with a `-j` flag | 20, one at `-j 1` | 0 | 20 sites parsed, none mismatched | PASS |
| 6 changed run blocks parse as shell | exit 0 for all 20 | 0 | no output from `bash -n` | PASS |
| 7 prose gate | 0 findings | 0 | see the run below the table | PASS |
| 8 codespell | 0 hits | 0 | see the run below the table | PASS |

Checks 5 and 6 re-read the workflow through the YAML parser rather than
through grep, so a folded `>-` run block whose flag lands mid-chain is
counted on the same footing as a one-line `run:`. The parsed flag value
is compared against the expected one per job, so a site that kept `-j 2`
or took `-j 1` outside the coverage lane would fail check 5 even if the
grep counts happened to agree.
