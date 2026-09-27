#!/usr/bin/env python3
"""Gate-effectiveness fixtures for tools/pmu_events/update_pmu_events.py
(T057; FR-045, US8 scenarios 1, 2, 5, plus the offline --to seam).

Builds repo-like roots under --out (tree, RECORD with hashes computed at
run time, a CMakeLists.txt holding the gate constant), drives the tool
against them, and asserts exit codes and named files in both directions.
Nothing here touches the real external/pmu-events tree, and no scenario
uses the network: the --to cases feed a synthetic local tarball.

CLI::

    python3 run_pmu_events_fixtures.py --out <dir> <fixture-dir>

Exit codes: 0 all asserts pass, 1 an assert failed, 2 the tool script is
missing.
"""

from __future__ import annotations

import argparse
import hashlib
import io
import json
import subprocess
import sys
import tarfile
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parent.parent
TOOL = REPO_ROOT / "tools" / "pmu_events" / "update_pmu_events.py"

EXIT_OK = 0
EXIT_ASSERT = 1
EXIT_TOOL_MISSING = 2

CYCLES = [{"EventName": "cycles", "EventCode": "0x3c", "UMask": "0x00"}]
METRICS = [{"MetricName": "IPC", "MetricExpr": "instructions/cycles"}]
MAPFILE = "GenuineIntel,6,10,\nAuthenticAMD,21,*,\n"

CLEAN_FILES = {
    "arch/x86/fakecpu/metrics.json": json.dumps(METRICS).encode(),
    "arch/x86/fakecpu/core.json": json.dumps(CYCLES).encode(),
    "arch/x86/mapfile.csv": MAPFILE.encode(),
}


def run_tool(*tool_args: str | Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(TOOL), *(str(a) for a in tool_args)],
        capture_output=True,
        text=True,
        check=False,
    )


def write_record(root: Path, ref: str, entries: dict[str, bytes]) -> None:
    tree = root / "external" / "pmu-events"
    lines = [
        f"ref: {ref}",
        "url: https://example.invalid/snapshot.tar.gz",
        "date: 2026-01-01",
        "tarball-sha256: " + "0" * 64,
        "license: dual MIT / GPL-2.0-or-later",
        "scope: tools/perf/pmu-events/arch/x86 byte-exact, paths relative to tools/perf/pmu-events",
        "exclusions: build scripts and generators (Build, *.py)",
        "----",
    ]
    lines += [
        f"{hashlib.sha256(data).hexdigest()}  {rel}" for rel, data in sorted(entries.items())
    ]
    (tree / "RECORD").write_text("\n".join(lines) + "\n")


def make_root(out: Path, name: str, ref: str, entries: dict[str, bytes]) -> Path:
    root = out / name
    tree = root / "external" / "pmu-events"
    for rel, data in entries.items():
        target = tree / rel
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
    write_record(root, ref, entries)
    (root / "CMakeLists.txt").write_text(f"set(_pmu_events_expected_ref {ref})\n")
    return root


def make_tarball(path: Path, ref: str, files: dict[str, bytes]) -> None:
    with tarfile.open(path, "w:gz") as archive:
        for rel, data in files.items():
            info = tarfile.TarInfo(f"{ref}/tools/perf/pmu-events/{rel}")
            info.size = len(data)
            archive.addfile(info, io.BytesIO(data))


class Checker:
    def __init__(self) -> None:
        self.failures = 0

    def expect(self, condition: bool, label: str, detail: str = "") -> None:
        if condition:
            print(f"PASS {label}")
        else:
            self.failures += 1
            print(f"FAIL {label} {detail}", file=sys.stderr)


