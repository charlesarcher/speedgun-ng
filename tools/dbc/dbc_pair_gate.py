#!/usr/bin/env python3
"""Doc-to-enforcement pairing half of the Phase 0 DBC coverage gate (FR-028).

CLI::

    python3 tools/dbc/dbc_pair_gate.py \\
        --src <header-dir> --src2 <source-dir> --registry tools/dbc/macros.yaml \\
        --doc-matrix <path> --out <matrix>

Takes the documentation-presence matrix (``--doc-matrix``) and fills
``enforced_kinds`` by scanning ``--src`` and ``--src2`` for exact
registered macro names as tokens (FR-025/FR-026). No source-text
heuristics beyond those registry identifiers: comments, strings, and
``#define`` lines are blanked so a mention of ``SG_REQUIRE`` is not an
enforcement.

Both drift directions fail the gate (FR-028):

- documented-not-enforced
- enforced-not-documented

The explicit ``none`` marker (FR-030) is an empty documented set for
that kind; a matching enforcement is then enforced-not-documented.

Class-invariant macros (``SG_INVARIANT`` / ``SG_INVARIANT_ALWAYS``)
found in a member definition are attributed to the enclosing class,
matching FR-006 (invariant is a type contract checked at member
entry/exit). Function rows pair precondition / postcondition /
assertion only.

Exit 0 = pass, 1 = gaps, 2 = usage. The complete matrix is written to
``--out`` on every successful parse (including gap runs).
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

from dbc_gate_common import (
    EXIT_GAPS,
    EXIT_OK,
    STATUS_DRIFT,
    STATUS_MISSING_ENFORCEMENT,
    STATUS_OK,
    die,
    load_registry,
    read_matrix,
    registry_kinds,
    write_matrix,
)

HEADER_SUFFIXES = {".h", ".hh", ".hpp", ".hxx", ".H", ".tpp", ".inl"}
SOURCE_SUFFIXES = HEADER_SUFFIXES | {".c", ".cc", ".cpp", ".cxx", ".C"}

IDENT = re.compile(r"[A-Za-z_]\w*")
NESTED_NS = re.compile(
    r"namespace\s+(?P<names>[A-Za-z_]\w*(?:\s*::\s*[A-Za-z_]\w*)*)\s*\{"
)
ANON_NS = re.compile(r"namespace\s*\{")
CLASS_KW = re.compile(r"\b(?:class|struct)\b")
FUNC_START = re.compile(
    r"(?P<qual>(?:[A-Za-z_]\w*::)*)(?P<dtor>~)?(?P<name>[A-Za-z_]\w*)\s*\("
)

NOT_FUNCTION = {
    "if",
    "while",
    "for",
    "switch",
    "catch",
    "return",
    "sizeof",
    "static_assert",
    "elif",
    "ifdef",
    "ifndef",
    "define",
    "pragma",
    "do",
    "else",
    "try",
    "throw",
    "new",
    "delete",
    "case",
    "decltype",
    "typeid",
    "alignof",
    "alignas",
    "requires",
    "concept",
    "static_cast",
    "dynamic_cast",
    "const_cast",
    "reinterpret_cast",
}

SPEC_WORDS = {
    "const",
    "volatile",
    "override",
    "final",
    "noexcept",
    "mutable",
    "constexpr",
    "consteval",
    "constinit",
    "try",
    "and",
    "or",
    "not",
    "char",
    "int",
    "void",
    "auto",
    "long",
    "short",
    "unsigned",
    "signed",
    "bool",
    "double",
    "float",
    "wchar_t",
    "char8_t",
    "char16_t",
    "char32_t",
}


@dataclass
class Region:
    """A named class or function body in scanned source."""

    qualified: str
    kind: str  # "class" or "function"
    start: int
    end: int
    file: Path


@dataclass
class Scan:
    regions: list[Region] = field(default_factory=list)
    invocations: list[tuple[Path, int, str]] = field(default_factory=list)


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        prog="dbc_pair_gate.py",
        description=(
            "Fail if documented contract kinds and registry-macro "
            "enforcement drift in either direction."
        ),
    )
    parser.add_argument(
        "--src",
        type=Path,
        required=True,
        help="directory of public headers to scan",
    )
    parser.add_argument(
        "--src2",
        type=Path,
        default=Path("source"),
        help="directory of implementation TUs (default: source/)",
    )
    parser.add_argument(
        "--registry",
        type=Path,
        required=True,
        help="path to tools/dbc/macros.yaml",
    )
    parser.add_argument(
        "--doc-matrix",
        type=Path,
        required=True,
        help="matrix emitted by dbc_doc_gate.py",
    )
    parser.add_argument(
        "--out",
        type=Path,
        required=True,
        help="DBC matrix output path (.yaml / .yml / .json)",
    )
    return parser.parse_args(argv)


def iter_sources(root: Path) -> list[Path]:
    if not root.is_dir():
        return []
    files = [
        path
        for path in sorted(root.rglob("*"))
        if path.is_file() and path.suffix in SOURCE_SUFFIXES
    ]
    return files


def blank_comments_and_strings(text: str) -> str:
    """Replace comments and quoted strings with spaces; keep newlines."""
    out: list[str] = []
    i = 0
    n = len(text)
    while i < n:
        if text.startswith("//", i):
            j = text.find("\n", i)
            if j < 0:
                out.append(" " * (n - i))
                break
            out.append(" " * (j - i))
            i = j
            continue
        if text.startswith("/*", i):
            j = text.find("*/", i + 2)
            if j < 0:
                out.append("".join("\n" if c == "\n" else " " for c in text[i:]))
                break
            chunk = text[i : j + 2]
            out.append("".join("\n" if c == "\n" else " " for c in chunk))
            i = j + 2
            continue
        if text[i] in "\"'":
            quote = text[i]
            out.append(" ")
            i += 1
            while i < n:
                if text[i] == "\\":
                    out.append("  " if i + 1 < n else " ")
                    i += 2
                    continue
                if text[i] == quote:
                    out.append(" ")
                    i += 1
                    break
                out.append("\n" if text[i] == "\n" else " ")
                i += 1
            continue
        out.append(text[i])
        i += 1
    return "".join(out)


def blank_preprocessor(text: str) -> str:
    """Blank #if/#define lines (including continuations) so macros are not sites."""
    lines = text.splitlines(keepends=True)
    out: list[str] = []
    continuing = False
    for line in lines:
        stripped = line.lstrip()
        is_pp = continuing or stripped.startswith("#")
        if is_pp:
            out.append("".join("\n" if c == "\n" else " " for c in line))
            continuing = stripped.rstrip("\n\r").endswith("\\")
        else:
            out.append(line)
            continuing = False
    return "".join(out)


