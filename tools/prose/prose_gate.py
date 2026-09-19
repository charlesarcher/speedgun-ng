#!/usr/bin/env python3
"""Prose and commit-message gate kernel for speedgun-ng (FR-001).

CLI::

    python3 tools/prose/prose_gate.py [OPTIONS]

      --check {prose,commit,all}  what to check (default: all)
      --mode  {range,tree}        changed-range or whole tree (default: range)
      --base  REF                 left edge of the range, exclusive
      --head  REF                 right edge of the range, inclusive (HEAD)
      --paths PATH ...            narrow the prose check (local debugging)
      --rules PATH                rule data file (default: prose_rules.yaml
                                  beside this script)
      --version                   print gate version and rule-data version

Every run loads and validates the Rule Data File per
specs/002-prose-commit-lint/contracts/rule-data.md, --version included;
any load-time failure exits 2 naming the offending field path. The
prose check covers tracked Markdown and comments in C, C++, CMake, and
shell sources over a changed range or the whole tree; the commit check
covers the Pull Request Quality template over a resolved commit range,
with the prose rules applied to each title and body.

Exit codes per specs/002-prose-commit-lint/contracts/cli.md:
0 = no findings, 1 = at least one finding, 2 = usage error or
unreadable, invalid rule data.
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
from pathlib import Path
from typing import Any

try:
    import yaml
except ImportError as exc:  # pragma: no cover - environment guard
    sys.stderr.write(
        "prose gate requires PyYAML (install python3-yaml): "
        f"{exc}\n"
    )
    raise SystemExit(2) from exc

GATE_VERSION = "0.2.0"

EXIT_OK = 0
EXIT_GAPS = 1
EXIT_USAGE = 2

# The seven canonical prose rule ids the coverage probe checks, id by
# id (contracts/rule-data.md, namespace table).
CANONICAL_IDS = (
    "XI1.EMDASH",
    "XI1.DOUBLE-HYHEN",
    "XI2.CONTRASTIVE",
    "XI3.VOUCHER",
    "XI4.META-EDITORIALIZING",
    "XI5.FILLER",
    "XI5.MARKETING",
)

RULE_ID = re.compile(r"^[A-Z0-9][A-Z0-9._-]*$")
KINDS = {"codepoint", "regex", "vocabulary", "list-membership"}
SCOPES = {
    "markdown",
    "c-comment",
    "shell-comment",
    "cmake-comment",
    "commit-title",
    "commit-body",
}
FORBIDDEN_WILDCARDS = (r"\w*", r"\W*", ".*", ".+")

# Codepoint probe pair (contracts/rule-data.md): the hit sentence holds
# an em dash, the miss sentence holds only en dashes between digits and
# must stay silent. The characters are escapes so this source file
# carries no dash code points of its own.
CODEPOINT_HIT_SENTENCE = "a sentence with the char \u2014 inside"
CODEPOINT_MISS_SENTENCE = "P0-P3 and P0\u2013P3 numeric ranges"

# Characters that end a plain-literal run when extracting regex probe
# literals from a pattern source.
REGEX_META = {"\\", "^", ".", "[", "]", "(", ")", "{", "}", "?", "*", "+", "|"}
REPEAT = re.compile(r"\{(\d+)(?:,(\d*))?\}")

# --- prose pipeline (T013-T017) -------------------------------------------

# Markdown roots (FR-006): a root-level README*, a root-level AGENTS.md, or
# anything under these three directory prefixes. Comment languages are
# in scope wherever they live.
MD_ROOT_PREFIXES = (".specify/memory/", "specs/", "docs/")
C_EXTS = {".c", ".h", ".cc", ".cpp", ".cxx", ".hh", ".hpp", ".hxx"}
SHELL_EXTS = {".sh"}
CMAKE_EXTS = {".cmake"}

# Auto-exempt construct detectors, one per precedence row (contract:
# contracts/rule-data.md "Exemption precedence"). Rows 1, 2, and 8 are
# markdown block idioms and apply only to markdown sources; rows 3 to 7
# apply to any unit text.
FENCE_MARKERS = ("```", "~~~")
INDENTED_CODE_RE = re.compile(r"^(?: {4}|\t)")
INLINE_CODE_RE = re.compile(r"`[^`\n]+`")
URL_RE = re.compile(r"(?:https?|ftp)://|www\.")
PATH_TOKEN_RE = re.compile(r"(?<![\w/.-])((?:[\w.+-]+/)+[\w.+-]*)")
SHELL_COMMAND_RE = re.compile(r"^\s*\$\s")
BLOCKQUOTE_RE = re.compile(r"^\s*>")
# Row 8, markdown only: thematic breaks, table delimiter rows, and bare
# HTML comment delimiters are markup structure (FR-004, row 8 of the
# precedence table in contracts/rule-data.md), never prose.
MD_STRUCTURAL_RE = re.compile(r"^[-: |]*--[-: |]*$|^(?:<!--|--!?>|<!-->)$")

# The built-in meta-finding outside the rule data (contracts/rule-data.md
# namespace table): family XI.6, constitution reference Principle XI.6.
MARKER_NO_REASON_ID = "MARKER.NO-REASON"
MARKER_NO_REASON_FAMILY = "XI.6"
MARKER_NO_REASON_CONSTITUTION = "Principle XI.6"
MARKER_NO_REASON_MESSAGE = "the exemption marker needs a non-empty reason=\"...\""

HUNK_RE = re.compile(r"^@@ -\d+(?:,\d+)? \+(\d+)(?:,\d+)? @@")
DIFF_FILE_RE = re.compile(r"^diff --git a/(\S+) b/(\S+)")


def die(message: str, code: int = EXIT_USAGE) -> None:
    sys.stdout.flush()
    sys.stderr.write(f"{message}\n")
    raise SystemExit(code)


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        prog="prose_gate.py",
        description=(
            "Check repository prose and commit messages against "
            "constitution Principle XI and the Pull Request Quality "
            "template."
        ),
    )
    parser.add_argument(
        "--check",
        choices=("prose", "commit", "all"),
        default="all",
        help="what to check (default: all)",
    )
    parser.add_argument(
        "--mode",
        choices=("range", "tree"),
        default="range",
        help="changed range or whole tree (default: range)",
    )
    parser.add_argument(
        "--base",
        default=None,
        help="left edge of the range, exclusive",
    )
    parser.add_argument(
        "--head",
        default="HEAD",
        help="right edge of the range, inclusive (default: HEAD)",
    )
    parser.add_argument(
        "--paths",
        nargs="+",
        default=None,
        help="repo-relative paths narrowing the prose check",
    )
    parser.add_argument(
        "--rules",
        type=Path,
        default=Path(__file__).resolve().parent / "prose_rules.yaml",
        help="rule data file (default: prose_rules.yaml beside this script)",
    )
    parser.add_argument(
        "--version",
        action="store_true",
        help="print the gate version and the rule-data version",
    )
    args = parser.parse_args(argv)
    if args.mode == "tree" and args.check == "commit":
        die(
            "--mode tree combined with --check commit is a usage error: "
            "commit checking needs a range"
        )
    if args.paths is not None and args.check == "commit":
        die(
            "--paths combined with --check commit is a usage error: "
            "--paths narrows the prose check only"
        )
    return args


def load_rules(path: Path) -> dict[str, Any]:
    """Load and validate the Rule Data File, exiting 2 on any defect.

    Every failure message names the offending field path, per the
    load-time validation contract in contracts/rule-data.md.
    """
    try:
        text = path.read_text(encoding="utf-8")
    except OSError as exc:
        die(f"rules: cannot read {path}: {exc}")
    try:
        data = yaml.safe_load(text)
    except yaml.YAMLError as exc:
        die(f"rules: parse failure: {exc}")
    if not isinstance(data, dict):
        die(f"rules: parse failure: {path} does not hold a YAML mapping")

    _validate_version(data)
    raw_rules = _validate_rules_list(data)
    rules = [_validate_rule(index, entry) for index, entry in enumerate(raw_rules)]
    _check_ids(rules)
    _validate_file_level(data)
    _validate_thresholds(data)
    for rule in rules:
        _probe_rule(rule)
    return data


def _validate_version(data: dict[str, Any]) -> None:
    version = data.get("version")
    if isinstance(version, bool) or version != 1:
        die(f"rules.version: must be present and equal to 1, got {version!r}")


def _validate_rules_list(data: dict[str, Any]) -> list[Any]:
    rules = data.get("rules")
    if not isinstance(rules, list) or not rules:
        die("rules: missing, not a list, or empty")
    return rules


def _validate_rule(index: int, entry: Any) -> dict[str, Any]:
    """Validate one rule's fields and compile its pattern."""
    if not isinstance(entry, dict):
        die(f"rules[{index}]: not a mapping")

    rule_id = entry.get("id")
    if not isinstance(rule_id, str) or RULE_ID.fullmatch(rule_id) is None:
        die(
            f"rules[{index}].id: missing or not matching "
            f"^[A-Z0-9][A-Z0-9._-]*$: {rule_id!r}"
        )

    for key in ("family", "constitution"):
        value = entry.get(key)
        if not isinstance(value, str) or not value.strip():
            die(f"rules[{index}].{key}: missing or empty")

    kind = entry.get("kind")
    if kind not in KINDS:
        die(f"rules[{index}].kind: {kind!r} not in {sorted(KINDS)}")

    pattern = entry.get("pattern")
    compiled: re.Pattern[str] | None = None
    if kind in ("codepoint", "regex"):
        if not isinstance(pattern, str) or not pattern:
            die(f"rules[{index}].pattern: required for kind {kind}")
        for wildcard in FORBIDDEN_WILDCARDS:
            if wildcard in pattern:
                die(f"rules[{index}].pattern wildcard forbidden: {wildcard}")
        if kind == "regex":
            try:
                compiled = re.compile(pattern)
            except re.error as exc:
                die(f"rules[{index}].pattern: does not compile: {exc}")

    tokens = entry.get("tokens")
    if kind == "vocabulary":
        if not isinstance(tokens, list) or not tokens:
            die(f"rules[{index}].tokens: missing or empty for kind vocabulary")
        for token in tokens:
            if not isinstance(token, str) or not re.search(r"\w", token):
                die(
                    f"rules[{index}].tokens: token {token!r} is empty, "
                    "whitespace-only, or punctuation-only"
                )

    scope = entry.get("scope")
    if not isinstance(scope, list) or not scope:
        die(f"rules[{index}].scope: missing or empty")
    bad = [value for value in scope if not isinstance(value, str) or value not in SCOPES]
    if bad:
        die(f"rules[{index}].scope: invalid values {bad}")

    message = entry.get("message")
    if not isinstance(message, str) or not message.strip():
        die(f"rules[{index}].message: missing or empty")

    return {
        "index": index,
        "id": rule_id,
        "kind": kind,
        "pattern": pattern,
        "compiled": compiled,
        "tokens": tokens,
    }


