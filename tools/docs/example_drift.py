#!/usr/bin/env python3
"""Keep prose code blocks equal to the example regions they quote.

Contracts: specs/018-docs-main-page/contracts/example-drift.md (R-1 to R-7,
R-10) and specs/018-docs-main-page/contracts/main-page-structure.md (C-B,
C-C, C-D, C-F). Reads repository sources only; never opens a build
directory. Exit 0 clean, 1 with any finding, 2 on a usage error.
"""

from __future__ import annotations

import argparse
import os
import re
import sys
from pathlib import Path

BEGIN = "sg-example:begin"
END = "sg-example:end"
MARKER = re.compile(r"^\s*(?://|#)\s*sg-example:(begin|end)\s+([a-z0-9][a-z0-9-]*)\s*$")
FENCE = re.compile(r"^\s{0,3}(?P<run>`{3,}|~{3,})(?P<info>[^`]*)$")
PREMARK = re.compile(r"^\s*<!--\s*(sg-example|sg-sample-output):\s*(\S+)\s*-->\s*$")
REQUIRES = re.compile(r"^\s*<!--\s*sg-requires:\s*(\S+)\s*-->\s*$")
INFO_TOKEN = re.compile(r"^(sg-example|sg-sample-output)=(\S+)$")

# The twenty-one capabilities of the main page as the H3 titles of "How you do
# it": the twelve of spec FR-009 at the H3 audit point, the six specs/018
# adds (FR-028, FR-044, research D-13), the two specs/020 adds (FR-029), and
# the one specs/021 add (FR-027).
SECTIONS = (
    "The run loop",
    "Registration",
    "Argument families and ranges",
    "Fixtures",
    "Run control",
    "Timing modes",
    "Pause and resume",
    "The time unit",
    "Hardware counter metrics",
    "Skips",
    "The command line",
    "Reading the report",
    "User counters",
    "Throughput",
    "Labels",
    "Custom statistics",
    "Aggregate-only reporting",
    "Complexity fit",
    "Execution contexts",
    "Threads",
    "Percentiles",
)

# The example file each section owns its snippets by (FR-028, FR-032, D-12, D-13).
# Each section has exactly one owner: a shell block or a recorded run belongs to
# the user guide page that quotes it, not to a main page section.
SECTION_OWNERS = {
    "The run loop": ("run_loop.cpp",),
    "Registration": ("registration.cpp",),
    "Argument families and ranges": ("argument_families.cpp",),
    "Fixtures": ("fixtures.cpp",),
    "Run control": ("run_control.cpp",),
    "Timing modes": ("timing_modes.cpp",),
    "Pause and resume": ("pause_and_resume.cpp",),
    "The time unit": ("time_unit.cpp",),
    "Hardware counter metrics": ("counter_metrics.cpp",),
    "Skips": ("skips.cpp",),
    "The command line": ("command_line.cpp",),
    "Reading the report": ("reading_the_report.cpp",),
    "User counters": ("user_counters.cpp",),
    "Throughput": ("throughput.cpp",),
    "Labels": ("labels.cpp",),
    "Custom statistics": ("custom_statistics.cpp",),
    "Aggregate-only reporting": ("aggregates_only.cpp",),
    "Complexity fit": ("complexity_fit.cpp",),
    "Execution contexts": ("execution_contexts.cpp",),
    "Threads": ("threads.cpp",),
    "Percentiles": ("percentiles.cpp",),
}

# The distinctively named jobs of .github/workflows/ci.yml, plus the
# generic names only in their job-reference phrase: the page may not name
# a CI job, while ordinary English stays writable.
FORBIDDEN_JOBS = (
    "coverage-linux",
    "consumer-release",
    "consumer-audit",
    "prose-lint",
    "dbc-gate",
    "value-reuse-audit",
)
GENERIC_JOBS = ("lint", "test", "build", "docs", "configure", "format", "spell")

