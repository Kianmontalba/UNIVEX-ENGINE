# UNIVEX ENGINE — ADDITIONAL CORE & AAA ROADMAP

> **Consolidated status authority:** [`ROADMAP_INDEX.md`](ROADMAP_INDEX.md) is the single concise execution view. This document remains the detailed domain history and evidence record; status changes must be synchronized with the consolidated roadmap.



> **Purpose:** Additional capabilities to strengthen UNIVEX as a production/AAA-oriented engine. No Increment numbers are assigned so the implementation agent can determine the correct order. Existing roadmap capabilities should be integrated rather than duplicated.

---

## **Motion Query Inventory Execution Authority**

The complete filename inventory supplied for Motion Query/Motion Matching is maintained in [`MOTION_QUERY_INVENTORY_ROADMAP.md`](MOTION_QUERY_INVENTORY_ROADMAP.md). That document preserves every original filename, adds the UVE-native `_uve.h`/`_uve.cpp` target, maps each item to `engine/plugins/Animation/motion_query/`, and records whether the work is **COMPLETED**, **PARTIAL**, or **PLANNED**. It is an implementation planning authority, not permission to copy foreign-engine code blindly. Existing UNIVEX runtime, editor, diagnostics, and plugin contracts remain authoritative, and `control_rig` remains the sibling plugin under `engine/plugins/Animation/`.

---

## **Animation Authoring & Runtime**

| Capability | What it adds |
|---|---|
| **AnimationTree / Animation Graph** | An engine-native runtime graph for combining and controlling animation clips. It should support graph evaluation, state flow, blending, parameters, transitions, layered animation, and integration with the AnimationPlayer/animation database. |
| **AnimationTree Node Library** | Core nodes should include Animation Clip/Player, Blend, Blend1D, Blend2D, Blend3, Additive Blend, Blend by Parameter, State Machine, State, Transition, One-Shot, Time Scale, Time Seek, Sync, Random/Selector, Subtree, Pose Cache, Pose Layer/Mask, Output Pose, and procedural/IK nodes where appropriate. |
| **Animation State Machine** | Authoring and runtime state machines with conditions, transition rules, blend durations, interruption policies, exit time, transition priorities, and debug visualization. |
| **Animation Blend System** | Layered and masked blending, additive poses, upper/lower-body separation, sync groups, animation curves, root-motion handling, and predictable blend evaluation. |
| **Animation Curves & Events** | Typed animation curves, notifies/events, markers, sync points, parameter driving, audio/VFX/gameplay event hooks, and editor inspection. |
| **Animation Retargeting** | Universal source/target mapping with a required-bone profile, missing-root and IK-control generation, reference-pose adaptation, A/T/custom pose conversion, bone-orientation correction, and retarget validation. |

---

## **Control Rig & Procedural Animation**

| Capability | What it adds |
|---|---|
| **Control Rig Authoring** | An editor tool for creating animator-friendly controls that drive skeletons through constraints, IK, transforms, and rig logic. |
| **Control Rig Graph** | Node-based rig logic with transform, math, condition, space conversion, bone read/write, constraints, IK, variables, and custom rig nodes. |
| **Control Rig Controls** | Position/rotation/scale controls, custom shapes, local/world/bone spaces, parent switching, limits, and animator-friendly handles. |
| **IK & Constraint Suite** | Two-bone IK, FABRIK/full-body solving where appropriate, pole vectors, foot IK, hand IK, aim/look-at, parent/orient/position constraints, limits, and space switching. |
| **Animation Baking from Control Rig** | Evaluate the rig across a timeline and bake the resulting poses into an animation clip, preserving curves/events where applicable. |
| **Runtime Control Rig** | Allow selected rig logic to run during gameplay for procedural aiming, weapon IK, foot placement, recoil, look-at, leaning, climbing, and pose correction. Increment 465 hardens the existing TwoBoneIK runtime slice to reject finite control positions whose pole-minus-root vector overflows before fallback projection or pose publication; broader runtime Control Rig behavior remains partial. |

---

## **Core Engine Systems to Add**