def _check_ids(rules: list[dict[str, Any]]) -> None:
    """Enforce id uniqueness and canonical id coverage, id by id."""
    seen: dict[str, int] = {}
    for rule in rules:
        rule_id = rule["id"]
        if rule_id in seen:
            die(
                f"rules: duplicate id {rule_id} in "
                f"rules[{seen[rule_id]}] and rules[{rule['index']}]"
            )
        seen[rule_id] = rule["index"]
    for canonical in CANONICAL_IDS:
        if canonical not in seen:
            die(f"rules: missing canonical id {canonical}")


def _validate_file_level(data: dict[str, Any]) -> None:
    for key in ("sections", "vague_titles", "non_imperative_shapes"):
        value = data.get(key)
        if not isinstance(value, list) or not value:
            die(f"{key}: missing or empty")
    marker = data.get("marker")
    if not isinstance(marker, str) or not marker.strip():
        die("marker: missing or empty")


def _validate_thresholds(data: dict[str, Any]) -> None:
    thresholds = data.get("thresholds")
    if not isinstance(thresholds, dict):
        die("thresholds: missing")
    bounds = {
        "title_max": (1, 100),
        "body_wrap": (40, 100),
        "trivial_max_changed_lines": (0, 50),
    }
    for key, (low, high) in bounds.items():
        value = thresholds.get(key)
        if isinstance(value, bool) or not isinstance(value, int):
            die(f"thresholds.{key}: must be an integer, got {value!r}")
        if not low <= value <= high:
            die(f"thresholds.{key}: must be in {low}..{high}, got {value!r}")