# The page carries six more capability sections and their snippets (FR-044);
# the per-section budget of FR-024 holds, which is what keeps a section a
# section rather than a second page.
# D-A of the PR #37 final pass (Charles, 2026-10-15): Registration teaches
# all three registration forms and Fixtures all three fixture macros, which
# adds 32 lines (25 to 38 and 24 to 43). No prose elsewhere on the page can
# give back 20 lines without gutting a result sentence, so the page cap rose
# by the smallest fitting amount, 550 to 570.
# specs/020 FR-034 measures its own move as the sections land: "Execution
# contexts" runs 25 lines and "Threads" 16, their quick-reference rows add
# 27, and the one disclosure result sentence of "Reading the report" adds 1,
# so 41 + 27 + 1 = 69 lines move the cap 570 to 639. The review then asks
# for prose trimmed before the cap moves: that disclosure sentence shares the
# paragraph line it annotates, so the page measures 638 lines and the cap
# sits there, with no snippet dropped.
# specs/021 FR-027 measures its own move: the "Percentiles" section runs 19
# lines and the two quick-reference option rows add 2, so 21 lines move the
# cap from 638 to 659, the measured length of the page as the feature lands.
# The metric-lookup region, its prose and its quick-reference and option
# rows are the 39 added lines that move the cap from 659 to 698, the
# measured length of the page as the metric section lands.
# D-15 of the PR #41 owner review (Charles, 2026-10-19): the metric section
# grew to 59 lines under a recorded raise, and the review keeps one snippet
# on the main page. The addMetric walk and the fold-and-read flow move to
# docs/pages/harness.md, the section keeps the metric-lookup region with one
# sentence each side at 24 lines, the higher cap is removed, and the page
# measures 666 lines: the 659 the percentile move left and the trimmed
# section with its rows, the format gate's canonical printf wrapping adding
# the one line the first measure hand-set away. The cap sits at that
# measured figure, below the 698 the metric landing first took.
# specs/022 FR-016 adds the placement quick-reference row: 1 line moves
# the cap to 667, the measured length as the parser lands.
PAGE_LINE_CAP = 667
SECTION_LINE_CAP = 25

# Only these sections carry a higher cap, each at the smallest span that
# holds its copied regions (D-A, Charles, 2026-10-15). Fixtures counts the
# sg-requires line the snippet gate reads beside the macro block.
SECTION_LINE_CAPS = {
    "Registration": 38,
    "Fixtures": 43,
}


class Finding(Exception):
    pass


def prose_pages(root: Path) -> list[Path]:
    pages = sorted(root.glob("README*"))
    agents = root / "AGENTS.md"
    if agents.is_file():
        pages.append(agents)
    pages.extend(sorted((root / "docs").rglob("*.md")))
    return [page for page in pages if page.is_file()]


def read_regions(path: Path) -> dict[str, list[str]]:
    if not path.is_file():
        raise Finding(f"{rel(path)}: example file not found")
    lines = path.read_text().splitlines()
    regions: dict[str, list[str]] = {}
    open_name: str | None = None
    start = 0
    for number, line in enumerate(lines, start=1):
        found = MARKER.match(line)
        if found is None:
            continue
        kind, name = found.groups()
        if kind == "begin":
            if open_name is not None:
                raise Finding(
                    f"{rel(path)}:{number}: marker nests inside region "
                    f"'{open_name}' opened at line {start}"
                )
            if name in regions:
                raise Finding(f"{rel(path)}:{number}: duplicate region '{name}'")
            open_name, start = name, number
        else:
            if open_name is None:
                raise Finding(f"{rel(path)}:{number}: end marker without a begin")
            if name != open_name:
                raise Finding(
                    f"{rel(path)}:{number}: end '{name}' closes region "
                    f"'{open_name}' opened at line {start}"
                )
            regions[name] = lines[start:number - 1]
            while regions[name] and not regions[name][-1].strip():
                # The formatter opens a blank line before a trailing comment,
                # and a blank line is not part of a quotable snippet.
                regions[name].pop()
            open_name = None
    if open_name is not None:
        raise Finding(f"{rel(path)}:{start}: region '{open_name}' is never closed")
    return regions


def rel(path: Path) -> str:
    return path.as_posix()


