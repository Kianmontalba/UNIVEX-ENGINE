<div align="center">

<h1><strong>UNIVEX ENGINE — RENDERING ROADMAP</strong></h1>

<strong>RHI, scene rendering, materials, lighting, viewport presentation, and future visual systems</strong>

</div>

> **Visual truth rule:** A renderer or viewport item is `COMPLETED` only when the real runtime output is visible through the intended presentation path and the claim is backed by build/test/smoke evidence. A black-only capture is a diagnostic result, not a visual completion claim.

| Status | Completed foundation | Delivered capability | Ongoing boundary |
|---|---|---|---|
| **COMPLETED** | **Increment 9** | Rendering math, transforms, bounds, planes, and frustum primitives. | Shared math stays backend-agnostic and reusable outside rendering. |
| **COMPLETED** | **Increment 10** | Explicit render-device and command-buffer RHI foundation. | Backends remain behind `IRenderDeviceUVE`; no backend types leak into public consumer APIs. |
| **COMPLETED** | **Increments 11–14** | Camera/frustum culling, render-facing assets, mesh submission, render queue, and `Renderer3DUVE` orchestration. | `Renderer3DUVE` owns the scene-to-window path. |
| **COMPLETED** | **Increment 20** | Real GLFW/OpenGL render execution path alongside Null backends. | Context lifecycle stays owned by the window layer; rendering owns GPU resources only. |
| **COMPLETED** | **Increment 21** | Shader loading, source preprocessing, hot reload, reflection, and binary-cache support. | Shader failure retains last-known-good output where the contract permits. |
| **COMPLETED** | **Increments 22–26** | Textured materials, directional/point/spot lights, PBR uniform plumbing, and shadow mapping. | This is a renderer foundation, not a complete asset-material workflow. |
| **COMPLETED** | **Increments 27–31** | Shadow-aware canonical shader, PCF filtering, camera-fitted cascades, transition blending, and cascade stabilization. | Shadow quality expansion requires profiling and representative scenes. |
| **COMPLETED** | **Increments 32–36** | Tangent-space normal mapping, separate material stages, GGX Smith lighting, deterministic render graph, and tone mapping. | New effects must compose through renderer-owned passes, not EngineCore demo paths. |
| **COMPLETED** | **Increment 58** | Built-in Cube, UV Sphere, and Plane renderer extraction with bounds/culling, material color, and grid feedback. | Built-in primitives are a foundation; real presentation verification remains partial. |
| **COMPLETED** | **Legacy Partial Slice — Typed UVE Envelope Importer Contracts** | The native asset importer now deterministically copy-registers existing `.uvemodel`, `.uvetex`, `.uveshader`, and `.uvemat` envelopes through EngineCore composition. | This closes the envelope reimport boundary only; it does not claim raw model/texture/material parsing or renderer preview generation. |
| **COMPLETED** | **Legacy Partial Slice — Asset Load Failure Diagnostics v1** | AssetManager and AssetHandle now expose copied deterministic failure reasons for unresolved paths, missing files, missing loaders, and loader rejection without transferring ownership. | This supplies failure observability only; renderer fallback resources and asset-preview presentation remain renderer/editor-owned PARTIAL work. |

<div align="center">

<h2><strong>PARTIAL — ACTIVE VIEWPORT AND RENDERER WORK</strong></h2>

</div>

| Status | Priority | Area | Required completion proof |
|---|---:|---|---|
| **COMPLETED** | **1** | **Increment 63 — Viewport Presentation & Render Verification v1** | Renderer-owned viewport presentation now preserves the actual primitive final-output path, adds a restrained blue-gray gradient through the alpha-over editor visual pass, and retains GPU grid/world-axis/orientation feedback without a parallel presentation owner. Renderer tests assert the editor visual pass is recorded; built-in shader physical/embedded parity passes; full native CTest 1013/1013, managed .NET 8 71/71, and a real 30-frame OpenGL/Xvfb editor smoke run passed. |
| **COMPLETED** | **2** | Selection and viewport visual feedback | The renderer-owned editor visual pass publishes bounded selection rectangles, active gizmo axis, and camera-forward orientation facts to the GPU shader; selection outline, RGB orientation widget, and diagnostics remain value-only and presentation-owned. Focused renderer tests assert the pass and uniform publication; full native/managed validation and real OpenGL/Xvfb smoke remain the required regression gate. |
| **PARTIAL** | **3** | Material and mesh asset workflow | Asset-backed mesh/material GUID references appear as copied read-only Inspector facts, existing typed UVE envelopes have deterministic copy-register import contracts, and AssetManager failures expose copied diagnostics. The remaining workflow still requires raw model/texture/material import contracts, renderer fallback-resource selection, registered Content Browser import/reimport actions, and renderer-backed asset previews. |
| **PARTIAL** | **4** | Post-processing | Bloom, ambient occlusion, anti-aliasing, exposure, and project-configured quality only after renderer profiling and pass contracts. |
| **PARTIAL** | **5** | Render-path breadth | Deliberate Forward+/deferred choice, transparent sorting, GPU-driven/compute work, skinning, particles, occlusion, and dynamic resolution. |
| **PARTIAL** | **6** | Platform rendering | Vulkan/DirectX/Metal or OpenGL ES selection only through separately validated backend/toolchain increments. |
| **PARTIAL** | **7** | Mobile visual pipeline | LOD, texture compression, adaptive quality, baked lighting, and light probes after asset/import/cooking foundations. |
| **PARTIAL** | **8** | Advanced visual systems | Decals, billboards/impostors, lightmap baking, screen-space effects, and plugin-provided effects. |

<div align="center">

<h2><strong>PARTIAL — ADVANCED VISUAL SYSTEMS</strong></h2>

</div>

| Status | System | Intended capability | Entry condition |
|---|---|---|---|
| **PARTIAL** | Decals | Projected albedo/normal/PBR marks with pooling, atlas management, culling, and forward/deferred compatibility. | Stable material/mesh asset pipeline and measured pass budget. |
| **PARTIAL** | Billboards and impostors | Camera-facing sprites and generated far-distance mesh substitutes. | Asset cooking, texture atlas workflow, LOD policy, and mobile profiling. |
| **PARTIAL** | Lightmap baking | Static-scene UV, bake, and runtime baked-light data. | Stable static meshes, material import, build/cooking path, and a chosen bake algorithm. |
| **PARTIAL** | Cinematic presentation | Camera/sequence-driven render capture and visually controlled shots. | Real animation, sequencer, asset, and viewport presentation contracts. |

<div align="center">

<h2><strong>RENDERING NON-NEGOTIABLES</strong></h2>

</div>

| Rule | Requirement |
|---|---|
| **No fake showcase** | Generated images or UI mockups never prove a renderer feature. |
| **One render path** | Visible scene content enters through ECS extraction into `Renderer3DUVE`; do not revive a separate EngineCore draw scaffold. |
| **Real capture** | Viewport-facing milestones require an authentic engine capture under the supported desktop/OpenGL environment. |
| **Measured scale** | Shadows, post-processes, dynamic resolution, and GPU-driven work require profiling before performance claims. |
| **Fallback behavior** | Resource/shader/driver failures must preserve diagnosable and safe behavior rather than silently misrepresenting feature state. |