| Capability | What it adds |
|---|---|
| **Frame Scheduler / Task Graph** | Dependency-aware jobs for animation, ECS, rendering preparation, physics, streaming, audio, and background asset work, with priorities and safe synchronization. |
| **Unified Time System** | Game time, real time, fixed simulation time, animation time, audio time, pause/time-scale domains, and deterministic fixed-step support; Increment 438 makes TimerUVE retain safe finite-positive max-delta and fixed-timestep configuration by ignoring invalid setter values. |
| **Transform Hierarchy & Pose Evaluation** | Centralized, cache-friendly transform/pose evaluation with dirty propagation, parent/child updates, local/global conversion, and safe multithreaded evaluation. |
| **Resource Lifetime & Handle System** | Stable resource handles, reference/lifetime tracking, dependency-aware unloading, generation IDs, and protection against stale references. |
| **Event / Signal Bus** | Typed engine events with scoped subscriptions, lifecycle safety, queued/immediate modes, and debugging without creating uncontrolled coupling. |
| **Input Action System** | Action mapping, device abstraction, contexts, rebinding, dead zones, chords, input buffering, and integration with gameplay/visual scripting. |
| **Serialization & Schema Versioning** | Versioned resource schemas, migration functions, validation, backward compatibility, and safe project upgrades. |
| **Reflection / Type Metadata** | Consistent runtime/editor metadata for properties, methods, resources, nodes, serialization, visual scripting, inspector generation, and tooling. |
| **Asset Dependency Graph** | Authoritative dependency tracking for scenes, meshes, materials, shaders, animations, scripts, VFX, audio, and generated data. |
| **Derived Data Cache** | Content-addressed cache for imported/compiled/generated data to reduce rebuild and reimport time. |
| **Profiler Framework** | Unified CPU, task, memory, animation, ECS, asset, scripting VM, and rendering instrumentation with capture sessions. |
| **Crash & Diagnostic System** | Crash-safe logging, stack traces, symbols, breadcrumbs, session metadata, assertion reports, and reproducible diagnostic bundles. |

---

## **Rendering / World / Simulation Additions**

| Capability | What it adds |
|---|---|
| **RenderGraph Completion** | Explicit pass/resource dependencies, transient resources, barriers, async opportunities, validation, and visual debugging. |
| **GPU-Driven Scene Submission** | GPU instance buffers, indirect draws, visibility/LOD integration, and reduced CPU draw submission overhead. |
| **Shader / Material Graph** | Typed visual shader authoring, material functions, previews, compilation diagnostics, caching, and platform variants; Increment 447 now rejects unknown `ShaderStageUVE` values before Null/OpenGL resource publication while retaining the documented Compute and Geometry enum values; Increment 448 now rejects unknown `BufferUsageUVE` values before Null/OpenGL buffer publication while retaining the documented Vertex, Index, and Uniform usages; Increment 449 now rejects unknown color/depth `LoadOpUVE` values before Null/OpenGL command-buffer publication or render-pass state mutation while retaining Clear, Load, and DontCare; Increment 450 now rejects unknown `VertexAttributeFormatUVE` values before Null/OpenGL shader-linked or binary pipeline publication while retaining Float2, Float3, and Float4; Increment 451 now rejects unknown `PipelineBlendModeUVE` values before Null/OpenGL shader-linked or binary pipeline publication while retaining Opaque and SourceAlphaOver; Increment 452 now rejects unknown `PrimitiveTopologyUVE` values before Null/OpenGL shader-linked or binary pipeline publication while retaining Triangles; Increment 453 now rejects buffer initialData larger than desc.sizeBytes before Null/OpenGL buffer publication; Increment 454 now rejects zero vertexStride when vertex attributes are present before GlRenderDeviceUVE shader-linked or binary pipeline allocation, while preserving empty-layout fullscreen pipelines and Null’s documented bookkeeping-only zero-stride behavior; Increment 455 now rejects non-empty OpenGL layouts whose attribute offset plus format width exceeds vertexStride before shader-linked or binary pipeline allocation; Increment 456 now rejects non-empty OpenGL pipelines whose uint32 vertexStride exceeds signed GLsizei before shader-linked or binary allocation, while preserving empty-layout fullscreen pipelines and Null bookkeeping behavior; Increment 457 now rejects DrawUVE vertexCount and DrawIndexedUVE indexCount values above signed GLsizei before OpenGL draw issuance while preserving Null’s full uint32 command recording; Increment 458 now caches GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS and rejects BindTextureUVE slots outside the driver-reported range before OpenGL texture-unit activation while preserving valid texture binding and Null recording behavior; Increment 459 now caches GL_MAX_UNIFORM_BUFFER_BINDINGS and rejects BindUniformBufferUVE slots outside the driver-reported range before OpenGL uniform-buffer binding while preserving valid slot-zero binding and Null recording behavior; Increment 460 now rejects CreatePipelineFromBinaryUVE binary spans larger than signed GLsizei before OpenGL program allocation or driver access while preserving the recoverable cache-miss path and Null’s bookkeeping behavior; Increment 461 now rejects CreateBufferUVE sizeBytes values larger than signed GLsizeiptr before OpenGL buffer allocation while preserving valid uint64 descriptor bookkeeping in NullRenderDeviceUVE; Increment 462 now rejects CreateTextureUVE width or height values larger than signed GLsizei before OpenGL texture allocation while preserving valid uint32 descriptor bookkeeping in NullRenderDeviceUVE; Increment 463 now uses subtraction-safe ValidateBufferUpdateUVE range validation so overflowed offsetBytes plus data.size() writes return false in both NullRenderDeviceUVE and GlRenderDeviceUVE before any OpenGL buffer write; Increment 464 now caches GL_MAX_VERTEX_ATTRIBS and rejects OpenGL pipeline vertex layouts exceeding the driver-reported attribute count before shader-linked or binary pipeline allocation while preserving Null’s backend-neutral layout bookkeeping. |
| **Post-Processing Stack** | Exposure, bloom, color grading, ambient occlusion, depth of field, motion blur, fog/volumetrics, and scalable platform tiers. |
| **World Streaming** | Asynchronous world/cell streaming, priorities, dependencies, activation ranges, safe unload, and editor visualization. |
| **Navigation & AI Foundation** | Navmesh, path queries, dynamic obstacles, behavior framework, blackboard, perception, and production debugging. |
| **VFX / GPU Compute Framework** | Particle/VFX runtime, GPU compute abstraction, trails/ribbons, mesh particles, events, simulation resources, and scalable mobile fallbacks. |

