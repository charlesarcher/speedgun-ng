#!/usr/bin/env bash
# The acceptance script of specs/043-fwq-ftq (FR-027, FR-028, R-21).
# It runs the fixed work quanta probe and the fixed time quanta probe on
# one idle cpu of each L3 group, at rest and under a load pinned to the
# SMT sibling thread, and it records the host state the figures need. It
# is a script and never a CI gate: the acceptance host is the only host
# it runs on, and CI never invokes it (SC-008).
#
# The window comes from a named benchmark run with `--min-time=10` and
# `--repetitions=1`, and the probe window stands at `precheck=10000`:
# ten seconds on one probe cpu (R-21). The `gaps=` value prices that
# span, 10,485,760 entries of one microsecond each for FWQ and 131,072
# entries of one quantum each for FTQ (FR-027).
#
# The script expects a configured build in build/ with the benchmark
# binary present (`cmake --preset=ci-rocky && cmake --build build`).

set -u
set -o pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="$ROOT/build/example"
OUT="$ROOT/specs/043-fwq-ftq/results"

STAMP="$(date +%F)"
RUN_ID="$(date -u +%Y%m%dT%H%M%SZ)"
RECORD="$OUT/acceptance-$STAMP.md"

mkdir -p "$OUT"

# One run() per command: the script appends the section heading, the
# facts of the section, the command line, the exit status, and the
# captured output to the record, then keeps going. META carries the
# facts, and LOG keeps the captured output for the figures block.
#
# The benchmark stands unbuffered: a run that dies on a signal leaves
# its stdio buffer behind, and the record loses every figure the run
# printed. stdbuf keeps the record whole for such a run.
META=""
LOG=""
STATUS=0

run() {
  local name="$1"
  shift
  {
    printf '\n## %s\n\n' "$name"
    if [ -n "$META" ]; then
      printf '%s\n\n' "$META"
    fi
    printf '```sh\n'
    printf '%q ' "$@"
    printf '\n```\n\n'
  } >> "$RECORD"
  LOG="$(mktemp)"
  STATUS=0
  "$@" > "$LOG" 2>&1 || STATUS=$?
  {
    printf 'Exit status: %s\n\n```text\n' "$STATUS"
    cat "$LOG"
    printf '```\n'
  } >> "$RECORD"
  META=""
  return 0
}

# field reads the last line of the captured output that opens with the
# name: the run-level block stands after the benchmark tables.
field() {
  awk -v key="$1:" '$1 == key { value = $2 }
       END { print (value == "" ? "absent" : value) }' "$LOG"
}

# span_ms states the kept span in milliseconds, the unit the record
# names it in (FR-027).
span_ms() {
  awk -v ns="$1" 'BEGIN {
    if (ns ~ /^[0-9]+$/) printf "%.1f", ns / 1000000; else printf "absent"
  }'
}

# periods names the five detected periods, strongest first.
periods() {
  local out="" i v sep=""
  for i in 1 2 3 4 5; do
    v="$(field "period${i}Ns")"
    out="$out$sep rank $i at $v ns"
    sep=","
  done
  printf '%s' "$out"
}

# figures names the figures FR-027 asks each section to carry, read from
# the captured output of the section above it.
figures() {
  local gaps="$1"
  {
    printf '\nThe figures of this section:\n\n'
    printf -- '- calibration count (`workCount`): %s\n' "$(field workCount)"
    printf -- '- measured unit time (`unitTimeNs`): %s ns\n' "$(field unitTimeNs)"
    printf -- '- noise fraction (`noisePercent`): %s\n' "$(field noisePercent)"
    printf -- '- `gaps=` value: %s\n' "$gaps"
    printf -- '- kept span (`seriesRuntimeNs`): %s ms\n' \
      "$(span_ms "$(field seriesRuntimeNs)")"
    printf -- '- drop count (`droppedSamples`): %s\n' "$(field droppedSamples)"
    printf -- '- detected periods (`periodNs`):%s\n' "$(periods)"
  } >> "$RECORD"
}

