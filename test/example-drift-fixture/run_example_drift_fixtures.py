#!/usr/bin/env python3
"""Fixture harness for tools/docs/example_drift.py (contracts/example-drift.md R-8).

Every case starts from a clean scratch tree, plants one difference, and
asserts one verdict of one tool run. The tree holds a mini main page plus
mini example, header, and CLI sources; the real README.md is never
touched. With the tool absent the runner prints its red state and fails.
"""

from __future__ import annotations

import re
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools" / "docs"))

from example_drift import (  # noqa: E402
    SECTION_LINE_CAPS,
    SECTIONS,
    SECTION_OWNERS,
)


def owner(title: str) -> str:
    return "example/docs/" + SECTION_OWNERS[title][0]

ROOT = Path()
FAILURES: list[str] = []


def slug(title: str) -> str:
    return title.lower().replace(",", "").replace(" ", "-")


def snippet(title: str) -> str:
    return f"auto bm{slug(title).replace('-', '')}(sg::State&) -> void {{}}"


def mini_page() -> str:
    parts = [
        "# speedgun-ng\n\nA C++23 benchmark framework for Linux.\n\n",
        "## Quick start\n\n",
        "<!-- sg-example: quickstart.sh#run -->\n",
        "```sh\n./build/first_benchmark\n```\n\n",
        "<!-- sg-sample-output: first_benchmark.expected#report -->\n",
        "```text\n",
        "speedgun-ng 0.8.0 build=Release\n",
        "bmSum   iterations=1024  real time/iter (ns)=1.5  "
        "cpu time/iter (ns)=1.4  overhead floor (ns)=680.000  "
        "endpoint samples=2\n",
        "```\n\n",
        "## How you do it\n",
    ]
    for title in SECTIONS:
        parts.append(
            f"\n### {title}\n\nOne purpose sentence.\n\n"
            f"<!-- sg-example: {owner(title)}#{slug(title)} -->\n"
            f"```cpp\n{snippet(title)}\n```\n\nOne result sentence.\n"
        )
    parts.append(
        "\n## Quick reference\n\n"
        "| Name | What it does | Teaches |\n| --- | --- | --- |\n"
        "| `SG_BENCHMARK` | Registers one family | The run loop |\n"
        "| `speedgunMain` | Runs the registry | The command line |\n"
        "| `clobberMemory` | Clobbers cached values | The run loop |\n\n"
        "| Option | What it does |\n| --- | --- |\n"
        "| `--filter` | Selects by name |\n| `--list` | Prints the registry |\n"
        "| `--repetitions` | Sets the repetition count |\n\n"
        "The maintainer facts live at "
        "[docs/pages/building.md](docs/pages/building.md).\n"
    )
    return "".join(parts)


def build_tree(root: Path) -> None:
    for leaf in ("example/docs", "include/speedgun-ng", "source/harness", "docs/pages"):
        (root / leaf).mkdir(parents=True, exist_ok=True)
    for stale in ("docs/pages/harness.md", "docs/pages/building.md"):
        (root / stale).unlink(missing_ok=True)

    for title in SECTIONS:
        leaf = root / owner(title)
        leaf.parent.mkdir(parents=True, exist_ok=True)
        leaf.write_text(
            '//include "speedgun-ng/benchmark.hpp"\n\n'
            f"// sg-example:begin {slug(title)}\n{snippet(title)}\n"
            f"// sg-example:end {slug(title)}\n"
        )
    (root / "example" / "quickstart.sh").write_text(
        "#!/usr/bin/env bash\n"
        "# sg-example:begin run\n./build/first_benchmark\n"
        "# sg-example:end run\n"
    )
    (root / "example" / "first_benchmark.expected").write_text(
        "# sg-example:begin report\n"
        "speedgun-ng 0.8.0 build=Release\n"
        "bmSum   iterations=1024  real time/iter (ns)=1.5  "
        "cpu time/iter (ns)=1.4  overhead floor (ns)=680.000  "
        "endpoint samples=2\n"
        "# sg-example:end report\n"
    )
    (root / "include" / "speedgun-ng" / "benchmark.hpp").write_text(
        "auto speedgunMain(int argc, char** argv) -> int;\n"
        "auto doNotOptimize(const T&) -> void;\n"
        "auto clobberMemory() -> void;\n"
        "class State {};\n"
        "class BenchmarkHandle {};\n"
        "class Fixture {};\n"
        "#define SG_BENCHMARK(fn) 1\n"
    )
    (root / "include" / "speedgun-ng" / "barrier.hpp").write_text(
        "auto barrier() -> void;\n"
    )
    (root / "source" / "harness" / "cli.cpp").write_text(
        "static option longOptions[] = {\n"
        "    {\"filter\", required_argument, nullptr, 'f'},\n"
        "    {\"list\", no_argument, nullptr, 'l'},\n"
        "    {\"repetitions\", required_argument, nullptr, 'r'},\n"
        "    {nullptr, 0, nullptr, 0},\n"
        "};\n"
    )
    (root / "README.md").write_text(mini_page())
    (root / "docs" / "pages" / "detail.md").write_text(
        "# Detail\n\nA marked block that matches its region:\n\n"
        f"<!-- sg-example: {owner(SECTIONS[0])}#{slug(SECTIONS[0])} -->\n"
        f"```cpp\n{snippet(SECTIONS[0])}\n```\n\nAn unmarked block stays alone:\n\n"
        "```cpp\nnot a translation unit at all\n```\n"
    )