def _probe_rule(rule: dict[str, Any]) -> None:
    """Run the built-in probe pair for one rule kind (load-time)."""
    kind = rule["kind"]
    if kind == "vocabulary":
        _probe_vocabulary(rule["index"], rule["tokens"])
    elif kind == "codepoint":
        _probe_codepoint(rule["index"], rule["pattern"])
    elif kind == "regex":
        _probe_regex(rule["index"], rule["compiled"], rule["pattern"])


def _probe_vocabulary(index: int, tokens: list[str]) -> None:
    first = tokens[0]
    pattern = re.compile(
        r"\b(?:" + "|".join(re.escape(token) for token in tokens) + r")\b",
        re.IGNORECASE,
    )
    if not pattern.search(f"probe {first} alone"):
        die(
            f"rules[{index}].tokens: vocabulary probe failed, "
            f"first token {first!r} does not match alone"
        )
    if pattern.search(first + "zz"):
        die(
            f"rules[{index}].tokens: vocabulary probe failed, "
            f"first token {first!r} matches {first}zz"
        )


def _probe_codepoint(index: int, char: str) -> None:
    if char not in CODEPOINT_HIT_SENTENCE:
        die(
            f"rules[{index}].pattern: codepoint probe failed, "
            f"{char!r} absent from the hit sentence"
        )
    if char in CODEPOINT_MISS_SENTENCE:
        die(
            f"rules[{index}].pattern: codepoint probe failed, "
            f"{char!r} matches the silent numeric en-dash sentence"
        )


def _probe_regex(
    index: int, compiled: re.Pattern[str] | None, pattern_text: str
) -> None:
    assert compiled is not None
    hit_candidate = None
    for candidate in _literal_candidates(pattern_text):
        if compiled.search(f"probe {candidate} alone"):
            hit_candidate = candidate
            break
    if hit_candidate is None:
        die(
            f"rules[{index}].pattern: regex probe failed, no literal "
            "segment of the pattern matches anything"
        )
    neighbor = _neighbor_case(hit_candidate)
    if compiled.search(neighbor):
        die(
            f"rules[{index}].pattern: regex probe failed, matches the "
            f"silent word-boundary neighbor case {neighbor!r}"
        )


