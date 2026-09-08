#!/usr/bin/env python3
"""Documentation-presence half of the Phase 0 DBC coverage gate (FR-027).

CLI::

    python3 tools/dbc/dbc_doc_gate.py \\
        --xml <doxygen-xml-dir> --registry tools/dbc/macros.yaml --out <matrix>

Scope is public functions and classes in the supplied Doxygen XML
(public headers). Closed exemptions (FR-029): private/protected members,
lambdas/local functions, defaulted/deleted functions, friend declarations,
constexpr-only interfaces. Nested ``detail`` namespaces are not public
API and are out of scope.

A public function fails if its documentation block lacks a dedicated
``\\pre`` or ``\\post`` simplesect (or the simplesect is empty). An
explicit ``none`` marker (``\\pre none``, form from speedgun-ng.hpp)
counts as present (FR-030). Mid-sentence ``\\pre``/``\\post`` in prose
do not count: Doxygen inlines those into a mixed paragraph, while a
real contract section is a paragraph of only ``simplesect`` children.

A class that documents ``\\invariant`` with empty text fails; classes
that do not declare an invariant are not required to have the section.

Exit 0 = pass, 1 = gaps, 2 = usage. The documented-kinds matrix is
written to ``--out`` on every successful parse (including gap runs).
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any
from xml.etree import ElementTree as ET

from dbc_gate_common import (
    EXIT_GAPS,
    EXIT_OK,
    EXIT_USAGE,
    STATUS_MISSING_DOC,
    STATUS_OK,
    die,
    element_text,
    is_none_marker,
    load_registry,
    write_matrix,
)

# Doxygen simplesect @kind -> contract kind (registry vocabulary).
SIMPLESECT_TO_KIND = {
    "pre": "precondition",
    "post": "postcondition",
    "invariant": "invariant",
}

FUNCTION_REQUIRED = ("precondition", "postcondition")

DEFAULTED_OR_DELETED = re.compile(r"=\s*(?:default|delete)\b")
HEADER_SUFFIXES = {".h", ".hh", ".hpp", ".hxx", ".H", ".tpp", ".inl"}
SKIP_INDEX_NAMES = {"index.xml", "Doxyfile.xml"}
COMPOUND_KINDS = {"namespace", "class", "struct", "interface"}
# Line-start Doxygen command in a doc comment (not mid-sentence prose).
SOURCE_CMD = re.compile(
    r"^\s*(?://[/!]?|\*)\s*[\\@](pre|post|invariant)\b\s*(.*)$"
)


@dataclass
class Sections:
    """Contract simplesects keyed by registry kind, with raw text."""

    texts: dict[str, str] = field(default_factory=dict)

    def present(self, kind: str) -> bool:
        return kind in self.texts

    def is_none(self, kind: str) -> bool:
        return kind in self.texts and is_none_marker(self.texts[kind])

    def documented_kinds(self) -> list[str]:
        return [kind for kind, text in self.texts.items() if text.strip()]

    def none_kinds(self) -> list[str]:
        return [kind for kind in self.texts if self.is_none(kind)]


@dataclass
class Member:
    xml_kind: str
    prot: str
    name: str
    argsstring: str
    definition: str
    constexpr: bool
    local: bool
    scope: str
    sections: Sections
    file: str
    line: str


@dataclass
class Compound:
    xml_kind: str
    name: str
    prot: str
    sections: Sections
    members: list[Member]
    file: str


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        prog="dbc_doc_gate.py",
        description=(
            "Fail if in-scope public functions lack \\pre/\\post "
            "(or invariant-bearing classes lack \\invariant)."
        ),
    )
    parser.add_argument(
        "--xml",
        type=Path,
        required=True,
        help="directory of Doxygen XML (index.xml + compound files)",
    )
    parser.add_argument(
        "--registry",
        type=Path,
        required=True,
        help="path to tools/dbc/macros.yaml",
    )
    parser.add_argument(
        "--out",
        type=Path,
        required=True,
        help="DBC matrix output path (.yaml / .yml / .json)",
    )
    return parser.parse_args(argv)


def is_detail_scope(qualified: str) -> bool:
    return "detail" in qualified.split("::")


def is_header_location(path: str) -> bool:
    if not path:
        return True
    suffix = Path(path).suffix
    if not suffix:
        return True
    return suffix in HEADER_SUFFIXES


def is_contract_para(para: ET.Element) -> bool:
    """True if *para* is a dedicated contract block (only simplesect children).

    Doxygen puts ``\\pre`` / ``\\post`` / ``\\invariant`` written as their
    own commands into a paragraph that contains nothing else. The same
    commands appearing mid-sentence (as in fixture_missing_docs.hpp
    prose) land in a mixed paragraph and must not count as documentation.
    """
    if para.text and para.text.strip():
        return False
    children = list(para)
    if not children or any(child.tag != "simplesect" for child in children):
        return False
    return all(not (child.tail and child.tail.strip()) for child in children)


def extract_xml_sections(description: ET.Element | None) -> Sections:
    """Parse simplesect kind pre/post/invariant from a Doxygen description.

    Only paragraphs whose children are exclusively ``simplesect`` count.
    Mid-sentence ``\\pre`` in prose (fixture_missing_docs) lands in a mixed
    paragraph and is ignored.
    """
    texts: dict[str, str] = {}
    if description is None:
        return Sections(texts)
    for para in description.findall("para"):
        if not is_contract_para(para):
            continue
        for sect in para.findall("simplesect"):
            mapped = SIMPLESECT_TO_KIND.get(sect.get("kind", ""))
            if mapped is None:
                continue
            texts[mapped] = element_text(sect)
    return Sections(texts)


def resolve_source(file: str) -> Path | None:
    if not file:
        return None
    path = Path(file)
    return path if path.is_file() else None


def preceding_comment_lines(lines: list[str], decl_line: int) -> list[str]:
    """Return the doc-comment lines immediately above *decl_line* (1-based)."""
    i = decl_line - 2
    while i >= 0:
        stripped = lines[i].strip()
        if not stripped or (
            stripped.startswith("[[") and stripped.endswith("]]")
        ):
            i -= 1
            continue
        break
    if i < 0:
        return []
    stripped = lines[i].strip()
    collected: list[str] = []
    if stripped.endswith("*/"):
        while i >= 0:
            collected.append(lines[i])
            if "/**" in lines[i] or "/*!" in lines[i]:
                break
            i -= 1
        collected.reverse()
        return collected
    if stripped.startswith("///") or stripped.startswith("//!"):
        while i >= 0:
            candidate = lines[i].strip()
            if candidate.startswith("///") or candidate.startswith("//!"):
                collected.append(lines[i])
                i -= 1
                continue
            if not candidate:
                i -= 1
                continue
            break
        collected.reverse()
        return collected
    return []


def extract_source_sections(file: str, line: str) -> Sections:
    path = resolve_source(file)
    if path is None or not line:
        return Sections()
    try:
        decl_line = int(line)
    except ValueError:
        return Sections()
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except OSError:
        return Sections()
    texts: dict[str, str] = {}
    for raw in preceding_comment_lines(lines, decl_line):
        match = SOURCE_CMD.match(raw.rstrip())
        if match is None:
            continue
        mapped = SIMPLESECT_TO_KIND.get(match.group(1), "")
        if not mapped:
            continue
        rest = match.group(2).strip()
        if rest.endswith("*/"):
            rest = rest[:-2].strip()
        texts[mapped] = rest
    return Sections(texts)


def extract_sections(
    description: ET.Element | None,
    file: str = "",
    line: str = "",
) -> Sections:
    """Prefer line-start source commands; fall back to XML simplesect.

    Source wins when the comment has dedicated ``\\pre``/``\\post``/
    ``\\invariant`` lines: that keeps a real ``\\pre x > 0`` visible even
    if later prose mentions ``\\pre`` and Doxygen mixes the paragraph
    (fixture_drift). XML-only fallback still fails fixture_missing_docs,
    whose ``\\pre`` tokens are mid-sentence.
    """
    source = extract_source_sections(file, line)
    if source.texts:
        return source
    return extract_xml_sections(description)


def parse_member(memberdef: ET.Element, scope: str) -> Member:
    location = memberdef.find("location")
    file = location.get("file", "") if location is not None else ""
    line = location.get("line", "") if location is not None else ""
    return Member(
        xml_kind=memberdef.get("kind", ""),
        prot=memberdef.get("prot", "public"),
        name=(memberdef.findtext("name") or "").strip(),
        argsstring=memberdef.findtext("argsstring") or "",
        definition=memberdef.findtext("definition") or "",
        constexpr=memberdef.get("constexpr") == "yes",
        local=memberdef.get("local") == "yes",
        scope=scope,
        sections=extract_sections(
            memberdef.find("detaileddescription"), file, line
        ),
        file=file,
        line=line,
    )


def parse_compound(path: Path) -> Compound | None:
    try:
        root = ET.parse(path).getroot()
    except ET.ParseError as exc:
        die(f"invalid Doxygen XML: {path}: {exc}")
    compounddef = root.find("compounddef")
    if compounddef is None:
        return None
    kind = compounddef.get("kind", "")
    if kind not in COMPOUND_KINDS:
        return None
    name = (compounddef.findtext("compoundname") or "").strip()
    location = compounddef.find("location")
    file = location.get("file", "") if location is not None else ""
    line = location.get("line", "") if location is not None else ""
    members = [
        parse_member(memberdef, name)
        for memberdef in compounddef.iter("memberdef")
    ]
    return Compound(
        xml_kind=kind,
        name=name,
        prot=compounddef.get("prot", "public"),
        sections=extract_sections(
            compounddef.find("detaileddescription"), file, line
        ),
        members=members,
        file=file,
    )


def load_compounds(xml_dir: Path) -> list[Compound]:
    if not xml_dir.is_dir():
        die(f"XML directory not found: {xml_dir}")
    compounds: list[Compound] = []
    xml_files = sorted(
        p
        for p in xml_dir.glob("*.xml")
        if p.name not in SKIP_INDEX_NAMES
    )
    if not xml_files and not (xml_dir / "index.xml").is_file():
        die(f"no Doxygen XML compounds under {xml_dir}")
    for path in xml_files:
        parsed = parse_compound(path)
        if parsed is not None:
            compounds.append(parsed)
    return compounds


def collect_friend_names(compounds: list[Compound]) -> set[str]:
    names: set[str] = set()
    for compound in compounds:
        for member in compound.members:
            if member.xml_kind == "friend" and member.name:
                names.add(member.name)
    return names


def is_lambda_or_local(member: Member) -> bool:
    if member.local:
        return True
    if not member.name:
        return True
    lowered = member.name.casefold()
    return "lambda" in lowered or member.name.startswith("{")


# Bound the header scan so a missing terminator cannot walk the file.
_DECL_SCAN_MAX_LINES = 16


def declaration_source_text(member: Member) -> str:
    """Declaration text at *member.file*/*member.line*, up to ``;`` or ``{``.

    Stops at the terminator so a later defaulted/deleted member cannot
    poison the match. 1-based *member.line* as emitted by Doxygen.
    """
    path = resolve_source(member.file)
    if path is None or not member.line:
        return ""
    try:
        decl_line = int(member.line)
    except ValueError:
        return ""
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except OSError:
        return ""
    if decl_line < 1 or decl_line > len(lines):
        return ""
    chunks: list[str] = []
    stop = min(len(lines), decl_line - 1 + _DECL_SCAN_MAX_LINES)
    for raw in lines[decl_line - 1 : stop]:
        end_semi = raw.find(";")
        end_brace = raw.find("{")
        ends = [pos for pos in (end_semi, end_brace) if pos >= 0]
        if ends:
            chunks.append(raw[: min(ends) + 1])
            break
        chunks.append(raw)
    return "\n".join(chunks)


def is_defaulted_or_deleted(member: Member) -> bool:
    """True if Doxygen or the header marks the member ``= default`` / ``= delete``.

    Rocky 10 Doxygen XML for trailing-return defaulted assignment omits
    ``= default`` from ``<argsstring>``. FR-029 still exempts those
    members, so also search ``<definition>`` and the source span.
    """
    if DEFAULTED_OR_DELETED.search(member.argsstring):
        return True
    if DEFAULTED_OR_DELETED.search(member.definition):
        return True
    return DEFAULTED_OR_DELETED.search(declaration_source_text(member)) is not None


def is_exempt_function(member: Member, friend_names: set[str]) -> bool:
    if member.xml_kind == "friend":
        return True
    if member.prot in {"private", "protected"}:
        return True
    if member.name in friend_names:
        return True
    if is_defaulted_or_deleted(member):
        return True
    if member.constexpr:
        return True
    if is_lambda_or_local(member):
        return True
    if is_detail_scope(member.scope):
        return True
    if not is_header_location(member.file):
        return True
    return False


def qualified_name(scope: str, name: str) -> str:
    if not scope:
        return name
    if name.startswith(scope + "::"):
        return name
    return f"{scope}::{name}"


def missing_function_sections(sections: Sections) -> list[str]:
    missing: list[str] = []
    for kind in FUNCTION_REQUIRED:
        if not sections.present(kind):
            missing.append(kind)
            continue
        text = sections.texts[kind].strip()
        if not text:
            missing.append(kind)
    return missing


def missing_class_sections(sections: Sections) -> list[str]:
    """Require invariant only when the class documents the section.

    FR-027 gates invariant-bearing classes. Phase 0 has only Doxygen XML,
    so a class is invariant-bearing if and only if it has an ``\\invariant``
    simplesect; an empty one fails, a missing one is not required
    (fixture_exempt::Exempt has none and must pass).
    """
    if not sections.present("invariant"):
        return []
    if not sections.texts["invariant"].strip():
        return ["invariant"]
    return []


def evaluate(
    compounds: list[Compound],
) -> tuple[list[dict[str, Any]], list[str]]:
    friend_names = collect_friend_names(compounds)
    rows: list[dict[str, Any]] = []
    diagnostics: list[str] = []
    seen: set[tuple[str, str, str, str]] = set()

    def add_row(
        interface: str,
        sections: Sections,
        missing: list[str],
        key: tuple[str, str, str, str],
    ) -> None:
        if key in seen:
            return
        seen.add(key)
        status = STATUS_MISSING_DOC if missing else STATUS_OK
        rows.append(
            {
                "interface": interface,
                "documented_kinds": sections.documented_kinds(),
                "none_kinds": sections.none_kinds(),
                "enforced_kinds": [],
                "status": status,
            }
        )
        for section in missing:
            diagnostics.append(
                f"{interface}: missing {section} documentation"
            )

    for compound in compounds:
        if compound.xml_kind in {"class", "struct", "interface"}:
            if compound.prot in {"private", "protected"}:
                continue
            if is_detail_scope(compound.name):
                continue
            if not is_header_location(compound.file):
                continue
            add_row(
                compound.name,
                compound.sections,
                missing_class_sections(compound.sections),
                ("compound", compound.name, compound.file, ""),
            )
        for member in compound.members:
            if member.xml_kind != "function":
                continue
            if is_exempt_function(member, friend_names):
                continue
            interface = qualified_name(member.scope, member.name)
            key = (member.file, member.line, member.name, member.argsstring)
            add_row(
                interface,
                member.sections,
                missing_function_sections(member.sections),
                key,
            )
    rows.sort(key=lambda row: row["interface"])
    return rows, diagnostics


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    xml_dir = args.xml.expanduser().resolve()
    registry_path = args.registry.expanduser().resolve()
    out_path = args.out.expanduser().resolve()

    entries = load_registry(registry_path)
    if not any(entry["kind"] == "precondition" for entry in entries):
        die("registry does not define a precondition kind")
    if not any(entry["kind"] == "postcondition" for entry in entries):
        die("registry does not define a postcondition kind")

    compounds = load_compounds(xml_dir)
    rows, diagnostics = evaluate(compounds)
    payload = {
        "gate": "doc",
        "interfaces": rows,
    }
    write_matrix(out_path, payload)

    for line in diagnostics:
        sys.stderr.write(f"{line}\n")
    n_ifaces = len(rows)
    n_gaps = sum(1 for row in rows if row["status"] != STATUS_OK)
    if diagnostics:
        sys.stderr.write(
            f"doc-gate: {n_gaps} of {n_ifaces} interfaces missing "
            "contract documentation\n"
        )
        return EXIT_GAPS
    print(
        f"doc-gate: {n_ifaces} interfaces, 0 gaps ({out_path})",
        flush=True,
    )
    return EXIT_OK


if __name__ == "__main__":
    sys.exit(main())
