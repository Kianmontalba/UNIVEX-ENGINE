#!/usr/bin/env python3
"""Rasterize the Content Browser's per-asset-type badge SVGs into an embedded
RGBA byte array.

Mirrors generate_editor_icon_bytes.py, but rasterizes with cairosvg instead
of shelling out to rsvg-convert: cairosvg is a pure-Python, pip-installable
SVG renderer, which avoids depending on a system binary that may not be
present on every machine that regenerates these assets. See
engine/editor/assets/icons/content_types/THIRD_PARTY_NOTICES.md for the
source glyphs' origin and license; each composed SVG is a colored rounded-
square badge with a Tabler Icons glyph centered on it in white.
"""
from __future__ import annotations

from pathlib import Path

import cairosvg

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "engine" / "editor" / "assets" / "icons" / "content_types"
OUTPUT = ROOT / "engine" / "editor" / "src" / "uve_content_type_icon_bytes.inc"
SIZE = 64


def render_svg(source: Path) -> bytes:
    png_bytes = cairosvg.svg2png(url=str(source), output_width=SIZE, output_height=SIZE)
    from io import BytesIO

    from PIL import Image

    with Image.open(BytesIO(png_bytes)) as image:
        rgba = image.convert("RGBA")
        if rgba.size != (SIZE, SIZE):
            raise ValueError(f"unexpected size for {source}: {rgba.size}")
        return rgba.tobytes()


def main() -> None:
    lines = ["// Generated from engine/editor/assets/icons/content_types/*.svg by\n",
             "// tools/generate_content_type_icon_bytes.py. See that directory's\n",
             "// THIRD_PARTY_NOTICES.md for glyph provenance and license.\n"]
    for source in sorted(SOURCE.glob("*.svg")):
        stem = source.stem
        name = f"uve_content_type_icon_{stem}_rgba"
        pixels = render_svg(source)
        lines.append(f"inline constexpr std::array<std::uint8_t, {len(pixels)}> {name}{{{{\n")
        for offset in range(0, len(pixels), 16):
            chunk = pixels[offset : offset + 16]
            lines.append("    " + ", ".join(f"0x{value:02X}" for value in chunk) + ",\n")
        lines.append("}};\n")
    OUTPUT.write_text("".join(lines), encoding="utf-8")


if __name__ == "__main__":
    main()
