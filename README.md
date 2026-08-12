# UNIVEX-ENGINE

A next-generation 3D game engine focused on modular architecture, real-time camera behavior, and streamlined deployment for interactive experiences.

## Status

**Increment 1: Foundation Layer** — a headless, fully unit-tested core consisting of
`LoggerUVE`, `TimerUVE`, `EventSystemUVE`, and `EngineCoreUVE` (the `Init → Load →
BeginFrame → Update → LateUpdate → Render → EndFrame → Shutdown` pipeline). No windowing or
rendering yet — see `docs/MASTER_SPEC.md` for the full engine vision and `docs/
CODING_STANDARDS.md` for the conventions this codebase follows.

## Building & Testing (Linux)

Requires CMake ≥ 3.24 and a C++20 compiler (GCC ≥ 11 or Clang ≥ 14; verified with GCC 13 and
Clang 18).

```bash
# Configure + build (Debug, GCC)
cmake -S . -B build/gcc-debug -DCMAKE_CXX_COMPILER=g++
cmake --build build/gcc-debug -j"$(nproc)"

# Run the unit test suite
ctest --test-dir build/gcc-debug --output-on-failure

# Run the headless proof-of-life executable
./build/gcc-debug/engine/app/uve_runtime --headless

# Run Editor Foundation v1 without a display (CI smoke test)
./build/gcc-debug/engine/app/uve_editor --headless --frames 2

# Run the desktop editor with an existing scene document
./build/gcc-debug/engine/app/uve_editor --scene path/to/level.uvescene

# Virtual-display smoke test on a platform capped at OpenGL 4.5
xvfb-run -a ./build/gcc-debug/engine/app/uve_editor --gl-version 4.5 --frames 3
```

Swap `-DCMAKE_CXX_COMPILER=clang++` to build with Clang instead. `UVE_BUILD_TESTS` (default
`ON`) can be set to `OFF` to skip building the GoogleTest suite.

## Repository layout

```
engine/
├── platform/   — UVE::Platform  — compiler/OS macros
├── debug/      — UVE::Debug     — LoggerUVE, log sinks, UVE_ASSERT, logging macros
├── utilities/  — UVE::Utilities — TimerUVE
├── events/     — UVE::Events    — EventSystemUVE
├── core/       — UVE::Core      — EngineCoreUVE and supporting types
├── editor/     — UVE::Editor    — Editor Foundation v1 session and UI composition
└── app/        — uve_runtime and the standalone uve_editor executables
tests/          — GoogleTest suite, mirrors the engine/ layout
docs/           — MASTER_SPEC.md (full design doc), CODING_STANDARDS.md
```

See `CONTRIBUTING.md` before making changes.
