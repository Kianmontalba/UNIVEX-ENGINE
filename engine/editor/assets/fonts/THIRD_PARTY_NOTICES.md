# Third-Party Notices — `engine/editor/assets/fonts`

## Tabler Icons (subsetted)

`tabler-icons-subset.ttf` is a subset of the Tabler Icons webfont, built from
the `@tabler/icons-webfont` npm package (version `3.46.0`,
`https://registry.npmjs.org/@tabler/icons-webfont/-/icons-webfont-3.46.0.tgz`).
Upstream project: <https://github.com/tabler/tabler-icons>.

Only the 13 glyphs the editor chrome actually uses are kept, to avoid
vendoring a multi-megabyte font for a dozen-odd icons. The subset was produced
with `fonttools`'s `pyftsubset`:

```
pyftsubset tabler-icons.ttf \
    --output-file=tabler-icons-subset.ttf \
    --unicodes=EA03,EA45,EA54,EA98,EAA4,EAAD,EB2E,EBD9,EDBA,F91D,FA97,FAF7,FAFA \
    --glyph-names --layout-features='*' --no-hinting --desubroutinize
```

`tools/generate_icon_font_bytes.py` compiles this file into
`engine/editor/src/uve_icon_font_bytes.inc`, which is what the editor
actually links against; the `.ttf` here is the checked-in source asset the
generator reads, following the same source-asset-plus-generator convention
already used for `engine/editor/assets/gizmos` and `engine/editor/assets/icons`.

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
