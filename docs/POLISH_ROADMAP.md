<div align="center">

# UNIVEX ENGINE — PRODUCTION POLISH ROADMAP

<strong>Ten heavy, dependency-ordered polish increments</strong><br/>
<strong>Polish 10 = Super-Optimized Polish</strong><br/>
<strong>Planning baseline: August 25, 2026</strong>

</div>

> This roadmap begins after the current engine completion boundary. Every level is a substantial production increment, not a cosmetic pass or a file-by-file cleanup. Each increment requires a whole-family audit, a coherent implementation batch, focused regression tests, full native and managed validation, visual or performance evidence where relevant, signed history, hosted CI, and a final diff review.

## 1. Purpose and polish standard

The engine completion queue is closed at the current supported boundaries. Polish mode is therefore about turning verified foundations into a cohesive, reliable, visually credible production tool. The first visible priority is the editor viewport: the current shell has Scene, 3D Viewport, Inspector, FileSystem, toolbar, and editor tabs, but the default project currently opens with an empty viewport and no authored day environment.

The polish program must not reopen completed foundations as scattered partials. Each number below is one heavy increment with a clear owner-facing outcome, explicit dependencies, and a hard completion gate. Future capabilities may remain planned after a level closes; they must not be silently treated as implemented merely because a neighboring system exists.

| Rule | Requirement |
|---|---|
| **One number, one cohesive increment** | Each level groups the entire audited family needed for its outcome. No artificial four-file or one-symbol slices. |
| **Evidence before status** | A level is complete only after source review, focused tests, full regression gates, and its own visual/performance evidence pass. |
| **Production ownership** | Every new state has a clear owner, lifecycle, failure path, persistence rule, and caller boundary. |
| **No speculative systems** | Do not add fake services, placeholder integrations, or duplicate algorithms to make a roadmap row appear complete. |
| **No platform binary generation** | This polish roadmap does not create `.exe`, `.apk`, installers, or other platform packages. CI artifacts are reports and diagnostics unless a future scope decision explicitly changes this rule. |
| **Design consistency** | The editor uses a deliberate dark, high-contrast, dense tool layout with clear hierarchy, stable focus states, compact information architecture, restrained motion, and accessible contrast. |

## 2. Ten-level execution map

| Level | Heavy polish increment | Primary outcome | Depends on |
|---:|---|---|---|
| **1** | **Viewport Daylight Foundation** | A credible default 3D editor scene with sky, sun, ground, PBR material, camera framing, shadows, and a safe empty-project fallback. | Current viewport and renderer foundations |
| **2** | **AAA Editor UX and Visual System** | A consistent editor design system, responsive panel behavior, selection/interaction feedback, keyboard workflow, and persisted professional layout. | 1 |
| **3** | **Scene Authoring and Content Workflow** | Fast, failure-safe scene composition with multi-selection, inspector editing, transforms, undo/redo, content navigation, and clear diagnostics. | 1–2 |
| **4** | **Rendering Quality and Lighting Polish** | Production-grade viewport render modes, calibrated PBR/material preview, environment controls, shadow quality controls, and visual debugging. | 1–3 |
| **5** | **Asset Pipeline and Live Iteration** | A reliable content browser/import/reimport/hot-reload loop with thumbnails, dependency visibility, stale-state diagnostics, and descriptor-aware project flow. | 2–4 |
| **6** | **Gameplay, Animation, and VFX Authoring Polish** | Editor-first preview and authoring workflows for gameplay entities, character motion, animation playback, particles, audio cues, and interaction feedback. | 3–5 |
| **7** | **Performance and Memory Engineering** | Measured frame budgets, profiling, allocation reduction, render submission efficiency, cache discipline, and stable editor responsiveness. | 1–6 |
| **8** | **Reliability, Diagnostics, and CI Evidence** | Failure recovery, actionable diagnostics, automated test/report artifacts, probe evidence, and CI visibility without release binaries. | 1–7 |
| **9** | **Compatibility, Soak, and Production Hardening** | Long-run stability, migration safety, corrupted-content recovery, deterministic validation, sanitizer coverage, and production resilience. | 1–8 |
| **10** | **Super-Optimized Polish** | End-to-end optimization and refinement across startup, viewport quality, editor interaction, memory, frame pacing, diagnostics, and validated production budgets. | 1–9 |