class Block:
    def __init__(
        self,
        page: Path,
        root: Path,
        fence_line: int,
        language: str,
        body: list[str],
        region: str | None,
        sample: bool,
    ) -> None:
        self.page = page
        self.root = root
        self.fence_line = fence_line
        self.language = language
        self.body = body
        self.region = region
        self.sample = sample
        self.requires: list[str] = []

    @property
    def label(self) -> str:
        return self.region or f"fence at line {self.fence_line}"

    def example_file(self) -> Path:
        name = (self.region or "").split("#", 1)[0]
        base = self.root if name.startswith("example/") else self.root / "example"
        return (base / name).resolve()


def scan_blocks(page: Path, root: Path) -> list[Block]:
    lines = page.read_text().splitlines()
    blocks: list[Block] = []
    index = 0
    while index < len(lines):
        found = FENCE.match(lines[index])
        if found is None:
            index += 1
            continue
        fence_line = index + 1
        run, info = found.group("run"), found.group("info").strip()
        tokens = info.split()
        language = tokens[0] if tokens else ""
        region, sample = None, False
        for token in tokens[1:]:
            mark = INFO_TOKEN.match(token)
            if mark is not None:
                region, sample = mark.group(2), mark.group(1) == "sg-sample-output"
        if region is None:
            behind = index - 1
            while behind >= 0 and not lines[behind].strip():
                behind -= 1
            if behind >= 0:
                pre = PREMARK.match(lines[behind])
                if pre is not None:
                    region = pre.group(2)
                    sample = pre.group(1) == "sg-sample-output"
                    probe = behind - 1
                else:
                    probe = behind
            else:
                probe = behind
        else:
            probe = index - 1
        block_requires: list[str] = []
        while probe >= 0:
            while probe >= 0 and not lines[probe].strip():
                probe -= 1
            mark = REQUIRES.match(lines[probe]) if probe >= 0 else None
            if mark is None:
                break
            block_requires.append(mark.group(1))
            probe -= 1
        body: list[str] = []
        index += 1
        while index < len(lines) and not lines[index].startswith(found.group("run")):
            body.append(lines[index])
            index += 1
        blocks.append(Block(page, root, fence_line, language, body, region, sample))
        blocks[-1].requires = list(reversed(block_requires))
        index += 1
    return blocks


def region_lines(block: Block, cache: dict[Path, dict[str, list[str]]]) -> list[str]:
    path = block.example_file()
    if path not in cache:
        cache[path] = read_regions(path)
    regions = cache[path]
    region = block.region.split("#", 1)[1] if "#" in block.region else ""
    if region not in regions:
        raise Finding(
            f"{rel(block.page)}: block '{block.label}' names region '{region}' "
            f"which {rel(path)} does not hold"
        )
    return regions[region]


def compare(block: Block, region: list[str]) -> list[str]:
    if block.body == region:
        return []
    name = block.region.split("#", 1)[-1] if block.region else block.label
    lines = [
        f"{rel(block.page)}: block '{name}' at line {block.fence_line} differs "
        "from its region"
    ]
    page = rel(block.page)
    source = rel(block.example_file())
    for offset in range(max(len(block.body), len(region))):
        left = block.body[offset] if offset < len(block.body) else None
        right = region[offset] if offset < len(region) else None
        if left == right:
            continue
        if left is not None:
            lines.append(f"    {page}: line {block.fence_line + offset + 1}: {left}")
        else:
            lines.append(f"    {page}: line {block.fence_line + offset + 1}: absent")
        if right is not None:
            lines.append(f"    {source}: line {offset + 1}: {right}")
        else:
            lines.append(f"    {source}: line {offset + 1}: absent")
    return lines


def prose_text(page: Path) -> str:
    """Page text with fenced blocks and inline code spans removed (C-B)."""
    kept: list[str] = []
    inside: str | None = None
    for line in page.read_text().splitlines():
        found = FENCE.match(line)
        if inside is None and found is not None:
            inside = found.group("run")
            continue
        if inside is not None:
            if line.startswith(inside):
                inside = None
            continue
        kept.append(re.sub(r"`[^`\n]*`", "", line))
    return "\n".join(kept)


