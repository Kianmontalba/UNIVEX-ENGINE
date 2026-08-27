#!/usr/bin/env python3
"""Render supplied UniVex gizmo SVGs into embedded RGBA C++ arrays."""
from __future__ import annotations

import subprocess
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "engine" / "editor" / "assets" / "gizmos"
OUT = ROOT / "engine" / "editor" / "src"
ASSET_OUT = SOURCE
ICONS = [
    "move.svg",
    "rotate.svg",
    "scale.svg",
    "universal.svg",
    "viewport_nav_gizmo.svg",
]
SIZE = 24


def render_svg(source: Path, output: Path) -> None:
    subprocess.run(
        ["rsvg-convert", "-w", str(SIZE), "-h", str(SIZE), "-o", str(output), str(source)],
        check=True,
    )


def write_inc(images: dict[str, bytes]) -> None:
    lines = ["// Generated from the supplied UNIVEX_Gizmo_Package SVG assets.\n"]
    for stem, pixels in images.items():
        name = "uve_gizmo_" + stem + "_rgba"
        lines.append(f"inline constexpr std::array<std::uint8_t, {len(pixels)}> {name}{{{{\n")
        for offset in range(0, len(pixels), 16):
            chunk = pixels[offset : offset + 16]
            lines.append("    " + ", ".join(f"0x{value:02X}" for value in chunk) + ",\n")
        lines.append("}};\n")
    (OUT / "uve_gizmo_icon_display_bytes.inc").write_text("".join(lines), encoding="utf-8")


def main() -> None:
    ASSET_OUT.mkdir(parents=True, exist_ok=True)
    rendered: dict[str, bytes] = {}
    for icon in ICONS:
        source = SOURCE / icon
        if not source.is_file():
            raise FileNotFoundError(source)
        png = ASSET_OUT / (Path(icon).stem + ".png")
        render_svg(source, png)
        with Image.open(png) as image:
            rgba = image.convert("RGBA")
            if rgba.size != (SIZE, SIZE):
                raise ValueError(f"unexpected size for {icon}: {rgba.size}")
            rendered[Path(icon).stem] = rgba.tobytes()
        png.unlink()
    write_inc(rendered)


if __name__ == "__main__":
    main()
