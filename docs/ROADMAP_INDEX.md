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
| Core Runtime broad PARTIAL families | **0** | The authoritative engine queue is complete through Networking Increment 662; future additions are separate architecture rather than remaining broad partial families. |
| Native C++ Visual Scripting | **COMPLETED** | Closed by Increment 650; it is not included in the completed Core Runtime family count. |
| Motion Query inventory PARTIAL entries | **178 historical targets** | Reconciled into shared UVE-native authorities by Increment 651; excluded from the Core Runtime count. |
| Developer Tooling top-level PARTIAL milestones | **0** | The tooling sequence is complete at the documented top-level boundaries; future tooling additions follow normal evidence gates. |

## Completed foundations

The engine already has a verified foundation across the frame lifecycle, memory, jobs, configuration, ECS and scene ownership, serialization, virtual files, asset identity, physics and collision primitives, input/action basics, audio and save/checkpoint services, desktop windowing, OpenGL rendering, shader/material/PBR and shadow foundations, project indexing/import queues, the first-generation Scene Editor, native/managed bridge transport, visual-script graph/runtime/editor contracts, animation/pose/retargeting foundations, plugin registration, profiler/diagnostic capture, data-table validation, and contract-inventory generation.

The **Native C++ Visual Scripting** family is complete as of Increment 650. Its node registry, typed graph validation, IR/bytecode, bounded VM dispatch, per-entity typed ticks, editor/canvas contracts, persistence, debugger, hot reload, and typed value families are integrated. The final compiler boundary is a deterministic bounded Kahn scheduler for validated non-execution data dependencies, including composed/deeper links, valid fan-out, immediate producer-local transfers, deterministic cycle rejection, and preserved flow-dispatch safety.

## Core Runtime — no remaining broad PARTIAL families

These are the authoritative engine-only families. Each family must be completed as one whole-family increment or a clearly bounded two-to-three-increment closure, not as repeated file-by-file hardening slices.

