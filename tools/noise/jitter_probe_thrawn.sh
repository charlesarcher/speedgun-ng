#!/usr/bin/env bash
# The thrawn acceptance script of specs/027-jitter-probe (FR-042, FR-043).
# It runs the N4 preset placement check, the N13-V probes, and it records
# the host state the results need. It is a script and never a CI gate:
# thrawn is the only host it runs on, and CI never invokes it (SC-008).
#
# The spec names each N4 preset with `--threads` at 24 and at 48. The CLI
# of this tree carries no `--threads` option: the option table of
# source/harness/cli.cpp lists no such entry, contracts/cli-options.md
# does not declare one, and no requirement of the spec adds one. The
# N4 check below therefore runs each preset at the context counts the
# registered benchmarks carry, and the results record states the cause
# beside the captured output.
#
# The script expects a configured build in build/ with the example
# binaries present (`cmake --preset=ci-rocky && cmake --build build`).

set -u
set -o pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="$ROOT/build/example"
OUT="$ROOT/specs/027-jitter-probe/results"

STAMP="$(date +%F)"
RUN_ID="$(date -u +%Y%m%dT%H%M%SZ)"
RECORD="$OUT/thrawn-acceptance-$STAMP.md"

mkdir -p "$OUT"

# One run() per command: the script appends the command line, the exit
# status, and the captured output to the record, then keeps going.
run() {
  local name="$1"
  shift
  {
    printf '\n## %s\n\n```sh\n' "$name"
    printf '%q ' "$@"
    printf '\n```\n\n'
  } >> "$RECORD"
  local log
  log="$(mktemp)"
  local status=0
  "$@" > "$log" 2>&1 || status=$?
  {
    printf 'Exit status: %s\n\n```text\n' "$status"
    cat "$log"
    printf '```\n'
  } >> "$RECORD"
  rm -f "$log"
  return 0
}

: > "$RECORD"

cat >> "$RECORD" <<EOF
# thrawn acceptance run: jitter probe (specs/027-jitter-probe)

Run id: $RUN_ID
Script: tools/noise/jitter_probe_thrawn.sh (FR-042, FR-043, SC-008)

## Host state

EOF

{
  printf -- '- Date: %s\n' "$(date +%F)"
  printf -- '- Kernel: %s\n' "$(uname -srm)"
  printf -- '- SMT state: %s\n' \
    "$(cat /sys/devices/system/cpu/smt/active 2>/dev/null || echo unknown)"
  printf -- '- CPU frequency governor: %s\n' \
    "$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null \
       || echo unknown)"
  printf -- '- Huge pages: %s\n' \
    "$(cat /sys/kernel/mm/transparent_hugepage/enabled 2>/dev/null \
       || echo unknown)"
  printf -- '- Kernel command line: `%s`\n' \
    "$(cat /proc/cmdline 2>/dev/null)"
  printf -- '- isolated cpus: `%s`\n' \
    "$(cat /sys/devices/system/cpu/isolated 2>/dev/null; echo)"
  printf -- '- nohz_full cpus: `%s`\n' \
    "$(cat /sys/devices/system/cpu/nohz_full 2>/dev/null; echo)"
} >> "$RECORD"

{
  printf '\nThe visible BIOS-side settings stand in the mitigation '
  printf 'records the kernel exposes:\n\n'
  for f in /sys/devices/system/cpu/vulnerabilities/*; do
    printf -- '- %s: %s\n' "$(basename "$f")" "$(cat "$f")"
  done
} >> "$RECORD"

cat >> "$RECORD" <<'EOF'

The machine: AMD Ryzen Threadripper 9960X, 24 cores, 48 threads, four
L3 groups. The isolated and nohz_full lists stand empty: the kernel
command line carries no isolcpus and no nohz_full setting, so the
probe shares the machine with the ordinary host load (FR-043).

## The N4 preset check

Each N4 preset runs on `docs_threads`, whose `bmSharded` registers
four contexts. The report discloses the planned cpu and the L3 group
of each context (FR-042). The `--threads` dimension of FR-042 stands
unrunnable on this tree: the long option table of
`source/harness/cli.cpp` (the `getopt_long` call site) carries no
`threads` entry, `contracts/cli-options.md` declares none, and no
requirement of the spec adds one. The runs below cover the presets at
the context counts the registered benchmarks carry, and the `smt`
preset run shows the standing refusal when a plan cannot place every
requested context.
EOF

for preset in none core l3 smt compact; do
  run "N4 preset $preset" \
    "$BIN/docs_threads" --placement="$preset" --filter=bmSharded
done

cat >> "$RECORD" <<'EOF'

## The N13-V probes

The idle preset resolves one probe cpu per L3 group (FR-016): the
lowest free cpu of each group the plans touch.
EOF

run "N13-V idle one probe per L3 group" \
  "$BIN/docs_threads" --placement=l3 --filter=bmSharded \
  --jitter-probe=idle

cat >> "$RECORD" <<'EOF'

The overlap check: cpu 0 is a planned cpu of the l3 plan, so the
alongside run refuses it with one line and exit status 2 before any
benchmark runs (FR-018, D-3).
EOF

run "N13-V overlap error on a planned cpu" \
  "$BIN/docs_threads" --placement=l3 --filter=bmSharded \
  --jitter-probe=0

cat >> "$RECORD" <<'EOF'

The overhead measurement (D-8): the wall time of the same benchmark
run with and without the probe alongside. `bmTouch` runs unpinned, so
the with-probe run narrows the process affinity mask around the probe
cpu and discloses the change (D-5, FR-019).
EOF

{
  printf '\n```text\n'
  {
    printf 'without probe: '
    TIMEFORMAT='%R s'
    { time "$BIN/benchmark_example" --filter=bmTouch > /dev/null; } 2>&1
  }
  {
    printf 'with probe:    '
    TIMEFORMAT='%R s'
    { time "$BIN/benchmark_example" --filter=bmTouch \
        --jitter-probe=2 > /dev/null; } 2>&1
  }
  printf '```\n'
} >> "$RECORD"

run "N13-V full report with the probe" \
  "$BIN/benchmark_example" --filter=bmTouch --jitter-probe=2

cat >> "$RECORD" <<'EOF'

## Result

The runs above carry their exit status and captured output inline.
The host state of this record and the gap note on the `--threads`
dimension close what FR-043 asks the results to carry.
EOF

echo "record: $RECORD"
