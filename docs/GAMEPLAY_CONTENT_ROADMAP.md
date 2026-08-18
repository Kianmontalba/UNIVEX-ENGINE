<div align="center">

<h1><strong>UNIVEX ENGINE — GAMEPLAY & CONTENT ROADMAP</strong></h1>

<strong>User-facing nodes, simulation content, animation, procedural tools, and cinematic/gameplay systems</strong>

</div>

> This roadmap describes content-facing systems that must grow from the existing C++ scene, physics, audio, input, save, renderer, and editor foundations. User-facing node names remain clean and do not carry the `UVE` suffix; internal engine APIs do.

| Status | Completed foundation | Delivered capability | Ongoing boundary |
|---|---|---|---|
| **COMPLETED** | **Increments 5–6** | ECS scene graph, transforms, serializable scenes, and prefabs. | These are the base for all later gameplay/component work. |
| **COMPLETED** | **Increments 15–16** | Collision, simulation foundation, raycasts, and physics materials. | Character/rigid-body/area feature depth remains partial. |
| **COMPLETED** | **Increments 17–19** | Input, audio, and save/checkpoint foundations. | GamepadInputSystemUVE bounded snapshots, AudioSystemUVE mixer-group routing/multiplier diagnostics, and SaveGameSystemUVE current-schema dispatch with unsupported-version diagnostics are now proven foundations; full gameplay integration, effects, cloud/save-domain modeling, and content policy remain partial. |
| **COMPLETED** | **Increments 38–58** | Real Scene Editor authoring: selection, transforms, hierarchy, history, Play sandbox, and built-in primitives. | Editor primitives are not a complete gameplay-node catalog. |

<div align="center">

<h2><strong>PARTIAL — USER-FACING NODE AND COMPONENT SET</strong></h2>

</div>

| Status | System | Intended capability | Completion proof / boundary |
|---|---|---|---|
| **PARTIAL** | Core 3D nodes | `Node3D`, mesh/material references, camera variants, directional/point/spot lights, collision shapes, rigid/static/character bodies, areas, audio sources, and script attachments. | Camera, light, collider, rigid-body, and audio-source validity plus invalid-scene rollback are proven; collider-vs-collider layer/mask filtering now requires symmetric acceptance before deterministic AABB pairs are emitted; AreaComponentUVE persistence, a bounded read-only area-to-collider overlap query, a generation-safe copied enter/exit lifecycle tracker, and EngineCore typed queued Entered/Exited event publication are also proven; MeshComponent reference coherence, Script path validation, and Particle budget validation with rollback are also proven; validated ScriptComponent ownership now reconciles empty-path detach and graph-to-runtime attach without filesystem ownership; asset loading, replacement reload, camera/light/collider/rigid-body/audio authoring commands, script execution depth, particle runtime, exact sphere/capsule interactions, expanded shape authoring, constraints, character depth, mixer behavior, raw mesh import, and each remaining node/component still require real runtime behavior and tests; no empty node shells. |
| **PARTIAL** | Core 2D/UI nodes | `Node2D`, sprite, label, button, panel, and progress UI elements. | Requires an actual runtime UI/render/input architecture; editor UI does not substitute for game UI. |
| **PARTIAL** | Character and interaction | Kinematic character controller, triggers/areas, component querying, spawning/destruction, and gameplay state conventions. | AreaComponentUVE serialization, bounded read-only area-to-collider overlap facts, a generation-safe copied Entered/Exited tracker with truncation-safe baseline retention, EngineCore queued Entered/Exited event delivery, CharacterControllerUVE bounded kinematic substep movement with wall blocking, tangential sliding, and symmetric layer/mask filtering, deterministic BVH-backed collider candidate generation with legacy pair ordering, and optional bounded distance/hinge positional constraints are now available; ground/step behavior, component querying, spawning/destruction, gameplay state conventions, angular joint behavior/motors/limits, dynamic-body push policy, full CCD/TOI integration, locomotion/animation integration, and a representative sample scene still require defined behavior under physics/input tests; CharacterControllerUVE also exposes an opt-in bounded swept-AABB TOI path for tunneling prevention. |
| **PARTIAL** | Animation | Animation assets, playback, blending, events, state graphs, and later IK. | Shared SkeletonDefinitionUVE/PoseBufferUVE/AnimationEvaluationContextUVE contracts now converge Motion Query and Control Rig validation/evaluation with copied bounded identity/time data; native Motion Query database authoring now includes revision-safe copied one-entry clipboard commands with identity-remapped paste and bounded 32-entry snapshot-based Undo/Redo, while asset format/import/runtime binding, full playback/blending/events, retargeting, procedural animation, IK depth, and broader Animation Editor authoring remain partial. |
| **PARTIAL** | Particles/VFX | GPU particle emitter, render integration, authoring controls, and lifecycle/culling policy. | Particle emitter budget validation and rollback are proven; ParticleRuntimeUVE provides bounded CPU emission/update state with finite inputs, deterministic sequence IDs, semi-implicit integration, and lifetime culling; ParticleRenderBridgeUVE copies enabled state into RenderQueueUVE, Renderer3DUVE consumes it through a borrowed per-frame input seam with copied diagnostics, ParticleDrawRecorderUVE records bounded GPU-independent command DTOs, and the built-in OpenGL path submits bounded non-instanced world-axis quads with lifetime alpha; camera-facing billboarding, particle textures/assets, GPU simulation/compute, occlusion/culling depth, authoring controls, and measured renderer/compute design remain partial. |
| **PARTIAL** | Audio gameplay | Source/listener binding, music/SFX routing, attenuation, mixer groups, and effects. | AudioSource value validation, spatial distance semantics, invalid-scene rollback, entity-to-voice ownership, play-on-awake, active-camera listener updates, listener-relative attenuation, and AudioSystemUVE bounded named mixer groups with Master default, explicit source routing, volume/pitch multipliers, and copied diagnostics are proven; music/SFX content policy, effects, clip asset resolution, streaming, and concrete platform backend selection remain partial. |

