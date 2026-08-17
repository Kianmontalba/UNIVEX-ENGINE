<div align="center">

<h1><strong>UNIVEX ENGINE — GAMEPLAY & CONTENT ROADMAP</strong></h1>

<strong>User-facing nodes, simulation content, animation, procedural tools, and cinematic/gameplay systems</strong>

</div>

> This roadmap describes content-facing systems that must grow from the existing C++ scene, physics, audio, input, save, renderer, and editor foundations. User-facing node names remain clean and do not carry the `UVE` suffix; internal engine APIs do.

| Status | Completed foundation | Delivered capability | Ongoing boundary |
|---|---|---|---|
| **COMPLETED** | **Increments 5–6** | ECS scene graph, transforms, serializable scenes, and prefabs. | These are the base for all later gameplay/component work. |
| **COMPLETED** | **Increments 15–16** | Collision, simulation foundation, raycasts, and physics materials. | Character/rigid-body/area feature depth remains partial. |
| **COMPLETED** | **Increments 17–19** | Input, audio, and save/checkpoint foundations. | Full gameplay integration, mixer, and save-domain modeling remain partial. |
| **COMPLETED** | **Increments 38–58** | Real Scene Editor authoring: selection, transforms, hierarchy, history, Play sandbox, and built-in primitives. | Editor primitives are not a complete gameplay-node catalog. |

<div align="center">

<h2><strong>PARTIAL — USER-FACING NODE AND COMPONENT SET</strong></h2>

</div>

| Status | System | Intended capability | Completion proof / boundary |
|---|---|---|---|
| **PARTIAL** | Core 3D nodes | `Node3D`, mesh/material references, camera variants, directional/point/spot lights, collision shapes, rigid/static/character bodies, areas, audio sources, and script attachments. | Camera, light, collider, rigid-body, and audio-source validity plus invalid-scene rollback are proven; MeshComponent reference coherence, Script path validation, and Particle budget validation with rollback are also proven; validated ScriptComponent ownership now reconciles empty-path detach and graph-to-runtime attach without filesystem ownership; asset loading, replacement reload, camera/light/collider/rigid-body/audio authoring commands, script execution depth, particle runtime, shape breadth, constraints, character depth, mixer behavior, raw mesh import, and each remaining node/component still require real runtime behavior and tests; no empty node shells. |
| **PARTIAL** | Core 2D/UI nodes | `Node2D`, sprite, label, button, panel, and progress UI elements. | Requires an actual runtime UI/render/input architecture; editor UI does not substitute for game UI. |
| **PARTIAL** | Character and interaction | Kinematic character controller, triggers/areas, component querying, spawning/destruction, and gameplay state conventions. | Defined behavior under physics/input tests and a representative sample scene. |
| **PARTIAL** | Animation | Animation assets, playback, blending, events, state graphs, and later IK. | Asset format/import/runtime binding must exist before Animation Editor work. |
| **PARTIAL** | Particles/VFX | GPU particle emitter, render integration, authoring controls, and lifecycle/culling policy. | Particle emitter budget validation and rollback are proven; ParticleRuntimeUVE provides bounded CPU emission/update state with finite inputs, deterministic sequence IDs, semi-implicit integration, and lifetime culling; ParticleRenderBridgeUVE copies enabled state into RenderQueueUVE, Renderer3DUVE consumes it through a borrowed per-frame input seam with copied diagnostics, and ParticleDrawRecorderUVE records bounded GPU-independent command DTOs; GPU simulation, camera culling, actual GPU draw submission, final particle shader/backend integration, authoring controls, and measured renderer/compute design remain partial. |
| **PARTIAL** | Audio gameplay | Source/listener binding, music/SFX routing, attenuation, mixer groups, and effects. | AudioSource value validation, spatial distance semantics, and invalid-scene rollback are proven; listener binding, routing, mixer groups, effects, streaming, and asset resolution remain partial. |

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
| **One scene model** | Gameplay, editor, serializer, visual scripting, and physics operate on the same scene/entity ownership model. The Visual Scripting canvas now has copied native palette descriptors, managed search filtering, right-click node search, keyboard/click selection, cursor-position insertion, eight production Vector3 graph descriptors, a pure typed Vector3 math kernel, bounded typed Vector3 runtime storage, explicit dependency-aware VM dispatch for all eight Vector3 node IDs, and per-entity typed runtime tick execution; broader runtime coverage remains partial. |
| **Deterministic authoring** | Commands, simulation state, saves, and procedural seeds must have explicit replay/validation behavior where appropriate. |
| **Sample project last** | A full sample game becomes a completion proof after the required runtime/content systems exist; it is not a substitute for their implementation. |