# The load is a busy loop pinned to the SMT sibling of the probe cpu
# (FR-028). The record names its command line and its cpu id.
LOAD_PID=""
LOAD_CMD=""

start_load() {
  local cpu="$1"
  LOAD_CMD="taskset -c $cpu sh -c 'while :; do :; done'"
  taskset -c "$cpu" sh -c 'while :; do :; done' &
  LOAD_PID=$!
}

stop_load() {
  if [ -n "$LOAD_PID" ]; then
    kill "$LOAD_PID" 2>/dev/null
    wait "$LOAD_PID" 2>/dev/null
  fi
  LOAD_PID=""
}

# cpu_ids lists every online cpu id in numeric order.
cpu_ids() {
  local d
  for d in /sys/devices/system/cpu/cpu[0-9]*; do
    printf '%s\n' "${d##*/cpu}"
  done | sort -n
}

# l3_group_ids lists the L3 group ids in the order their lowest cpu
# stands, one group per probe cpu (FR-027).
l3_group_ids() {
  local c id out=""
  for c in $(cpu_ids); do
    id="$(cat "/sys/devices/system/cpu/cpu$c/cache/index3/id" 2>/dev/null)"
    [ -n "$id" ] || continue
    case " $out " in
      *" $id "*) ;;
      *) out="$out $id" ;;
    esac
  done
  printf '%s' "${out# }"
}

# probe_cpu_of answers the lowest cpu of the L3 group named by $1.
probe_cpu_of() {
  local c id
  for c in $(cpu_ids); do
    id="$(cat "/sys/devices/system/cpu/cpu$c/cache/index3/id" 2>/dev/null)"
    if [ "$id" = "$1" ]; then
      printf '%s' "$c"
      return 0
    fi
  done
  printf 'none'
}

# l3_cpus_of answers the cpu list the L3 cache of cpu $1 is shared over.
l3_cpus_of() {
  cat "/sys/devices/system/cpu/cpu$1/cache/index3/shared_cpu_list" 2>/dev/null
}

# sibling_of answers the SMT sibling of cpu $1, the cpu the load runs on.
sibling_of() {
  local c="$1" sibs part s other
  sibs="$(cat "/sys/devices/system/cpu/cpu$c/topology/thread_siblings_list" 2>/dev/null)"
  [ -n "$sibs" ] || {
    printf 'none'
    return 0
  }
  local IFS=','
  for part in $sibs; do
    if [ "$part" != "$c" ]; then
      printf '%s' "$part"
      return 0
    fi
    other="$part"
  done
  for part in $sibs; do
    case "$part" in
      *-*)
        for s in $(seq "${part%-*}" "${part#*-}"); do
          if [ "$s" != "$c" ]; then
            printf '%s' "$s"
            return 0
          fi
        done
        ;;
    esac
  done
  printf '%s' "${other:-none}"
}

# CONFIG_HZ comes from `/proc/config.gz`, then `/boot/config-$(uname -r)`,
# and the record reads `unknown` when both stand absent (FR-027).
HZ="unknown"
HZ_SOURCE="both sources stand absent"

config_hz_from() {
  local file="$1" reader="cat" value=""
  [ -r "$file" ] || return 1
  case "$file" in
    *.gz) reader="zcat" ;;
  esac
  value="$("$reader" "$file" 2>/dev/null | sed -n 's/^CONFIG_HZ=\(.*\)/\1/p' |
    head -n 1)"
  [ -n "$value" ] || return 1
  HZ="$value"
  HZ_SOURCE="$file"
  return 0
}

if ! config_hz_from /proc/config.gz; then
  config_hz_from "/boot/config-$(uname -r)" || true
fi

# The probe mode and the window length: the pre-check window of the
# named run, ten seconds long, the span the `gaps=` values price.
PROBE_MODE="precheck"
PRECHECK_MS="10000"

