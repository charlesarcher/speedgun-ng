#!/usr/bin/env bash
# The command lines of the docs_command_line example, in the words a reader
# types. Every flag stands spelled out, and the file runs from a configured
# tree that has built the example.

set -eu

# sg-example:begin options
./build/example/docs_command_line --list
./build/example/docs_command_line --filter='bmWrite' --min-time=0.2
./build/example/docs_command_line --counter=machine/monotonic --dry-run
./build/example/docs_command_line --catalog
printf '%s\n' 'filter=bmWrite' 'min-time=0.2' > run.options
./build/example/docs_command_line --options=run.options
SPEEDGUN_FILTER=bmWrite SPEEDGUN_MIN_TIME=0.2 ./build/example/docs_command_line
./build/example/docs_command_line --threads=1,4
./build/example/docs_command_line --threads=2:16
./build/example/docs_command_line --interleave=7
printf '%s\n' 'time@median:lower:5%' > rules.txt
./build/example/docs_command_line \
  --compare=test/data/gate_ok_base.json,test/data/gate_ok_contender.json \
  --compare-rules=rules.txt
# sg-example:end options