<div align="center">

<h2><strong>PARTIAL — AUTHORING AND ADVANCED CONTENT SYSTEMS</strong></h2>

</div>

| Status | System | Intended capability | Entry condition |
|---|---|---|---|
| **PARTIAL** | Spline system | Control points, curve evaluation, mesh generation, followers, visual handles, loops, tangents, and events. | Stable viewport presentation, tool-session lifecycle, and mesh generation contract. |
| **PARTIAL** | Procedural generation | Noise, procedural meshes, terrain foundations, dungeons, biome/erosion work, and deterministic seed policy. | Measured content/renderer data paths; no monolithic world generator. |
| **PARTIAL** | Data-driven gameplay | Data tables, typed rows, validation, references, and selected visual-script bindings. | Tooling/import contracts and stable component/node IDs. |
| **PARTIAL** | Cinematic sequencer | Sequence asset, tracks, keyframes, curves, scrubbing, events, camera/audio/transform binding, and later export. | Animation, audio, camera, asset, and viewport contracts are stable. |
| **PARTIAL** | Decals / billboards / impostors | Gameplay-visible projected marks and scalable environment representation. | Material/mesh asset workflow, LOD policy, and renderer performance budget. |
| **PARTIAL** | Lightmap baking | Static-scene lighting asset generation for constrained/mobile targets. | Mesh import, UV policy, material workflow, cooking/build pipeline, and bake algorithm choice. |

<div align="center">

<h2><strong>GAMEPLAY CONTENT GATE</strong></h2>

</div>

| Requirement | Meaning |
|---|---|
| **No fake components** | A component appears in menus/Inspector only after it has real runtime, persistence, and test behavior. |
| **One scene model** | Gameplay, editor, serializer, visual scripting, and physics operate on the same scene/entity ownership model. The Visual Scripting canvas now has copied native palette descriptors, managed search filtering, right-click node search, keyboard/click selection, cursor-position insertion, a 16-node production catalog covering Float math, Vector3 math, and Boolean logic, a pure typed Vector3 math kernel, bounded typed VM values including Boolean storage, explicit dependency-aware VM dispatch for all 16 built-in node IDs, and per-entity typed runtime tick execution; broader runtime/editor coverage remains partial. |
| **Deterministic authoring** | Commands, simulation state, saves, and procedural seeds must have explicit replay/validation behavior where appropriate. |
| **Sample project last** | A full sample game becomes a completion proof after the required runtime/content systems exist; it is not a substitute for their implementation. |