def _literal_candidates(pattern_text: str) -> list[str]:
    """Plain-literal runs of a regex source, quantified runs expanded.

    Metacharacters, classes, and escapes like ``\\b`` end the current
    run; ``x{m,n}`` and ``x+`` repeat the literal, ``x*`` and ``x?``
    contribute it once. Candidates are verified against the compiled
    pattern by the probe, so a false candidate is merely skipped.
    """
    candidates: list[str] = []
    run: list[str] = []

    def flush() -> None:
        if run:
            candidates.append("".join(run))
            run.clear()

    i = 0
    length = len(pattern_text)
    while i < length:
        ch = pattern_text[i]
        if ch == "\\":
            if i + 1 < length and not pattern_text[i + 1].isalnum():
                run.append(pattern_text[i + 1])
            else:
                flush()
            i += 2
            continue
        if ch in REGEX_META:
            repeat = REPEAT.match(pattern_text, i) if ch == "{" else None
            if repeat is not None and run:
                for _ in range(max(int(repeat.group(1)) - 1, 0)):
                    run.append(run[-1])
                i = repeat.end()
                continue
            flush()
            i += 1
            continue
        run.append(ch)
        if i + 1 < length and pattern_text[i + 1] == "+":
            run.append(ch)
            i += 2
            continue
        if i + 1 < length and pattern_text[i + 1] in "*?":
            i += 2
            continue
        i += 1
    flush()
    return candidates


def _neighbor_case(candidate: str) -> str:
    """A word-boundary neighbor case that must stay silent.

    A literal with word characters is embedded between plain words
    ("this not that"); a punctuation-only literal such as ``--`` gets
    the single-hyphen word case ("whole-file").
    """
    letters = " ".join(re.findall(r"\w+", candidate))
    if letters:
        return f"this {letters} that"
    return "whole-file"


# --- git access (T013, T016) ------------------------------------------------


def run_git(repo: Path, git_args: list[str], purpose: str) -> str:
    """Run git in repo, returning stdout; any failure is an exit-2 error."""
    try:
        proc = subprocess.run(
            ["git", *git_args],
            cwd=repo,
            capture_output=True,
            text=True,
            check=False,
        )
    except FileNotFoundError:
        die("git: command not found; the prose gate needs git")
    if proc.returncode != 0:
        die(f"{purpose}: git {' '.join(git_args)} failed: {proc.stderr.strip()}")
    return proc.stdout


def resolve_repo_root() -> Path:
    try:
        proc = subprocess.run(
            ["git", "rev-parse", "--show-toplevel"],
            capture_output=True,
            text=True,
            check=False,
        )
    except FileNotFoundError:
        die("git: command not found; the prose gate needs git")
    if proc.returncode != 0:
        die(f"prose gate: not a git repository: {Path.cwd()}")
    return Path(proc.stdout.strip())


_DEGENERATE_WARNED = False


def resolve_base(repo: Path, base_arg: str | None, head: str) -> str:
    if base_arg is not None:
        base = base_arg
    else:
        try:
            proc = subprocess.run(
                ["git", "merge-base", "origin/master", head],
                cwd=repo,
                capture_output=True,
                text=True,
                check=False,
            )
        except FileNotFoundError:
            die("git: command not found; the prose gate needs git")
        if proc.returncode != 0 or not proc.stdout.strip():
            die(
                f"range: cannot resolve a default base: no merge-base between "
                f"origin/master and {head}; pass --base explicitly"
            )
        base = proc.stdout.strip()
    _warn_degenerate(repo, base, head)
    return base


def _warn_degenerate(repo: Path, base: str, head: str) -> None:
    """Warn once per run on a degenerate range (spec.md:111, 2026-09-16).

    CI exits 2 on base equals head; locally the same condition warns on
    stderr and keeps its exit status, because a vacuous pass is never
    taken silently.
    """
    global _DEGENERATE_WARNED
    if _DEGENERATE_WARNED:
        return
    base_sha = run_git(repo, ["rev-parse", base], "range base")
    head_sha = run_git(repo, ["rev-parse", head], "range head")
    if base_sha.strip() == head_sha.strip():
        _DEGENERATE_WARNED = True
        sys.stderr.write(
            "prose-lint: warning: resolved range is degenerate "
            "(base equals head): nothing was checked\n"
        )


# --- discovery and language classification (T013) ---------------------------


def classify_source(path: str) -> str | None:
    """Return the unit scope of a tracked path, or None if out of scope.

    FR-006 verbatim: Markdown is read in the repository root and under
    the three directory prefixes; comment languages are in scope
    wherever they live.
    """
    name = path.rsplit("/", 1)[-1]
    suffix = Path(name).suffix
    if suffix in (".md", ".markdown"):
        if "/" not in path or path.startswith(MD_ROOT_PREFIXES):
            return "markdown"
        return None
    if suffix in C_EXTS:
        return "c-comment"
    if suffix in SHELL_EXTS:
        return "shell-comment"
    if suffix in CMAKE_EXTS or name == "CMakeLists.txt":
        return "cmake-comment"
    return None


def in_narrowed(path: str, paths: list[str] | None) -> bool:
    if paths is None:
        return True
    return any(path == p or path.startswith(p.rstrip("/") + "/") for p in paths)


def collect_candidates(
    repo: Path, mode: str, base: str | None, head: str
) -> tuple[list[str], dict[str, dict[int, str]] | None]:
    """Tracked in-scope-range paths plus the range authorship map."""
    if mode == "tree":
        listing = run_git(repo, ["ls-files"], "discovery")
        return sorted(p for p in listing.split("\n") if p), None
    resolved = resolve_base(repo, base, head)
    diff = run_git(
        repo,
        ["diff", "-U1", "--no-color", "--no-renames", f"{resolved}...{head}"],
        "range",
    )
    authorship = parse_authorship(diff)
    return sorted(authorship), authorship


