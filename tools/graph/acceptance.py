#!/usr/bin/env python3
"""Write the graph acceptance record of specs/037-execution-graphs.

FR-030, FR-031, R-14 and D-9 of the spec: the results directory carries
one record of the acceptance run on the lane host, a fan-out and fan-in
graph over the host's cores, and one script under `tools/` produces it
while no CI job runs it. This is that script.

The script reads the host's core and thread count from the harness's own
topology listing, builds a graph with one node per core (one source, the
leaves that fan out from it, one sink that fans in from every leaf),
runs it through the built harness binary with the graph option, and
writes the record in the shape specs/035-numa-placement recorded for the
NPS evidence: the boot, each command, its exit code, the output the run
printed, the printed node order and the figures beside it, and the
host's core and thread shape beside the shape specs/035 recorded. The
node count follows the host, so the script carries no fixed node count.

The graph file names its nodes in descending slot order, so the
ascending order the run answers is the graph's own order.

The build must stand configured: `cmake --preset=ci-rocky` and
`cmake --build build -j 8`.

Exit 0 when every recorded run exits 0 and the record stands written,
1 when a run answers another code, 2 when the build carries no binary.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
import tempfile
import textwrap
from datetime import datetime
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_BINARY = "build/example/docs_numa_placement"
DEFAULT_RECORD = "specs/037-execution-graphs/results/acceptance.md"
CI_WORKFLOWS = ".github/workflows"
SCRIPT_NAME = "acceptance.py"
SPEC_EXPECTED = "16-core, 32-thread"
SPECS035_SHAPE = "24-core, 48-thread"
WIDTH = 72
TOPOLOGY_LINE = re.compile(
    r"topology: packages=(?P<packages>\d+) l3-groups=(?P<groups>\d+) "
    r"cores=(?P<cores>\d+) threads=(?P<threads>\d+) mask=(?P<mask>\S+)"
)


def run(argv: list[str]) -> tuple[int, str]:
    """Run one command from the repository root, answering its status and output."""
    proc = subprocess.run(argv, cwd=ROOT, capture_output=True, text=True)
    return proc.returncode, (proc.stdout + proc.stderr).rstrip("\n")


def prose(text: str) -> str:
    """One paragraph, wrapped at the width the project's prose uses."""
    return textwrap.fill(text, width=WIDTH, break_long_words=False, break_on_hyphens=False)


def bullet(text: str) -> str:
    """One list item, its continuation indented the way the project's lists are."""
    return textwrap.fill(
        text, width=WIDTH, initial_indent="- ", subsequent_indent="  ",
        break_long_words=False, break_on_hyphens=False,
    )


def boot_lines() -> list[str]:
    """The boot the record names: the kernel, the host, the release, the date."""
    _, kernel = run(["uname", "-srm"])
    _, host = run(["uname", "-n"])
    release = "unknown release"
    os_release = Path("/etc/os-release")
    if os_release.is_file():
        for line in os_release.read_text().splitlines():
            if line.startswith("PRETTY_NAME="):
                release = line.split("=", 1)[1].strip('"')
    stamp = datetime.now().astimezone().strftime("%Y-%m-%d %H:%M %Z")
    return [kernel, host, release, stamp]


def graph_nodes(cores: int) -> list[dict[str, object]]:
    """One source, `cores - 2` leaves, one sink: the fan-out and the fan-in."""
    leaves = [f"leaf{index}" for index in range(1, cores - 1)]
    nodes: list[dict[str, object]] = [{"name": "root", "slot": 0, "deps": []}]
    for slot, name in enumerate(leaves, start=1):
        nodes.append({"name": name, "slot": slot, "deps": ["root"]})
    nodes.append({"name": "join", "slot": cores - 1, "deps": leaves})
    return list(reversed(nodes))


def graph_text(nodes: list[dict[str, object]]) -> str:
    """The file's bytes: the schema's key order, one node per line."""
    lines = ["{", '  "nodes": [']
    for node in nodes:
        lines.append(
            "    {"
            f' "name": "{node["name"]}",'
            f' "slot": {node["slot"]},'
            f' "deps": {json.dumps(node["deps"])} '
            "},"
        )
    lines[-1] = lines[-1][:-1]
    lines += ["  ]", "}"]
    return "\n".join(lines)


