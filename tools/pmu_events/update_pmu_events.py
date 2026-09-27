#!/usr/bin/env python3
"""Vendored pmu-events integrity check and re-pin tool (specs/007 T058/T059).

Modes:
  --check          Verify every file under the vendored tree against the
                   per-file sha256 in RECORD, and the gate constant in the
                   root CMakeLists.txt against the RECORD ref line. One
                   line per problem on stderr naming the file. Zero
                   network code paths are reachable from this mode.
  --to <kernel-ref>
                   Re-pin the tree to a kernel tag. Default ref when
                   --to is omitted: the running kernel, resolved from
                   `uname -r` with distro and localversion suffixes
                   stripped (`7.2.4-1-cachyos` resolves to `linux-7.2.4`),
                   per the source-ref policy DCR (owner directive
                   2026-09-26, specs/007 plan.md). Fetches the kernel.org
                   cgit snapshot, GitHub mirror (gregkh/linux) as the
                   documented fallback. Replace-last: the tree is
                   rewritten only after every validation passes.
  --tarball FILE   Re-pin from a local snapshot tarball instead of
                   fetching. Offline test seam for the extract, validate,
                   and replace path; never used by --check.

Testability: --root points the tool at a repo-like root (default: the
repository holding this script). The tree is <root>/external/pmu-events,
RECORD is <root>/external/pmu-events/RECORD, and the gate constant is
parsed from <root>/CMakeLists.txt with the pattern
`set(_pmu_events_expected_ref <ref>)`.

Exit codes (house convention, same as tools/prose/prose_gate.py):
0 clean, 1 findings or failed validation, 2 usage error.
"""

from __future__ import annotations

import argparse
import datetime
import hashlib
import json
import os
import re
import shutil
import sys
import tempfile
from pathlib import Path

EXIT_OK = 0
EXIT_FINDINGS = 1
EXIT_USAGE = 2

TREE_REL = Path("external") / "pmu-events"
RECORD_NAME = "RECORD"
GATE_PATTERN = re.compile(r"set\(_pmu_events_expected_ref +([^\s)]+)\)")
ENTRY_PATTERN = re.compile(r"^([0-9a-f]{64})  (\S+)$")
RECORD_KEYS = (
    "ref",
    "url",
    "date",
    "tarball-sha256",
    "license",
    "scope",
    "exclusions",
)
LICENSE = (
    "dual MIT / GPL-2.0-or-later (kernel tools/perf/pmu-events; "
    "T002 verdict permits BSD-3 redistribution of this data tree)"
)
SCOPE = (
    "tools/perf/pmu-events/arch/x86 byte-exact, paths relative to "
    "tools/perf/pmu-events"
)
EXCLUSIONS = (
    "build scripts and generators (Build, *.py), generated-code template "
    "and header (empty-pmu-events.c, pmu-events.h), README, all non-x86 "
    "architecture tables"
)
CGIT_URL = (
    "https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/"
    "snapshot/linux-{ver}.tar.gz"
)
GITHUB_URL = "https://github.com/gregkh/linux/archive/refs/tags/linux-{ver}.tar.gz"
MEMBER_PREFIX = re.compile(r"^[^/]+/tools/perf/pmu-events/(arch/x86/.+)$")
# Names excluded inside the scope, matching the RECORD exclusions line.
EXCLUDED_BASENAMES = frozenset(
    {"Build", "README", "empty-pmu-events.c", "pmu-events.h"}
)


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    return sha256_bytes(path.read_bytes())


def read_record(record_path: Path) -> tuple[dict[str, str], dict[str, str]]:
    """Parse RECORD into (header dict, {relpath: sha256}).

    Layout: header `key: value` lines, a `----` separator, then
    `sha256  path` entries. Malformed input exits 2.
    """
    if not record_path.is_file():
        sys.stderr.write(f"{record_path}: RECORD is missing\n")
        raise SystemExit(EXIT_USAGE)
    lines = record_path.read_text().splitlines()
    try:
        sep = lines.index("----")
    except ValueError:
        sys.stderr.write(f"{record_path}: missing '----' separator\n")
        raise SystemExit(EXIT_USAGE) from None
    header: dict[str, str] = {}
    for line in lines[:sep]:
        key, _, value = line.partition(":")
        header[key.strip()] = value.strip()
    for key in RECORD_KEYS:
        if key not in header:
            sys.stderr.write(f"{record_path}: header line '{key}:' is missing\n")
            raise SystemExit(EXIT_USAGE)
    entries: dict[str, str] = {}
    for lineno, line in enumerate(lines[sep + 1 :], start=sep + 2):
        if not line.strip():
            continue
        match = ENTRY_PATTERN.match(line)
        if match is None:
            sys.stderr.write(f"{record_path}:{lineno}: malformed entry line\n")
            raise SystemExit(EXIT_USAGE)
        entries[match.group(2)] = match.group(1)
    return header, entries


