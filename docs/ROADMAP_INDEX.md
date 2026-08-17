<div align="center">

<h1><strong>UNIVEX ENGINE — ROADMAP INDEX</strong></h1>

<strong>One entry point for every active engineering roadmap</strong><br/>
<strong>Last updated: August 2026</strong>

</div>

> **Status language:** `COMPLETED` records a delivered milestone. `PARTIAL` records planned or incomplete work. A roadmap entry is never promoted to `COMPLETED` without real implementation and the verification evidence named in its own document.

| Status | Roadmap | Scope | Current priority |
|---|---|---|---|
| **COMPLETED / PARTIAL** | [Complete Development Roadmap](EDITOR_ROADMAP.md) | Every completed Increment **1–74**, immediate Scene Editor increments, and the first-generation editor completion gate. | Plan Increment 75 while retaining native C++ renderer/OpenGL ownership and the managed host's explicitly deferred viewport surface boundary; Increment 74 surface lifecycle is completed as an explicit typed unavailable/native-owned contract. |
| **COMPLETED / PARTIAL** | [Core & Runtime Roadmap](CORE_RUNTIME_ROADMAP.md) | Engine loop, memory, threading, ECS, scene, asset, physics, input, audio, save, and platform foundations. | Preserve existing contracts while filling only proven core-runtime gaps. |
| **COMPLETED / PARTIAL** | [Rendering Roadmap](RENDERING_ROADMAP.md) | RHI, renderer, shaders, materials, lights, shadows, viewport presentation, and future visual systems. | Visible viewport correctness before broader visual features. |
| **COMPLETED / PARTIAL** | [Editor & Workflow Roadmap](EDITOR_ROADMAP.md) | Scene authoring, selection, gizmos, Inspector, FileSystem, import monitoring, preferences, versioned bridge transport, managed host foundation, fixed C# shell layout, and bounded snapshot-backed C# panel presentation. | Preserve copied DTO, named native-command, revision, bounded/truncation, request-serialization, explicit-save-only layout, and headless-only stdio rules; keep managed GL hosting and broader authoring/import workflows deferred until their explicit increments. |
| **COMPLETED / PARTIAL** | [Developer Tooling Roadmap](TOOLING_ROADMAP.md) | Visual scripting, plugins, project diagnostics, data tooling, console, CI, and developer documentation. | Project health and stable editor command/property contracts. |
| **PARTIAL** | [Motion Query Inventory Roadmap](MOTION_QUERY_INVENTORY_ROADMAP.md) | Complete DOCX-derived Motion Query/Motion Matching filename inventory translated to UVE-native target paths, dispositions, and dependency-ordered implementation phases. | Account for every source filename without blind copying or duplicate authorities; complete runtime, editor, diagnostics, profiling, and validation evidence before promotion. |
| **COMPLETED / PARTIAL** | [Gameplay & Content Roadmap](GAMEPLAY_CONTENT_ROADMAP.md) | Nodes/components, animation, particles, splines, procedural tools, sequencer, and gameplay-facing content systems. | Start only after stable core/editor/tooling entry conditions. |
| **COMPLETED / PARTIAL** | [Platform & Release Roadmap](PLATFORM_RELEASE_ROADMAP.md) | Build matrix, platform abstraction, cooking, packaging, sample project, release documentation, and shipping path. | Maintain GCC/Clang CI; defer shipping expansion until asset/project-health foundations exist. |
| **PARTIAL** | [Ecosystem Roadmap](ECOSYSTEM_ROADMAP.md) | UniVex Hub, public site/news, account service, backend, and community-facing delivery work. | Remains separate from core-engine execution. |

<div align="center">

<h2><strong>ROADMAP GOVERNANCE</strong></h2>

</div>

| Rule | Requirement |
|---|---|
| **One source per domain** | Each roadmap owns its domain-specific sequence; this index is navigation and high-level status only. |
| **No duplicate architecture** | Roadmap work must extend established scene, renderer, asset, command, and service boundaries rather than creating parallel systems. |
| **Evidence before claims** | Progress reports use **[DESIGNED]**, **[IMPLEMENTED]**, **[COMPILED]**, **[TESTED]**, **[PROFILED]**, and **[HARDENED]** accurately. |
| **Small reviewable increments** | Every implementation increment includes source, headers, tests, documentation, cross-compiler validation, and editor smoke coverage when user-visible behavior changes. |
| **Future scope stays visible** | `PARTIAL` items remain visible but cannot displace the active critical path without an explicit owner decision. |

> The full original vision remains in [`MASTER_SPEC.md`](MASTER_SPEC.md). This index and its focused roadmaps are the execution-oriented planning layer; they use clean Markdown headings and tables instead of legacy separator-line formatting, and avoid turning one large vision document into a daily task checklist.
