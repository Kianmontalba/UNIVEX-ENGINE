<div align="center">

<h1><strong>UNIVEX ENGINE — MULTI-LANGUAGE EDITOR ROADMAP</strong></h1>

<strong>C++20 backend, future C# experience layer, and GLSL viewport visuals</strong><br/>
<strong>Last updated: August 2026</strong>

</div>

> **Status:** **Increment 69 — COMPLETED.** The owner-approved architecture keeps C++20 authoritative for the engine/editor backend, reserves C#/.NET for a future editor experience layer, and reserves GLSL for viewport visuals. Increments 70–81 remain **PARTIAL** until separately implemented and verified. Increment 69 itself adds no managed runtime, C# host, or external transport.

## Decision

The future UniVex Editor will no longer be planned as a **C++-only editor surface**. It will use a deliberate three-layer architecture:

| Layer | Primary language | Owns | Must not own |
|---|---|---|---|
| **Engine and Editor Backend** | **C++20** | ECS, scene/asset ownership, command history, Play Mode, renderer/RHI, physics/audio/input, project index, serialization, validation, undo/redo, headless tools | Managed UI lifetime, arbitrary editor mutations, view-only layout state |
| **Editor Experience Layer** | **C# / .NET** | Docking shell, panels, menus, Inspector presentation, Hierarchy/Content Browser interaction, graph/timeline views, settings screens, tooling workflows, accessibility, rich editor UX | Direct pointers to engine memory, direct ECS edits, raw serialization writes, GL resource ownership |
| **Viewport Visual Layer** | **GLSL** | Grid, camera orientation widget, selection outline, gizmo colors/handles, overlays, object-ID picking passes, debug visualization, editor-only GPU effects | Scene ownership, business rules, asset mutation, UI docking state |

> **Core safety rule:** C# talks to the C++ backend only through an explicit versioned editor bridge. It never receives raw C++ pointers, never owns ECS objects, and never writes scene files directly. All authored changes still enter the C++ command layer, preserving validation, history, dirty state, Play Mode guards, and deterministic tests.

## Why This Is Safer Than “Just Add C#”

A direct mixed C++/C# editor without a bridge would make crashes, ABI changes, threading, GPU context ownership, and Undo/Redo behavior hard to debug. The bridge makes language boundaries observable and testable.

| Concern | Proposed rule |
|---|---|
| **Commands** | C# sends typed command requests; C++ validates and returns immutable result/diagnostic DTOs. |
| **State** | C# consumes copied snapshots/deltas with revision numbers. C++ remains authoritative. |
| **Threading** | Bridge requests have explicit UI/backend thread ownership; no managed callback may mutate engine state re-entrantly. |
| **Rendering** | C++ owns OpenGL and GLSL resources. Viewport embedding is added only after an explicit native-surface protocol is validated per platform. |
| **Errors** | All bridge responses contain stable code, severity, user message, and recovery detail. |
| **Compatibility** | Protocol version negotiation prevents a newer C# host from silently misreading an older C++ backend. |
| **Shipping** | C# editor host is a development tool. The game/runtime executable stays C++ native and does not require .NET. |

## Revised Immediate Sequence

The new order starts by creating a safe multi-language architecture **before** migrating editor screens. Native Visual Scripting moves later in the sequence but remains C++ from graph model through VM.

| Status | Increment | Title | Main languages | Concrete outcome |
|---|---:|---|---|---|
| **COMPLETED** | **69** | **Editor Bridge Contract v1** | C++ | Versioned editor command/state DTOs, main-thread backend bridge service, capability discovery, revisioned copied snapshots, typed diagnostics, stale mutation protection, and headless protocol tests. No C# host or external transport yet. |
| **PARTIAL** | **70** | **C# Editor Host Foundation v1** | C++, C# | Optional .NET 8 C# editor-host project, local bridge client, connection/error screen, backend capability display, protocol compatibility checks, and process lifecycle policy. No direct engine pointers. |
| **PARTIAL** | **71** | **C# Editor Shell & Docking v1** | C#, C++ | Cross-platform C# desktop shell with original UniVex layout: menu, dock regions, workspace tabs, persistent presentation layout, and backend-provided panel visibility commands. Existing C++ editor remains fallback. |
| **PARTIAL** | **72** | **C# Hierarchy, Inspector & Content Browser v1** | C#, C++ | C# panels consume immutable backend snapshots; selection and edits invoke C++ commands. Hierarchy context, Inspector drawers, breadcrumbs, filters, and asset records remain C++ authoritative. |
| **PARTIAL** | **73** | **GLSL Viewport Visuals v1** | C++, GLSL | Editor grid/background contract, top-right X/Y/Z orientation widget, axis highlights, selection outline, gizmo visual states, color/accessibility constants, and GPU-visible regression tests. |
| **PARTIAL** | **74** | **Native Viewport Surface Bridge v1** | C++, C#, GLSL | Platform-scoped viewport-surface integration with explicit focus/input/resize lifecycle. C++ still owns OpenGL; C# only hosts the surface. Start with Linux/X11 evidence, then define Windows policy. |
| **PARTIAL** | **75** | **C# Scene Authoring Workflow v1** | C++, C# | Command-backed creation, selection, rename, reparent, transform controls, safe undo/redo feedback, Play Mode restrictions, and clear diagnostic/toast workflow. |
| **PARTIAL** | **76** | **C# Content and Import Review v1** | C++, C# | Content Browser navigation, asset metadata, import/change-review journal, project-health report viewer, and non-destructive recovery actions. |
| **PARTIAL** | **77** | **GLSL Picking, Overlays & Debug Views v1** | C++, GLSL | Object-ID picking pass, editor overlays, bounds/collision/light visualizers, debug mode contract, and measured render-cost diagnostics. |
| **PARTIAL** | **78** | **Native Visual Scripting Graph Foundation v1** | C++ | Typed graph, node registry, pins, links, validation, stable `.uvescript` serialization, and Script asset integration. No C# runtime. |
| **PARTIAL** | **79** | **C# Scripting Workspace & Graph Canvas v1** | C++, C# | C# graph canvas consumes C++ graph snapshots and issues typed C++ graph commands; node palette, connection feedback, layout metadata, and diagnostics. |
| **PARTIAL** | **80** | **Visual Script Compiler, Bytecode & VM v1** | C++ | Native compiler IR, verified bytecode, bounded VM, source-node mapping, and selected explicit engine bindings. |
| **PARTIAL** | **81** | **C# Script Debugger & Program Reload v1** | C++, C# | Breakpoints, compile diagnostics, watches, execution controls, last-known-good reload, and managed presentation over native runtime data. |