---

## **Production Editor & Developer Experience**

| Capability | What it adds |
|---|---|
| **UNIVEX Editor Design System** | Centralized icons and visual language for Node3D, Scene Tree, Inspector, toolbar, visual scripting, animation, Control Rig, materials, and debugging tools. |
| **Animation Editor** | Timeline, curve editor, dope sheet, pose editing, animation events, markers, blend preview, root-motion inspection, and clip management. |
| **AnimationTree Editor** | Visual graph editor with node palette, search, connections, parameter panels, state-machine visualization, live preview, and runtime debugging. |
| **Control Rig Editor** | Viewport controls, rig graph, control selection, constraint visualization, pose tools, keying, baking, and rig debugging. |
| **Dependency / Reference Inspector** | Find references, dependencies, circular links, orphaned resources, and safe refactoring operations. |
| **Automated Editor Tests** | Headless and interactive regression tests for serialization, graph editing, asset operations, animation graphs, Control Rig, and UI state. |

---

## **Production Networking / Audio / Packaging**

| Capability | What it adds |
|---|---|
| **Replication & Prediction Framework** | Relevance-based replication, ownership, RPC validation, client prediction, reconciliation, interpolation, and dedicated-server support. |
| **Production Audio Graph** | Mixer buses, routing, effects, snapshots, spatialization abstraction, attenuation, occlusion hooks, and runtime diagnostics. |
| **Asset Cooking & Build Graph** | Deterministic cooking, dependency closure, platform variants, incremental builds, packaging, chunking, and patch manifests. |
| **Compatibility & Migration System** | Engine/project version compatibility checks, resource migrations, plugin compatibility, and safe upgrade tooling. |

---

## **QA & Core-Engine Hardening**

| Capability | What it adds |
|---|---|
| **Deterministic Replay** | Record inputs/state and replay sessions for debugging, regression tests, animation verification, and multiplayer reproduction. |
| **Soak / Stress Testing** | Long-running tests for editor, runtime, streaming, animation, asset hot reload, networking, memory, and resource lifetime. |
| **Performance Budgets** | Explicit CPU/GPU/memory/asset/package budgets per platform, with automated regression thresholds. |
| **Capability & Platform Matrix** | Runtime detection of GPU/CPU/platform features and controlled feature tiers/fallbacks, especially for Android/mobile. |

---

## **Recommended Dependency Logic**

Manus should order these by dependency rather than by the order shown here. A sensible dependency chain is: 
1. Core type/reflection/resource/transform foundations
2. Animation runtime
3. AnimationTree
4. Control Rig/IK
5. Retargeting integration
6. Motion matching/procedural animation
7. Editor tooling
8. Profiling/QA

*Rendering, VFX, world streaming, networking, audio, and packaging can proceed in parallel where their foundations permit.*

