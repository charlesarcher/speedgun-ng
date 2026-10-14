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
# sg-example:end options