# --- comment extraction (T013) ----------------------------------------------


def _skipped_string(text: str, index: int) -> int:
    """Index just past the quoted span opening at index, escapes honored."""
    quote = text[index]
    i = index + 1
    while i < len(text):
        if text[i] == "\\":
            i += 2
            continue
        if text[i] == quote:
            return i + 1
        i += 1
    return i


def extract_c_comment_units(lines: list[str]) -> list[tuple[int, str]]:
    """Quote-aware C/C++ comment text as (line, text) in original coordinates.

    Block-comment continuation markers (a leading run of asterisks) are
    stripped; units whose text is empty after stripping are dropped.
    """
    units: list[tuple[int, str]] = []
    in_block = False
    for lineno, line in enumerate(lines, 1):
        collected: list[str] = []
        i = 0
        while i < len(line):
            if in_block:
                end = line.find("*/", i)
                if end == -1:
                    collected.append(line[i:])
                    i = len(line)
                else:
                    collected.append(line[i:end])
                    in_block = False
                    i = end + 2
                continue
            if line.startswith("//", i):
                collected.append(line[i + 2 :])
                break
            if line.startswith("/*", i):
                in_block = True
                i += 2
                continue
            if line[i] in ("\"", "'"):
                i = _skipped_string(line, i)
                continue
            i += 1
        text = re.sub(r"^\s*\*+\s*", "", "".join(collected)).strip()
        if text:
            units.append((lineno, text))
    return units


def extract_hash_comment_units(
    lines: list[str], skip_shebang: bool
) -> list[tuple[int, str]]:
    """Quote-aware '#' comment text for shell and CMake sources.

    A '#' opens a comment only at line start or after whitespace, so
    word-internal hashes stay code.
    """
    units: list[tuple[int, str]] = []
    for lineno, line in enumerate(lines, 1):
        if skip_shebang and lineno == 1 and line.startswith("#!"):
            continue
        i = 0
        start = -1
        while i < len(line):
            if line[i] == "\\":
                i += 2
                continue
            if line[i] in ("\"", "'"):
                i = _skipped_string(line, i)
                continue
            if line[i] == "#" and (i == 0 or line[i - 1] in " \t"):
                start = i
                break
            i += 1
        if start == -1:
            continue
        text = line[start + 1 :].strip()
        if text:
            units.append((lineno, text))
    return units


def build_units(
    lang: str, lines: list[str]
) -> list[tuple[int, str]] | None:
    """Candidate units for a source; None marks a markdown source."""
    if lang == "markdown":
        return None
    if lang == "c-comment":
        return extract_c_comment_units(lines)
    return extract_hash_comment_units(lines, skip_shebang=lang == "shell-comment")


# --- authorship from git diff (T016) ----------------------------------------


def parse_authorship(diff_text: str) -> dict[str, dict[int, str]]:
    """New-file line statuses from `git diff -U1 --no-color --no-renames`.

    ``+`` lines become ``new``; context lines inside a hunk that also
    carries ``+`` lines become ``modified`` (the R-08 amendment);
    context of pure-deletion hunks stays unlisted, hence grandfathered.
    Files whose new side is ``/dev/null`` get no entry at all, so
    deleted-side lines are never reported.
    """
    statuses: dict[str, dict[int, str]] = {}
    file_map: dict[int, str] | None = None
    lines = diff_text.split("\n")
    i = 0
    while i < len(lines):
        line = lines[i]
        file_match = DIFF_FILE_RE.match(line)
        if file_match is not None:
            file_map = statuses.setdefault(file_match.group(2), {})
            i += 1
            continue
        if line.startswith("+++ "):
            target = line[4:]
            if target == "/dev/null":
                file_map = None
            elif target.startswith("b/"):
                file_map = statuses.setdefault(target[2:], {})
            i += 1
            continue
        hunk = HUNK_RE.match(line)
        if hunk is None or file_map is None:
            i += 1
            continue
        i = _consume_hunk(lines, i + 1, int(hunk.group(1)), file_map)
    return statuses


def _consume_hunk(
    lines: list[str], index: int, new_no: int, file_map: dict[int, str]
) -> int:
    body: list[tuple[str, int]] = []
    i = index
    while i < len(lines):
        body_line = lines[i]
        if body_line.startswith(("@@", "diff --git")):
            break
        marker = body_line[:1]
        if marker == "+":
            body.append(("new", new_no))
            new_no += 1
        elif marker == "-":
            pass
        elif marker in (" ", "\\"):
            if body_line.startswith("\\"):
                pass
            else:
                body.append(("context", new_no))
                new_no += 1
        else:
            break
        i += 1
    has_new = any(kind == "new" for kind, _ in body)
    for kind, lineno in body:
        if kind == "new":
            file_map[lineno] = "new"
        elif has_new:
            file_map[lineno] = "modified"
    return i


# --- exemption precedence, marker, matching (T014, T015) --------------------


