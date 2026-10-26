# Phase 9 Rocky dnf Package Cache

**Surface**: the `env` block and the install steps of the `test-rocky`
job in `.github/workflows/ci.yml` (T048, FR-035).
**Head sha**: e77b713 with the rocky block of this task in the working
tree
**Run id**: local
**Command**: the readings under Verification, run from the repository
root
**Numbers**: the job count, the step order of the rocky job, and the
count of each package name before and after

Phase 9 begins from the claim that package lists survive across runs, so
a 991 s install step stops deciding the wall time (tasks.md:233 to 234).
The rocky lane had no survival: its install step runs inside the
`rockylinux/rockylinux:10` image, the cache directory of that image
starts empty in every run, and the RPMs a run downloads leave with the
container when the job ends. FR-035 closes the gap with a package cache.
A pushed image and `packages: write` sit outside the approved scope
(spec.md:371 to 373).

## The new steps

Lines 482 to 515 of the working tree, printed by `grep -n`:

```text
482:    env:
483:      CCACHE_DIR: ${{ github.workspace }}/.ccache
484:      CCACHE_KEY_PREFIX: ccache-${{ github.job }}-ci-rocky
485:      CCACHE_MAXSIZE: 768MB
486:      DNF_BOOTSTRAP: tar zstd
487:      DNF_PACKAGES: gcc-c++ cmake git python3 python3-pip python3-pyyaml make doxygen lcov gzip diffutils ccache autoconf automake libtool patch
488:
489:    steps:
490:    # actions/cache unpacks its archive with tar and zstd, so those two go in
491:    # ahead of it. keepcache=1 leaves the RPMs under /var/cache/dnf for it.
492:    - name: Install dnf bootstrap packages
493:      run: |
494:        set -euo pipefail
495:        sed -i '/^\[main\]/a keepcache=1' /etc/dnf/dnf.conf
496:        dnf -y install $DNF_BOOTSTRAP
497:    - name: Resolve dnf package cache key
498:      id: dnf-pkgs
499:      run: |
500:        set -euo pipefail
501:        echo "key=$(printf '%s' "$DNF_BOOTSTRAP $DNF_PACKAGES" | sha256sum | cut -c1-16)" >> "$GITHUB_OUTPUT"
502:    - name: Restore dnf package cache
503:      uses: actions/cache@v4
504:      with:
505:        path: /var/cache/dnf
506:        key: dnf-rockylinux10-${{ steps.dnf-pkgs.outputs.key }}
507:        restore-keys: dnf-rockylinux10-
508:    - name: Install toolchain
509:      run: |
510:        set -euo pipefail
511:        dnf -y install dnf-plugins-core
512:        dnf config-manager --set-enabled crb || true
513:        dnf -y install epel-release || \
514:          dnf -y install https://dl.fedoraproject.org/pub/epel/epel-release-latest-10.noarch.rpm
515:        dnf -y install $DNF_PACKAGES
```

Four steps sit where one used to sit. The bootstrap step, the key step
and the cache step come first, and the toolchain step keeps its name and
its place ahead of `actions/checkout`, so the parse order of the job is
unchanged from step 4 onward.

## Path and key

| Item | Value |
| --- | --- |
| Path | `/var/cache/dnf` |
| Key | `dnf-rockylinux10-14743b10cb3e6167` |
| Restore keys | `dnf-rockylinux10-` |

`/var/cache/dnf` is the `cachedir` of dnf. Rocky 10 ships dnf 4.20.0 in
BaseOS, and for that family `keepcache` is off by default while a true
value leaves every downloaded RPM in the cache directory beside the
metadata of its repository:

https://www.mankier.com/5/dnf.conf
https://dl.rockylinux.org/pub/rocky/10/BaseOS/x86_64/os/Packages/d/

The first line of the bootstrap step turns the option on, written after
the `[main]` header of `/etc/dnf/dnf.conf`.

One `actions/cache@v4` step covers the restore and the upload after the
job. The key is the first sixteen hex digits of `sha256sum` over the
package list, which is the pair of `env` values, so it moves when the
list moves and stays put when `ci.yml` changes for another reason. A key
step computes it because a cache key is an Actions expression and an
expression carries no hashing function:

```text
key=14743b10cb3e6167
```

