# Phase 6 Lane Matrix Flag

**Surface**: the `SPEEDGUN_DBC_MATRIX` cache variable of the five
lane configure presets in `CMakePresets.json`, and the `dbc_matrix`
entries its guard registers in each lane's tree.
**Head sha**: 3cf4d639a3ae85af42d2dd087a5aa83094eb7943
**Run id**: local
**Command**: the five configure lines and the five listing lines under
Readings, each run from the repository root
**Numbers**: the preset read, the edit diff, and the table of exits and
listing counts

P-01 and D-5 place the five matrix cells in the three lanes that
build the project on its ordinary flags, and hold them out of the
two lanes whose instrumentation would pay for them twice.
`contracts/presets.md` states the same split in its cache variable
table, and T034 is that split in the preset file.

## The preset read

A `python3` read of the committed file, run with exit 0:

```python
import json
d = json.load(open('CMakePresets.json'))
want = ['ci-ubuntu-test', 'ci-rocky', 'ci-tsan', 'ci-coverage', 'ci-sanitize']
for p in d['configurePresets']:
    if p['name'] in want:
        cv = p.get('cacheVariables', {})
        print(p['name'], '->',
              json.dumps({k: cv[k] for k in cv if k == 'SPEEDGUN_DBC_MATRIX'})
              or 'SPEEDGUN_DBC_MATRIX unset',
              'keys=%d' % len(cv))
```

```text
ci-coverage -> {} keys=1
ci-sanitize -> {} keys=2
ci-tsan -> {"SPEEDGUN_DBC_MATRIX": "ON"} keys=4
ci-ubuntu-test -> {"SPEEDGUN_DBC_MATRIX": "ON"} keys=1
ci-rocky -> {"SPEEDGUN_DBC_MATRIX": "ON"} keys=1
```

The braces hold the block's own `SPEEDGUN_DBC_MATRIX` row, so empty
braces say the preset carries no such key, and `keys` counts every
cache variable in that block.

| Preset | `SPEEDGUN_DBC_MATRIX` | The block |
| --- | --- | --- |
| `ci-ubuntu-test` | `ON` | one key, set by T005 and kept as it stood |
| `ci-rocky` | `ON` | one key, in a block this task added |
| `ci-tsan` | `ON` | a fourth key, after the three tsan flag keys |
| `ci-coverage` | unset | one key, the analyzer-free flag string |
| `ci-sanitize` | unset | two keys, the sanitizer config and its flags |

## The edit

`git diff --stat` for this task is one file, 6 insertions and 2
deletions. No other key moved and no other line was reformatted.

```diff
@@ -134,7 +134,8 @@
         "CMAKE_CXX_FLAGS_RELEASE": "-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3 -O2 -g -DNDEBUG -fsanitize=thread -fno-omit-frame-pointer -fno-common",
         "CMAKE_EXE_LINKER_FLAGS_RELEASE": "-fsanitize=thread",
-        "CMAKE_SHARED_LINKER_FLAGS_RELEASE": "-fsanitize=thread"
+        "CMAKE_SHARED_LINKER_FLAGS_RELEASE": "-fsanitize=thread",
+        "SPEEDGUN_DBC_MATRIX": "ON"
       }
     },
@@ -165,7 +166,10 @@
     {
       "name": "ci-rocky",
-      "inherits": ["ci-build", "ci-linux", "dev-mode", "ci-cache"]
+      "inherits": ["ci-build", "ci-linux", "dev-mode", "ci-cache"],
+      "cacheVariables": {
+        "SPEEDGUN_DBC_MATRIX": "ON"
+      }
     },
```

## Readings

One configure per lane, each in its own tree, launched together:

```sh
cmake --preset=ci-ubuntu-test -B build/flag-ut
cmake --preset=ci-rocky -B build/flag-rocky
cmake --preset=ci-tsan -B build/flag-tsan
cmake --preset=ci-coverage -B build/flag-cov
cmake --preset=ci-sanitize -B build/flag-san
```

Then one listing per tree, which needs no build:

```sh
ctest --test-dir build/flag-ut -N -R dbc_matrix
ctest --test-dir build/flag-rocky -N -R dbc_matrix
ctest --test-dir build/flag-tsan -N -R dbc_matrix
ctest --test-dir build/flag-cov -N -R dbc_matrix
ctest --test-dir build/flag-san -N -R dbc_matrix
```

| Tree | Configure exit | Listing exit | Entries listed |
| --- | --- | --- | --- |
| `build/flag-ut` | 0 | 0 | 5 |
| `build/flag-rocky` | 0 | 0 | 5 |
| `build/flag-tsan` | 0 | 0 | 5 |
| `build/flag-cov` | 0 | 0 | 0 |
| `build/flag-san` | 0 | 0 | 0 |

Configure logs sit at `/tmp/t034-<tag>-cfg.log`. The three registering
trees print the same five names, numbered 21 to 25:
`dbc_matrix_off_o0`, `dbc_matrix_off_o2`, `dbc_matrix_on_o0`,
`dbc_matrix_on_o2`, `dbc_matrix_observe_off_o0`, each under
`Total Tests: 5`. The two other trees print no name line,
`Total Tests: 0`.

The cache says the same thing from the other side. The grep over
`build/flag-*/CMakeCache.txt` prints three lines:

```text
build/flag-ut/CMakeCache.txt:SPEEDGUN_DBC_MATRIX:UNINITIALIZED=ON
build/flag-rocky/CMakeCache.txt:SPEEDGUN_DBC_MATRIX:UNINITIALIZED=ON
build/flag-tsan/CMakeCache.txt:SPEEDGUN_DBC_MATRIX:UNINITIALIZED=ON
```

`UNINITIALIZED` is how CMake records a `-D` entry that no `option()`
declares, and the guard at `test/CMakeLists.txt:158` reads the variable
directly, so the type is only the shape of the evidence here.

The five listing rows also need the five-entry registration of T033,
which sits under that same guard. That file belongs to its own node,
was modified in this shared working tree while the configures ran,
and this task wrote nothing to it.

## Verdict

PASS. The three registering presets carry `SPEEDGUN_DBC_MATRIX` at
`ON`, the two dropping presets carry no such key, all five configures
exit 0, and the listings count 5, 5, 5, 0 and 0 in that order.