## **Key Design Rule**

> The engine should avoid creating isolated systems. AnimationTree, Control Rig, Retargeting, IK, Motion Matching, Visual Scripting, Sequencer, and runtime animation must share the same skeleton/pose representation and evaluation pipeline. This prevents multiple incompatible animation systems from developing.

---

## **Motion Query / Motion Matching Integration — Complete File Inventory**

The following inventory contains UNIVEX-native target filenames only. These names are not copied source identities or instructions to reproduce another engine's architecture. Implementation must be written from publicly documented Motion Matching techniques—such as the original Ubisoft GDC talk, published papers, and general nearest-neighbor search literature—not by referencing or reproducing any other engine's actual source code, even indirectly, even if the underlying concept originated there. Each target must be mapped to the appropriate UNIVEX subsystem, checked for duplication or obsolete dependencies, and independently verified.

### **Build / Configuration**
| File | Integration status |
|---|---|
| CMakeLists.txt | **COMPLETED** — Increment 313 verified the existing Animation plugin CMake target compiles the organized Motion Query runtime/editor sources and exports the correct include roots. |
| Config/PluginDescriptor.json | **COMPLETED** — Increment 313 added bounded JSON parsing, semantic version checks, relative layout validation, and parity against the native Motion Query descriptor. |

### **Editor — Private**
| File | Integration status |
|---|---|
| motion_matching_graph_node_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_history_collector_graph_node_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_column_editor_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_customization_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_database_asset_browser_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_database_asset_browser_uve.h | To audit → adapt → integrate → verify |
| motion_query_database_asset_list_item_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_database_asset_tree_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_database_asset_tree_node_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_database_data_details_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_database_editor_mode_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_database_editor_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_database_editor_clipboard_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_database_editor_clipboard_uve.h | To audit → adapt → integrate → verify |
| motion_query_database_editor_commands_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_database_editor_reflection_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_database_editor_utils_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_database_editor_utils_uve.h | To audit → adapt → integrate → verify |
| motion_query_database_factory_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_database_preview_scene_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_database_view_model_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_database_viewport_client_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_debugger_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_debugger_uve.h | To audit → adapt → integrate → verify |
| motion_query_debugger_database_columns_uve.h | To audit → adapt → integrate → verify |
| motion_query_debugger_database_row_uve.h | To audit → adapt → integrate → verify |
| motion_query_debugger_database_row_data_uve.h | To audit → adapt → integrate → verify |
| motion_query_debugger_database_view_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_debugger_database_view_uve.h | To audit → adapt → integrate → verify |
| motion_query_debugger_reflection_uve.h | To audit → adapt → integrate → verify |
| motion_query_debugger_settings_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_debugger_settings_uve.h | To audit → adapt → integrate → verify |
| motion_query_debugger_track_creator_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_debugger_view_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_debugger_view_uve.h | To audit → adapt → integrate → verify |
| motion_query_debugger_view_model_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_debugger_view_model_uve.h | To audit → adapt → integrate → verify |
| motion_query_editor_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_interaction_asset_editor_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_interaction_asset_factory_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_mesh_component_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_mesh_component_uve.h | To audit → adapt → integrate → verify |
| motion_query_normalization_set_factory_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_schema_factory_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_asset_definitions_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_asset_definitions_uve.h | To audit → adapt → integrate → verify |
| motion_query_database_viewport_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_database_viewport_toolbar_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_diagnostics_analyzer_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_diagnostics_analyzer_uve.h | To audit → adapt → integrate → verify |
| motion_query_diagnostics_module_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_diagnostics_module_uve.h | To audit → adapt → integrate → verify |
| motion_query_diagnostics_provider_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_diagnostics_provider_uve.h | To audit → adapt → integrate → verify |

