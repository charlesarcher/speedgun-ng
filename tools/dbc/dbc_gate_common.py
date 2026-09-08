#!/usr/bin/env python3
"""Shared helpers for the Phase 0 DBC coverage gates.

Consumed by ``dbc_doc_gate.py`` (documentation-presence, T025) and
``dbc_pair_gate.py`` (doc-to-enforcement pairing, T026). Neither gate
hard-codes a macro list: both load ``tools/dbc/macros.yaml``.

Public API
----------
EXIT_OK, EXIT_GAPS, EXIT_USAGE
    Process exit codes: 0 = pass, 1 = contract gaps, 2 = usage.

die(message, code=EXIT_USAGE)
    Write ``message`` to stderr and raise ``SystemExit(code)``.

load_registry(path)
    Parse the macro registry YAML. Returns a list of dicts with keys
    ``macro``, ``enforcement_function``, ``kind``, ``always_on``.
    Exits 2 on a missing or invalid registry.

registry_kinds(entries)
    Unique contract kinds in first-seen order.

is_none_marker(text)
    True if and only if stripped text is the explicit empty-contract marker
    ``none`` (case-insensitive). Canonical form is ``\\pre none`` as in
    ``include/speedgun-ng/speedgun-ng.hpp``.

element_text(el)
    Recursive concatenated character data of an XML element.

write_matrix(path, payload)
    Emit the DBC matrix. A ``.json`` suffix selects JSON; every other
    suffix (including ``.yaml``) selects YAML. Parent directories are
    created. Doc-gate payload shape::

        gate: doc
        interfaces:
          - interface: <qualified name>
            documented_kinds: [precondition, postcondition, ...]
            none_kinds: [precondition, ...]
            enforced_kinds: []
            status: ok | missing-doc | missing-enforcement | drift

read_matrix(path)
    Load a matrix written by ``write_matrix``. Exits 2 if the file is
    missing or not a mapping with an ``interfaces`` list.

The pair gate (T026) fills ``enforced_kinds`` and may overwrite
``status``. ``none_kinds`` is the subset of ``documented_kinds`` whose
simplesect / comment text is the explicit ``none`` marker (FR-030);
pairing must not require enforcement for those kinds.
"""

from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Any, Mapping
from xml.etree.ElementTree import Element

try:
    import yaml
except ImportError as exc:  # pragma: no cover - environment guard
    sys.stderr.write(
        "dbc gates require PyYAML (install python3-yaml): "
        f"{exc}\n"
    )
    raise SystemExit(2) from exc

EXIT_OK = 0
EXIT_GAPS = 1
EXIT_USAGE = 2

NONE_MARKER = "none"

STATUS_OK = "ok"
STATUS_MISSING_DOC = "missing-doc"
STATUS_MISSING_ENFORCEMENT = "missing-enforcement"
STATUS_DRIFT = "drift"


def die(message: str, code: int = EXIT_USAGE) -> None:
    sys.stdout.flush()
    sys.stderr.write(f"{message}\n")
    raise SystemExit(code)


def load_registry(path: Path) -> list[dict[str, Any]]:
    """Load ``macros.yaml``; exit 2 if it is missing or not a mapping list."""
    if not path.is_file():
        die(f"registry not found: {path}")
    try:
        loaded = yaml.safe_load(path.read_text(encoding="utf-8"))
    except yaml.YAMLError as exc:
        die(f"registry is not valid YAML: {exc}")
    if not isinstance(loaded, list) or not loaded:
        die("registry must be a non-empty YAML list of macro entries")
    entries: list[dict[str, Any]] = []
    seen: set[str] = set()
    for raw in loaded:
        if not isinstance(raw, dict) or "macro" not in raw or "kind" not in raw:
            die(f"registry entry missing 'macro' or 'kind': {raw!r}")
        name = str(raw["macro"])
        if name in seen:
            die(f"registry has duplicate entry for {name}")
        seen.add(name)
        entries.append(
            {
                "macro": name,
                "enforcement_function": str(raw.get("enforcement_function", "")),
                "kind": str(raw["kind"]),
                "always_on": bool(raw.get("always_on", False)),
            }
        )
    return entries


def registry_kinds(entries: list[dict[str, Any]]) -> list[str]:
    kinds: list[str] = []
    seen: set[str] = set()
    for entry in entries:
        kind = entry["kind"]
        if kind not in seen:
            seen.add(kind)
            kinds.append(kind)
    return kinds


def is_none_marker(text: str) -> bool:
    return text.strip().casefold() == NONE_MARKER


def element_text(el: Element | None) -> str:
    if el is None:
        return ""
    return "".join(el.itertext())


def write_matrix(path: Path, payload: Mapping[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    suffix = path.suffix.lower()
    if suffix == ".json":
        path.write_text(
            json.dumps(payload, indent=2, ensure_ascii=False) + "\n",
            encoding="utf-8",
        )
        return
    path.write_text(
        yaml.safe_dump(payload, sort_keys=False, allow_unicode=True),
        encoding="utf-8",
    )


def read_matrix(path: Path) -> dict[str, Any]:
    """Load a DBC matrix; exit 2 if it is missing or malformed."""
    if not path.is_file():
        die(f"doc matrix not found: {path}")
    text = path.read_text(encoding="utf-8")
    try:
        if path.suffix.lower() == ".json":
            loaded = json.loads(text)
        else:
            loaded = yaml.safe_load(text)
    except (json.JSONDecodeError, yaml.YAMLError) as exc:
        die(f"doc matrix is not valid: {exc}")
    if not isinstance(loaded, dict) or "interfaces" not in loaded:
        die("doc matrix must be a mapping with an 'interfaces' list")
    if not isinstance(loaded["interfaces"], list):
        die("doc matrix 'interfaces' must be a list")
    return loaded
