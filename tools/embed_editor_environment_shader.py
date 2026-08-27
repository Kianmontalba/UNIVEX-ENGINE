#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
shader_path = root / "engine/render/shader/built_in/editor_viewport_environment.glsl"
source_path = root / "engine/render/shader/src/built_in_shaders_uve.cpp"
shader = shader_path.read_text(encoding="utf-8")
source = source_path.read_text(encoding="utf-8")
start_marker = "const std::string_view kEditorViewportEnvironmentSource = R\"GLSLSRC("
end_marker = ")GLSLSRC\";\n"
if start_marker in source:
    start = source.index(start_marker)
    end = source.index(end_marker, start) + len(end_marker)
    replacement = f'{start_marker}{shader}{end_marker}'
    source = source[:start] + replacement + source[end:]
else:
    insertion_marker = "const std::string_view kParticleSource = R\"GLSLSRC("
    insertion = f'const std::string_view kEditorViewportEnvironmentSource = R\"GLSLSRC({shader}{end_marker}\n'
    if insertion_marker not in source:
        raise SystemExit("particle embedded source marker not found")
    source = source.replace(insertion_marker, insertion + insertion_marker, 1)
source_path.write_text(source, encoding="utf-8")
