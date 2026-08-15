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
| **PARTIAL — planned** | **Increment 64 — Editor Tool Sessions v1** | Reusable begin/preview/commit/cancel tool interaction lifecycle for existing transform gizmos. | Behavior parity tests for gizmo interaction, snapping, history, cancellation, and authoring restrictions. |
| **PARTIAL — planned** | **Increment 68 — Project Health & Headless Automation v1** | `uve_project_check` validates registry entries, envelopes, primitive payloads, and broken references without a window. | Deterministic human/machine output, CI fixtures, and recovery-oriented diagnostics. |
| **COMPLETED** | **Increment 75 — Visual scripting graph core** | Native C++ graph model, stable node registry, typed pins/links, structural mutation guards, and deterministic validation diagnostics. | Compiler IR, versioned bytecode, VM, bindings, debugger, script state, hot reload, and managed presentation remain separate partial milestones; no C# runtime or managed bridge is implied. |
| **PARTIAL** | Visual scripting editor | Graph canvas, palette, connections, validation, source mapping, breakpoints, and watches. | A tested native compiler/VM exists first; no fake graph UI. |
| **PARTIAL** | Bytecode program reload | Validated program replacement, compatible state preservation, last-known-good fallback, and diagnostics. | Versioned bytecode/schema compatibility tests. |
| **PARTIAL** | Plugin architecture | **Increment 85 completed the static native seam:** versioned manifests, bounded capabilities, generation-checked registration scopes, and explicit close lifecycle. Dynamic loading, ABI negotiation, filesystem manifests, safe unload of plugin-owned registrations, and security policy remain partial. | A separately designed ABI/lifecycle/security proposal is still required; plugins do not become a shortcut around engine ownership. Focused Increment 85 registry suite passed 3/3. |
| **PARTIAL** | Data tables | Typed data assets, CSV/JSON/TSV import, validation, and selected editor workflow. | Deliberate parser/library choices, stable asset pipeline, and test fixtures. |
| **PARTIAL** | Developer console | Runtime commands, CVARs, history, completion, log filtering, and shipping exclusion. | Explicit command registration/security model; no cheat/debug surface silently ships. |
| **PARTIAL** | Documentation delivery | API reference, user manual, visual-script reference, plugin guide, and build/deployment guide. | Generated/manual docs must match implemented behavior, not aspirational spec text. |

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
