#!/usr/bin/env python3
"""Rasterize supplied editor SVG icons into deterministic embedded RGBA arrays."""
from __future__ import annotations

import subprocess
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "engine" / "editor" / "assets" / "icons"
OUTPUT = ROOT / "engine" / "editor" / "src"
SIZE = 20

GROUPS = {
    "node": SOURCE / "3d_node_set",
    "component": SOURCE / "component_set",
    "general": SOURCE / "general",
}


def render_svg(source: Path, output: Path) -> bytes:
    subprocess.run(
        ["rsvg-convert", "-w", str(SIZE), "-h", str(SIZE), "-o", str(output), str(source)],
        check=True,
    )
    with Image.open(output) as image:
        rgba = image.convert("RGBA")
        if rgba.size != (SIZE, SIZE):
            raise ValueError(f"unexpected size for {source}: {rgba.size}")
        return rgba.tobytes()


def write_group(group: str, sources: list[Path]) -> None:
    lines = ["// Generated from supplied UniVex editor SVG icon assets.\n"]
    for source in sources:
        stem = source.stem
        name = f"uve_{group}_icon_{stem}_rgba"
        png = source.with_suffix(".generated.png")
        pixels = render_svg(source, png)
        png.unlink()
        lines.append(f"inline constexpr std::array<std::uint8_t, {len(pixels)}> {name}{{{{\n")
        for offset in range(0, len(pixels), 16):
            chunk = pixels[offset : offset + 16]
            lines.append("    " + ", ".join(f"0x{value:02X}" for value in chunk) + ",\n")
        lines.append("}};\n")
    (OUTPUT / f"uve_{group}_icon_bytes.inc").write_text("".join(lines), encoding="utf-8")


def main() -> None:
    for group, directory in GROUPS.items():
        sources = sorted(directory.glob("*.svg"))
        if not sources:
            raise FileNotFoundError(directory)
        write_group(group, sources)


if __name__ == "__main__":
    main()