| Priority | Family | Current boundary and next completion proof |
|---:|---|---|
| 1 | **Scene components and user-facing nodes — COMPLETED (Increment 654)** | Delivered typed, validation-first add/update/remove authoring for camera, mesh, light, collider, rigid-body, audio-source, particle emitter, script, and AnimationPlayer components; Mesh GUID asset references; canonical scene persistence; Undo/Redo and Play-mode/failure-atomic guards; stable native Inspector drawers and Add Component UX; copied native/stdio/managed bridge visibility; and EngineCore reconciliation of authored particle emitters through the existing simulation/render path. AnimationPlayer now provides a validated project-relative authored clip identity and deterministic playback settings; clip-format import, clip decoding, skeletal pose application, and advanced particle/VFX depth remain separate future asset/render boundaries. Completed Visual Scripting is not part of this family. |
| 2 | **Prefab maturity — COMPLETED (Increment 655)** | Delivered deterministic source-envelope revision identity, revision-stamped instantiation, clean-instance refresh with parent preservation, explicit Merge Required handling for local overrides, force-refresh discard semantics, canonical `.uveprefab` extension enforcement, nested prefab data preservation without recursive re-instantiation, native save/refresh/discard authoring commands, prefab Inspector conflict facts, native/stdio/managed bridge visibility, and focused persistence/rollback/conflict tests. Existing value-level apply/revert/merge helpers remain the typed property-boundary authority; arbitrary reflection and automatic asset decoding are not claimed. |
| 3 | **Asset-type importers — COMPLETED (Increment 656)** | Delivered the supported UVE-native import boundary: virtual paths, typed Mesh/Texture/Material/Shader/Audio/Animation envelopes, bounded BMP/PNG/TGA/JPEG/OBJ/glTF/MTL/shader/WAV bridges, `.uveanim` JSON clip serialization, explicit source classification and parser-authority diagnostics, failure-atomic publication, generation-aware derived-data cache validation, built-in AssetManager reachability, EngineCore composition, and editor bridge visibility. Raw FBX/DAE/audio codecs, skeletal animation decoding/retargeting, broader glTF scene/material/image conversion, GPU cooking/compression, and external dependency/licensing integrations remain separate future asset-format families rather than hidden partial work. |
| 4 | **Physics depth — COMPLETED (Increment 657)** | Delivered the supported runtime composition boundary: EngineCore-owned bounded distance/hinge constraint registry attached to normal fixed-step PhysicsSystem execution; EngineServices reachability for constraints and a stateless query façade delegating deterministic Sphere/Box/Capsule casts, area-overlap snapshots, and caller-owned character movement; dependency-safe lifetime ordering; contract inventory; focused EngineCore/service/query regressions; full native/managed/editor/probe/software-GL validation; and hosted GCC/.NET CI. Broader shape/body authoring, full backend physics replacement, additional advanced joint families, measured simulation/instrumentation, and gameplay integration remain separate future boundaries rather than hidden partial work. |
| 5 | **Audio depth — COMPLETED (Increment 658)** | Delivered the supported runtime composition boundary: existing bounded PCM16 refill-window planning and PCM gain-effect scheduling are now owned per live AudioSystem source and reachable through the normal EngineCore/EngineServices path; caller-owned decoded samples remain outside AudioSystem ownership; atomic effect publication, deterministic stream cursors, invalid-handle failure closure, contract inventory, focused runtime regressions, full native/managed/editor/probe/software-GL validation, and hosted GCC/.NET CI are verified. Real platform audio backends, automatic AssetManager-owned streaming buffers, music/SFX policy, side-chain processing, and editor mixer authoring remain separate future boundaries rather than hidden partial work. |
| 6 | **Input breadth — COMPLETED (Increment 659)** | Delivered the supported runtime composition boundary: EngineCore-owned bounded GamepadInputSystemUVE and MobileInputSystemUVE snapshots, a thin MobileGestureSystemUVE adapter over the existing recognizer, gamepad injection into the existing InputSystemUVE action layer, EngineServices reachability for all three non-desktop services, deterministic frame/update and dependency-safe shutdown ordering, focused adapter/EngineCore/service regressions, contract inventory, full native/managed/editor/probe/software-GL validation, and hosted GCC/.NET CI. Android/iOS polling, permissions, lifecycle, safe-area mapping, haptics, platform gesture backends, and editor-camera behavior remain separate future boundaries rather than hidden partial work. |
| 7 | **Save-game depth — COMPLETED (Increment 660)** | Delivered the supported runtime persistence boundary: existing SaveGameSystemUVE framing, metadata validation, bounded compression, schema migration dispatch, metadata-only reads, scratch-scene rollback, atomic temp-file publication, reserved numbered/autosave/manual slots, and CheckpointManagerUVE interval/playtime/retry policy are production-composed and covered by focused Save/Checkpoint tests. Increment 660 adds copied caller-owned checkpoint metadata policy and EngineCore composition of canonical engine-version fields into normal autosave/manual checkpoint metadata, with full native/managed/editor/probe/software-GL validation and hosted GCC/.NET CI. Cloud transport/conflict resolution, encryption/key management, thumbnails, gameplay-domain state policy, platform durability, and broader schema/domain migration remain separate future boundaries rather than hidden partial work. |
| 8 | **Platform abstraction — COMPLETED (Increment 661)** | Evidence-closed at the supported runtime boundary: `WindowManagerUVE` owns the GLFW3 desktop window/OpenGL-context lifecycle, event/input polling, resize/focus/close events, VSync, backend-confirmed primary-monitor fullscreen, validated framebuffer dimensions, copied monitor enumeration, native-handle/backend diagnostics, and safe failure behavior; `NullWindowManagerUVE` provides the tested headless substitution and EngineCore composes the correct WindowManager/RenderDevice lifetime ordering. The full Platform/EngineCore focused set passed 46/46, with full native/managed/editor/probe/software-GL gates green. Mobile lifecycle/safe areas, power management, hot-plug policy, native display-mode enumeration/application, and additional OS backends remain separate future platform boundaries rather than hidden partial work. |
| 9 | **Networking — COMPLETED (Increment 662)** | Evidence-closed at the supported bounded primitive boundary: exact little-endian reliable header/fragment wire codecs, payload budgeting/planning, bounded fragmentation and aggregate reassembly, duplicate/conflict handling, wrap-safe receive windows, cumulative/selective acknowledgements, exponential retry backoff, caller-owned retry scheduling, and failure-safe retransmission policy are implemented and covered by 39/39 focused Networking tests. No socket, peer, transport-thread, authentication, replication, ownership/RPC, prediction, reconciliation, interpolation, or dedicated-server architecture exists in the current engine; those remain separate future networking/gameplay boundaries rather than hidden partial work. Full native/managed/editor/probe/software-GL gates are green. |