def prepare(text: str) -> str:
    return blank_preprocessor(blank_comments_and_strings(text))


def match_bracket(text: str, i: int) -> int | None:
    """Return index of the matching closer for text[i], or None."""
    openers = {"(": ")", "[": "]", "{": "}"}
    opener = text[i]
    closer = openers.get(opener)
    if closer is None:
        return None
    depth = 0
    n = len(text)
    j = i
    while j < n:
        c = text[j]
        if c == opener:
            depth += 1
        elif c == closer:
            depth -= 1
            if depth == 0:
                return j
        j += 1
    return None


def last_nonspace(text: str, i: int) -> str:
    while i >= 0 and text[i].isspace():
        i -= 1
    return text[i] if i >= 0 else ""


def skip_trailing_return(text: str, j: int) -> int:
    n = len(text)
    while j < n:
        while j < n and text[j].isspace():
            j += 1
        if j >= n or text[j] in "{;":
            return j
        if text[j] == "<":
            close = match_bracket(text, j)
            j = (close + 1) if close is not None else j + 1
            continue
        if text[j] in "*&":
            j += 1
            continue
        if text.startswith("::", j):
            j += 2
            continue
        if text[j].isalnum() or text[j] == "_":
            while j < n and (text[j].isalnum() or text[j] == "_"):
                j += 1
            continue
        return j
    return j


def skip_ctor_init(text: str, colon: int) -> tuple[str, int] | None:
    """After a ctor ':' initializer list, find the function-body '{'."""
    j = colon + 1
    n = len(text)
    brace = 0
    paren = 0
    while j < n:
        c = text[j]
        if c == ";" and brace == 0 and paren == 0:
            return ("decl", j)
        if c == "(":
            paren += 1
        elif c == ")" and paren:
            paren -= 1
        elif c == "{":
            if brace == 0 and paren == 0:
                prev = last_nonspace(text, j - 1)
                if prev in ":," or prev.isalnum() or prev == "_":
                    brace += 1
                else:
                    return ("body", j)
            else:
                brace += 1
        elif c == "}" and brace:
            brace -= 1
        j += 1
    return None


