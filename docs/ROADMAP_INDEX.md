<div align="center">

<h1><strong>UNIVEX ENGINE — CONSOLIDATED ROADMAP</strong></h1>

<strong>Single status, scope, dependency, and execution authority</strong><br/>
<strong>Last updated: August 25, 2026</strong>

</div>

> This document is the consolidated execution view for the UNIVEX Engine. The former domain roadmaps have been summarized and retired; this file is now the single status, scope, and execution authority. Detailed implementation history remains in Git commits and source documentation where applicable.

## Status language and counting

`COMPLETED` means that the capability has real implementation, integration, tests, and the required platform/editor verification. `PARTIAL` means that meaningful foundations exist but the stated family boundary is not yet complete. `PLANNED` describes future capability that has not entered implementation. `ACTIVE` describes an execution queue, not a completed feature.

Counts in this document are **planning units, not additive feature totals**. The Core Runtime count is the owner-facing engine metric. Domain roadmaps may describe the same capability from different ownership perspectives, so their row counts must not be summed. Motion Query file-inventory counts remain separate from the engine count.

| Metric | Current state | Meaning |
|---|---:|---|
| Core Runtime broad PARTIAL families | **9** | The authoritative engine queue: scene components, prefabs, importers, physics, audio, input, save, platform, and networking. |
| Native C++ Visual Scripting | **COMPLETED** | Closed by Increment 650; it is not included in the nine remaining Core Runtime families. |
| Motion Query inventory PARTIAL entries | **179** | Separate plugin/file-inventory work; excluded from the Core Runtime count. |
| Developer Tooling top-level PARTIAL milestones | **0** | The tooling sequence is complete at the documented top-level boundaries; future tooling additions follow normal evidence gates. |

## Completed foundations

The engine already has a verified foundation across the frame lifecycle, memory, jobs, configuration, ECS and scene ownership, serialization, virtual files, asset identity, physics and collision primitives, input/action basics, audio and save/checkpoint services, desktop windowing, OpenGL rendering, shader/material/PBR and shadow foundations, project indexing/import queues, the first-generation Scene Editor, native/managed bridge transport, visual-script graph/runtime/editor contracts, animation/pose/retargeting foundations, plugin registration, profiler/diagnostic capture, data-table validation, and contract-inventory generation.

The **Native C++ Visual Scripting** family is complete as of Increment 650. Its node registry, typed graph validation, IR/bytecode, bounded VM dispatch, per-entity typed ticks, editor/canvas contracts, persistence, debugger, hot reload, and typed value families are integrated. The final compiler boundary is a deterministic bounded Kahn scheduler for validated non-execution data dependencies, including composed/deeper links, valid fan-out, immediate producer-local transfers, deterministic cycle rejection, and preserved flow-dispatch safety.

## Core Runtime — nine remaining broad PARTIAL families

These are the authoritative engine-only families. Each family must be completed as one whole-family increment or a clearly bounded two-to-three-increment closure, not as repeated file-by-file hardening slices.

| Priority | Family | Current boundary and next completion proof |
|---:|---|---|
| 1 | **Scene components and user-facing nodes** | Camera, mesh, light, collider, rigid-body, audio-source, particle, animation, and script foundations are validated. Remaining work is real component authoring/runtime breadth, asset-backed behavior, expanded shapes, particle/render depth, and component-specific editor commands; completed Visual Scripting is no longer part of this partial. |
| 2 | **Prefab maturity** | Source/instance identity, revisions, bounded overrides, apply/revert, merge, refresh, rollback, and conflict facts exist. Remaining work is nested prefab policy, source mutation/asset revision tracking, and conflict-resolution UX. |
| 3 | **Asset-type importers** | Virtual paths, typed envelopes, importer registration, bounded parsing, failure diagnostics, and generation-aware cache foundations exist. Remaining work is production model, texture, audio, material, animation, shader, and derived-data import workflows with licensing and dependency policy. |
| 4 | **Physics depth** | Deterministic collision/narrow-phase, rigid-body foundations, queries, constraints, BVH candidate planning, and character-controller slices exist. Remaining work is broader shape/body authoring, trigger and joint depth, CCD/TOI integration, measured simulation, and gameplay integration. |
| 5 | **Audio depth** | Source/listener ownership, mixer groups, routing, attenuation, multipliers, play-state, and failure-safe binding exist. Remaining work is effects, streaming, music/SFX policy, concrete platform backends, and editor mixer authoring. |
| 6 | **Input breadth** | Desktop actions, bindings, bounded injectable snapshots, and threshold policy exist. Remaining work is gamepad/touch/gyro/mobile gestures, remapping/chords/buffering breadth, and platform-specific policy. |
| 7 | **Save-game depth** | Versioned payloads, schema dispatch, migration registration, bounded framing, and failure-safe persistence foundations exist. Remaining work is migration breadth, compression/encryption, cloud hooks, thumbnails, and gameplay-domain state. |
| 8 | **Platform abstraction** | Desktop window/monitor/framebuffer validation, inert invalid-window behavior, and NUL-safe monitor snapshots exist. Remaining work is mobile lifecycle/safe areas, power management, hot-plug policy, and native display-mode application/enumeration breadth. |
| 9 | **Networking** | Bounded reliable sequence windows, packet/reassembly invariants, and failure-safe primitives exist. Remaining work is reliable UDP channels, replication, ownership/RPC policy, prediction, reconciliation, interpolation, and dedicated-server integration. |