## 3. Detailed increments

### Polish 1 — Viewport Daylight Foundation

This is the first visible transformation. The editor must open a valid default scene instead of an empty dark viewport when no project content has been authored. The scene should use engine-owned, deterministic preview content: a sky or environment background, a directional sun, a ground plane, a calibrated neutral PBR material, a framed camera, visible shadows, and the existing grid/gizmo system. The preview must remain clearly labeled as editor preview content and must never mutate the user’s project silently.

The increment must also define the empty-project behavior. If the project has no content root, the viewport should show a deliberate empty state with a clear action path instead of looking like a renderer failure. If the preview scene cannot be created or loaded, the editor must fail visibly and safely while retaining the shell, diagnostics, and project health state.

**Required evidence:** deterministic viewport capture on the default scene, empty-project capture, valid-scene capture, light/shadow verification, software-GL smoke, native rendering tests, editor bridge tests where affected, and no non-finite render state. The increment is not complete if it only changes the clear color or adds an untested hard-coded mesh.

### Polish 2 — AAA Editor UX and Visual System

This increment establishes one coherent visual and interaction language for the editor. It covers panel hierarchy, dark-theme tokens, typography, spacing, borders, selection colors, active-tool state, hover/pressed/focus-visible states, empty/loading/error/success states, compact status presentation, and predictable keyboard navigation. The viewport, Scene panel, Inspector, FileSystem, toolbar, and bottom tabs must feel like one tool rather than separate demonstrations.

The interaction layer must include deliberate selection feedback, transform-tool state, clear command acknowledgement, undo/redo feedback, escape routes for modal or drag operations, and layout persistence through the existing session boundary. High-frequency viewport updates must remain separate from presentation state. Motion is feedback only, must be interruptible, and must provide a reduced-motion or static fallback where applicable.

**Required evidence:** visual baselines at the supported editor window sizes, keyboard and pointer interaction coverage, focus/contrast review, persisted-layout round trip, empty/loading/error state coverage, managed/native bridge regression tests, and a complete screenshot review after implementation.

### Polish 3 — Scene Authoring and Content Workflow

This increment makes scene construction feel production-ready. It consolidates entity creation, component authoring, multi-selection, transform editing, parent/child operations, inspector field editing, content navigation, scene save/load, and undo/redo into one failure-safe workflow. The user should be able to create a small lit scene, inspect it, modify it, save it, reopen it, and recover from a rejected edit without losing the last valid state.

The workflow must expose actionable validation messages near the relevant operation. Long-running or rejected content actions must not freeze the editor or partially mutate the scene. Multi-selection must define mixed-value behavior, transform pivot behavior, and deterministic ordering. Scene and project state must remain separate from transient presentation state.

**Required evidence:** an end-to-end authoring scenario, multi-selection and mixed-value tests, undo/redo and failed-operation rollback tests, save/reopen round trip, content-root diagnostics, bridge snapshots, and a stable automated editor scenario.

### Polish 4 — Rendering Quality and Lighting Polish

This increment improves the visible quality of the viewport beyond the daylight baseline. It defines calibrated material preview, environment intensity and exposure controls, shadow quality modes, normal/tangent diagnostics, wireframe and unlit modes, overdraw or bounds visualization where supported, and a stable render-debug overlay. The controls must be explicit, bounded, and reversible; they must not hide invalid assets or silently replace authored content.

The renderer must preserve a predictable quality hierarchy so that a user can understand whether a visual difference comes from material data, lighting, camera state, fallback resources, or a debug mode. The increment should include a small deterministic reference scene containing multiple materials, a directional light, a secondary fill, transparent or particle content where already supported, and visible depth ordering.

