<div align="center">

<h1><strong>UNIVEX ENGINE — DEVELOPER TOOLING ROADMAP</strong></h1>

<strong>Visual scripting, plugins, diagnostics, content-data tools, console, CI, and engineering documentation</strong>

</div>

> Tooling exists to make verified engine systems easier to author and diagnose. It must not invent parallel runtime ownership or present controls that cannot affect real engine state.

| Status | Completed foundation | Delivered capability | Ongoing boundary |
|---|---|---|---|
| **COMPLETED** | **Increment 4** | Configuration and command-line foundation. | Future tools reuse versioned settings and explicit command-line contracts. |
| **COMPLETED** | **Increment 7** | Generic asset import/hot-reload/bundle foundation. | Specific import formats and automatic behavior remain partial. |
| **COMPLETED** | **Increment 21** | Shader authoring and hot-reload tooling foundation. | Shader diagnostics stay renderer-owned, not a generic editor-console substitute. |
| **COMPLETED** | **Increment 37** | GCC CI verification and X11/GLFW build prerequisites. | CI remains the build/test evidence baseline. |
| **COMPLETED** | **Increments 59–61** | Project content index, deterministic import jobs/cache, and explicit change-review journal. | FileSystem/Import views remain safe, read-only monitoring surfaces until deliberate workflow increments. |
| **COMPLETED** | **Increment 62** | Deterministic Inspector drawer registry. | Drawers route authored writes through validated editor commands; this is not reflection or a plugin ABI. |

<div align="center">

<h2><strong>PARTIAL — ACTIVE TOOLING SEQUENCE</strong></h2>

</div>