def path_like(text: str) -> bool:
    return any(
        re.search(r"[A-Za-z]", token) for token in PATH_TOKEN_RE.findall(text)
    )


def inspect_marker(text: str, marker_literal: str) -> str:
    """Classify the line's marker as none, valid, or invalid.

    Grammar per contracts/rule-data.md: valid is the marker literal
    followed by reason="..." closing at the next straight double quote
    with non-empty stripped text; the literal appearing without that
    shape (absent, unbalanced, empty) is invalid.
    """
    verdict = "none"
    start = 0
    while True:
        index = text.find(marker_literal, start)
        if index == -1:
            return verdict
        rest = text[index + len(marker_literal) :]
        reason = re.search(r'\s+reason="([^"]*)"', rest)
        if reason is not None and reason.group(1).strip():
            return "valid"
        verdict = "invalid"
        start = index + len(marker_literal)


def compile_prose_matchers(rules: list[dict[str, Any]]) -> list[dict[str, Any]]:
    """Attach one scan function per rule, vocabulary compiled longest-first."""
    matchers: list[dict[str, Any]] = []
    for entry in rules:
        kind = entry["kind"]
        if kind == "vocabulary":
            ordered = sorted(entry["tokens"], key=len, reverse=True)
            scan = re.compile(
                r"\b(?:" + "|".join(re.escape(t) for t in ordered) + r")\b",
                re.IGNORECASE,
            ).finditer
        elif kind == "codepoint":
            scan = re.compile(re.escape(entry["pattern"])).finditer
        elif kind == "regex":
            scan = re.compile(entry["pattern"]).finditer
        else:
            continue
        matchers.append(
            {
                "id": entry["id"],
                "family": entry["family"],
                "constitution": entry["constitution"],
                "message": entry["message"],
                "scope": entry["scope"],
                "scan": scan,
            }
        )
    return matchers


# --- evaluation and verdict (T016, T017) -------------------------------------

Finding = tuple[str, int, str, str, str, str, str]


def read_source(repo: Path, path: str) -> tuple[list[str] | None, str | None]:
    """File lines with CRLF tails stripped, or None plus a skip reason."""
    source_path = repo / path
    if source_path.is_symlink():
        return None, "symlink"
    try:
        raw = source_path.read_bytes()
    except OSError as exc:
        return None, f"unreadable: {exc}"
    if raw.startswith(b"\xef\xbb\xbf"):
        return None, "byte-order mark present"
    try:
        text = raw.decode("utf-8")
    except UnicodeDecodeError as exc:
        return None, f"invalid UTF-8 at byte {exc.start}"
    lines = text.split("\n")
    if lines and lines[-1] == "":
        lines.pop()
    return [ln[:-1] if ln.endswith("\r") else ln for ln in lines], None


def evaluate_unit(
    unit_text: str,
    lang: str,
    inside_fence: bool,
    fence_line: bool,
    marker_literal: str,
    matchers: list[dict[str, Any]],
) -> list[tuple[str, str, str, str]]:
    """Apply precedence rows 1 to 11 to one examined unit."""
    if lang in ("markdown", "commit-body") and (inside_fence or fence_line):
        return []
    if lang == "markdown" and INDENTED_CODE_RE.match(unit_text):
        return []
    if (
        INLINE_CODE_RE.search(unit_text)
        or URL_RE.search(unit_text)
        or path_like(unit_text)
        or SHELL_COMMAND_RE.match(unit_text)
        or BLOCKQUOTE_RE.match(unit_text)
    ):
        return []
    if lang == "markdown" and MD_STRUCTURAL_RE.match(unit_text):
        return []
    marker_state = inspect_marker(unit_text, marker_literal)
    if marker_state == "valid":
        return []
    if marker_state == "invalid":
        return [
            (
                MARKER_NO_REASON_ID,
                MARKER_NO_REASON_FAMILY,
                marker_literal,
                f"{MARKER_NO_REASON_MESSAGE} ({MARKER_NO_REASON_CONSTITUTION})",
            )
        ]
    hits: list[tuple[str, str, str, str]] = []
    for matcher in matchers:
        if lang not in matcher["scope"]:
            continue
        for match in matcher["scan"](unit_text):
            hits.append(
                (
                    matcher["id"],
                    matcher["family"],
                    match.group(0),
                    f"{matcher['message']} ({matcher['constitution']})",
                )
            )
    return hits