The nine families above are the authoritative Core Runtime queue. The owner-facing metric after Increment 650 is **64 → 63 engine-only broad PARTIAL families**.

## Editor and workflow — active partial boundaries

The completed Scene Editor, bridge, canvas, debugger, animation foundations, and plugin registration are retained as delivered history. The remaining editor/workflow boundaries are:

| Family | Remaining scope |
|---|---|
| **Plugin ABI, capability, and dynamic lifecycle** | Static manifest negotiation and generation-checked scopes exist; dynamic library discovery/loading, OS handle ownership, ABI symbol negotiation, and secure unload remain. |
| **Developer Console runtime security** | Bounded authorization and shipping denial exist; identity/session ownership, audit persistence, and command-specific permissions remain. |
| **Documentation delivery and contract generation** | Generated contract references exist; complete API reference, user manual, Visual Scripting reference, plugin guide, and deployment guide remain. |
| **Asset pipeline depth** | Format-specific model, texture, audio, material, thumbnail, and derived-data workflows remain after selection of format/licensing contracts. |
| **Advanced editor tools** | Animation authoring depth, terrain, world streaming, profiling UI, cinematics, source control, and collaboration remain. |
| **Platform and shipping workflow** | Cooking, packaging, deployment, and sample-project growth remain after asset/project validation and renderer presentation are stable. |

The editor and tooling histories were summarized into the family boundaries above. The stale Editor Visual Scripting row is synchronized to `COMPLETED` in the current documentation update.

## Rendering — active partial boundaries

The renderer has verified RHI, Null/OpenGL execution, shader/material/PBR, lights, shadows, normal mapping, render graph foundations, built-in primitives, and viewport presentation. Remaining rendering work is grouped below to prevent long historical rows from obscuring the actual boundary.

| Family | Remaining scope |
|---|---|
| **Material and mesh asset workflow** | Raw model/texture/material import, GUID-backed material/mesh authoring, thumbnails, cache invalidation, and production presentation. |
| **Post-processing** | Bloom, ambient occlusion, anti-aliasing, exposure, and quality-tiered post-process passes. |
| **Render-path breadth** | Transparent sorting, Forward+/deferred choice, GPU-driven/compute submission, skinning, particles, occlusion, and dynamic resolution. |
| **Platform rendering** | Validated Vulkan, DirectX, Metal, or OpenGL ES backend/toolchain choices. |
| **Mobile visual pipeline** | LOD, texture compression, adaptive quality, baked lighting, and light probes. |
| **Decals and projected marks** | Decals with pooling, atlas management, culling, and forward/deferred compatibility. |
| **Billboards and impostors** | Camera-facing sprites and generated far-distance mesh substitutes. |
| **Lightmap baking** | Static-scene UV, bake, and runtime baked-light data. |
| **Cinematic presentation** | Camera/sequence-driven controlled render capture. |
| **Advanced visual systems** | Scalable effects and plugin-provided visual systems after renderer budgets and ownership are explicit. |

The renderer’s completed increments and verification boundaries are summarized above and remain available through Git history.

## Gameplay and content — active partial boundaries

