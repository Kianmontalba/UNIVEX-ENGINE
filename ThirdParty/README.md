# UniVex Third-Party Integration Boundary

This directory is the only repository-owned boundary for third-party source or generated integration metadata. Native UniVex modules own the public API, lifecycle, memory ownership, threading model, error handling, and serialization rules. Third-party implementations may sit behind an adapter, but they must not leak third-party types into public UVE headers or engine-wide service contracts.

## Required integration sequence

Every dependency must have an entry in `manifest.json` and `../THIRD_PARTY_NOTICES.md` before source or binaries are added. The entry records the exact repository, commit, license, nested dependencies, selected components, build options, modifications, platform coverage, and distribution obligations.

A dependency is not considered integrated merely because its repository was cloned or its headers are present. It must be built through the owning CMake target, connected through a native UVE adapter, covered by integration and regression tests, and verified on the supported build matrix.

## Intended layout

```text
ThirdParty/
├── Jolt/
├── ImGuIZMO/
├── Spartan/
└── Stride/
```

The current increment records the audit and reserves these boundaries. It intentionally does not vendor or link the full SpartanEngine or Stride repositories: their nested dependency trees include separately licensed components, including restricted SDK terms and copyleft components. Only a component-level integration with a complete license review may move those entries from audit-only to integrated.

## Ownership rule

`Native/` and `engine/` remain the UniVex architecture. A third-party adapter must be replaceable without changing game-facing UVE APIs. No third-party dependency may create hidden ECS entities, authored scene content, runtime lights, serialized data, or editor state outside its documented owner.