def after_params(text: str, close_paren: int) -> tuple[str, int] | None:
    """Classify the declarator tail after a parameter list's ')'."""
    j = close_paren + 1
    n = len(text)
    while j < n:
        while j < n and text[j].isspace():
            j += 1
        if j >= n:
            return None
        if text[j] == "{":
            return ("body", j)
        if text[j] == ";":
            return ("decl", j)
        if text[j] == "=":
            return ("decl", j)
        if text[j] == ":":
            return skip_ctor_init(text, j)
        if text.startswith("->", j):
            j = skip_trailing_return(text, j + 2)
            continue
        if text.startswith("noexcept", j) and not (
            j + 8 < n and (text[j + 8].isalnum() or text[j + 8] == "_")
        ):
            j += 8
            while j < n and text[j].isspace():
                j += 1
            if j < n and text[j] == "(":
                close = match_bracket(text, j)
                j = (close + 1) if close is not None else j + 1
            continue
        if text[j].isalpha() or text[j] == "_":
            k = j
            while k < n and (text[k].isalnum() or text[k] == "_"):
                k += 1
            word = text[j:k]
            if word in SPEC_WORDS:
                j = k
                continue
            return None
        if text[j] in "&*":
            j += 1
            continue
        return None
    return None


def qualify(scope: list[str], name: str) -> str:
    parts = [part for part in scope if part]
    parts.append(name)
    return "::".join(parts)


def skip_enum(text: str, i: int) -> int:
    n = len(text)
    j = i + 4
    while j < n and text[j].isspace():
        j += 1
    if text.startswith("class", j) or text.startswith("struct", j):
        j += 5 if text.startswith("class", j) else 6
    while j < n and text[j] not in "{;":
        j += 1
    if j < n and text[j] == "{":
        close = match_bracket(text, j)
        return (close + 1) if close is not None else n
    return j + 1 if j < n else n


def parse_class_head(text: str, i: int) -> tuple[str, int] | None:
    """Return (class_name, index_of_body_brace) or None for a forward decl."""
    match = CLASS_KW.match(text, i)
    if match is None:
        return None
    j = match.end()
    n = len(text)
    last_ident: str | None = None
    while j < n:
        while j < n and text[j].isspace():
            j += 1
        if j >= n:
            return None
        if text.startswith("[[", j):
            close = text.find("]]", j + 2)
            j = (close + 2) if close >= 0 else n
            continue
        if text[j] == "{":
            if last_ident is None:
                return None
            return last_ident, j
        if text[j] == ";":
            return None
        if text[j] == ":":
            while j < n and text[j] != "{" and text[j] != ";":
                if text[j] == "{":
                    break
                j += 1
            continue
        ident = IDENT.match(text, j)
        if ident is not None:
            last_ident = ident.group()
            j = ident.end()
            continue
        j += 1
    return None


def parse_regions(text: str, path: Path, macros: set[str]) -> list[Region]:
    regions: list[Region] = []
    scope: list[tuple[str, int]] = []
    n = len(text)
    i = 0
    while i < n:
        if text[i].isspace():
            i += 1
            continue
        if text.startswith("enum", i) and (
            i == 0 or not (text[i - 1].isalnum() or text[i - 1] == "_")
        ):
            rest = i + 4
            if rest >= n or not (text[rest].isalnum() or text[rest] == "_"):
                i = skip_enum(text, i)
                continue
        ns = NESTED_NS.match(text, i)
        if ns is not None:
            names = re.sub(r"\s+", "", ns.group("names"))
            brace = ns.end() - 1
            close = match_bracket(text, brace)
            if close is None:
                i = ns.end()
                continue
            scope.append((names, close))
            i = brace + 1
            continue
        anon = ANON_NS.match(text, i)
        if anon is not None:
            brace = anon.end() - 1
            close = match_bracket(text, brace)
            if close is None:
                i = anon.end()
                continue
            scope.append(("", close))
            i = brace + 1
            continue
        if CLASS_KW.match(text, i) and (
            i == 0 or not (text[i - 1].isalnum() or text[i - 1] == "_")
        ):
            parsed = parse_class_head(text, i)
            if parsed is not None:
                name, brace = parsed
                close = match_bracket(text, brace)
                if close is not None:
                    qname = qualify([s[0] for s in scope], name)
                    regions.append(
                        Region(qname, "class", brace, close, path)
                    )
                    scope.append((name, close))
                    i = brace + 1
                    continue
        func = FUNC_START.match(text, i)
        if func is not None and (
            i == 0 or not (text[i - 1].isalnum() or text[i - 1] == "_")
        ):
            name = func.group("name")
            if name not in NOT_FUNCTION and name not in macros:
                open_paren = func.end() - 1
                close_paren = match_bracket(text, open_paren)
                if close_paren is not None:
                    tail = after_params(text, close_paren)
                    if tail is not None and tail[0] == "body":
                        brace = tail[1]
                        close = match_bracket(text, brace)
                        if close is not None:
                            dtor = "~" if func.group("dtor") else ""
                            local = f"{func.group('qual')}{dtor}{name}"
                            qname = qualify([s[0] for s in scope], local)
                            regions.append(
                                Region(qname, "function", brace, close, path)
                            )
                            i = close + 1
                            continue
        if text[i] == "}":
            while scope and scope[-1][1] <= i:
                scope.pop()
            i += 1
            continue
        i += 1
    return regions


