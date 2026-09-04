# Third-Party Notices — `engine/editor/assets/icons/content_types`

## Tabler Icons (glyphs only)

Each `*_content_type.svg` in this directory is a composed badge: a colored
rounded-square background (UniVex-authored) with a single glyph from Tabler
Icons (MIT licensed, `https://github.com/tabler/tabler-icons`) recolored
white and centered on top. The glyphs came from the `tabler-icons` PyPI
package (version `0.5.0`), which bundles the upstream outline SVG set;
`tools/generate_content_type_icon_bytes.py` regenerates the composed SVGs'
rasterized `.inc` byte array but does not regenerate the SVGs themselves —
see that script's companion composition step for how each badge was built if
a glyph or color needs to change.

| File | Source glyph | Accent color |
|---|---|---|
| `scene_content_type.svg` | `movie.svg` | `#D98A3D` |
| `prefab_content_type.svg` | `puzzle.svg` | `#3DA9D9` |
| `bundle_content_type.svg` | `package.svg` | `#A87C4F` |
| `mesh_content_type.svg` | `3d-cube-sphere.svg` | `#4A7FD9` |
| `texture_content_type.svg` | `photo.svg` | `#4CAF7D` |
| `shader_content_type.svg` | `code.svg` | `#9B59B6` |
| `material_content_type.svg` | `palette.svg` | `#E85D9E` |
| `save_content_type.svg` | `device-floppy.svg` | `#6B7785` |
| `motion_query_content_type.svg` | `walk.svg` | `#E8735D` |
| `file_content_type.svg` | `file.svg` | `#8A939E` |

### License

Tabler Icons is MIT licensed:

```
MIT License

Copyright (c) 2020-2026 Paweł Kuna

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
```
