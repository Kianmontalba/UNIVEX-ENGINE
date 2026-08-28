#!/usr/bin/env python3
from html.parser import HTMLParser
from pathlib import Path

class Validator(HTMLParser):
    def __init__(self):
        super().__init__()
        self.stack = []
        self.tags = []
        self.errors = []

    def handle_starttag(self, tag, attrs):
        self.tags.append(tag)
        if tag not in {"meta", "link", "img", "input", "br", "hr", "source", "area", "base", "embed", "param", "track", "wbr"}:
            self.stack.append(tag)

    def handle_startendtag(self, tag, attrs):
        self.tags.append(tag)

    def handle_endtag(self, tag):
        if tag in {"meta", "link", "img", "input", "br", "hr", "source", "area", "base", "embed", "param", "track", "wbr"}:
            self.errors.append(f"unexpected closing tag </{tag}>")
            return
        if not self.stack or self.stack[-1] != tag:
            self.errors.append(f"unbalanced closing tag </{tag}>")
            return
        self.stack.pop()

for path in sorted(Path("engine/editor/assets/gizmos/html").glob("*.html")):
    parser = Validator()
    parser.feed(path.read_text(encoding="utf-8"))
    parser.close()
    if parser.stack:
        parser.errors.append(f"unclosed tags: {parser.stack}")
    required = {"html", "head", "body", "script"}
    if path.name != "gizmo_toolbar.html":
        required.add("canvas")
    missing = sorted(required - set(parser.tags))
    parser.errors.extend(f"missing <{tag}>" for tag in missing)
    if parser.errors:
        raise SystemExit(f"{path}: " + "; ".join(parser.errors))
    print(f"OK {path}")