def page(root: Path) -> Path:
    return root / "README.md"


def run_tool(*flags: str) -> tuple[int, str]:
    result = subprocess.run(
        [sys.executable, str(ROOT / "tool.py"), str(ROOT / "tree"), *flags],
        capture_output=True,
        text=True,
        check=False,
    )
    return result.returncode, result.stdout + result.stderr


def check(case: str, want_pass: bool, needles: tuple[str, ...], code: int,
          output: str) -> None:
    ok = (code == 0) == want_pass and all(needle in output for needle in needles)
    if ok:
        print(f"PASS {case}")
        return
    head = output.strip().splitlines()
    FAILURES.append(
        f"{case}: want pass={want_pass} needles={needles}, exit {code}: "
        f"{head[0] if head else 'no output'}"
    )
    print(f"FAIL {case}")
    for line in output.strip().splitlines()[:6]:
        print(f"    {line}")


def case(case_name: str, mutate, want_pass: bool, needles: tuple[str, ...],
         *flags: str) -> None:
    tree = ROOT / "tree"
    mutate(tree)
    code, output = run_tool(*flags)
    check(case_name, want_pass, needles, code, output)
    build_tree(tree)


def main() -> int:
    repo = Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
    tool = repo / "tools" / "docs" / "example_drift.py"
    if not tool.is_file():
        print(f"red state: the example drift gate has no tool at {tool}")
        return 1

    global ROOT
    with tempfile.TemporaryDirectory() as name:
        ROOT = Path(name)
        (ROOT / "tool.py").write_text(
            "import sys\n"
            f"sys.path.insert(0, {str(tool.parent)!r})\n"
            "from example_drift import main\n"
            "sys.exit(main(sys.argv[1:]))\n"
        )
        tree = ROOT / "tree"
        tree.mkdir()
        build_tree(tree)

        code, output = run_tool()
        check("clean scratch tree", True, (), code, output)

        drift = snippet(SECTIONS[0]).replace("State&", "State& ")
        case(
            "planted one-character drift",
            lambda root: page(root).write_text(
                page(root).read_text().replace(snippet(SECTIONS[0]), drift, 1)
            ),
            False,
            ("example/docs/" + SECTION_OWNERS[SECTIONS[0]][0], f"block '{slug(SECTIONS[0])}' at line",
             "differs from its region"),
        )
        case(
            "main-page block with no marker",
            lambda root: page(root).write_text(
                page(root).read_text().replace(
                    f"<!-- sg-example: {owner(SECTIONS[0])}#{slug(SECTIONS[0])} -->\n", "", 1
                )
            ),
            False,
            ("carries no region marker", "block at line"),
        )
        case(
            "detail-page marked block with drift",
            lambda root: write_detail(
                root, snippet(SECTIONS[0]).replace("{}", "{ }")
            ),
            False,
            ("detail.md", "differs from its region"),
        )
        case(
            "detail-page unmarked block",
            lambda root: (root / "docs" / "pages" / "detail.md").write_text(
                "# Detail\n\n```cpp\nanything at all\n```\n"
            ),
            True,
            (),
        )
        case(
            "region moved inside its file",
            lambda root: move_region(root, SECTIONS[-1]),
            True,
            (),
        )
        case(
            "section quotes another section's file",
            lambda root: page(root).write_text(
                page(root)
                .read_text()
                .replace(
                    f"<!-- sg-example: {owner(SECTIONS[0])}#{slug(SECTIONS[0])} -->",
                    f"<!-- sg-example: {owner(SECTIONS[1])}#{slug(SECTIONS[0])} -->",
                    1,
                )
            ),
            False,
            ("README.md", "belongs to another section"),
        )
        case(
            "unbalanced region markers",
            lambda root: drop_line(root / owner(SECTIONS[1]),
                                   f"// sg-example:end {slug(SECTIONS[1])}"),
            False,
            ("example/docs/" + SECTION_OWNERS[SECTIONS[1]][0], "is never closed"),
        )
        case(
            "--names row absent from the headers",
            lambda root: (root / "include" / "speedgun-ng" / "benchmark.hpp").write_text(
                (root / "include" / "speedgun-ng" / "benchmark.hpp")
                .read_text()
                .replace("clobberMemory", "otherName")
            ),
            False,
            ("is not in", "clobberMemory"),
            "--names",
        )
        case(
            "--options table missing one row",
            lambda root: page(root).write_text(
                page(root).read_text().replace(
                    "| `--repetitions` | Sets the repetition count |\n", ""
                )
            ),
            False,
            ("has no option row",),
            "--options",
        )
        case(
            "planted commit-hash-shaped token",
            lambda root: replace_prose(
                root, "Built from 9f3c1ab2d4e5f60718293a4b5c6d7e8f90123456."
            ),
            False,
            ("forbidden commit hash",),
            "--forbidden",
        )
        case(
            "planted literal header path in prose",
            lambda root: replace_prose(
                root, "The call lives in include/speedgun-ng/x.hpp now."
            ),
            False,
            ("forbidden source path",),
            "--forbidden",
        )
        case(
            "the same header path inside a fenced block",
            fenced_header_path,
            True,
            (),
            "--forbidden",
        )
        case(
            "user-guide page with spec provenance",
            user_page,
            False,
            ("docs/pages/harness.md",
             "forbidden requirement identifier: FR-005",
             "forbidden feature number: Feature 017",
             "forbidden contract label: C-3"),
            "--forbidden",
        )
        case(
            "build page keeps pins, paths, and job names",
            build_page,
            True,
            (),
            "--forbidden",
        )
        case(
            "--names owner-qualified member the headers declare",
            lambda root: add_api_row(root, "State::rangeMultiplier", True),
            True,
            (),
            "--names",
        )
        case(
            "--names owner-qualified member no header declares",
            lambda root: add_api_row(root, "State::missingMember", False),
            False,
            ("is not in", "State::missingMember"),
            "--names",
        )
        case(
            "mini page missing one required section",
            lambda root: page(root).write_text(
                page(root).read_text().replace(
                    f"### {SECTIONS[-1]}\n", f"### {SECTIONS[-1]} Renamed\n", 1
                )
            ),
            False,
            (f"no '### {SECTIONS[-1]}'",),
            "--sections",
        )
        case(
            "block whose sg-requires names no region",
            lambda root: page(root).write_text(
                page(root).read_text().replace(
                    f"<!-- sg-example: {owner(SECTIONS[0])}#{slug(SECTIONS[0])} -->\n",
                    f"<!-- sg-requires: {owner(SECTIONS[0])}#nosuch -->\n"
                    f"<!-- sg-example: {owner(SECTIONS[0])}#{slug(SECTIONS[0])} -->\n",
                    1,
                )
            ),
            False,
            ("requires region 'nosuch'",),
        )
        case(
            "block whose sg-requires names a real region",
            lambda root: page(root).write_text(
                page(root).read_text().replace(
                    f"<!-- sg-example: {owner(SECTIONS[0])}#{slug(SECTIONS[0])} -->\n",
                    f"<!-- sg-requires: {owner(SECTIONS[0])}#{slug(SECTIONS[0])} -->\n"
                    f"<!-- sg-example: {owner(SECTIONS[0])}#{slug(SECTIONS[0])} -->\n",
                    1,
                )
            ),
            True,
            (),
        )
        case(
            "Registration at its own override cap passes",
            lambda root: pad_registration(root, SECTION_LINE_CAPS["Registration"]),
            True,
            (),
            "--sections",
        )
        case(
            "Registration one line over its own override cap fails",
            lambda root: pad_registration(root, SECTION_LINE_CAPS["Registration"] + 1),
            False,
            (
                f"section 'Registration' runs {SECTION_LINE_CAPS['Registration'] + 1} lines",
                f"the cap is {SECTION_LINE_CAPS['Registration']}",
            ),
            "--sections",
        )
        case(
            "relative documentation link",
            lambda root: None,
            True,
            (),
        )
        emit_case()
    return 1 if FAILURES else 0


