#!/usr/bin/env python3
"""Embed the subsetted Tabler Icons font as a compiled-in C++ byte array.

Mirrors generate_gizmo_icon_bytes.py / generate_editor_icon_bytes.py: a small
source asset under engine/editor/assets/ is compiled into an .inc byte array
under engine/editor/src/, so the editor has no runtime file-load dependency.
See engine/editor/assets/fonts/THIRD_PARTY_NOTICES.md for the font's origin
and license, and for the exact pyftsubset command used to produce the source
.ttf this script reads.
"""
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "engine" / "editor" / "assets" / "fonts" / "tabler-icons-subset.ttf"
OUT = ROOT / "engine" / "editor" / "src" / "uve_icon_font_bytes.inc"


def write_inc(data: bytes) -> None:
    lines = ["// Generated from the subsetted Tabler Icons font asset; see\n",
             "// engine/editor/assets/fonts/THIRD_PARTY_NOTICES.md for license and provenance.\n",
             f"inline constexpr std::array<std::uint8_t, {len(data)}> uve_icon_font_ttf_bytes{{{{\n"]
    for offset in range(0, len(data), 16):
        chunk = data[offset : offset + 16]
        lines.append("    " + ", ".join(f"0x{value:02X}" for value in chunk) + ",\n")
    lines.append("}};\n")
    OUT.write_text("".join(lines), encoding="utf-8")


def main() -> None:
    if not SOURCE.is_file():
        raise FileNotFoundError(SOURCE)
    write_inc(SOURCE.read_bytes())


if __name__ == "__main__":
    main()