MAIN_CLASSES = (
    ("commit hash", r"\b[0-9a-f]{7,40}\b"),
    ("kernel ref", r"\b\d+\.\d+\.\d+-\d+[0-9A-Za-z.-]*\b"),
    ("pinned version string", r"\b\d+\.\d+\.\d+\b"),
    ("compiler version", r"\b(?i:(gcc|g\+\+|clang|clang-cl|msvc))[ -]\d[\d.]*"),
    ("spec identifier", r"\b\d{3}-[a-z][0-9a-z-]*\b"),
    ("requirement identifier", r"\b(FR|SC)-\d+\b"),
    ("task identifier", r"\bT\d{2,4}\b"),
    ("constitution principle number", r"\b[IVX]{1,5}\.\d+\b"),
    ("re-pinning step", r"\b(?i:(re-pin|repin|update_pmu_events))\b"),
    ("source path", r"\b(?:include|source|test|tools|external)/[^\s,;)]*"),
    ("build file", r"\b[\w./-]*CMakeLists\.txt\b"),
)

# The documentation pages under the User guide heading of docs/conf.py.in.
# The Internals pages keep their spec provenance, so the docs classes cover
# this pair.
USER_GUIDE_PAGES = ("docs/pages/building.md", "docs/pages/harness.md")

# The classes a documentation page carries along with its facts. The pinned
# version, compiler version, re-pinning, source path, build file, and CI job
# classes stay off this list: the build page documents the vendor pins, the
# test locations, and the quality gates by name. A user page states counts of
# nanoseconds, so a hex run counts as a commit only when it holds a letter,
# since a bare digit run could be either.
DOCS_CLASSES = (
    (
        "commit hash",
        r"\b(?=[0-9a-f]{7,40}\b)(?=[0-9a-f]*[a-f][0-9a-f]*\b)[0-9a-f]{7,40}\b",
    ),
    ("kernel ref", r"\b\d+\.\d+\.\d+-\d+[0-9A-Za-z.-]*\b"),
    ("spec identifier", r"\b\d{3}-[a-z][0-9a-z-]*\b"),
    ("requirement identifier", r"\b(FR|SC)-\d+\b"),
    ("task identifier", r"\bT\d{2,4}\b"),
    ("constitution principle number", r"\b[IVX]{1,5}\.\d+\b"),
    ("feature number", r"\b(?i:feature)[ -]\d+\b|\b\d{3} C-\d+\b"),
    ("contract label", r"\b[A-Z]-\d+\b"),
)


def forbidden_hits(text: str, classes: tuple[tuple[str, str], ...]) -> list[str]:
    hits = [
        f"{label}: {match.group(0)}"
        for label, pattern in classes
        for match in re.finditer(pattern, text)
        # FR-038 excepts an example program path, and a region marker names
        # the example file it quotes, its CMake file included.
        if not (label == "build file" and match.group(0).startswith("example/"))
    ]
    if classes is MAIN_CLASSES:
        for job in FORBIDDEN_JOBS:
            for match in re.finditer(rf"\b{re.escape(job)}\b", text):
                hits.append(f"CI job name: {match.group(0)}")
        for job in GENERIC_JOBS:
            for match in re.finditer(
                rf"\b(?i:{re.escape(job)}) (?:job|target|step|gate)\b", text
            ):
                hits.append(f"CI job name: {match.group(0)}")
    return hits


def table_rows(page: Path, header: str) -> list[str]:
    """The first cell of every row of the table whose header matches."""
    lines = page.read_text().splitlines()
    rows: list[str] = []
    index = 0
    while index < len(lines):
        if lines[index].strip().startswith(header):
            index += 2
            while index < len(lines) and lines[index].strip().startswith("|"):
                cells = [cell.strip() for cell in lines[index].strip().strip("|").split("|")]
                rows.append(cells[0].strip("`").strip())
                index += 1
            return rows
        index += 1
    raise Finding(f"{rel(page)}: no table with header '{header}'")


def header_names(root: Path) -> set[str]:
    names: set[str] = set()
    for header in (
        "benchmark.hpp",
        "barrier.hpp",
        "execution_context.hpp",
        "counters_metrics.hpp",
    ):
        path = root / "include" / "speedgun-ng" / header
        if path.is_file():
            names.update(re.findall(r"\b[A-Za-z_][A-Za-z0-9_]{2,}\b", path.read_text()))
    return names


