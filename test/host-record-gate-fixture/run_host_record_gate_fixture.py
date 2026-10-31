#!/usr/bin/env python3
"""Fixture for tools/ci/host_record_gate.sh (specs/031-host-validation).

The gate runs the reader over the records this fixture plants under a scratch
root, so the committed-record check stands proven against a good fake record
and against a bad fake record that no hardware read reaches (FR-035). The
scratch tree carries the pinned schema and a mapfile whose row names the fake
identity, the shape the reader resolves under the tree root it reads
(FR-008, FR-020, FR-028).

Usage: run_host_record_gate_fixture.py <good|bad> <reader> <repo-root>
"""

from __future__ import annotations

import json
import subprocess
import sys
import tempfile
from pathlib import Path

SCHEMA = "docs/schema/host-record-v1.json"
MAPFILE = "external/pmu-events/arch/x86/mapfile.csv"
RECORD_KEY = "GenuineIntel-6-C6-2"
MAPFILE_ROWS = (
    "Family-model,Version,Filename,EventType\n"
    "GenuineIntel-6-C[56],v1.17,arrowlake,core\n"
)


def record(version: str) -> str:
    """The document one host writes, the four root keys in the order the
    schema spells them (FR-020, FR-021)."""
    return json.dumps(
        {
            "recordVersion": version,
            "cpu": {
                "vendor": "GenuineIntel",
                "family": 6,
                "model": 198,
                "stepping": 2,
                "key": RECORD_KEY,
                "directory": "arch/x86/arrowlake/",
            },
            "machine": {
                "packages": 1,
                "l3Groups": 2,
                "cores": 20,
                "threads": 20,
                "mask": "0-19",
                "l3": [
                    {"ordinal": 0, "cpus": "0-7"},
                    {"ordinal": 1, "cpus": "8-19"},
                ],
                "nodes": [{"number": 0, "cpus": "0-19"}],
            },
            "evidence": [],
        },
        separators=(",", ":"),
    )


def main() -> int:
    scenario = sys.argv[1] if len(sys.argv) > 1 else ""
    reader = Path(sys.argv[2]) if len(sys.argv) > 2 else None
    repo = Path(sys.argv[3]) if len(sys.argv) > 3 else None
    if scenario not in ("good", "bad") or reader is None or repo is None:
        print(
            "usage: run_host_record_gate_fixture.py"
            " <good|bad> <reader> <repo-root>",
            file=sys.stderr,
        )
        return 2

    with tempfile.TemporaryDirectory() as scratch:
        root = Path(scratch)
        (root / SCHEMA).parent.mkdir(parents=True)
        (root / SCHEMA).write_text((repo / SCHEMA).read_text())
        (root / MAPFILE).parent.mkdir(parents=True)
        (root / MAPFILE).write_text(MAPFILE_ROWS)
        records = root / "validation"
        records.mkdir()
        version = "1.0" if scenario == "good" else "0.9"
        (records / f"{RECORD_KEY}.json").write_text(record(version))
        run = subprocess.run(
            ["bash", str(repo / "tools" / "ci" / "host_record_gate.sh"),
             str(reader), str(root), str(records)],
            capture_output=True,
            text=True,
            check=False,
        )
        out = run.stdout + run.stderr
        want = 0 if scenario == "good" else 2
        named = f"{records / f'{RECORD_KEY}.json'}" in out
        if scenario == "good":
            ok = run.returncode == want and named and f"{RECORD_KEY}  valid" in out
        else:
            ok = (
                run.returncode == want
                and named
                and "  stale  " in out
                and "0.9" in out
                and "1.0" in out
            )
        if ok:
            print(f"PASS {scenario} record")
            return 0
        print(f"FAIL {scenario} record: exit {run.returncode}, want {want}")
        for line in out.strip().splitlines()[:6]:
            print(f"    {line}")
        return 1


if __name__ == "__main__":
    sys.exit(main())
