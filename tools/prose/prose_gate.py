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
prose and commit checks themselves land in later tasks (T013-T017 and
T019-T022): until then a valid load prints the zero summary line and
exits 0.

Exit codes per specs/002-prose-commit-lint/contracts/cli.md:
0 = no findings, 1 = at least one finding, 2 = usage error or
unreadable, invalid rule data.
"""

from __future__ import annotations

import argparse
import re
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


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    data = load_rules(args.rules)
    if args.version:
        print(
            f"prose-gate {GATE_VERSION} "
            f"(rule data version {data['version']})"
        )
        return EXIT_OK
    # TODO(T013-T017): prose discovery, extraction, exemption precedence,
    # marker handling, rule matching, and findings for --check prose/all.
    # TODO(T019-T022): commit range resolution, commit-template rules,
    # authorship attribution, and the commit verdict for --check commit/all.
    print("prose-lint: 0 sources, 0 units examined, 0 findings, 0 skipped")
    return EXIT_OK


if __name__ == "__main__":
    sys.exit(main())