def cli_options(root: Path) -> list[str]:
    path = root / "source" / "harness" / "cli.cpp"
    if not path.is_file():
        raise Finding(f"{rel(path)}: CLI source not found")
    text = path.read_text()
    table = re.search(r"option\s+\w+\s*\[\]\s*=\s*\{(.*?)\n\s*\};", text, re.S)
    if table is None:
        raise Finding(f"{rel(path)}: no long-option table found")
    return re.findall(r'\{\s*"([a-z0-9-]+)"', table.group(1))


def headings(page: Path) -> list[tuple[int, str]]:
    """(line number, text) of each ATX heading outside a fenced block."""
    found: list[tuple[int, str]] = []
    inside: str | None = None
    for number, line in enumerate(page.read_text().splitlines(), start=1):
        fence = FENCE.match(line)
        if inside is None and fence is not None:
            inside = fence.group("run")
            continue
        if inside is not None:
            if line.startswith(inside):
                inside = None
            continue
        if line.startswith("#"):
            found.append((number, line.strip()))
    return found


def feature_sections(page: Path) -> dict[str, tuple[int, int]]:
    """The H3 sections of the feature part, mapped to their line span.

    A span runs from its heading line through the line before the next
    heading of any level, the count the section cap measures.
    """
    marks = headings(page)
    parts = [index for index, (_, text) in enumerate(marks) if text == "## How you do it"]
    if not parts:
        raise Finding(f"{rel(page)}: no '## How you do it' part")
    part = parts[-1]
    last = page.read_text().splitlines()
    spans: dict[str, tuple[int, int]] = {}
    for index in range(part + 1, len(marks)):
        number, text = marks[index]
        if not text.startswith("### "):
            if text.startswith("#"):
                break
            continue
        end = marks[index + 1][0] - 1 if index + 1 < len(marks) else len(last)
        spans[text[4:].strip()] = (number, end)
    return spans


def check_page(page: Path, root: Path, is_main: bool) -> list[str]:
    findings: list[str] = []
    cache: dict[Path, dict[str, list[str]]] = {}
    samples = 0
    for block in scan_blocks(page, root):
        if block.region is None:
            if is_main:
                findings.append(
                    f"{rel(page)}: block at line {block.fence_line} carries no "
                    "region marker"
                )
            continue
        try:
            findings.extend(compare(block, region_lines(block, cache)))
        except Finding as error:
            findings.append(str(error))
        for ref in block.requires:
            dep = Block(page, root, block.fence_line, block.language, [], ref, False)
            try:
                region_lines(dep, cache)
            except Finding as error:
                findings.append(str(error).replace("names region", "requires region"))
        if block.sample:
            samples += 1
            if is_main and block.region.split("#")[-1] != "report":
                findings.append(
                    f"{rel(page)}: the sample output block names "
                    f"'{block.region}', not 'first_benchmark.expected#report'"
                )
    if is_main and samples != 1:
        findings.append(
            f"{rel(page)}: {samples} sample output blocks, exactly one required"
        )
    return findings


def check_forbidden(page: Path, classes: tuple[tuple[str, str], ...]) -> list[str]:
    return [
        f"{rel(page)}: forbidden {hit}"
        for hit in forbidden_hits(prose_text(page), classes)
    ]