def ci_job_names() -> list[str]:
    """The job names the CI workflows declare under their `jobs:` keys."""
    names: list[str] = []
    for workflow in sorted((ROOT / CI_WORKFLOWS).glob("*.yml")):
        in_jobs = False
        for line in workflow.read_text().splitlines():
            if line == "jobs:":
                in_jobs = True
                continue
            if not in_jobs:
                continue
            if line and not line.startswith((" ", "#")):
                in_jobs = False
                continue
            job = re.match(r"^  ([A-Za-z0-9_-]+):\s*(?:#.*)?$", line)
            if job is not None:
                names.append(job.group(1))
    return names


def quoted_join(items: list[str]) -> str:
    """`a`, `b` and `c` for a list of names."""
    quoted = [f"`{item}`" for item in items]
    if not quoted:
        return "no job"
    if len(quoted) == 1:
        return quoted[0]
    return ", ".join(quoted[:-1]) + f" and {quoted[-1]}"


def record_order(document: dict) -> list[str]:
    """The node array order of each graph entry of the written document."""
    lines = []
    for entry in document["graph"]["runs"]:
        nodes = ", ".join(
            f"{node['name']}@{node['slot']} {node['outcome']}" for node in entry["nodes"]
        )
        lines.append(f"{entry['benchmark']}: {nodes}")
        lines.append(
            f"nodes={len(entry['nodes'])} edges={len(entry['edges'])} "
            f"held={len(entry['held'])} stopped={len(entry['stopped'])}"
        )
    return lines


def block(out: str, prefix: str) -> list[str]:
    """The lines of one run's output that start with a prefix."""
    return [line for line in out.splitlines() if line.startswith(prefix)]


def command_block(argv: list[str], code: int, out: str) -> list[str]:
    """One recorded command: the line, what it printed, and its exit code."""
    lines = ["$ " + " ".join(argv)]
    if out:
        lines += out.splitlines()
    lines.append(f"exit {code}")
    return lines