def scenarios(out: Path) -> int:
    checker = Checker()
    check = lambda root: run_tool("--check", "--root", root)  # noqa: E731

    # 1. clean tree: exit 0.
    root = make_root(out, "clean", "linux-7.2.4", CLEAN_FILES)
    result = check(root)
    checker.expect(result.returncode == 0, "clean tree exits 0", result.stderr)

    # 2. one file's bytes drift from the RECORD sha256: exit 1 naming it.
    root = make_root(out, "hash-drift", "linux-7.2.4", CLEAN_FILES)
    drifted = root / "external" / "pmu-events" / "arch" / "x86" / "fakecpu" / "core.json"
    drifted.write_bytes(b'[{"EventName": "tampered"}]\n')
    result = check(root)
    checker.expect(result.returncode == 1, "hash drift exits 1")
    checker.expect(
        "arch/x86/fakecpu/core.json" in result.stderr,
        "hash drift names the file on stderr",
        result.stderr,
    )

    # 3. RECORD ref disagrees with the gate constant: exit 1 naming both.
    root = make_root(out, "gate-drift", "linux-7.2.4", CLEAN_FILES)
    (root / "CMakeLists.txt").write_text("set(_pmu_events_expected_ref linux-1.2.3)\n")
    result = check(root)
    checker.expect(result.returncode == 1, "gate disagreement exits 1")
    checker.expect(
        "CMakeLists.txt" in result.stderr and "disagrees" in result.stderr,
        "gate disagreement names the CMakeLists.txt file",
        result.stderr,
    )

    # 4. hash listed, file missing from disk: exit 1 naming it.
    root = make_root(out, "missing-file", "linux-7.2.4", CLEAN_FILES)
    (root / "external" / "pmu-events" / "arch" / "x86" / "mapfile.csv").unlink()
    result = check(root)
    checker.expect(
        result.returncode == 1 and "arch/x86/mapfile.csv" in result.stderr,
        "RECORD-listed-but-missing exits 1 naming the file",
        result.stderr,
    )

    # 5. file on disk absent from RECORD: exit 1 naming it.
    root = make_root(out, "extra-file", "linux-7.2.4", CLEAN_FILES)
    (root / "external" / "pmu-events" / "arch" / "x86" / "surprise.json").write_bytes(b"[]")
    result = check(root)
    checker.expect(
        result.returncode == 1 and "arch/x86/surprise.json" in result.stderr,
        "tree file absent from RECORD exits 1 naming the file",
        result.stderr,
    )

    # 6. --to with a synthetic tarball (offline): replaces the tree,
    # rewrites RECORD, bumps the gate, then --check is clean.
    root = make_root(out, "repin", "linux-7.2.4", CLEAN_FILES)
    ball = out / "repin-tarball.tar.gz"
    make_tarball(
        ball,
        "linux-9.9.9",
        {
            "arch/x86/newcpu/core.json": json.dumps(CYCLES).encode(),
            "arch/x86/mapfile.csv": MAPFILE.encode(),
            "arch/x86/newcpu/Build": b"obj-y := .json\n",
            "arch/x86/newcpu/gen.py": b"raise SystemExit(1)\n",
            "arch/powerpc/never.json": json.dumps(CYCLES).encode(),
        },
    )
    result = run_tool("--to", "linux-9.9.9", "--tarball", ball, "--root", root)
    checker.expect(result.returncode == 0, "offline --to exits 0", result.stderr)
    tree = root / "external" / "pmu-events"
    checker.expect(
        (tree / "arch/x86/newcpu/core.json").read_bytes() == json.dumps(CYCLES).encode()
        and not (tree / "arch/x86/newcpu/Build").exists()
        and not (tree / "arch/x86/newcpu/gen.py").exists()
        and not (tree / "arch/powerpc/never.json").exists()
        and not (tree / "arch/x86/fakecpu/core.json").exists(),
        "--to replaces the tree with only in-scope files",
    )
    checker.expect(
        "ref: linux-9.9.9" in (tree / "RECORD").read_text()
        and "set(_pmu_events_expected_ref linux-9.9.9)" in (root / "CMakeLists.txt").read_text(),
        "--to rewrites the RECORD ref and bumps the gate constant",
    )
    checker.expect("fakecpu: 1 -> 0 event entries" in result.stdout, "--to prints the per-architecture digest", result.stdout)
    result = check(root)
    checker.expect(result.returncode == 0, "--check is clean after --to", result.stderr)

    # 7. validation failure: exit 1, tree and RECORD untouched.
    root = make_root(out, "repin-bad", "linux-7.2.4", CLEAN_FILES)
    record_before = (root / "external" / "pmu-events" / "RECORD").read_bytes()
    ball = out / "repin-bad.tar.gz"
    make_tarball(
        ball,
        "linux-9.9.9",
        {"arch/x86/newcpu/core.json": b"{not json", "arch/x86/mapfile.csv": MAPFILE.encode()},
    )
    result = run_tool("--to", "linux-9.9.9", "--tarball", ball, "--root", root)
    checker.expect(result.returncode == 1, "--to exits 1 on invalid JSON", result.stdout)
    checker.expect(
        (root / "external" / "pmu-events" / "RECORD").read_bytes() == record_before
        and (root / "external" / "pmu-events" / "arch/x86/fakecpu/core.json").exists(),
        "failed validation leaves the tree and RECORD untouched",
    )

    return EXIT_ASSERT if checker.failures else EXIT_OK


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--out", type=Path, required=True, help="scratch dir for fixture roots")
    parser.add_argument("fixture_dir", type=Path, nargs="?", default=SCRIPT_DIR)
    args = parser.parse_args()
    if not TOOL.is_file():
        sys.stderr.write(f"tool script not present: {TOOL}\n")
        return EXIT_TOOL_MISSING
    args.out.mkdir(parents=True, exist_ok=True)
    return scenarios(args.out)


if __name__ == "__main__":
    sys.exit(main())