def find_invocations(
    text: str, path: Path, macro_to_kind: dict[str, str]
) -> list[tuple[Path, int, str]]:
    found: list[tuple[Path, int, str]] = []
    for match in IDENT.finditer(text):
        kind = macro_to_kind.get(match.group())
        if kind is None:
            continue
        j = match.end()
        n = len(text)
        while j < n and text[j].isspace():
            j += 1
        if j < n and text[j] == "(":
            found.append((path, match.start(), kind))
    return found


def scan_tree(roots: list[Path], macro_to_kind: dict[str, str]) -> Scan:
    seen: set[Path] = set()
    scan = Scan()
    macros = set(macro_to_kind)
    for root in roots:
        for path in iter_sources(root):
            resolved = path.resolve()
            if resolved in seen:
                continue
            seen.add(resolved)
            try:
                raw = path.read_text(encoding="utf-8")
            except OSError as exc:
                die(f"cannot read {path}: {exc}")
            text = prepare(raw)
            scan.regions.extend(parse_regions(text, path, macros))
            scan.invocations.extend(
                find_invocations(text, path, macro_to_kind)
            )
    return scan


def innermost(
    regions: list[Region], path: Path, offset: int
) -> Region | None:
    containing = [
        region
        for region in regions
        if region.file.resolve() == path.resolve()
        and region.start <= offset < region.end
    ]
    if not containing:
        return None
    return min(containing, key=lambda region: region.end - region.start)


def kinds_by_region(scan: Scan) -> dict[str, set[str]]:
    collected: dict[str, set[str]] = {}
    for path, offset, kind in scan.invocations:
        region = innermost(scan.regions, path, offset)
        if region is None:
            continue
        collected.setdefault(region.qualified, set()).add(kind)
    return collected


def class_names(scan: Scan) -> set[str]:
    return {region.qualified for region in scan.regions if region.kind == "class"}


def parent_class(qualified: str, classes: set[str]) -> str | None:
    if "::" not in qualified:
        return None
    parent = qualified.rsplit("::", 1)[0]
    if parent in classes:
        return parent
    # Constructor/destructor recorded as Class::Class / Class::~Class
    # even when the class region itself is just Class.
    simple = parent.split("::")[-1]
    if parent in classes or simple in classes:
        return parent if parent in classes else simple
    return None


def lookup_collected(
    interface: str, collected: dict[str, set[str]]
) -> set[str]:
    if interface in collected:
        return set(collected[interface])
    last = interface.split("::")[-1]
    hits = [
        qname
        for qname in collected
        if qname == last
        or qname.endswith("::" + last)
        or qname.endswith("::" + interface)
    ]
    aligned = [
        qname
        for qname in hits
        if interface == qname
        or interface.endswith("::" + qname)
        or qname.endswith("::" + interface)
    ]
    kinds: set[str] = set()
    for qname in aligned or hits:
        kinds |= collected[qname]
    return kinds


def enforced_for(
    interface: str,
    collected: dict[str, set[str]],
    classes: set[str],
    class_interfaces: set[str],
) -> set[str]:
    kinds = lookup_collected(interface, collected)
    parent = parent_class(interface, classes)
    if parent is not None and parent in class_interfaces:
        kinds.discard("invariant")
    return kinds


def harvest_class_invariants(
    interface: str,
    collected: dict[str, set[str]],
    classes: set[str],
) -> set[str]:
    kinds: set[str] = set(collected.get(interface, ()))
    last = interface.split("::")[-1]
    prefixes = (interface + "::", last + "::")
    for qname, found in collected.items():
        if "invariant" not in found:
            continue
        if qname.startswith(prefixes) or qname == interface or qname == last:
            kinds.add("invariant")
        elif parent_class(qname, classes) in {interface, last}:
            kinds.add("invariant")
    return kinds