def pad_registration(root: Path, span: int) -> None:
    text = page(root).read_text()
    start = text.index("### Registration\n")
    stop = text.index("\n### ", start)
    filler = "".join(f"Filler sentence {n}.\n" for n in range(span - 6))
    replacement = (
        "### Registration\n"
        f"<!-- sg-example: {owner('Registration')}#{slug('Registration')} -->\n"
        f"```cpp\n{snippet('Registration')}\n```\n"
        + filler
    )
    page(root).write_text(text[:start] + replacement + text[stop:])


def add_api_row(root: Path, name: str, declared: bool) -> None:
    page(root).write_text(
        page(root).read_text().replace(
            "| `speedgunMain` |",
            f"| `{name}` | Reads one argument position |\n| `speedgunMain` |",
        )
    )
    if declared:
        header = root / "include" / "speedgun-ng" / "benchmark.hpp"
        header.write_text(
            header.read_text() + "auto rangeMultiplier(int factor) -> int;\n"
        )


def user_page(root: Path) -> None:
    (root / "docs" / "pages" / "harness.md").write_text(
        "# The benchmark harness\n\nThe sweep of C-3 multiplies every bound "
        "by FR-005 of Feature 017.\n"
    )


def build_page(root: Path) -> None:
    (root / "docs" / "pages" / "building.md").write_text(
        "# Building speedgun-ng\n\nPin libxml2 at v2.13.8, read "
        "source/harness/cli.cpp before the compiler version clang-18.1.8 gate, "
        "and let the test job run example/CMakeLists.txt.\n"
    )


