# Phase 9 Ubuntu apt Archives Cache

**Surface**: the install steps of the eleven Ubuntu jobs in
`.github/workflows/ci.yml` (T049, FR-014 and FR-036).
**Head sha**: 24d9814, where `.github/workflows/ci.yml` is 1243 lines,
with the rocky block of T048 and the apt block of this task sitting in
the working tree
**Run id**: local
**Command**: the checks under "What the checks print", run from the
repository root, and the container readings under "Measured on an Ubuntu
image"
**Numbers**: 13 jobs before and after, 11 install steps, 92 package
tokens before and after, 1243 workflow lines at head, 1459 in the tree

Phase 9 rests on package lists surviving from one run to the next, so a
991 s install step stops deciding the wall time (tasks.md:233 to 234).
The rocky lane got its survival in T048. FR-036 asks for the equivalent
in the Ubuntu lanes, and spec.md:757 to 758 names the directory and the
key:

```text
757:- **FR-036**: The Ubuntu lanes shall cache `/var/cache/apt/archives` in a
758:  cache keyed on the package list of that job.
```

Eleven jobs install packages with `apt-get`: `coverage`, `sanitize`,
`tsan`, `static-analysis`, `test`, `shared-audit`,
`downstream-consumer`, `consumer-release`, `dbc-gate`, `prose-lint`,
`docs`. Each of them fetches its whole closure from an Ubuntu mirror on
every run. `lint` installs codespell and clang-format from pip, so it
has no apt archives to keep, and `test-rocky` installs with dnf inside
its container, which is the T048 cache. Those two stay as they are.

## The two steps in each job

Every install step gains a pair in front of it. The `coverage` instance,
printed with `grep -n` from the working tree:

```text
64:    # The Ubuntu image ships docker-clean, an apt hook that
65:    # deletes the .deb files in the archives after dpkg and
66:    # after apt-get update, so the directory holds nothing to
67:    # cache while it is in place. actions/cache restores as
68:    # the runner user, who needs write access to it.
69:    - name: Prepare apt package cache
70:      run: |
71:        set -euo pipefail
72:        sudo rm -f /etc/apt/apt.conf.d/docker-clean
73:        sudo chown -R "$(id -un)" /var/cache/apt/archives
74:
75:    - name: Restore apt package cache
76:      uses: actions/cache@v4
77:      with:
78:        path: /var/cache/apt/archives
79:        key: apt-${{ github.job }}-ubuntu-26.04-${{ hashFiles('.github/workflows/ci.yml') }}
80:        restore-keys: apt-${{ github.job }}-ubuntu-26.04-
```

The pair sits at these lines, one row per job, with the install step it
feeds:

| job | prepare | restore | install |
| --- | --- | --- | --- |
| coverage | 69 | 75 | 84 |
| sanitize | 163 | 169 | 178 |
| tsan | 246 | 252 | 261 |
| static-analysis | 338 | 344 | 353 |
| test | 469 | 475 | 485 |
| shared-audit | 657 | 663 | 673 |
| downstream-consumer | 800 | 806 | 816 |
| consumer-release | 999 | 1005 | 1014 |
| dbc-gate | 1235 | 1241 | 1250 |
| prose-lint | 1314 | 1320 | 1328 |
| docs | 1434 | 1440 | 1449 |

`prose-lint` indents its steps by six because its own step list does,
and its pair follows it. `actions/cache@v4` is the combined form that
T048 uses: it restores on the way in and saves in the post step, so a
warm `coverage` run uploads once and every later run finds the entry.

The chown carries the restore. `stat -c '%A %a %u %g %n'` inside the
image prints

```text
drwxr-xr-x 755 1000 1000 /var/cache/apt/archives
drwx------ 700 1000 1000 /var/cache/apt/archives/partial
```

The two id columns read 1000 because the container runtime maps the
image's root onto the host uid, and the packaged owner of that directory
is root. The modes are the reading that matters: `0755` for the archives
and `0700` for `partial`, so only the owner may create entries there.
The runner user extracts the archive, and a restore error reaches
`core.setFailed` in `restoreImpl` of the action, which turns a warm run
into a red job. Writing the directory over to the runner user first is
what makes a warm run possible, and the recursive form covers `partial`,
whose mode is `0700`. A save that a fork cannot perform stays a warning
in `saveImpl` of the same action, so the verdict of a pull request
survives either way, as T048 recorded for the rocky lane.

## The cache key