GROUP_IDS="$(l3_group_ids)"
MODEL="$(lscpu 2>/dev/null | sed -n 's/^Model name:[[:space:]]*//p')"
CORES="$(lscpu 2>/dev/null | sed -n 's/^Core(s) per socket:[[:space:]]*//p')"
THREADS="$(lscpu 2>/dev/null | sed -n 's/^Thread(s) per core:[[:space:]]*//p')"

: > "$RECORD"

cat >> "$RECORD" <<EOF
# acceptance run: fwq and ftq probes (specs/043-fwq-ftq)

Run id: $RUN_ID
Script: tools/noise/fwq_ftq_acceptance.sh (FR-027, FR-028, SC-008)

### Host state

- Date: $(date +%F)
- Host: $(uname -n)
- Kernel: $(uname -srm)
- CONFIG_HZ: $HZ (source: $HZ_SOURCE)
- Machine: $MODEL, $CORES cores per socket, $THREADS threads per core
- L3 group ids: ${GROUP_IDS// /, }
- Probe cpus: $(for id in $GROUP_IDS; do
    c="$(probe_cpu_of "$id")"
    printf 'cpu %s of L3 group %s over cpus %s; ' "$c" "$id" "$(l3_cpus_of "$c")"
  done)
- Load: a busy loop pinned to the SMT sibling of the probe cpu, run as
  \`taskset -c <cpu id> sh -c 'while :; do :; done'\`
- SMT state: $(cat /sys/devices/system/cpu/smt/active 2>/dev/null || echo unknown)
- CPU frequency governor: $(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null || echo unknown)
- Huge pages: $(cat /sys/kernel/mm/transparent_hugepage/enabled 2>/dev/null || echo unknown)
- Kernel command line: \`$(cat /proc/cmdline 2>/dev/null)\`
- isolated cpus: \`$(cat /sys/devices/system/cpu/isolated 2>/dev/null; echo)\`
- nohz_full cpus: \`$(cat /sys/devices/system/cpu/nohz_full 2>/dev/null; echo)\`

The two L3 groups carry the acceptance axis of the lane host (D-10):
FWQ and FTQ each run on one idle cpu of each group, at rest and under a
load on the SMT sibling thread, eight sections. The isolated and
nohz_full lists stand empty when the kernel command line carries no
isolcpus and no nohz_full setting, so the probe shares the machine with
the ordinary host load.
EOF

for method in fwq ftq; do
  gaps=10485760
  if [ "$method" = ftq ]; then
    gaps=131072
  fi
  for id in $GROUP_IDS; do
    cpu="$(probe_cpu_of "$id")"
    cpus="$(l3_cpus_of "$cpu")"
    sib="$(sibling_of "$cpu")"
    for load in idle loaded; do
      if [ "$load" = loaded ]; then
        start_load "$sib"
      fi
      META="- probe cpu: $cpu
- L3 group: $id over cpus $cpus
- load state: $load
- gaps: $gaps"
      if [ "$load" = loaded ]; then
        META="$META
- load cpu: $sib, the SMT sibling of probe cpu $cpu
- load command line: \`$LOAD_CMD\`"
      else
        META="$META
- load command line: none"
      fi
      run "${method^^} on cpu $cpu of L3 group $id, $load" \
        stdbuf --output=0 "$BIN/benchmark_example" \
        --filter=bmTouch --min-time=10 --repetitions=1 \
        --jitter-probe="$cpu,method=$method,mode=$PROBE_MODE,precheck=$PRECHECK_MS,gaps=$gaps"
      if [ "$load" = loaded ]; then
        stop_load
      fi
      figures "$gaps"
      rm -f "$LOG"
    done
  done
done

cat >> "$RECORD" <<'EOF'

### Result

The eight sections above carry their command lines, exit statuses and
captured output. The figures block of each section names the calibration
count, the measured unit time, the noise fraction, the `gaps=` value,
the kept span in milliseconds, the drop count and the detected periods
(FR-027, FR-028, SC-008).
EOF

echo "record: $RECORD"
