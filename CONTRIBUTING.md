# Contributing to UniVex Engine

Read `docs/CODING_STANDARDS.md` before writing any code — it covers namespace/folder
organization, naming conventions, the required copyright header, documentation and
thread-safety rules, and error-handling conventions. `docs/MASTER_SPEC.md` has the full
engine vision if you need broader context on where a system fits long-term.

## Build & test

```bash
cmake -S . -B build/gcc-debug -DCMAKE_CXX_COMPILER=g++
cmake --build build/gcc-debug -j"$(nproc)"
ctest --test-dir build/gcc-debug --output-on-failure
```

Also verify with Clang (`-DCMAKE_CXX_COMPILER=clang++`) before submitting changes — both
compilers must build warnings-clean (`-Werror` is on).

## Expectations

- No stub functions, no TODOs, no half-implemented classes — every change delivered must
  compile, run, and be covered by tests.
- Every public API needs an `///` doc comment; every class documents its thread-safety
  contract.
- No raw `new`/`delete` — RAII and smart pointers only.