| Status | Milestone | Intended capability | Required proof before completion |
|---|---|---|---|
| **COMPLETED** | **Increment 75 — Visual scripting graph core** | Native C++ graph model, stable node registry, typed pins/links, structural mutation guards, and deterministic validation diagnostics. | Compiler IR, versioned bytecode, VM, bindings, debugger, script state, hot reload, and managed presentation remain separate partial milestones; no C# runtime or managed bridge is implied. |
| **COMPLETED** | **Increment 86 — Visual Scripting Editor Canvas v1** | Native bounded canvas session with node layout, ordered selection, pan/zoom view state, typed links, deterministic palette/snapshot DTOs, named bridge commands, and exact graph/layout/selection Undo/Redo. Pan/zoom is explicitly non-undoable session state. | Source mapping, watch evaluation, breakpoint UI, and durable layout persistence remain separate partial work; C# receives copied DTOs only. Focused native canvas/bridge/stdio tests and managed parser tests are required. |
| **COMPLETED** | **Increment 87 — Managed Visual Scripting Canvas Presentation v1** | Avalonia presentation control for copied nodes, links, typed-pin indicators, selection, pan/zoom, diagnostics, and named command emission through the Increment 86 bridge. | The control remains presentation-only; geometry/hit-testing/command tests, managed build/tests, Xvfb shell smoke, and full native/managed CI passed before merge. |
| **COMPLETED** | **Increment 88 — Developer Console and Diagnostics v1** | Bounded native command registry, help/clear/cvar built-ins, explicit CVAR facts, bounded output/history snapshots, named bridge requests, stdio serialization, and managed lower-dock Console presentation. | No arbitrary command scripting or filesystem/process/network/ECS/renderer/plugin mutation. Focused native/bridge/stdio/managed tests and full CI validation passed before merge. |
| **COMPLETED** | **Increment 89 — Developer Console Policy and Discovery v1** | Explicit development/shipping availability policy, severity-filtered output, deterministic prefix completion, bounded history navigation, new named bridge requests, additive JSON DTO fields, and managed discovery controls. | Shipping builds reject discovery mutations; completion/history remain native-owned and bounded. No remote console, arbitrary scripting, process execution, filesystem mutation, or cheat surface is introduced. Focused native/bridge/stdio/managed tests and full CI validation passed before merge. |
| **COMPLETED** | **Increment 172 — Motion Query Plugin Module and Manifest Adaptation v1** | Adapt `CMakeLists.txt`, `Config/PluginDescriptor.json`, `MotionQueryModule.*`, `MotionQueryDefines.*`, and version/config seams to UNIVEX plugin validation. | Native `MotionQueryPluginDescriptorUVE` and registration helpers adapt the supplied module boundary to existing `NativePluginManifestUVE`, capability policy, generation-checked scope, and close lifecycle APIs. Focused plugin tests, GCC/Clang/CI-GCC validation, managed validation, and Avalonia Xvfb smoke passed. |
| **COMPLETED** | **Increment 214 — Motion Query Native Replay Workflow Readiness Hardening v1** | Publish an authoritative read-only status for registry generation/count, active baseline/fixture state, evidence readiness, history truncation, and bounded diagnostic reason. | `EditorBridgeMotionQueryReplayWorkflowStatusUVE` is computed from native lifecycle facts at snapshot time; reports ready only when a named copied fixture and complete live-debug evidence are available. Native bridge/stdio workflow coverage passed; full CTest passed. |
| **COMPLETED** | **Increment 215 — Native Bridge Publication of Bounded All-Baseline Regression Results v1** | Add native bridge publication of batch regression results through stdio and managed DTOs. | `RunMotionQueryReplayBaselineBatch` request kind and additive batch-result wire fields enable managed presentation of per-baseline outcomes. Native bridge/stdio JSON serialization and request mapping validated; focused bridge/batch tests passed; full CTest passed. |
| **COMPLETED** | **Reorganization Increment — Animation Plugin Category v1** | Consolidate animation-related plugins (`motion_query`, `control_rig`) into a categorized `engine/plugins/Animation` directory structure. | Redundant core files removed; include paths updated to `uve/plugins/`; CMake configurations adjusted; contract inventory synchronized; and full 1029/1029 test matrix passed. |
| **COMPLETED** | **Increment 216 — Motion Query Bounded Replay Batch History and Session Facts v1** | Record bounded batch regression history and cumulative session facts in the native bridge. | `replayBatchHistory` ring buffer and `replaySessionFacts` published through stdio; native bridge tests passed; full CTest passed 1029/1029. |
| **COMPLETED** | **Increment 217 — Managed Bridge Parsing of Bounded Replay Batch History and Session Facts v1** | Parse additive batch-result, batch-history, and session-facts DTOs in the managed bridge client. | `BridgeMotionQueryReplayBatchSnapshot`, `BridgeMotionQueryReplayBatchHistoryEntry`, and `BridgeMotionQueryReplaySessionFacts` parsed from copied stdio JSON; managed parser tests passed. |
| **COMPLETED** | **Increment 218 — Managed Presentation of Replay Batch History and Session Facts v1** | Display bounded batch regression history and cumulative session facts in the Avalonia Animator panel. | Animator panel renders `ReplayBatchHistory` and `ReplaySessionFacts` from the copied DTO; managed UI tests passed. |
| **COMPLETED** | **Increment 219 — Managed Presentation of Motion Query Replay Comparison History v1** | Display bounded individual comparison history in the Avalonia Animator panel. | Animator panel renders `ReplayComparisonHistory` from the copied diagnostics view; managed UI tests passed. |
| **COMPLETED** | **Increment 220 — Managed Motion Query Replay Baseline Export/Import v1** | Expose native registry export/import commands through the managed bridge and Avalonia UI. | Deterministic envelope serialization/deserialization via bridge requests; managed UI buttons for export/import; regression tests passed. |
| **COMPLETED** | **Increment 221 — Motion Query Replay Baseline Selection and Removal UI v1** | Add Load and Remove buttons to the managed baseline list. | Per-baseline Load/Remove buttons in Avalonia; bridge command routing for named baseline removal; managed UI tests passed. |
| **COMPLETED** | **Increment 222 — Motion Query Replay Baseline Rename Support v1** | Implement baseline renaming in the native registry, bridge, and managed UI. | `RenameUVE` in baseline registry; `RenameMotionQueryReplayBaseline` bridge command; managed UI rename controls; managed tests passed. |
| **COMPLETED** | **Increment 223 — Motion Query Live Debug Trace Event Selection v1** | Add trace event selection and inspection to the managed UI and native debugger. | `SelectEvent` live-debug command; `InspectEventUVE` in native debugger; managed Trace list with selection sync; managed tests passed. |
| **COMPLETED** | **Increment 224 — Motion Query Live Debug Trace Event Removal v1** | Implement individual trace event removal in the native logger, bridge, and managed UI. | `RemoveEventUVE` in trace logger; `RemoveEvent` live-debug command; managed UI per-event Remove buttons; managed tests passed. |
| **COMPLETED** | **Increment 225 — Motion Query Replay Baseline Evidence Export v1** | Add a way to export the current live debug trace as a replay fixture. | `ExportMotionQueryReplayEvidence` bridge command; `BuildMotionQueryTraceReplayFixtureUVE` usage; managed UI button for evidence export; managed tests passed. |

<div align="center">

<h2><strong>TOOLING DESIGN RULES</strong></h2>

</div>

| Rule | Requirement |
|---|---|
| **Command routing** | Editor tools mutate documents only through validated command paths with dirty-state/history/Play-mode semantics. |
| **No automatic destructive work** | Change detection, index refresh, import, and source mutation stay explicit until their safety model is implemented. |
| **Plugin discipline** | Runtime extension must be versioned and scoped; a generic callback registry is not a secure plugin system. |
| **Headless validation** | Project/build diagnostics must execute without a graphics window for CI usefulness. |
| **Native scripting direction** | Visual scripting remains native C++ graph/compiler/bytecode/VM work; C# is not introduced as a replacement system without a separately approved interoperability architecture. |
