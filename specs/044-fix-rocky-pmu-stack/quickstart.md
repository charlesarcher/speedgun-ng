# Quickstart: the Rocky PMU stack fix

Validation a reviewer runs at the feature head. Every step is a local
command; no GitHub runner result is evidence of a fix or of a bug
(FR-017). The two test targets are `counters_linux_pmu_seam_test`, the
B-1 seam suite, and `harness_jitter_window_test`, the B-2 gate suite.
Both carry the frameworkless `check()` convention and exit 0 on a pass.

## Build the two test targets

```sh
cmake --preset=ci-rocky
cmake --build build --target counters_linux_pmu_seam_test \
  harness_jitter_window_test -j "$(nproc)"
cmake --preset=ci-sanitize
cmake --build build/sanitize --target counters_linux_pmu_seam_test \
  harness_jitter_window_test -j "$(nproc)"
```

Both configures and both builds exit 0. The binaries land in
`build/test/` and in `build/sanitize/test/`, the directories CTest runs
them from.

## The acceptance loops (SC-002)

Each test runs 100 times per preset: 50 plain runs and 50 runs under
`taskset -c 0,1` (FR-016). At the `ci-rocky` build:

```sh
cd build/test
for index in $(seq 1 50); do
  ./counters_linux_pmu_seam_test || exit 1
  ./harness_jitter_window_test || exit 1
done
for index in $(seq 1 50); do
  taskset -c 0,1 ./counters_linux_pmu_seam_test || exit 1
  taskset -c 0,1 ./harness_jitter_window_test || exit 1
done
```

The two loops of the sanitize tree run the same way from
`build/sanitize/test`. PASS when all 200 runs per preset exit 0 and no
run reports a failure (SC-002).

## The 1000 seam-test repetitions (SC-001)

```sh
cd build/test
aborts=0
for index in $(seq 1 1000); do
  ./counters_linux_pmu_seam_test > /dev/null 2>&1 \
    || aborts=$((aborts + 1))
done
printf 'seam-test aborts: %d of 1000\n' "$aborts"
```

PASS when the line reads `seam-test aborts: 0 of 1000`. At the base,
about 3 of 100 runs abort with `*** stack smashing detected ***` from
`multiplexWindowScenario()`; paste the fixed count in the pull request
body (SC-005).

## The Rocky 10 container loop (FR-017)

The image and the package list are the `test-rocky` job's own
(`.github/workflows/ci.yml`): `gcc-c++ cmake git python3 python3-pip
python3-pyyaml make doxygen lcov gzip diffutils ccache autoconf automake
libtool patch`, plus the `tar zstd` bootstrap, `dnf-plugins-core`, the
`crb` repository, and EPEL. A host with no container tool reports the
loop as not run in the pull request body (FR-017). The mounted tree
carries the vendored submodules; run `git submodule update --init` on the
host when they stand missing.

```sh
podman run --rm -it --cap-add SYS_NICE -v "$PWD:/src" -w /src \
  rockylinux/rockylinux:10 bash
```

The `--cap-add SYS_NICE` flag is the job's own; the memory bind the node
preset makes sits behind it (specs/035 FR-013, FR-022). Inside the
container:

```sh
set -euo pipefail
sed -i '/^\[main\]/a keepcache=1' /etc/dnf/dnf.conf
dnf -y install tar zstd dnf-plugins-core
dnf config-manager --set-enabled crb || true
dnf -y install epel-release || \
  dnf -y install https://dl.fedoraproject.org/pub/epel/epel-release-latest-10.noarch.rpm
dnf -y install gcc-c++ cmake git python3 python3-pip python3-pyyaml make \
  doxygen lcov gzip diffutils ccache autoconf automake libtool patch
cmake --preset=ci-rocky
cmake --build build --config Release -j "$(nproc)"
cd build
ctest --output-on-failure --no-tests=error -C Release \
  -R 'counters_linux_pmu_seam_test|harness_jitter_window_test'
```

The job runs the whole suite; the two names above are the acceptance
pair. The 1000-repetition loop and the acceptance loops run inside the
container the same way they run on the host, from `build/test`; paste the
container counts in the pull request body beside the host counts
(SC-003, SC-005).