**Required evidence:** reference-scene captures for each render mode, PBR and shadow regression tests, finite-value and fallback tests, GPU-independent command diagnostics, software-GL verification, and visual comparison against the agreed design baseline.

### Polish 5 — Asset Pipeline and Live Iteration

This increment closes the editor’s content loop around the existing asset identity, typed envelopes, import queue, dependency/index, hot-reload, project-check, and `.uveditor` descriptor boundaries. It adds the production presentation needed to understand content: thumbnails or lightweight previews where supported, import/reimport status, source and derived identity, dependency visibility, stale-state explanations, retry or recovery actions, and clear unsupported-format diagnostics.

The `.uveditor` descriptor remains metadata/reference-only. Polish work may add a native editor adapter that reads the descriptor and presents project/content state, but it must not embed scenes/assets, execute arbitrary importers on the UI thread, mutate content without an explicit command, or create platform packages. The content browser must remain responsive while work is queued and must preserve the last known-good asset state on failure.

**Required evidence:** import/reimport/hot-reload scenarios, stale dependency and recovery tests, thumbnail or preview fallback tests, descriptor load/update validation, content-browser visual review, queue responsiveness evidence, and full asset/project regression coverage.

### Polish 6 — Gameplay, Animation, and VFX Authoring Polish

This increment turns the existing gameplay/content foundations into a coherent authoring and preview workflow. It covers scene-level gameplay entity setup, character and interaction feedback, animation playback/preview controls, particle and audio cue visibility, runtime-versus-editor state separation, and safe preview reset. The target is not a collection of disconnected node buttons; it is a dependable loop from authored content to visible preview to saved state.

Preview playback must define time ownership, reset behavior, pause/step behavior, selection preservation, and failure diagnostics. Particles, animation, audio, and interaction previews must not leak transient state into saved scene data. Authoring commands must remain bounded and testable, and any unsupported advanced behavior must be surfaced as a clear future capability rather than represented by a silent placeholder.

**Required evidence:** end-to-end preview sessions, pause/step/reset tests, transient-state isolation, character/entity interaction regressions, particle/audio/animation handoff tests, bridge presentation checks, and visual review of the authoring workflow.

### Polish 7 — Performance and Memory Engineering

This is a heavy measurement-driven optimization increment, not a generic cleanup. It establishes budgets for startup, editor idle, viewport interaction, scene load, content refresh, frame time, memory, allocations, and test duration. It instruments the high-frequency paths first, then reduces measurable cost through batching, cache reuse, bounded allocations, render submission discipline, dirty-region updates, and avoidance of unnecessary bridge serialization.

Every optimization must preserve ownership and failure semantics. The increment should produce before/after measurements for representative empty, small, and content-rich scenes. It must identify software-GL behavior separately from hardware-capable behavior and must not claim GPU improvements from CPU-only measurements.

**Required evidence:** repeatable benchmark harness, CPU/memory/frame-time profiles, allocation and startup measurements, regression thresholds, representative scene captures, no functional regressions, and a written explanation for every accepted trade-off.

### Polish 8 — Reliability, Diagnostics, and CI Evidence

This increment makes failures easy to understand and easy to reproduce. It covers structured editor diagnostics, actionable project/content health summaries, safe recovery after rejected writes or malformed content, test-result publication, editor-host probe logs, software-GL smoke logs, and CI artifact retention. The current `.github/workflows/uve-ci.yml` is the primary workflow to update for report artifacts; the intended outputs are test reports, logs, contract references, CMake diagnostics, and probe evidence.

The CI update must use `if: always()` for diagnostic uploads so failed jobs retain useful evidence. Managed tests should emit a TRX report, while native jobs should preserve CTest output and relevant testing diagnostics. This increment does not upload or produce `.exe`, `.apk`, installers, or other platform release packages. Any future machine-learning analysis must be an optional, separately justified job and must never gate core correctness without a measured use case.