The gameplay/content foundation shares one ECS scene model with the editor, serializer, physics, audio, input, renderer, and completed Visual Scripting runtime. The remaining families are **Core 3D nodes**, **Core 2D/UI nodes**, **Character and interaction**, **Animation**, **Particles/VFX**, **Audio gameplay**, **Spline system**, **Procedural generation**, **Data-driven gameplay**, **Cinematic sequencer**, **Decals/billboards/impostors**, and **Lightmap baking**. Existing validation and runtime slices are foundations only; each family still requires real authoring/runtime behavior and focused tests before promotion.

The per-family completion boundaries and one-scene-model gate are summarized above.

## Platform and release — active partial boundaries

The release path has desktop windowing, GCC CI, and reproducible core build foundations. Remaining work covers **Windows support**, **Linux release support**, **Android support**, **iOS support**, **build configurations**, **asset cooking**, **packaging**, **release automation**, **sample project**, **Engine API reference**, **Editor user manual**, **Visual Scripting reference**, **plugin guide**, and **build/deployment guide**. These are release and documentation units, not substitutes for unfinished engine behavior.

The fourteen platform/release boundaries are summarized above.

## Ecosystem — intentionally separate delivery track

The ecosystem is not part of the engine completion count. Its eight partial families are **UniVex Hub**, **project management**, **engine-to-Hub integration**, **account service**, **public website/news**, **cloud/collaboration**, **marketplace/community**, and **closed beta/public launch**. These require independent security, privacy, operational, release-artifact, and support ownership.

Ecosystem work must not displace the engine critical path or imply that account, payment, cloud, or public-launch contracts exist in the engine.

## Motion Query and Control Rig — separate plugin/future scope

Motion Query and Control Rig remain organized under the Animation plugin boundary and are **not counted as Core Runtime engine partials**. The retired Motion Query file inventory contained 179 file-level PARTIAL entries across runtime, editor, diagnostics, profiling, build, and validation areas; its UVE-native target naming, dependency order, dispositions, and public-technique authorship rule were summarized into this section before deletion. Motion Query must remain separate from Core Runtime implementation increments unless a direct dependency is demonstrated.

Control Rig has shared pose and validation foundations, but broader authoring/runtime behavior remains future scope. No speculative plugin edits are implied by completion of Native C++ Visual Scripting.

The Motion Query and AAA boundaries are summarized above and remain separate from the engine completion count.

## AAA future capability map

The AAA roadmap is a future-capability map, not an additional engine-count ledger. Its summarized capability groups are **Animation authoring/runtime**, **Control Rig/procedural animation**, **frame scheduling and task graphs**, **unified time**, **transform/pose evaluation**, **resource lifetime**, **event/signal bus**, **reflection/type metadata**, **asset dependency graphs**, **derived-data cache**, **profiling**, **crash diagnostics**, **RenderGraph completion**, **GPU-driven submission**, **post-processing**, **world streaming**, **navigation/AI**, **VFX/GPU compute**, **production editor tools**, **replication/prediction**, **production audio**, **cooking/build graphs**, **compatibility/migration**, **deterministic replay**, **soak/stress testing**, **performance budgets**, and **capability/platform matrices**. Existing foundations must be reused rather than duplicated.

The AAA future-capability groups are summarized above; they are not an additional implementation-count ledger.

## Consolidated execution rules

A new implementation increment begins with a whole-family audit across headers, sources, validators, registries, loaders, runtime owners, editor callers, tests, build wiring, and cross-module interfaces. The increment then implements the complete identified boundary coherently, proves RED-first behavior where a regression is introduced, runs focused and full validation, performs a final family audit, and only then changes status.

The execution state is **RED** when the intended behavior fails, **ORANGE** when implementation or review reveals an incomplete contract, and **GREEN** only after source, tests, build, integration, and hosted checks pass. Every merged increment requires signed history, a PR targeting `main`, hosted GCC and managed checks, squash merge, four-ref synchronization, and evidence recording. Protected unrelated work must never be mixed into the active family.

## Single-roadmap policy

This file is the repository’s only active roadmap. The former domain roadmaps and operational queue have been retired because they duplicated status and scope across multiple views. Their unique capability information has been summarized above; implementation history remains in Git commits, while the original product vision remains in [`MASTER_SPEC.md`](MASTER_SPEC.md), which is a specification rather than a roadmap.