### **Editor — Public**
| File | Integration status |
|---|---|
| motion_matching_graph_node_uve.h | To audit → adapt → integrate → verify |
| motion_query_history_collector_graph_node_uve.h | To audit → adapt → integrate → verify |
| multi_animation_asset_editor_uve.h | To audit → adapt → integrate → verify |
| motion_query_column_editor_uve.h | To audit → adapt → integrate → verify |
| motion_query_customization_uve.h | To audit → adapt → integrate → verify |
| motion_query_database_asset_list_item_uve.h | To audit → adapt → integrate → verify |
| motion_query_database_asset_tree_uve.h | To audit → adapt → integrate → verify |
| motion_query_database_asset_tree_node_uve.h | To audit → adapt → integrate → verify |
| motion_query_database_data_details_uve.h | To audit → adapt → integrate → verify |
| motion_query_database_editor_mode_uve.h | To audit → adapt → integrate → verify |
| motion_query_database_editor_uve.h | To audit → adapt → integrate → verify |
| motion_query_database_editor_commands_uve.h | To audit → adapt → integrate → verify |
| motion_query_database_editor_reflection_uve.h | To audit → adapt → integrate → verify |
| motion_query_database_factory_uve.h | To audit → adapt → integrate → verify |
| motion_query_database_preview_scene_uve.h | To audit → adapt → integrate → verify |
| motion_query_database_view_model_uve.h | To audit → adapt → integrate → verify |
| motion_query_database_viewport_client_uve.h | To audit → adapt → integrate → verify |
| motion_query_debugger_track_creator_uve.h | To audit → adapt → integrate → verify |
| motion_query_editor_uve.h | To audit → adapt → integrate → verify |
| motion_query_interaction_asset_editor_uve.h | To audit → adapt → integrate → verify |
| motion_query_interaction_asset_factory_uve.h | To audit → adapt → integrate → verify |
| motion_query_normalization_set_factory_uve.h | To audit → adapt → integrate → verify |
| motion_query_schema_factory_uve.h | To audit → adapt → integrate → verify |
| motion_query_database_viewport_uve.h | To audit → adapt → integrate → verify |
| motion_query_database_viewport_toolbar_uve.h | To audit → adapt → integrate → verify |

### **Runtime — Private**
| File | Integration status |
|---|---|
| motion_matching_animation_node_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_history_collector_animation_node_uve.cpp | To audit → adapt → integrate → verify |
| kd_tree_uve.cpp | To audit → adapt → integrate → verify |
| motion_matching_animation_node_library_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_animation_notifies_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_asset_definitions_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_asset_indexer_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_database_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_database_config_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_database_sampler_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_derived_data_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_feature_base_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_feature_body_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_feature_trajectory_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_history_uve.cpp | To audit → adapt → integrate → verify — Increment 444 adds X/Y/Z-only mirror-axis validation before staged frame publication, with unknown-axis failure-atomic regression coverage. |
| motion_query_library_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_module_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_normalization_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_schema_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_search_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_settings_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_diagnostics_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_asset_sampler_uve.cpp | To audit → adapt → integrate → verify |
| motion_query_database_viewport_uve.cpp | To audit → adapt → integrate → verify |

### **Runtime — Public**
| File | Integration status |
|---|---|
| motion_matching_animation_node_uve.h | To audit → adapt → integrate → verify |
| motion_query_history_collector_animation_node_uve.h | To audit → adapt → integrate → verify |
| kd_tree_uve.h | To audit → adapt → integrate → verify |
| motion_matching_animation_node_library_uve.h | To audit → adapt → integrate → verify |
| motion_query_animation_notifies_uve.h | To audit → adapt → integrate → verify |
| motion_query_asset_definitions_uve.h | To audit → adapt → integrate → verify |
| motion_query_asset_indexer_uve.h | To audit → adapt → integrate → verify |
| motion_query_database_uve.h | To audit → adapt → integrate → verify |
| motion_query_database_config_uve.h | To audit → adapt → integrate → verify |
| motion_query_database_sampler_uve.h | To audit → adapt → integrate → verify |
| motion_query_derived_data_uve.h | To audit → adapt → integrate → verify |
| motion_query_feature_base_uve.h | To audit → adapt → integrate → verify |
| motion_query_feature_body_uve.h | To audit → adapt → integrate → verify |
| motion_query_feature_trajectory_uve.h | To audit → adapt → integrate → verify |
| motion_query_history_uve.h | To audit → adapt → integrate → verify — Increment 444 documents unknown raw mirror-axis rejection and unchanged output on failure. |
| motion_query_library_uve.h | To audit → adapt → integrate → verify |
| motion_query_module_uve.h | To audit → adapt → integrate → verify |
| motion_query_normalization_uve.h | To audit → adapt → integrate → verify |
| motion_query_schema_uve.h | To audit → adapt → integrate → verify |
| motion_query_search_uve.h | To audit → adapt → integrate → verify |
| motion_query_settings_uve.h | To audit → adapt → integrate → verify |
| motion_query_diagnostics_uve.h | To audit → adapt → integrate → verify |
| motion_query_asset_sampler_uve.h | To audit → adapt → integrate → verify |