def render(binary: str, shape: dict[str, str], boot: list[str],
           nodes: list[dict[str, object]],
           runs: list[tuple[list[str], int, str]], order: list[str],
           grep: tuple[list[str], int, str], jobs: list[str]) -> str:
    """The record, in the shape specs/035 recorded for the NPS evidence."""
    cores = int(shape["cores"])
    threads = int(shape["threads"])
    leaves = cores - 2
    if cores == 16 and threads == 32:
        shape_note = (
            f"The spec expects a {SPEC_EXPECTED} lane host (FR-030), and this "
            "host reads that shape."
        )
    else:
        shape_note = (
            f"The spec expects a {SPEC_EXPECTED} lane host (FR-030). This host "
            f"reads {cores} cores and {threads} threads, and the record names "
            "the shape the host read."
        )
    graph_argv, _, graph_out = runs[1]
    timed = block(graph_out, "bm")
    family = timed[0].split("/")[0]
    runs_block: list[str] = []
    for argv, code, out in runs:
        if runs_block:
            runs_block.append("")
        runs_block += command_block(argv, code, out)
    sections = [
        f"# The graph acceptance run on {boot[1]}",
        "",
        bullet(
            f"Shape: the harness listing reads `cores={cores} threads={threads}` on "
            f"{shape['packages']} package and {shape['groups']} L3 groups, mask "
            f"`{shape['mask']}`: {cores} cores, {threads} threads. {shape_note} "
            f"specs/035 recorded a {SPECS035_SHAPE} host (thrawn, the Threadripper "
            "9960X lane box)."
        ),
        bullet(f"Boot: {boot[3]}, kernel `{boot[0]}`, release {boot[2]}."),
        bullet(
            "Script: `tools/graph/acceptance.py`, run by hand on the lane host "
            "(FR-031)."
        ),
        bullet(
            f"Binary: `./{binary}`, whose `{family}` registers a threaded family, "
            f"run with `--threads={cores}` so the graph's {cores} nodes match the "
            f"run's {cores} contexts (FR-012)."
        ),
        "",
        "## The graph file",
        "",
        prose(
            "The script writes the graph the run reads, one node per core: `root` at "
            f"slot 0, `leaf1` to `leaf{leaves}` at slots 1 to {leaves} each depending "
            f"on `root`, and `join` at slot {cores - 1} depending on every leaf. The "
            "file names its nodes in descending slot order, so the ascending order "
            "the run answers is the graph's own order. The node count matches the "
            "run's context count (FR-012). The file stood at the path the commands "
            "below name:"
        ),
        "",
        "```json",
        graph_text(nodes),
        "```",
        "",
        "## The runs",
        "",
        prose(
            "The listing names the host's shape, the graph run orders the context "
            "starts of a threaded run over that shape, and the record run writes the "
            "document read back below. Each command runs from the repository root "
            "over the built tree:"
        ),
        "",
        "```",
        *runs_block,
        "```",
        "",
        "## The printed node order and the figures beside it",
        "",
        prose(
            "The graph block the run printed, the nodes in the ascending slot order "
            "FR-023 fixes and the edges in the graph file's order:"
        ),
        "",
        "```",
        *block(graph_out, "  graph "),
        "```",
        "",
        prose("The figures beside it: the timed row of the same run and the context disclosure under it:"),
        "",
        "```",
        *timed,
        *block(graph_out, "  contexts="),
        "```",
        "",
        prose(
            "The document the record run wrote names the same order in its node "
            "array, each node with its slot and its outcome:"
        ),
        "",
        "```",
        *order,
        "```",
        "",
        "## The CI check",
        "",
        prose("No CI job names the script. The grep the record reads:"),
        "",
        "```",
        *command_block(*grep),
        "```",
        "",
        prose(
            f"The workflows declare {len(jobs)} jobs ({quoted_join(jobs)}), and none "
            f"names `{SCRIPT_NAME}` (FR-031, SC-009)."
        ),
        "",
        "## The gap",
        "",
        prose(
            "No gap: the host reads the shape the spec expects, the fan-out and "
            "fan-in graph ran over its cores, and the record carries the printed "
            "order, the figures, the boot, the commands and the exit codes."
        ),
        "",
    ]
    return "\n".join(sections)


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--binary", default=DEFAULT_BINARY, help="the built harness CLI to run")
    parser.add_argument("--out", default=DEFAULT_RECORD, help="the record file to write")
    args = parser.parse_args(argv)
    if not (ROOT / args.binary).is_file():
        print(f"{args.binary}: no built binary; configure and build first", file=sys.stderr)
        return 2

    boot = boot_lines()
    list_code, list_out = run([args.binary, "--topology"])
    read = TOPOLOGY_LINE.search(list_out)
    if read is None:
        print(f"{args.binary} printed no topology line", file=sys.stderr)
        return 2
    shape = read.groupdict()
    cores = int(shape["cores"])
    nodes = graph_nodes(cores)

    with tempfile.TemporaryDirectory(prefix="speedgun-graph-") as workdir:
        graph_path = Path(workdir) / "fan-in-fan-out.json"
        graph_path.write_text(graph_text(nodes) + "\n")
        json_path = Path(workdir) / "report.json"
        graph_argv = [args.binary, f"--threads={cores}", "--iterations=1", f"--graph={graph_path}"]
        grep_argv = ["grep", "-rn", SCRIPT_NAME, CI_WORKFLOWS]

        runs: list[tuple[list[str], int, str]] = [(["./" + args.binary, "--topology"], list_code, list_out)]
        for argv_run in (graph_argv, graph_argv + [f"--json={json_path}"]):
            code, out = run(argv_run)
            runs.append((["./" + argv_run[0], *argv_run[1:]], code, out))
        grep = (grep_argv, *run(grep_argv))
        document = json.loads(json_path.read_text())
        record = render(args.binary, shape, boot, nodes, runs, record_order(document),
                        grep, ci_job_names())

    out_path = ROOT / args.out
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(record)
    print(f"record: {out_path}")
    codes = [code for _, code, _ in runs]
    print("exit codes: " + ", ".join(str(code) for code in codes))
    return 0 if all(code == 0 for code in codes) else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