def evaluate_prose(
    args: argparse.Namespace, data: dict[str, Any]
) -> tuple[int, int, int, int]:
    repo = resolve_repo_root()
    candidates, authorship = collect_candidates(
        repo, args.mode, args.base, args.head
    )
    matchers = compile_prose_matchers(data["rules"])
    exclusions = tuple(data["exclusions"])
    marker_literal = data["marker"]
    annotations = os.environ.get("GITHUB_ACTIONS", "") == "true"
    findings: list[Finding] = []
    skip_reasons: list[str] = []
    sources = 0
    skipped = 0
    units_examined = 0
    for path in candidates:
        lang = classify_source(path)
        if lang is None or path.startswith(exclusions):
            continue
        if not in_narrowed(path, args.paths):
            continue
        lines, reason = read_source(repo, path)
        if lines is None:
            skipped += 1
            skip_reasons.append(f"{path}: skipped ({reason})")
            continue
        sources += 1
        built = build_units(lang, lines)
        stream = built if built is not None else list(enumerate(lines, 1))
        fence = False
        for lineno, unit_text in stream:
            fence_line = False
            inside_fence = False
            if lang == "markdown":
                stripped = unit_text.strip()
                fence_line = stripped.startswith(FENCE_MARKERS)
                inside_fence = fence
                if fence_line:
                    fence = not fence
            if not unit_text.strip():
                continue
            if authorship is None:
                status = "new"
            else:
                status = authorship.get(path, {}).get(lineno, "grandfathered")
            if status == "grandfathered":
                continue
            units_examined += 1
            for rule_id, family, token, shown in evaluate_unit(
                unit_text, lang, inside_fence, fence_line, marker_literal, matchers
            ):
                findings.append((path, lineno, rule_id, family, token, shown))
    findings.sort(key=lambda finding: (finding[0], finding[1], finding[2]))
    for path, lineno, rule_id, family, token, shown in findings:
        sys.stderr.write(
            f"{path}:{lineno}: {rule_id} family={family} '{token}': {shown}\n"
        )
        if annotations:
            sys.stderr.write(
                f"::error file={path},line={lineno}::{rule_id} {shown}\n"
            )
    for note in skip_reasons:
        sys.stderr.write(note + "\n")
    return sources, units_examined, len(findings), skipped


# --- commit check (T019-T022) ------------------------------------------------

TRAILER_RE = re.compile(r"^[A-Za-z][A-Za-z0-9-]*: \S")
TITLE_FORMAT_RE = re.compile(r"^([^: ]+): (\S.*)$")
APPROVAL_KEY = "approved-by"
CM_CONSTITUTION = "Pull Request Quality"
CM_MESSAGES = {
    "CM.TITLE-FORMAT": (
        "title reads `<Section>: <Imperative description>`, one space "
        "after the colon, no trailing period"
    ),
    "CM.TITLE-LENGTH": "title fits the configured maximum length",
    "CM.SECTION-UNKNOWN": "section token is one of the configured sections",
    "CM.NON-IMPERATIVE": "title begins with an imperative verb",
    "CM.VAGUE-TITLE": "title names the change, not a vague placeholder",
    "CM.BODY-REQUIRED": (
        "body explains the why; merge commits and changes within the "
        "trivial changed-line limit are exempt"
    ),
    "CM.BODY-WRAP": "body lines fit the configured wrap column",
    "CM.FOOTER-APPROVAL": "missing Approved-by footer",
}


def strip_marker_reason(text: str, marker_literal: str) -> str:
    """Remove the marker substring so width measures the visible text."""
    return re.sub(re.escape(marker_literal) + r'\s*reason="[^"]*"', "", text)


def parse_commit_message(
    message: str,
) -> tuple[str, list[str], list[tuple[str, str]]]:
    """Title, body lines without the footer paragraph, and parsed footers.

    The body is everything after the first blank line; the trailing
    paragraph counts as footers when every one of its lines reads
    `Key: value`.
    """
    lines = message.rstrip("\n").split("\n")
    title = lines[0] if lines else ""
    rest = lines[1:]
    start = 0
    while start < len(rest) and rest[start].strip() == "":
        start += 1
    paragraphs: list[list[str]] = []
    current: list[str] = []
    for line in rest[start:]:
        if line.strip() == "":
            if current:
                paragraphs.append(current)
                current = []
        else:
            current.append(line)
    if current:
        paragraphs.append(current)
    footers: list[tuple[str, str]] = []
    if paragraphs and all(TRAILER_RE.match(line) for line in paragraphs[-1]):
        footers = [
            (line[: line.index(":")], line[line.index(":") + 1 :].strip())
            for line in paragraphs[-1]
        ]
        paragraphs = paragraphs[:-1]
    body_lines = [line for paragraph in paragraphs for line in paragraph]
    return title, body_lines, footers


def count_changed_lines(repo: Path, sha: str) -> int:
    """Added plus removed with rename detection; binary rows count 0.

    A merge commit prints no numstat rows, so its changed lines are 0.
    """
    numstat = run_git(
        repo, ["show", "--numstat", "-M", "--format=", sha], "changed lines"
    )
    total = 0
    for row in numstat.split("\n"):
        columns = row.split("\t")
        if len(columns) >= 2 and columns[0] != "-" and columns[0].isdigit():
            total += int(columns[0]) + int(columns[1])
    return total


def build_commit_records(
    repo: Path, base: str, head: str
) -> list[dict[str, Any]]:
    """CommitRecords for the two-dot range `git rev-list BASE..HEAD`.

    An empty range is a success with zero records; a default base that
    cannot be resolved never becomes an empty pass because resolve_base
    exits 2 instead.
    """
    listing = run_git(repo, ["rev-list", f"{base}..{head}"], "commit range")
    records: list[dict[str, Any]] = []
    for sha in [entry for entry in listing.split("\n") if entry]:
        parents = run_git(
            repo, ["show", "-s", "--format=%P", sha], "commit parents"
        ).split()
        message = run_git(
            repo, ["show", "-s", "--format=%B", sha], "commit message"
        )
        title, body_lines, footers = parse_commit_message(message)
        records.append(
            {
                "sha": sha,
                "short": sha[:7],
                "title": title,
                "body_lines": body_lines,
                "footers": footers,
                "changed_lines": count_changed_lines(repo, sha),
                "is_merge": len(parents) >= 2,
            }
        )
    return records