def write_record(record_path: Path, header: dict[str, str], entries: dict[str, str]) -> None:
    lines = [f"{key}: {header[key]}" for key in RECORD_KEYS]
    lines.append("----")
    lines.extend(f"{entries[path]}  {path}" for path in sorted(entries))
    record_path.write_text("\n".join(lines) + "\n")


def disk_entries(tree: Path) -> dict[str, bytes]:
    """Every file in the tree as {relpath: bytes}, RECORD itself excluded."""
    found: dict[str, bytes] = {}
    for path in sorted(tree.rglob("*")):
        if path.is_file() and path.relative_to(tree).as_posix() != RECORD_NAME:
            found[path.relative_to(tree).as_posix()] = path.read_bytes()
    return found


def check(root: Path) -> int:
    tree = root / TREE_REL
    header, recorded = read_record(tree / RECORD_NAME)
    problems = 0

    def report(message: str) -> None:
        nonlocal problems
        sys.stderr.write(message + "\n")
        problems += 1

    present = disk_entries(tree)
    for path in sorted(recorded):
        if path not in present:
            report(f"{TREE_REL / path}: listed in RECORD, missing from the tree")
        elif sha256_bytes(present[path]) != recorded[path]:
            report(f"{TREE_REL / path}: bytes differ from the RECORD sha256")
    for path in sorted(present):
        if path not in recorded:
            report(f"{TREE_REL / path}: in the tree, absent from RECORD")

    cmake = root / "CMakeLists.txt"
    gate_match = GATE_PATTERN.search(cmake.read_text()) if cmake.is_file() else None
    if gate_match is None:
        report(f"{cmake}: no set(_pmu_events_expected_ref ...) gate constant found")
    elif gate_match.group(1) != header["ref"]:
        report(
            f"{cmake}: gate constant {gate_match.group(1)} disagrees with "
            f"{TREE_REL / RECORD_NAME} ref {header['ref']}"
        )
    return EXIT_FINDINGS if problems else EXIT_OK


def resolve_ref(to: str | None) -> str:
    """Normalize --to, or resolve the running kernel per the source-ref
    policy DCR (owner directive 2026-09-26): strip distro and localversion
    suffixes from `uname -r`, so `7.2.4-1-cachyos`, `6.6.hardening-1`, and
    `6.6+` all resolve to the upstream tag `linux-<x.y[.z]>`."""
    if to is not None:
        core = to[len("linux-") :] if to.startswith("linux-") else to
        if not re.fullmatch(r"\d+\.\d+(?:\.\d+)?", core):
            sys.stderr.write(f"--to {to!r}: not a kernel version (x.y or x.y.z)\n")
            raise SystemExit(EXIT_USAGE)
        return f"linux-{core}"
    release = os.uname().release
    match = re.match(r"(\d+\.\d+(?:\.\d+)?)", release)
    if match is None:
        sys.stderr.write(f"cannot resolve a kernel version from uname -r: {release!r}\n")
        raise SystemExit(EXIT_USAGE)
    return f"linux-{match.group(1)}"


def fetch(url: str) -> bytes:
    """Download one snapshot tarball (only reachable from --to)."""
    import urllib.request

    with urllib.request.urlopen(url, timeout=120) as response:
        return response.read()


def extract_scope(tar_bytes: bytes) -> dict[str, bytes]:
    """In-scope files from a snapshot tarball as {relpath: bytes}.

    Scope: tools/perf/pmu-events/arch/x86/** (mapfile.csv lives inside
    arch/x86 upstream, so the vendored layout needs no extra step).
    Excluded per RECORD: Build, *.py, README, empty-pmu-events.c,
    pmu-events.h.
    """
    import io
    import tarfile

    files: dict[str, bytes] = {}
    with tarfile.open(fileobj=io.BytesIO(tar_bytes)) as archive:
        for member in archive.getmembers():
            if not member.isfile():
                continue
            match = MEMBER_PREFIX.match(member.name)
            if match is None:
                continue
            rel = match.group(1)
            base = rel.rsplit("/", 1)[-1]
            if base in EXCLUDED_BASENAMES or base.endswith(".py"):
                continue
            extracted = archive.extractfile(member)
            assert extracted is not None
            files[rel] = extracted.read()
    return files


def validate(files: dict[str, bytes]) -> list[str]:
    """Schema sanity: every .json parses and holds at least one JSON
    object; a file with EventName entries must also carry EventCode;
    mapfile.csv regex fields compile."""
    problems: list[str] = []
    for rel in sorted(files):
        data = files[rel]
        if rel.endswith(".json"):
            try:
                parsed = json.loads(data)
            except ValueError as exc:
                problems.append(f"{rel}: does not parse as JSON ({exc})")
                continue
            objects = [d for d in (parsed if isinstance(parsed, list) else [parsed]) if isinstance(d, dict)]
            if not objects:
                problems.append(f"{rel}: holds no JSON object")
            elif any("EventName" in d for d in objects) and not any("EventCode" in d for d in objects):
                problems.append(f"{rel}: EventName entries without any EventCode")
        elif rel.endswith("mapfile.csv"):
            for lineno, line in enumerate(data.decode().splitlines(), start=1):
                if not line.strip() or line.startswith("#"):
                    continue
                fields = line.split(",")
                if len(fields) < 3:
                    problems.append(f"{rel}:{lineno}: fewer than 3 fields")
                    continue
                try:
                    re.compile(fields[0])
                except re.error as exc:
                    problems.append(f"{rel}:{lineno}: regex field does not compile ({exc})")
    return problems


