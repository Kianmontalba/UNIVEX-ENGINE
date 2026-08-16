#!/usr/bin/env python3
# Copyright (c) 2026 UniVex Studios. All Rights Reserved.
"""Validate and deterministically render the UniVex contract inventory."""

from __future__ import annotations

import argparse
import hashlib
import pathlib
import sys
from dataclasses import dataclass


@dataclass(frozen=True)
class ContractRow:
    identifier: str
    authority: str
    anchor: str
    role: str
    consumers: str
    boundary: str


def parse_inventory(path: pathlib.Path) -> list[ContractRow]:
    rows: list[ContractRow] = []
    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        if not line.startswith("|") or line.startswith("|---") or line.startswith("| ID "):
            continue
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        if len(cells) != 6:
            raise ValueError(f"{path}:{line_number}: expected six table cells")
        row = ContractRow(cells[0], cells[1], cells[2].strip("`"), cells[3], cells[4], cells[5])
        if not row.identifier or row.identifier == "-":
            raise ValueError(f"{path}:{line_number}: contract identifier is empty")
        if any(existing.identifier == row.identifier for existing in rows):
            raise ValueError(f"{path}:{line_number}: duplicate contract identifier {row.identifier}")
        if not row.authority or row.authority.startswith("/") or ".." in pathlib.PurePosixPath(row.authority).parts:
            raise ValueError(f"{path}:{line_number}: authority must be a repository-relative path")
        if not row.anchor or "\n" in row.anchor:
            raise ValueError(f"{path}:{line_number}: anchor must be a non-empty single-line string")
        rows.append(row)
    if not rows:
        raise ValueError(f"{path}: inventory contains no contract rows")
    return sorted(rows, key=lambda row: row.identifier)


def validate_rows(root: pathlib.Path, rows: list[ContractRow]) -> None:
    for row in rows:
        authority_path = root / row.authority
        if not authority_path.is_file():
            raise ValueError(f"{row.identifier}: authority file does not exist: {row.authority}")
        content = authority_path.read_text(encoding="utf-8")
        if row.anchor not in content:
            raise ValueError(f"{row.identifier}: anchor not found in {row.authority}: {row.anchor}")


def render_generated(rows: list[ContractRow], inventory: pathlib.Path) -> str:
    inventory_hash = hashlib.sha256(inventory.read_bytes()).hexdigest()[:16]
    lines = [
        "<!-- GENERATED FILE. Do not edit directly. -->",
        "# UNIVEX ENGINE — GENERATED CONTRACT REFERENCE",
        "",
        "> This reference is generated from `docs/CONTRACT_INVENTORY.md`. Native and editor code remain authoritative; this file is documentation only.",
        "",
        f"Inventory revision: `{inventory_hash}`",
        "",
        "| ID | Authority | Anchor | Contract role | Allowed consumers | Ownership boundary |",
        "|---|---|---|---|---|---|",
    ]
    for row in rows:
        lines.append(
            f"| `{row.identifier}` | `{row.authority}` | `{row.anchor}` | {row.role} | {row.consumers} | {row.boundary} |"
        )
    lines.append("")
    return "\n".join(lines)


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path(__file__).resolve().parents[1])
    parser.add_argument("--inventory", type=pathlib.Path, default=pathlib.Path("docs/CONTRACT_INVENTORY.md"))
    parser.add_argument("--generated", type=pathlib.Path, default=pathlib.Path("docs/GENERATED_CONTRACT_REFERENCE.md"))
    parser.add_argument("--write", action="store_true", help="write the deterministic generated reference")
    parser.add_argument("--check", action="store_true", help="verify the generated reference is current")
    args = parser.parse_args(argv)
    if args.write and args.check:
        parser.error("--write and --check are mutually exclusive")
    root = args.root.resolve()
    inventory = (root / args.inventory).resolve() if not args.inventory.is_absolute() else args.inventory.resolve()
    generated = (root / args.generated).resolve() if not args.generated.is_absolute() else args.generated.resolve()
    try:
        rows = parse_inventory(inventory)
        validate_rows(root, rows)
        expected = render_generated(rows, inventory)
        if args.write:
            generated.write_text(expected, encoding="utf-8")
        elif args.check:
            actual = generated.read_text(encoding="utf-8") if generated.is_file() else None
            if actual != expected:
                raise ValueError(f"generated reference is stale or missing: {generated}")
        else:
            print(expected, end="")
    except (OSError, ValueError) as error:
        print(f"contract inventory validation failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