def write_detail(root: Path, body: str) -> None:
    detail = root / "docs" / "pages" / "detail.md"
    detail.write_text(
        detail.read_text().replace(snippet(SECTIONS[0]), body, 1)
    )


def move_region(root: Path, title: str) -> None:
    source = root / owner(title)
    text = source.read_text()
    block = (
        f"// sg-example:begin {slug(title)}\n{snippet(title)}\n"
        f"// sg-example:end {slug(title)}\n"
    )
    source.write_text(block + text.replace(block, "", 1))


def drop_line(source: Path, marker: str) -> None:
    source.write_text(source.read_text().replace(marker + "\n", "", 1))


def replace_prose(root: Path, sentence: str) -> None:
    page(root).write_text(
        page(root).read_text().replace(
            "A C++23 benchmark framework for Linux.", sentence
        )
    )


def fenced_header_path(root: Path) -> None:
    """The header path in a command block, and in the region behind it."""
    page(root).write_text(
        page(root).read_text().replace(
            "```sh\n./build/first_benchmark\n```",
            "```sh\n./build/first_benchmark\ncat include/speedgun-ng/x.hpp\n```",
        )
    )
    source = root / "example" / "quickstart.sh"
    source.write_text(
        source.read_text().replace(
            "./build/first_benchmark\n# sg-example:end run",
            "./build/first_benchmark\ncat include/speedgun-ng/x.hpp\n"
            "# sg-example:end run",
        )
    )


def emit_case() -> None:
    tree = ROOT / "tree"
    count = len(SECTIONS) + 1
    page(tree).write_text(
        page(tree).read_text().replace(
            "<!-- sg-sample-output: first_benchmark.expected#report -->",
            f"<!-- sg-example: {owner(SECTIONS[1])}#{slug(SECTIONS[1])} -->\n"
            f"```cpp\n{snippet(SECTIONS[1])}\n```\n\n"
            "<!-- sg-sample-output: first_benchmark.expected#report -->",
        )
    )
    out = ROOT / "snippets"
    code, output = run_tool("--emit-blocks", str(out))
    written = sorted(out.glob("*.snippet")) if out.is_dir() else []
    expected = f"emitted {count} cpp snippet of {count} cpp fence"
    shells = [path for path in written if "./build/" in path.read_text()]
    named = all(re.match(r"^\d{2}_[a-z0-9-]+\.snippet$", path.name) for path in written)
    ok = (
        code == 0
        and expected in output
        and len(written) == count
        and named
        and not shells
    )
    check("--emit-blocks skips shell and cmake fences", True, (expected,), code,
          output + "\n".join(path.name for path in written) + "\n".join(
              path.read_text() for path in shells
          ))
    build_tree(tree)


if __name__ == "__main__":
    sys.exit(main())