**Required evidence:** green and intentionally failed CI runs with retained artifacts, native and managed report validation, probe and smoke log inspection, recovery tests, no secret leakage in artifacts, and hosted GCC/.NET checks.

### Polish 9 — Compatibility, Soak, and Production Hardening

This increment validates the engine over time and across bad inputs rather than only on a clean short run. It covers schema/version migration, descriptor compatibility, corrupted or incomplete asset handling, interrupted atomic publication, stale project state, repeated import/reload cycles, long editor sessions, repeated scene open/close, memory growth, and deterministic diagnostics. Sanitizer and warning configurations should be added where the repository toolchain supports them.

The hardening pass must define what is recoverable, what is rejected, and what requires user intervention. It must preserve last-known-good state, avoid silent data loss, and make any incompatible content clearly actionable. Soak scenarios should cover the editor shell, viewport, scene authoring, content refresh, managed bridge, and headless runtime paths.

**Required evidence:** long-duration soak runs, repeated open/close and reload cycles, corruption/recovery fixtures, migration fixtures, sanitizer or equivalent diagnostics, memory-growth comparison, clean shutdown evidence, and full hosted CI validation.

### Polish 10 — Super-Optimized Polish

Polish 10 is the final optimization and refinement program. It may begin only after Levels 1–9 have independent evidence. The goal is a single production scorecard covering startup time, editor responsiveness, viewport frame pacing, memory high-water mark, scene/content refresh latency, managed bridge latency, render quality, diagnostics clarity, test duration, and CI evidence completeness.

The work must prioritize the measured bottlenecks that most affect daily authoring. It may include cross-system scheduling improvements, cache topology changes, render-state reduction, lower allocation pressure, faster project indexing, bridge payload compaction, optimized UI invalidation, and quality-tier tuning. It must also include a final visual pass over the day scene, empty state, authoring workflow, content browser, and diagnostics surfaces. No optimization is accepted solely because it sounds faster; every accepted change needs a before/after measurement and a regression guard.

**Required evidence:** final scorecard with targets and actuals, cold and warm startup measurements, representative scene frame pacing, memory and allocation deltas, content refresh timings, managed/native latency measurements, visual regression captures, full native and managed test gates, hosted CI with retained reports, clean probe output, clean software-GL smoke, and a final whole-repository diff review.

## 4. Definition of done for every level

A polish level is **GREEN** only when the complete family has been audited, the implementation is integrated with existing ownership boundaries, focused tests pass, full native and managed tests pass, the relevant visual or performance evidence is reviewed, the contract inventory is regenerated when public APIs change, hosted checks are green, the signed PR is merged, and the final branch is clean and synchronized.

A level is **ORANGE** when a meaningful capability works but has an unresolved limitation, weak evidence, or an unexplained performance or UX regression. It must remain explicitly bounded and cannot be reported as fully polished until the issue is resolved or formally accepted as future scope.

A level is **RED** when build, test, integration, data-safety, runtime, or required evidence fails. The workflow is stop, diagnose the root cause, fix, validate again, and review the entire resulting diff again.

## 5. Execution sequence

The recommended order is **1 → 2 → 3 → 4 → 5 → 6 → 7 → 8 → 9 → 10**. Levels 1–6 establish the visible production experience. Level 7 measures and optimizes it. Level 8 makes evidence and failures operationally useful. Level 9 proves resilience over time. Level 10 applies the final measured optimization and visual refinement across the complete system.

No level should be split into scattered micro-increments merely to reduce the apparent difficulty. If a genuine dependency blocks a level, the dependency should be recorded explicitly, the current level should remain ORANGE, and the next change should continue to serve the same heavy outcome.

## 6. Current recommended starting point

The immediate starting point is **Polish 1 — Viewport Daylight Foundation**. The current captured editor proves the shell and viewport presentation path exist, but it also proves that the default project has no authored content and no day environment. The first polish increment should therefore create a deterministic, engine-owned day preview and a deliberate empty-project experience before deeper visual or UX refinement begins.
