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
| **PARTIAL** | Core 3D nodes | `Node3D`, mesh/material references, camera variants, directional/point/spot lights, collision shapes, rigid/static/character bodies, areas, audio sources, and script attachments. | Camera component validity and invalid-scene rollback are proven; camera variants, Inspector authoring/repair commands, and each remaining node/component still require runtime behavior, serialization, validation, and tests; no empty node shells. |
| **PARTIAL** | Core 2D/UI nodes | `Node2D`, sprite, label, button, panel, and progress UI elements. | Requires an actual runtime UI/render/input architecture; editor UI does not substitute for game UI. |
| **PARTIAL** | Character and interaction | Kinematic character controller, triggers/areas, component querying, spawning/destruction, and gameplay state conventions. | Defined behavior under physics/input tests and a representative sample scene. |
| **PARTIAL** | Animation | Animation assets, playback, blending, events, state graphs, and later IK. | Asset format/import/runtime binding must exist before Animation Editor work. |
| **PARTIAL** | Particles/VFX | GPU particle emitter, render integration, authoring controls, and lifecycle/culling policy. | Requires measured renderer/compute design and a bounded first effect set. |
| **PARTIAL** | Audio gameplay | Source/listener binding, music/SFX routing, attenuation, mixer groups, and effects. | Requires runtime audio component contracts and automated/non-interactive validation strategy. |

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
| **One scene model** | Gameplay, editor, serializer, visual scripting, and physics operate on the same scene/entity ownership model. |
| **Deterministic authoring** | Commands, simulation state, saves, and procedural seeds must have explicit replay/validation behavior where appropriate. |
| **Sample project last** | A full sample game becomes a completion proof after the required runtime/content systems exist; it is not a substitute for their implementation. |
