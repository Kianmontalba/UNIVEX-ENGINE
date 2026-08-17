from __future__ import annotations

import pathlib
import re
from collections import Counter

DOCX_TEXT = pathlib.Path('/home/ubuntu/upload/6e26b79ebb435f55d5e84cb5c2d4c58b_extracted.txt')
REPO_ROOT = pathlib.Path('/home/ubuntu/UNIVEX-ENGINE')
OUTPUT = REPO_ROOT / 'docs' / 'MOTION_QUERY_INVENTORY_ROADMAP.md'

SECTIONS = {
    'Build / Configuration': ('Build / Configuration', None),
    'Editor — Private': ('Editor — Private', 'Editor'),
    'Editor — Public': ('Editor — Public', 'Editor'),
    'Runtime — Private': ('Runtime — Private', 'Runtime'),
    'Runtime — Public': ('Runtime — Public', 'Runtime'),
}
FILE_RE = re.compile(r'^[A-Za-z0-9_]+\.(?:cpp|h|inl|json)$')
SECTION_RE = re.compile(r'^(Build / Configuration|Editor — Private|Editor — Public|Runtime — Private|Runtime — Public)$')


def snake_case(value: str) -> str:
    value = re.sub(r'([a-z0-9])([A-Z])', r'\1_\2', value)
    value = re.sub(r'([A-Z]+)([A-Z][a-z])', r'\1_\2', value)
    value = value.replace('-', '_').replace(' ', '_')
    value = re.sub(r'[^A-Za-z0-9_]', '_', value)
    return re.sub(r'_+', '_', value).strip('_').lower()


def target_for(section: str, original: str) -> tuple[str, str]:
    if original == 'CMakeLists.txt':
        return 'engine/plugins/Animation/motion_query/CMakeLists.txt', 'CMakeLists.txt'
    if original == 'Config/PluginDescriptor.json':
        return 'engine/plugins/Animation/motion_query/Config/PluginDescriptor.json', 'PluginDescriptor.json'
    stem, extension = original.rsplit('.', 1)
    normalized = f'{snake_case(stem)}_uve.{extension.lower()}'
    if section == 'Editor — Private':
        root = 'engine/plugins/Animation/motion_query/Source/Editor/src'
    elif section == 'Editor — Public':
        root = 'engine/plugins/Animation/motion_query/Source/Editor/include/uve/plugins'
    elif section == 'Runtime — Private':
        root = 'engine/plugins/Animation/motion_query/Source/Runtime/src'
    else:
        root = 'engine/plugins/Animation/motion_query/Source/Runtime/include/uve/plugins'
    if extension.lower() == 'inl':
        root = root.replace('/src', '/include/uve/plugins')
    return f'{root}/{normalized}', normalized


def canonical_tokens(value: str) -> set[str]:
    tokens = set(snake_case(value).split('_'))
    aliases = {
        'debugger': 'debug', 'debugging': 'debug', 'trace': 'trace',
        'traces': 'trace', 'channels': 'channel', 'nodes': 'node',
        'libraries': 'library', 'definitions': 'definition',
    }
    return {aliases.get(token, token) for token in tokens if token not in {'uve', 'source', 'private', 'public'}}


def existing_records() -> list[tuple[pathlib.Path, set[str]]]:
    records: list[tuple[pathlib.Path, set[str]]] = []
    for path in (REPO_ROOT / 'engine/plugins/Animation').rglob('*'):
        if path.suffix not in {'.h', '.cpp', '.inl', '.json'}:
            continue
        records.append((path, canonical_tokens(path.stem)))
    return records


def classify(section: str, original: str, target_path: str, target_name: str, existing: list[tuple[pathlib.Path, set[str]]]) -> tuple[str, str]:
    target = REPO_ROOT / target_path
    if target.exists():
        return 'COMPLETED', 'Existing UVE authority; retain and verify against the shared contract.'
    if original in {'CMakeLists.txt', 'Config/PluginDescriptor.json'}:
        return 'PARTIAL', 'Adapt the existing plugin build/manifest seam; do not introduce a second module system.'
    source_tokens = canonical_tokens(original.rsplit('.', 1)[0])
    candidates = []
    for path, tokens in existing:
        overlap = len(source_tokens & tokens)
        if overlap >= 2 and 'motion' in source_tokens and 'query' in source_tokens:
            candidates.append((overlap, path))
    if candidates:
        candidates.sort(key=lambda item: (-item[0], str(item[1])))
        authority = candidates[0][1].relative_to(REPO_ROOT).as_posix()
        return 'PARTIAL', f'Merge or extend the existing UVE authority at `{authority}`; no duplicate subsystem.'
    return 'PLANNED', 'Rewrite as a bounded UVE-native contract after dependencies and ownership are approved.'