def check_sections(page: Path, root: Path) -> list[str]:
    findings: list[str] = []
    lines = page.read_text().splitlines()
    if len(lines) > PAGE_LINE_CAP:
        findings.append(f"{rel(page)}: {len(lines)} lines, the cap is {PAGE_LINE_CAP}")
    try:
        spans = feature_sections(page)
    except Finding as error:
        return [str(error)]
    blocks = scan_blocks(page, root)
    for title in SECTIONS:
        if title not in spans:
            findings.append(f"{rel(page)}: no '### {title}' section")
            continue
        start, end = spans[title]
        cap = SECTION_LINE_CAPS.get(title, SECTION_LINE_CAP)
        if end - start + 1 > cap:
            findings.append(
                f"{rel(page)}: section '{title}' runs {end - start + 1} lines, "
                f"the cap is {cap}"
            )
        if not any(
            start <= block.fence_line <= end and block.region is not None
            for block in blocks
        ):
            findings.append(f"{rel(page)}: section '{title}' holds no marked block")
        for block in blocks:
            if block.region is None or not start <= block.fence_line <= end:
                continue
            owner = block.region.split("#", 1)[0].rsplit("/", 1)[-1]
            if owner not in SECTION_OWNERS[title]:
                findings.append(
                    f"{rel(page)}: section '{title}' quotes '{block.region}', whose "
                    f"file belongs to another section ('{owner}')"
                )
    for title in spans:
        if title not in SECTIONS:
            findings.append(
                f"{rel(page)}: section '{title}' is not one of the "
                f"{len(SECTIONS)} capability sections"
            )
    return findings


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("root", nargs="?", default=".", type=Path)
    parser.add_argument("--names", action="store_true", help="check the API table")
    parser.add_argument("--options", action="store_true", help="check the option table")
    parser.add_argument("--forbidden", action="store_true", help="scan forbidden tokens")
    parser.add_argument("--sections", action="store_true", help="check feature sections")
    parser.add_argument("--emit-blocks", type=Path, metavar="DIR",
                        help="write the main-page cpp blocks as snippets")
    try:
        args = parser.parse_args(argv)
    except SystemExit as error:
        return 2 if error.code else 0
    root = args.root.resolve()
    if not root.is_dir():
        print(f"usage error: {args.root} is not a directory", file=sys.stderr)
        return 2
    main_page = root / "README.md"

    try:
        if args.emit_blocks is not None:
            return emit_blocks(main_page, args.emit_blocks)
        findings = run_all(root, main_page, args)
    except Finding as error:
        print(str(error))
        return 1

    for line in dict.fromkeys(findings):
        print(line)
    return 1 if findings else 0


def emit_blocks(main_page: Path, directory: Path) -> int:
    if not main_page.is_file():
        print(f"usage error: {main_page} not found", file=sys.stderr)
        return 2
    directory.mkdir(parents=True, exist_ok=True)
    cpp = 0
    written = 0
    cache: dict[Path, dict[str, list[str]]] = {}
    for block in scan_blocks(main_page, main_page.parent):
        if block.region is None or block.sample:
            continue
        if block.language != "cpp":
            continue
        cpp += 1
        name = block.region.split("#", 1)[-1]
        body = list(block.body)
        for ref in block.requires:
            dep = Block(main_page, main_page.parent, block.fence_line, "cpp", [], ref,
                        False)
            body = region_lines(dep, cache) + [""] + body
        path = directory / f"{cpp:02d}_{name}.snippet"
        path.write_text("\n".join(body) + "\n")
        written += 1
    print(f"emitted {written} cpp snippet of {cpp} cpp fence")
    return 0


def run_all(root: Path, main_page: Path, args: argparse.Namespace) -> list[str]:
    """The scans the default run owns, narrowed by the flags it was given."""
    every = not (args.names or args.options or args.forbidden or args.sections)
    findings: list[str] = []
    for page in prose_pages(root):
        findings.extend(check_page(page, root, page.name == "README.md"))
    if not main_page.is_file():
        return findings + [f"{rel(main_page)}: main page not found"]
    if args.sections or every:
        findings.extend(check_sections(main_page, root))
    if args.forbidden or every:
        findings.extend(check_forbidden(main_page, MAIN_CLASSES))
        for name in USER_GUIDE_PAGES:
            page = root / name
            if page.is_file():
                findings.extend(check_forbidden(page, DOCS_CLASSES))
    if args.names or every:
        present = header_names(root)
        findings.extend(
            f"{rel(main_page)}: quick-reference name '{name}' is not in "
            "benchmark.hpp, barrier.hpp, or execution_context.hpp"
            for name in table_rows(main_page, "| Name |")
            if any(part not in present for part in name.split("::") if part)
        )
    if args.options or every:
        rows = table_rows(main_page, "| Option |")
        declared = [f"--{option}" for option in cli_options(root)]
        findings.extend(
            f"{rel(main_page)}: option row '{row}' is not a long option of "
            "speedgunMain"
            for row in rows
            if row not in declared
        )
        findings.extend(
            f"{rel(main_page)}: long option '{option}' has no option row"
            for option in declared
            if option not in rows
        )
    return findings


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
