# Numbers: specs/051-raw-events (T022, T023)

**Read**: one batch of commands run over this checkout at the moment this
record was written. Every figure below comes from the command printed
beside it.
**Base**: `0fce6651`, the specs/049 squashed head and the parent of this
lane's documents.

```
git log --oneline -1 0fce6651
0fce6651 Docs: Remove names and user paths from old records
```

The branch head at the read:

```
git rev-parse HEAD
abcda8f30b9e07e169ba5f86522dd35f66b314b4
```

## The four numbers of D-6 (FR-012, SC-006)

Each block below holds one command line, then the output that command
printed.

```
grep -n '^    VERSION 0.35.0' CMakeLists.txt
87:    VERSION 0.35.0
```

```
grep -n '^    SOVERSION 8' CMakeLists.txt
182:    SOVERSION 8
```

```
grep -n '^    SOVERSION 22' CMakeLists.txt
466:    SOVERSION 22
```

```
grep -n 'kSchemaVersion = "1.16"' source/harness/json_report.cpp
58:constexpr std::string_view kSchemaVersion = "1.16";
```

T022 adds a five-line comment above the version line and a five-line
comment above the counters number, so these three line numbers sit at 87,
182 and 466 where the plan names 82, 172 and 456 at the base.

## The counters case: the kept reading at 8

The counters number answers 8, the reading specs/050 landed, and it is the
kept one. This lane moves no public header, so the condition hard rule 12
names, a public header change, stays unanswered and the hand-kept number
stands where specs/050 left it. `CMakeLists.txt:177-181` carries that
condition beside the number, in the style of the specs/048 note at
`:453-455`.

The 9 reading is the case D-6 states for a lane that moves a public
header. That case did not land here, and this record keeps 9 as the
reading it would give.

## The empty diff over include/ and docs/schema/

```
git diff --name-only 0fce6651 -- include/ docs/schema/
```

That command prints no line: no tracked file under `include/` and no
tracked file under `docs/schema/` differs from the base at this read. The
one-path read the task names:

```
git diff --name-only 0fce6651 -- include/
```

prints no line.

## The schema constant stays put

```
git diff --name-only 0fce6651 -- source/harness/json_report.cpp
```

prints no line, so the file that carries the schema constant is out of the
diff and 1.16 stands at `json_report.cpp:58`.