```text
apt-${{ github.job }}-ubuntu-26.04-${{ hashFiles('.github/workflows/ci.yml') }}
restore-keys: apt-${{ github.job }}-ubuntu-26.04-
```

Three parts, each doing one job. `ubuntu-26.04` names the runner image,
because the `.deb` files in the directory belong to that image. The job
term is the discriminator the ccache keys already use at `ci.yml:57`,
where `CCACHE_KEY_PREFIX` opens with the job name, and it gives every
job its own entry, which is what FR-036 asks when it says the key is
that job's package list. The `hashFiles` term is
what a changed package list needs: the action keeps an existing entry
once the primary key matches, so without a term that moves when the list
moves, a newly added package could never enter the cache. Editing a line
that has nothing to do with packages also moves it, and the
`restore-keys` prefix still lands on the entry from before, so the cost
of such an edit is one upload. `ubuntu-26.04` and the job name together
form the prefix.

The key lives in the `with` block because the package lists live in the
install steps, which the task holds fixed. Restating a list to hash it
would give a job two package lists, and the two could drift apart.

## The package lists, line by line

`git show HEAD:.github/workflows/ci.yml` against the working tree, per
install step, in file order:

```text
coverage
  before (8): lcov ccache doxygen python3-yaml autoconf automake libtool patch
  after  (8): lcov ccache doxygen python3-yaml autoconf automake libtool patch
  equal: True
sanitize
  before (9): clang ccache doxygen python3-yaml lcov autoconf automake libtool patch
  after  (9): clang ccache doxygen python3-yaml lcov autoconf automake libtool patch
  equal: True
tsan
  before (9): clang ccache doxygen python3-yaml lcov autoconf automake libtool patch
  after  (9): clang ccache doxygen python3-yaml lcov autoconf automake libtool patch
  equal: True
static-analysis
  before (10): clang-tidy cppcheck ccache doxygen python3-yaml lcov autoconf automake libtool patch
  after  (10): clang-tidy cppcheck ccache doxygen python3-yaml lcov autoconf automake libtool patch
  equal: True
test
  before (8): ccache doxygen python3-yaml lcov autoconf automake libtool patch
  after  (8): ccache doxygen python3-yaml lcov autoconf automake libtool patch
  equal: True
shared-audit
  before (8): autoconf automake libtool patch doxygen python3-yaml lcov ccache
  after  (8): autoconf automake libtool patch doxygen python3-yaml lcov ccache
  equal: True
downstream-consumer
  before (11): autoconf automake libtool patch libhwloc-dev libsimdjson-dev zlib1g-dev doxygen python3-yaml lcov ccache
  after  (11): autoconf automake libtool patch libhwloc-dev libsimdjson-dev zlib1g-dev doxygen python3-yaml lcov ccache
  equal: True
consumer-release
  before (8): doxygen python3-yaml lcov autoconf automake libtool patch ccache
  after  (8): doxygen python3-yaml lcov autoconf automake libtool patch ccache
  equal: True
dbc-gate
  before (8): doxygen python3-yaml lcov autoconf automake libtool patch ccache
  after  (8): doxygen python3-yaml lcov autoconf automake libtool patch ccache
  equal: True
prose-lint
  before (8): python3-yaml doxygen lcov autoconf automake libtool patch ccache
  after  (8): python3-yaml doxygen lcov autoconf automake libtool patch ccache
  equal: True
docs
  before (5): doxygen autoconf automake libtool patch
  after  (5): doxygen autoconf automake libtool patch
  equal: True
aggregate tokens before=92 after=92
every per-job list equal: True
sorted aggregate equal: True
```

The eleven install command lines are also identical byte for byte. The
head lines are 66, 142, 207, 281, 395, 547, 672, 852, 1070, 1130 and
1233, and each of them appears in the tree at the line in the table
above, with the same text. `diff` of the pre-edit tree against the tree
after this task prints 198 added lines and 0 removed lines.

## What the checks print

```text
$ python3 -c "import yaml; d=yaml.safe_load(open('.github/workflows/ci.yml')); print(len(d['jobs']))"
13

$ python3 /tmp/t049/check_pkgs.py
apt-get install lines: before=11 after=11
package tokens: before=92 after=92
sorted multiset equal: True
in-order sequence equal: True
per-line package sets equal: True
symmetric difference of package names: []

$ python3 /tmp/t049/check_steps.py
install steps covered: 11
key occurrences in file: 11 restore-keys: 11
chown occurrences: 11

$ python3 /tmp/chk.py .github/workflows/ci.yml
evaluate_prose -> (0, 0, 0, 0)
```

