# Quickstart: Full Command Line

**Date:** 2026-10-22 · **Spec:** [spec.md](./spec.md)

A reviewer drives the whole feature from a fresh clone with the
checked-in release preset. The `dev` presets are machine-local and
absent from a fresh clone, so every command here runs `ci-rocky`.

## Prerequisites

A CMake 3.20 or newer generator for a release developer-mode build, and
a Linux host. Nothing else: simdjson 4.6.11 is vendored at
`external/simdjson`, and `getopt_long` comes from the C library the
harness already calls at `source/harness/cli.cpp:377`.

## Configure and build

```sh
cmake --preset=ci-rocky
cmake --build build
```

The harness archive grows by one translation unit,
`source/harness/options.cpp`, through the `GLOB_RECURSE` at
`CMakeLists.txt:196` to `:199`; no CMake edit is needed for it.

## The feature's own tests

```sh
ctest --test-dir build --output-on-failure \
  -R 'harness_cli_table_test|harness_cli_sources_test'
ctest --test-dir build --output-on-failure \
  -R 'harness_cli_threads_test|harness_cli_interleave_test'
ctest --test-dir build --output-on-failure -R base_cli_matrix
```

The first pair drives the table, the scan and the gather with `argv`
arrays, environment vectors and file texts built in the test, so no case
reads the process environment or the file system (FR-012).
`base_cli_matrix` runs the built binary over the twenty-six command lines
of `test/data/base_cli_matrix.txt` and compares each status and fault
fragment, which is the standing check that the master behavior of the
fifteen command lines the base runs and the eleven it refuses did not
move (FR-003, FR-059, SC-007).

## The table, by hand

```sh
./build/example/docs_command_line --help
./build/example/docs_command_line --version
echo $?
```

The help prints one line per table row in table order, the name with its
argument kind and the row's help text, and the version line answers
`speedgun-ng <version>` at status 0 (FR-015, FR-016).

```sh
./build/example/docs_command_line --fil=x
./build/example/docs_command_line --i
./build/example/docs_command_line --filter
echo $?
```

Each answers one line on standard error at status 2: the unknown name
with the nearest table name, the ambiguous prefix with its candidates in
table order, and the missing value (FR-007 to FR-009). The base printed
two lines for each of these, the C library's own diagnostic beside the
house line; the scan sets `opterr = 0` and the second line is gone
(research D-06).

## The environment source

```sh
SPEEDGUN_MIN_TIME=0.2 SPEEDGUN_FILTER=bmWrite \
  ./build/example/docs_command_line
```

The run takes its minimum time and its filter from the environment, and
the console opens with the disclosure line naming both, in table order
(FR-019, FR-039). A shell sets these for one command; no test does
(FR-081).

```sh
SPEEDGUN_NO_PERCENTILES=yes ./build/example/docs_command_line
```

One line naming the variable and its value, at status 2 (FR-025).

## The option file

```sh
printf '# the run of the morning\nmin-time=0.2\nfilter=bmWrite\n' > /tmp/run.opts
./build/example/docs_command_line --options=/tmp/run.opts
./build/example/docs_command_line --options=/tmp/missing.opts
echo $?
```

The file sets its options under the command-line rules, a comment holds
a line number, and the missing path answers one line naming it at status
2 (FR-029 to FR-034). A second `--options` on one command line answers
one line naming the option (FR-032).

## Threads and interleaving

```sh
./build/example/docs_command_line --threads=1:4 --list
./build/example/docs_command_line --interleave --filter='bm'
```

The thread list replaces the registered counts of every family before
the expansion, and the names carry `/threads:<n>` above one (FR-044,
FR-048). The interleaved run walks the selected instances in the seeded
order and prints its rows, and each family's fit, in registration order
after the run (FR-051, FR-056). `--interleave=7` states its seed in the
console line and in the `run` object (FR-058).

## The rules file

```sh
printf 'time@median:lower:5%%\n# the memory rule\ncounter:bytes@mean:upper:10%%\n' > /tmp/rules.txt
./build/example/docs_command_line \
  --compare=test/data/gate_ok_base.json,test/data/gate_ok_contender.json \
  --compare-rules=/tmp/rules.txt
```

The rules read in the specs/026 grammar, a comment and a blank line hold
line numbers, and a refused line names the file, the line number and the
parser's own text at status 2 (FR-061 to FR-065).

## The disclosure in the document

```sh
SPEEDGUN_REPETITIONS=3 ./build/example/docs_command_line --json=/tmp/report.json
python3 -c "import json;d=json.load(open('/tmp/report.json'));print(d['run'])"
ctest --test-dir build --output-on-failure -R 'report_shape'
```

The document carries `run` between `speedgun` and `benchmarks`, with the
seed and one entry per disclosed option in table order, and the schema
lists it as an optional key with open members (FR-040, FR-043). A run
with no file, no environment and no interleaving writes no such key
(FR-041).

## The gates

```sh
ctest --test-dir build --output-on-failure \
  -R 'cli_shape|loop_shape|time_source_gate|example_drift|report_shape|snippet_compile'
cmake --build build --target dbc-gate
cmake --build build --target spell-check
ctest --test-dir build --output-on-failure
```

The three shape gates run with no edit: the walk order and the held-row
print stand outside the loop range `test/cli_shape.sh:32` reads, and the
timed loop is untouched (FR-060, FR-083). The drift gate reads the table
at its new path and the fixture harness proves the option-row check can
still fail (FR-075). The prose gate runs through its CMake target once
the artifacts are committed, because its default mode reads a changed
range:

```sh
cmake --build build --target prose-lint
python3 tools/prose/prose_gate.py --mode tree --paths specs/028-full-command-line
```

The coverage preset closes the feature:

```sh
cmake --preset=ci-coverage
cmake --build build/coverage
ctest --test-dir build/coverage --output-on-failure
cmake --build build/coverage --target coverage
```

100% of the new lines and branches are covered and no `LCOV_EXCL` marker
stands under `source/harness/` (FR-080, SC-010).