The Core Runtime broad PARTIAL queue is complete at Increment 662. The owner-facing metric is **0 engine-only broad PARTIAL families**; Motion Query and Control Rig remain separate completed plugin families and are not counted in this metric.

## Editor and workflow — supported boundary completed (Increment 663)
The supported Editor/workflow boundary is complete at Increment 663: native editor/bridge/session foundations, static plugin manifest and capability validation, bounded Developer Console authorization with caller-owned principal/session audit records, project health checks, asset import/index/hot-reload foundations, managed shell persistence, and contract-reference generation are integrated and verified. Dynamic external services and advanced production workflows remain explicit future boundaries below.

| Family | Verified supported boundary and explicit future exclusions |
|---|---|
| **Plugin ABI, capability, and dynamic lifecycle — COMPLETED (Increment 663)** | Supported boundary: bounded static manifest validation, protocol negotiation, capability policy, generation-checked registration scopes, and busy-safe unregister are implemented and tested. Dynamic library discovery/loading, OS handle ownership, symbol lookup, and secure unload remain separate future platform/plugin architecture. |
| **Developer Console runtime security — COMPLETED (Increment 663)** | Supported boundary: bounded Denied/ReadOnly/Full authorization, irreversible Shipping denial, copied caller-labeled principal/session context, monotonic accepted/denied audit records, and exception-isolated caller sink are integrated and tested. External identity resolution, audit persistence, and command-specific service permissions remain future architecture. |
| **Documentation delivery and contract generation — COMPLETED (Increment 663)** | Supported boundary: authoritative contract inventory and deterministic generated contract reference are validated in the native test gate, with source-level ownership/exclusion notes retained. Full API reference, user manual, Visual Scripting reference, plugin guide, and deployment guide remain future documentation delivery. |
| **Asset pipeline depth — COMPLETED (Increment 663)** | Supported boundary: virtual paths, GUID-backed asset identity, typed native envelopes, bounded model/texture/material/shader/audio/animation importers, asynchronous import queue, dependency/index services, hot reload, derived-artifact invalidation, project checks, and editor content-browser bridge are integrated and tested. Additional codecs, licensing contracts, thumbnails, GPU cooking, and broader derived-data production remain future asset architecture. |
| **Advanced editor tools — COMPLETED (Increment 663)** | Supported boundary: first-generation Scene Editor, hierarchy/inspector/content-browser bridge, visual-scripting canvas/debugger, prefab/data-table authoring, session layout persistence, and deterministic regression harness are integrated and verified. Deeper animation authoring, terrain, world streaming, profiling UI, cinematics, source control, and collaboration remain separate future editor products. |
| **Platform and shipping workflow — COMPLETED (Increment 663)** | Supported Editor/workflow boundary: deterministic project health validation, reproducible native/managed editor-host builds, contract checks, CI gates, editor probe, and headless smoke are integrated and verified. The user-directed `.uveditor` project/content format and ecosystem import/update boundary are reserved for the separate Platform/release increment; cooking, packaging, deployment, and sample-project growth remain future release architecture. |

The editor and tooling histories were summarized into the supported boundaries above. Increment 663 adds the bounded Developer Console principal/session audit seam; all larger external-service and advanced-production boundaries remain explicit future work.

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

## Motion Query — COMPLETED UVE-native plugin family; Control Rig remains separate

**Motion Query is COMPLETE as the current UVE-native plugin family in Increment 651.** The whole-family audit reconciled the retired 198-entry inventory—178 historical PARTIAL targets, 15 PLANNED targets, and 5 COMPLETED targets—against 24 plugin headers, 24 implementation sources, 23 Motion Query test sources, the `uve_plugins` target, runtime/editor callers, asset ingestion, derived-data/search-index seams, and the bridge transport. The historical filename rows were target names, not 198 missing physical files: they were fulfilled by 13 shared UVE-native authorities and are now reclassified through implemented or rewritten-native dispositions rather than duplicated as foreign-engine-shaped files.

Increment 651 completed the remaining concrete family boundary: validated clip-to-motion-library preprocessing with bounded future trajectories, generalized multi-candidate database factory construction, shared editor database-entry validation/factory utilities, stable dynamic property metadata, and native/stdio bridge exposure. The result is covered by 167/167 focused Motion Query tests, the full native suite, managed editor tests, editor probe, and software-GL smoke. Motion Query is no longer an active PARTIAL count in the engine roadmap.