def evaluate_commit_rules(
    record: dict[str, Any], data: dict[str, Any]
) -> list[str]:
    """The eight CM rules for one record, as lines ordered by rule id."""
    thresholds = data["thresholds"]
    title = record["title"]
    title_match = TITLE_FORMAT_RE.match(title)
    hits: set[str] = set()
    if title_match is None or title.endswith("."):
        hits.add("CM.TITLE-FORMAT")
    if len(title) > thresholds["title_max"]:
        hits.add("CM.TITLE-LENGTH")
    # CM.VAGUE-TITLE per the data-model row: the whole title equals, case
    # insensitively, a vague_titles entry, so a bare ``wip`` names the
    # vague rule beside the format rule (FR-012, US2 scenario 8).
    if title.strip().lower() in {
        vague.lower() for vague in data["vague_titles"]
    }:
        hits.add("CM.VAGUE-TITLE")
    if title_match is not None:
        lowered = title_match.group(2).strip().lower()
        if title_match.group(1) not in data["sections"]:
            hits.add("CM.SECTION-UNKNOWN")
        if any(
            re.match(rf"(?:{re.escape(shape.lower())})\b", lowered)
            for shape in data["non_imperative_shapes"]
        ):
            hits.add("CM.NON-IMPERATIVE")
    if (
        not record["body_lines"]
        and not record["is_merge"]
        and record["changed_lines"] > thresholds["trivial_max_changed_lines"]
    ):
        hits.add("CM.BODY-REQUIRED")
    for line in record["body_lines"]:
        measured = strip_marker_reason(line, data["marker"])
        if len(measured) > thresholds["body_wrap"]:
            hits.add("CM.BODY-WRAP")
            break
    if not any(key.lower() == APPROVAL_KEY for key, _ in record["footers"]):
        hits.add("CM.FOOTER-APPROVAL")
    return [
        f"commit {record['short']}: {rule_id}: {CM_MESSAGES[rule_id]} "
        f"({CM_CONSTITUTION})"
        for rule_id in sorted(hits)
    ]


def evaluate_prose_over_commits(
    record: dict[str, Any],
    matchers: list[dict[str, Any]],
    marker_literal: str,
) -> list[str]:
    """Prose rules over commit text: title and body as separate scopes."""
    results: list[tuple[str, str]] = []
    for scope, unit_lines in (
        ("commit-title", [record["title"]]),
        ("commit-body", record["body_lines"]),
    ):
        fence = False
        for unit_text in unit_lines:
            stripped = unit_text.strip()
            fence_line = stripped.startswith(FENCE_MARKERS)
            inside_fence = fence
            if fence_line:
                fence = not fence
            if not stripped:
                continue
            for rule_id, family, token, shown in evaluate_unit(
                unit_text,
                scope,
                inside_fence,
                fence_line,
                marker_literal,
                matchers,
            ):
                results.append(
                    (
                        rule_id,
                        f"commit {record['short']}: {rule_id} "
                        f"family={family} '{token}': {shown}",
                    )
                )
    results.sort(key=lambda item: item[0])
    return [line for _, line in results]


def evaluate_commits(
    args: argparse.Namespace, data: dict[str, Any]
) -> tuple[int, int, int]:
    """Check the commit range; print findings, return record/unit counts."""
    repo = resolve_repo_root()
    base = resolve_base(repo, args.base, args.head)
    records = build_commit_records(repo, base, args.head)
    matchers = compile_prose_matchers(data["rules"])
    marker_literal = data["marker"]
    units = 0
    findings = 0
    for record in records:
        units += 1 + len(record["body_lines"])
        lines = evaluate_commit_rules(record, data)
        lines += evaluate_prose_over_commits(record, matchers, marker_literal)
        for line in lines:
            sys.stderr.write(line + "\n")
        findings += len(lines)
    return len(records), units, findings


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    data = load_rules(args.rules)
    if args.version:
        print(
            f"prose-gate {GATE_VERSION} "
            f"(rule data version {data['version']})"
        )
        return EXIT_OK
    sources = 0
    units_examined = 0
    findings = 0
    skipped = 0
    if args.check in ("prose", "all"):
        sources, units_examined, findings, skipped = evaluate_prose(
            args, data
        )
    if args.check in ("commit", "all"):
        commit_sources, commit_units, commit_findings = evaluate_commits(
            args, data
        )
        sources += commit_sources
        units_examined += commit_units
        findings += commit_findings
    print(
        f"prose-lint: {sources} sources, {units_examined} units examined, "
        f"{findings} findings, {skipped} skipped"
    )
    return EXIT_GAPS if findings else EXIT_OK


if __name__ == "__main__":
    sys.exit(main())