`check_pkgs.py` reads the head copy through `git show`, drops each token
that begins with a hyphen, which leaves the package names and the
version pins, and compares the two files. `check_steps.py` walks the
parsed document, and for each install step asserts the two steps ahead
of it, their names, the two `sudo` commands, `actions/cache@v4`, the
path, the key and the prefix. Both files sit in `/tmp/t049` for the run
that reads them.

The prose gate prints `0` sources for `.github/workflows/ci.yml`, as
`classify_source` has no language for `.yml` and the gate reads no units
there, so a clean tuple over that file covers no text. The 198 added
lines were measured directly instead: 0 of them are non-ASCII, 0 comment
lines pass 72 columns, and the longest added lines are the `key:` values
at 92 columns, 94 in `prose-lint`, inside a file that already holds 149
lines above 72 and a longest line of 144.

`grep -n` finds no `apt-get clean`, no `apt-get autoclean` and no
`rm -rf /var/cache` anywhere in the workflow, so nothing wipes the
directory between the install step and the post-step save.

## Measured on an Ubuntu image

This host runs Arch, so the apt behaviour was measured inside an Ubuntu
24.04 image, with the container started as

```text
apptainer exec --writable-tmpfs --fakeroot \
  --bind /tmp/t049/q3:/var/cache/apt/archives ubuntu.sif bash
```

The archives directory was bound to a host directory, so it survived
from one container to the next. Three readings, in order, with `lsof` as
the installed package:

```text
A2, image as shipped plus a keep-config drop-in:
  apt-config dump APT::Keep-Downloaded-Packages   (prints "true")
  install rc=0
  debs_left=0

A3, with /etc/apt/apt.conf.d/docker-clean removed:
  install rc=0
  debs_left=9
  964K    /var/cache/apt/archives

B3, fresh container over the directory A3 left:
  debs_present=9
  no-download rc=0
  ii  lsof           4.95.0-1build3 amd64        utility to list open files
  debs_left_after=9
```

A2 is the reason for the `rm -f`. The image ships one line that decides
the whole design, printed from `/etc/apt/apt.conf.d/docker-clean`:

```text
DPkg::Post-Invoke { "rm -f /var/cache/apt/archives/*.deb /var/cache/apt/archives/partial/*.deb /var/cache/apt/*.bin || true"; };
APT::Update::Post-Invoke { "rm -f /var/cache/apt/archives/*.deb /var/cache/apt/archives/partial/*.deb /var/cache/apt/*.bin || true"; };
Dir::Cache::pkgcache ""; Dir::Cache::srcpkgcache "";
```

It deletes the archives after `dpkg` runs and again after `apt-get
update` runs, so a cached directory would be emptied by the `apt-get
update -q` that opens every install step, before the install reads it.
`APT::Keep-Downloaded-Packages "true"` changes nothing while that hook
is in place, which A2 shows with `apt-config` reporting the value it was
given. The
hook is a Docker-image size concession, the apt default is to keep what
it downloads, and A3 is that default: nine `.deb` files left behind. B3
is the payoff: a fresh container whose archives directory holds those
nine files installs `lsof` with the no-download option set and exits 0,
so the restored closure satisfies the install with the network unused,
and the files are still there afterwards.

## What is not shown here

The runner image of `ubuntu-26.04` sits beyond this host: no container
runtime here can open it, so whether it ships `docker-clean` stays
unmeasured. The `rm -f` is written with `-f` for both cases: the file
present and the file absent, where the command prints nothing and the
apt default already keeps the archives.

A partial miss, where some `.deb` files are cached and some must arrive,
was not measured. apt drops its download helper to the `_apt` user, and
a user-namespace container here maps only one uid, so the drop cannot
happen on this host and the path needs a runner. Root writing into a
directory owned by the runner user is what the chown asks of POSIX, and
POSIX permits it.

The bytes per entry, the hit rate and the install time of a warm run
belong to T051, which reads them off the runs FR-052 counts. Eleven
entries share the repository cache quota with the ccache entries, and
the `hashFiles` term adds apt entries whenever `ci.yml` changes; GitHub
evicts the least recently used entry once the quota is reached, and the
restore keeps every apt entry hot, so the entries that leave are the
ccache entries of runs that are already over. T051 sees the list.

FR-037 and T050 cover the step names: both new steps are named, and
neither prints anything beyond what the action prints itself.

VERDICT: PASS