Optional future capabilities such as learned/neural feature compression remain separate research scope and are not represented as missing current plugin foundations. Motion Query and Control Rig remain outside the nine Core Runtime engine PARTIAL families.

Increment 653 adds the first production-polish trajectory boundary without reopening the completed family: one bounded, value-only, time-sampled trajectory stream now carries broad animation context metadata and optional per-sample capsule dimensions. Motion Query consumes that stream for intent and matching, while Physics consumes the identical stream for read-only collision prediction and capsule sweeps. Native editor bridge and stdio snapshots expose copied trajectory, shape, and collision-preview facts for future visualization. This boundary supports classification for locomotion, turns, hop, slide, jump, fall, light/heavy landings, takedown, ragdoll, combat, interaction, and custom contexts; it does not claim playback orchestration, full ragdoll/takedown state machines, raw animation-format import, or direct renderer-owned viewport drawing.

**Control Rig is COMPLETE as the current UVE-native plugin family in Increment 652.** The whole-family audit covered the existing runtime foundation, shared skeleton/pose/time contracts, native editor viewport/gizmo seams, bridge snapshot transport, build/test registration, and the absence of foreign-engine-shaped authoring files. The completed boundary adds deterministic mapped-skeleton autorig generation; root/spine/head, hand IK/pole, and foot IK/pole controls; animator-visible Box/Circle/Arrow control shapes; explicit role bindings; bounded validation; existing solver integration; reset and side-mirror operations; skeleton-matched bake transfer; native authoring selection/tool/transform sessions; chronological bake-sample capture; revisioned viewport-ready snapshots; and native/stdio bridge exposure.

The verified Control Rig boundary is covered by 34/34 focused runtime, autorig, editor-authoring, and bridge tests; the full native suite remains 2,098/2,098 GREEN with the expected eight shader-parity skips; managed editor-host tests remain 83/83 GREEN; the editor-host probe reports 0 warnings and 0 errors; and software-GL smoke reaches EndFrame 30 with clean shutdown. The runtime and bridge contracts remain value-only and non-owning. A future Polish/production extension may add direct ImGui control-shape drawing and viewport hit testing, persisted rig assets/importers, richer constraint graph authoring, managed Animator presentation, advanced control shapes, and bake/transfer policy for additional animation backends, but those are not retroactively treated as missing from the completed current family boundary.

## AAA future capability map

The AAA roadmap is a future-capability map, not an additional engine-count ledger. Its summarized capability groups are **Animation authoring/runtime**, **Control Rig/procedural animation**, **frame scheduling and task graphs**, **unified time**, **transform/pose evaluation**, **resource lifetime**, **event/signal bus**, **reflection/type metadata**, **asset dependency graphs**, **derived-data cache**, **profiling**, **crash diagnostics**, **RenderGraph completion**, **GPU-driven submission**, **post-processing**, **world streaming**, **navigation/AI**, **VFX/GPU compute**, **production editor tools**, **replication/prediction**, **production audio**, **cooking/build graphs**, **compatibility/migration**, **deterministic replay**, **soak/stress testing**, **performance budgets**, and **capability/platform matrices**. Existing foundations must be reused rather than duplicated.

The AAA future-capability groups are summarized above; they are not an additional implementation-count ledger.

## Consolidated execution rules

A new implementation increment begins with a whole-family audit across headers, sources, validators, registries, loaders, runtime owners, editor callers, tests, build wiring, and cross-module interfaces. The increment then implements the complete identified boundary coherently, proves RED-first behavior where a regression is introduced, runs focused and full validation, performs a final family audit, and only then changes status.

The execution state is **RED** when the intended behavior fails, **ORANGE** when implementation or review reveals an incomplete contract, and **GREEN** only after source, tests, build, integration, and hosted checks pass. Every merged increment requires signed history, a PR targeting `main`, hosted GCC and managed checks, squash merge, four-ref synchronization, and evidence recording. Protected unrelated work must never be mixed into the active family.

## Single-roadmap policy

This file is the repository’s only active roadmap. The former domain roadmaps and operational queue have been retired because they duplicated status and scope across multiple views. Their unique capability information has been summarized above; implementation history remains in Git commits, while the original product vision remains in [`MASTER_SPEC.md`](MASTER_SPEC.md), which is a specification rather than a roadmap.