## Editor Screen Ownership After the Transition

| Editor area | C++ backend | C# experience layer | GLSL viewport layer |
|---|---|---|---|
| **Main menu / workspace** | Commands, capability flags, settings persistence | Menus, tabs, docking, keyboard UX | Not applicable |
| **Hierarchy** | Entity tree snapshots; create/rename/reparent commands | Tree view, search, context menus, breadcrumbs | Selection visualization only |
| **Inspector** | Drawer registry, property schema, validation, mutation commands | Forms, sections, tooltips, error presentation | Transform/gizmo visual feedback |
| **Content Browser** | Index, registry correlation, health/import/change facts | Navigation, list/grid presentation, filters, review screens | Thumbnails only after a separate renderer/caching contract |
| **Viewport** | Camera, selection, picking result, renderer, input ownership | Host shell, focus affordance, toolbar presentation | Grid, axes, gizmos, outlines, overlays, visual debugging |
| **Visual scripting** | Graph, compiler, bytecode, VM, bindings, debug snapshots | Graph canvas, palette, timeline, debugger UI | Optional node-preview/render diagnostic shaders later |

## Required Technology Decisions Before Increment 70

| Decision | Recommended v1 choice | Reason |
|---|---|---|
| **Managed UI runtime** | .NET 8 LTS | Modern C# toolchain with a supported long-term runtime. |
| **Cross-platform C# UI framework** | Avalonia UI | Desktop Linux/Windows orientation and dockable-editor potential; chosen only after a small proof-of-build spike. |
| **Bridge transport** | Local framed JSON-RPC over stdio for v1 | Observable, testable, process-isolated, and avoids exposing ABI/pointers across language boundaries. |
| **Data schema** | Versioned JSON DTOs with generated/validated C++ and C# models | Human-debuggable protocol plus deterministic fixture tests. |
| **Viewport embed** | Deferred until Increment 74 | Prevents an unstable OpenGL/context bridge from blocking normal UI workflow. |

> A proof-of-build must validate .NET 8, Avalonia package restore, Linux desktop startup, and C++ bridge communication before an approved C# host increment claims cross-platform editor support.

## Non-Negotiable Boundaries

| Never allowed | Why |
|---|---|
| C# direct writes to `.uvescene`, `.uveassetdb`, or `.uvescript` | Bypasses C++ validation, history, Play Mode, and recovery policy. |
| C# direct pointers/references to ECS or renderer objects | Creates lifetime/ABI/threading failures. |
| C# ownership of OpenGL resources | Breaks current native renderer lifecycle and future RHI portability. |
| Shader code deciding editor commands or asset semantics | GPU code must remain visual/picking behavior only. |
| A C# runtime dependency in game builds | The editor toolchain must not inflate or destabilize native shipped games. |
| Replacing C++ visual-scripting compiler/VM with C# | Visual scripting remains a native C++ runtime system; C# is the authoring/debug UX layer. |

## Validation Gates for Every Multi-Language Increment

| Gate | Required proof |
|---|---|
| **Protocol contract** | C++/C# fixture parity, version mismatch diagnostics, deterministic DTO ordering, and malformed-request isolation. |
| **Command safety** | Every C#-originated mutation proves C++ command/history/dirty/Play-mode parity. |
| **Process stability** | Managed host crash/restart cannot corrupt C++ project/scene state. |
| **Rendering stability** | Native viewport context remains C++ owned; resize/focus/input lifecycle is tested per supported platform. |
| **Build matrix** | Existing GCC/Clang/CI continues; C# host adds a separate `dotnet test`/restore/build gate only after its approved introduction. |
| **No mock claims** | A panel is shown as available only if its backend capability exists and has real integration tests. |

## What This Means for the Next Approval

**Increment 69 — Editor Bridge Contract v1 is complete.** It establishes the authority, copied DTO, revision, capability, diagnostic, and native-command-routing foundation required for a professional editor designed with C++, C#, and GLSL together.

Increment 70 can now evaluate the C# host safely; Increment 73 remains the planned GLSL viewport-visuals foundation. Neither step may weaken C++ ownership or bypass the bridge contract.