def order_kinds(kinds: set[str], registry_order: list[str]) -> list[str]:
    rank = {kind: index for index, kind in enumerate(registry_order)}
    return sorted(kinds, key=lambda kind: (rank.get(kind, 100), kind))


def pair_status(
    documented: set[str], enforced: set[str]
) -> tuple[str, list[tuple[str, str]]]:
    """Return (status, list of (direction, kind))."""
    missing_enforcement = sorted(documented - enforced)
    extra = sorted(enforced - documented)
    drifts: list[tuple[str, str]] = []
    for kind in missing_enforcement:
        drifts.append(("documented-not-enforced", kind))
    for kind in extra:
        drifts.append(("enforced-not-documented", kind))
    if missing_enforcement and extra:
        return STATUS_DRIFT, drifts
    if missing_enforcement:
        return STATUS_MISSING_ENFORCEMENT, drifts
    if extra:
        return STATUS_DRIFT, drifts
    return STATUS_OK, drifts


def evaluate(
    doc: dict[str, Any],
    scan: Scan,
    registry_order: list[str],
) -> tuple[list[dict[str, Any]], list[str]]:
    collected = kinds_by_region(scan)
    classes = class_names(scan)
    rows_in = doc.get("interfaces") or []
    class_interfaces: set[str] = set()
    class_qnames = classes
    for raw in rows_in:
        if not isinstance(raw, dict):
            continue
        name = str(raw.get("interface", ""))
        last = name.split("::")[-1]
        if name in class_qnames or last in class_qnames:
            # A row is a class if a class region matches and it is not
            # Class::Class (constructor).
            if name in class_qnames or (
                last in class_qnames and "::" not in name
            ):
                class_interfaces.add(name)
                class_interfaces.add(last)

    rows: list[dict[str, Any]] = []
    diagnostics: list[str] = []
    for raw in rows_in:
        if not isinstance(raw, dict) or "interface" not in raw:
            die(f"doc matrix row missing 'interface': {raw!r}")
        interface = str(raw["interface"])
        documented_kinds = [
            str(kind) for kind in (raw.get("documented_kinds") or [])
        ]
        none_kinds = {str(kind) for kind in (raw.get("none_kinds") or [])}
        effective = {
            kind for kind in documented_kinds if kind not in none_kinds
        }
        is_class = interface in class_interfaces or (
            interface.split("::")[-1] in class_interfaces
            and "::" not in interface
        )
        if is_class:
            enforced = harvest_class_invariants(interface, collected, classes)
            # Classes pair on invariant; leftover pre/post on the class
            # row itself (unusual) still participate.
            enforced |= collected.get(interface, set())
        else:
            enforced = enforced_for(
                interface, collected, classes, class_interfaces
            )
        status, drifts = pair_status(effective, enforced)
        rows.append(
            {
                "interface": interface,
                "documented_kinds": documented_kinds,
                "none_kinds": sorted(none_kinds),
                "enforced_kinds": order_kinds(enforced, registry_order),
                "status": status,
            }
        )
        for direction, kind in drifts:
            diagnostics.append(f"{interface}: {direction} {kind}")
    return rows, diagnostics


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    src = args.src.expanduser().resolve()
    src2 = args.src2.expanduser()
    if not src2.is_absolute():
        src2 = src2.resolve()
    registry_path = args.registry.expanduser().resolve()
    doc_path = args.doc_matrix.expanduser().resolve()
    out_path = args.out.expanduser().resolve()

    if not src.is_dir():
        die(f"header directory not found: {src}")

    entries = load_registry(registry_path)
    macro_to_kind = {entry["macro"]: entry["kind"] for entry in entries}
    if not macro_to_kind:
        die("registry does not define any macros")

    doc = read_matrix(doc_path)
    scan = scan_tree([src, src2], macro_to_kind)
    rows, diagnostics = evaluate(doc, scan, registry_kinds(entries))
    payload = {
        "gate": "pair",
        "interfaces": rows,
    }
    write_matrix(out_path, payload)

    for line in diagnostics:
        sys.stderr.write(f"{line}\n")
    n_ifaces = len(rows)
    n_gaps = sum(1 for row in rows if row["status"] != STATUS_OK)
    if diagnostics:
        sys.stderr.write(
            f"pair-gate: {n_gaps} of {n_ifaces} interfaces drifted "
            "(documented-not-enforced / enforced-not-documented)\n"
        )
        return EXIT_GAPS
    print(
        f"pair-gate: {n_ifaces} interfaces, 0 gaps ({out_path})",
        flush=True,
    )
    return EXIT_OK


if __name__ == "__main__":
    sys.exit(main())