def event_counts(sources: dict[str, bytes]) -> dict[str, int]:
    """EventName entry count per architecture directory (arch/x86/<cpu>)."""
    counts: dict[str, int] = {}
    for rel, data in sources.items():
        if not rel.endswith(".json"):
            continue
        directory = rel.rsplit("/", 1)[0] if "/" in rel else "."
        try:
            parsed = json.loads(data)
        except ValueError:
            continue
        entries = parsed if isinstance(parsed, list) else []
        counts[directory] = counts.get(directory, 0) + sum(
            1 for entry in entries if isinstance(entry, dict) and "EventName" in entry
        )
    return counts


def repin(root: Path, ref: str, tarball: Path | None) -> int:
    tree = root / TREE_REL
    url = CGIT_URL.format(ver=ref[len("linux-") :])
    if tarball is not None:
        blob = tarball.read_bytes()
        url = tarball.resolve().as_uri()
    else:
        try:
            blob = fetch(url)
        except OSError as exc:
            url = GITHUB_URL.format(ver=ref[len("linux-") :])
            sys.stderr.write(f"cgit fetch failed ({exc}); trying {url}\n")
            try:
                blob = fetch(url)
            except OSError as fallback_exc:
                sys.stderr.write(f"GitHub mirror fetch failed: {fallback_exc}\n")
                return EXIT_FINDINGS

    files = extract_scope(blob)
    if not files:
        sys.stderr.write("snapshot holds no in-scope arch/x86 tables\n")
        return EXIT_FINDINGS
    problems = validate(files)
    if problems:
        for problem in problems:
            sys.stderr.write(f"validation: {problem}\n")
        sys.stderr.write("validation failed; the vendored tree is untouched\n")
        return EXIT_FINDINGS

    cmake = root / "CMakeLists.txt"
    text = cmake.read_text()
    if GATE_PATTERN.search(text) is None:
        sys.stderr.write(f"{cmake}: no set(_pmu_events_expected_ref ...) constant to bump\n")
        return EXIT_FINDINGS

    # Replace-last: staging holds the validated tree until this point.
    old = disk_entries(tree)
    old_counts = event_counts(old)
    with tempfile.TemporaryDirectory(prefix="pmu-events-stage-") as stage_name:
        stage = Path(stage_name)
        for rel, data in files.items():
            target = stage / rel
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
        for rel in old:
            (tree / rel).unlink()
        for rel, data in files.items():
            target = tree / rel
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)

    write_record(
        tree / RECORD_NAME,
        {
            "ref": ref,
            "url": url,
            "date": datetime.date.today().isoformat(),
            "tarball-sha256": sha256_bytes(blob),
            "license": LICENSE,
            "scope": SCOPE,
            "exclusions": EXCLUSIONS,
        },
        {rel: sha256_bytes(data) for rel, data in files.items()},
    )
    cmake.write_text(GATE_PATTERN.sub(f"set(_pmu_events_expected_ref {ref})", text, count=1))

    new_counts = event_counts(files)
    for directory in sorted(old_counts.keys() | new_counts.keys()):
        print(f"{directory}: {old_counts.get(directory, 0)} -> {new_counts.get(directory, 0)} event entries")
    print(f"re-pinned {TREE_REL} to {ref}")
    return EXIT_OK


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        prog="update_pmu_events.py",
        description=__doc__.splitlines()[0],
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="Modes: --check (no network) or --to <kernel-ref> "
        "[--tarball FILE]. --root relocates the whole check to a "
        "repo-like directory (fixtures use it).",
    )
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--check", action="store_true", help="verify tree, RECORD, and gate constant")
    mode.add_argument("--to", metavar="KERNEL-REF", help="re-pin to a kernel version or linux-<ver> tag")
    parser.add_argument("--tarball", type=Path, metavar="FILE", help="with --to: local snapshot tarball, no network")
    parser.add_argument(
        "--root",
        type=Path,
        default=Path(__file__).resolve().parents[2],
        help="repo-like root (default: this repository)",
    )
    args = parser.parse_args(argv)
    if args.tarball is not None and args.to is None:
        parser.error("--tarball requires --to")
    return args


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    root: Path = args.root
    if args.check:
        return check(root)
    return repin(root, resolve_ref(args.to), args.tarball)


if __name__ == "__main__":
    sys.exit(main())