`restore-keys` drops the hash, so an edited list reads the newest older
cache and downloads the difference on top of it. A run whose key already
exists uploads nothing, which is what holds one entry per list inside
the 10 GB cache budget of the repository.

## The package split

FR-035 names the bootstrap set: `dnf -y install tar zstd` (spec.md:754
to 756). The cache action needs a POSIX `tar` on the execution path
inside a container to unpack its archive, and `zstd` is the compressor
it reaches for, so those two belong ahead of the restore and nothing
else does (https://github.com/actions/cache/blob/main/README.md).

| Set | Names | Step |
| --- | --- | --- |
| Bootstrap | `tar zstd` | Install dnf bootstrap packages |
| The rest | `DNF_PACKAGES`, 16 names | Install toolchain |
| Repos | `dnf-plugins-core`, `epel-release` | Install toolchain |

`tar` moved from the toolchain line to the bootstrap line and `zstd` is
the one new name. The other 18 names keep the strings they had. The two
`dnf` transactions that installed the compiler set and the autotools set
are now one transaction over `DNF_PACKAGES`, and the `dnf-plugins-core`,
`crb` and `epel-release` lines keep their order at the head of the
toolchain step, since `config-manager` arrives with `dnf-plugins-core`.

The bootstrap RPMs download on every run, because the cache that would
hold them is restored by the step that follows. That is the shape FR-035
asks for, and the bootstrap pair is two of the twenty packages the lane
installs.

## Verification

The job count, the required reading, on the working tree and at head:

```text
$ python3 -c "import yaml; d=yaml.safe_load(open('.github/workflows/ci.yml')); print(len(d['jobs']))"
13
$ python3 -c "import yaml,subprocess; d=yaml.safe_load(subprocess.run(['git','show','HEAD:.github/workflows/ci.yml'],capture_output=True,text=True).stdout); print(len(d['jobs']))"
13
```

Thirteen jobs, with `test-rocky` the only job whose content differs. The
step list of that job, in parse order:

```text
0 Install dnf bootstrap packages
1 Resolve dnf package cache key
2 Restore dnf package cache
3 Install toolchain
4 actions/checkout@v4
5 Record compiler version
6 Restore ccache
7 Configure
```

`Restore dnf package cache` is step 2 and `Install toolchain` is step 3,
so the cache precedes the install. Each of the nineteen names the job
installed before is still one install argument, and the count of each
across the whole job, read from the `env` values and the `run` blocks
together, did not move:

| Name | Before | After | Name | Before | After |
| --- | --- | --- | --- | --- | --- |
| `dnf-plugins-core` | 1 | 1 | `doxygen` | 1 | 1 |
| `epel-release` | 1 | 1 | `lcov` | 1 | 1 |
| `gcc-c++` | 1 | 1 | `tar` | 1 | 1 |
| `cmake` | 4 | 4 | `gzip` | 1 | 1 |
| `git` | 1 | 1 | `diffutils` | 1 | 1 |
| `python3` | 1 | 1 | `ccache` | 3 | 3 |
| `python3-pip` | 1 | 1 | `autoconf` | 1 | 1 |
| `python3-pyyaml` | 1 | 1 | `automake` | 1 | 1 |
| `make` | 1 | 1 | `libtool` | 1 | 1 |
| `patch` | 1 | 1 | `zstd` | 0 | 1 |

`cmake` and `ccache` reach those counts through the build, install and
ccache steps around the install step, and hold one argument each inside
the install set. `dnf -y install` appears five times before and five
times after.

`bash -n` reads all ten `run` blocks of the job clean, and the `sed`
line was run against a copy of the distribution file layout:

```text
[main]
keepcache=1
gpgcheck=1
installonly_limit=3
clean_requirements_on_remove=True
best=True
skip_if_unavailable=False
```

`configparser` reports `keepcache` inside the `main` section with
`gpgcheck` untouched. The same `sed` against a file whose `[main]` block
is followed by another section leaves the option in `main` and the other
section intact, which is why the line anchors on the header.

## What is not shown here

No `dnf` ran on this host: it has no RPM toolchain and no runtime
holding the `rockylinux/rockylinux:10` image. Three things wait for CI.
The warm install time belongs to T051. The upload size and the hit rate
show up in the cache list of the repository. The restore miss is
answered by FR-037 and by the action itself, which turns a denied or
failed save into a warning and leaves the job green, so a pull request
from a fork keeps its verdict.

VERDICT: PASS