def load_inventory() -> list[tuple[str, str]]:
    rows: list[tuple[str, str]] = []
    current: str | None = None
    started = False
    for raw in DOCX_TEXT.read_text(encoding='utf-8-sig').splitlines():
        line = raw.strip()
        if 'Motion Query / Motion Matching Integration' in line:
            started = True
            continue
        if not started:
            continue
        if line == 'Integration rule':
            break
        match = SECTION_RE.match(line)
        if match:
            current = match.group(1)
            continue
        if current and FILE_RE.match(line):
            rows.append((current, line))
    return rows


def main() -> None:
    inventory = load_inventory()
    existing = existing_records()
    statuses = Counter()
    lines = [
        '# UNIVEX Engine — Motion Query / Motion Matching Inventory Roadmap',
        '',
        '> This roadmap preserves every filename from the supplied inventory while translating each target into the UNIVEX-native `*_uve.h` / `*_uve.cpp` contract style. The original names are planning inputs, not code-copy instructions.',
        '',
        '## Integration rules',
        '',
        'Every inventory item is accounted for. Existing UNIVEX authorities are extended rather than duplicated. Unreal-specific reflection, tracing, asset, editor, or runtime assumptions must be rewritten behind C++20/UVE ownership boundaries. Each item is complete only after runtime behavior, editor authoring where applicable, diagnostics/profiling, and automated validation are present.',
        '',
        'The target root is `engine/plugins/Animation/motion_query/`; `control_rig` remains a sibling plugin under `engine/plugins/Animation/` and shares the engine pose/evaluation contracts. JSON and build files retain their configuration role and are not renamed as C++ symbols.',
        '',
        '| Original inventory count | Completed/existing UVE authority | Partial/merge into UVE authority | Planned UVE-native adaptation |',
        '|---:|---:|---:|---:|',
    ]
    classified: list[tuple[str, str, str, str, str, str]] = []
    for section, original in inventory:
        target_path, target_name = target_for(section, original)
        status, disposition = classify(section, original, target_path, target_name, existing)
        statuses[status] += 1
        classified.append((section, original, target_name, target_path, status, disposition))
    lines.append(f'| {len(inventory)} | {statuses["COMPLETED"]} | {statuses["PARTIAL"]} | {statuses["PLANNED"]} |')
    lines.append('')
    lines.append('## Dependency-ordered implementation phases')
    lines.extend([
        '',
        '1. **Foundation and configuration:** plugin manifest/build seams, shared reflection/type metadata, resource handles, serialization/versioning, asset dependency and derived-data contracts.',
        '2. **Runtime Motion Query:** database/schema/settings/context/result/event contracts, feature channels, sampling, history, trajectory, search, interaction, and animation-node integration.',
        '3. **Editor authoring:** database editor, asset browser/tree, details, viewport, factories, clipboard, chooser columns, and editor view models using copied DTOs.',
        '4. **Diagnostics and trace tooling:** debugger, trace provider/analyzer, bounded trace logging, replay evidence, persistence, and managed presentation through the existing bridge.',
        '5. **Validation and disposition closure:** deterministic fixtures, import/export compatibility, performance/soak tests, and documented merged/replaced/rejected reasons for every inventory item.',
        '',
    ])
    for section in SECTIONS:
        rows = [row for row in classified if row[0] == section]
        if not rows:
            continue
        lines.append(f'## {section}')
        lines.append('')
        lines.append('| Original filename | UVE-native target filename | UVE-native target path | Status | Disposition / next proof |')
        lines.append('|---|---|---|---|---|')
        for _, original, target_name, target_path, status, disposition in rows:
            lines.append(f'| `{original}` | `{target_name}` | `{target_path}` | **{status}** | {disposition} |')
        lines.append('')
    lines.extend([
        '## Completion gate',
        '',
        'The inventory is not considered complete because filenames have been normalized. Completion requires evidence that the implementation is the authoritative UNIVEX path, has no duplicate owner, obeys bounded value-oriented contracts, is exposed through the correct editor boundary where applicable, and is covered by native/managed tests and real runtime/editor verification.',
        '',
        'The next implementation plan must select only from the **PARTIAL** or **PLANNED** rows after the existing repository partial queue is reviewed. This inventory roadmap does not authorize speculative bulk generation of 198 source files.',
        '',
    ])
    OUTPUT.write_text('\n'.join(lines) + '\n', encoding='utf-8')
    print(f'generated {OUTPUT} with {len(inventory)} inventory rows: {dict(statuses)}')


if __name__ == '__main__':
    main()
