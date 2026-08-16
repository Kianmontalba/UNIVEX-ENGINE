# UNIVEX ENGINE — ADDITIONAL CORE & AAA ROADMAP

> **Purpose:** Additional capabilities to strengthen UNIVEX as a production/AAA-oriented engine. No Increment numbers are assigned so the implementation agent can determine the correct order. Existing roadmap capabilities should be integrated rather than duplicated.

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
| **Runtime Control Rig** | Allow selected rig logic to run during gameplay for procedural aiming, weapon IK, foot placement, recoil, look-at, leaning, climbing, and pose correction. |

---

## **Core Engine Systems to Add**

| Capability | What it adds |
|---|---|
| **Frame Scheduler / Task Graph** | Dependency-aware jobs for animation, ECS, rendering preparation, physics, streaming, audio, and background asset work, with priorities and safe synchronization. |
| **Unified Time System** | Game time, real time, fixed simulation time, animation time, audio time, pause/time-scale domains, and deterministic fixed-step support. |
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
| **Shader / Material Graph** | Typed visual shader authoring, material functions, previews, compilation diagnostics, caching, and platform variants. |
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

The following inventory is taken from the supplied `motion_query_foundation.zip`. All listed files must be reviewed during integration. They are an implementation inventory, not instructions to blindly copy the original architecture. Each file should be mapped to the appropriate UNIVEX subsystem, adapted to UNIVEX conventions, and verified for duplication or obsolete dependencies.

### **Build / Configuration**
| File | Integration status |
|---|---|
| CMakeLists.txt | To audit → adapt → integrate → verify |
| Config/PluginDescriptor.json | To audit → adapt → integrate → verify |

### **Editor — Private**
| File | Integration status |
|---|---|
| AnimGraphNode_MotionMatching.cpp | To audit → adapt → integrate → verify |
| AnimGraphNode_MotionQueryHistoryCollector.cpp | To audit → adapt → integrate → verify |
| Chooser_MotionQueryColumnEditor.cpp | To audit → adapt → integrate → verify |
| MotionQueryCustomization.cpp | To audit → adapt → integrate → verify |
| MotionQueryDatabaseAssetBrowser.cpp | To audit → adapt → integrate → verify |
| MotionQueryDatabaseAssetBrowser.h | To audit → adapt → integrate → verify |
| MotionQueryDatabaseAssetListItem.cpp | To audit → adapt → integrate → verify |
| MotionQueryDatabaseAssetTree.cpp | To audit → adapt → integrate → verify |
| MotionQueryDatabaseAssetTreeNode.cpp | To audit → adapt → integrate → verify |
| MotionQueryDatabaseDataDetails.cpp | To audit → adapt → integrate → verify |
| MotionQueryDatabaseEdMode.cpp | To audit → adapt → integrate → verify |
| MotionQueryDatabaseEditor.cpp | To audit → adapt → integrate → verify |
| MotionQueryDatabaseEditorClipboard.cpp | To audit → adapt → integrate → verify |
| MotionQueryDatabaseEditorClipboard.h | To audit → adapt → integrate → verify |
| MotionQueryDatabaseEditorCommands.cpp | To audit → adapt → integrate → verify |
| MotionQueryDatabaseEditorReflection.cpp | To audit → adapt → integrate → verify |
| MotionQueryDatabaseEditorUtils.cpp | To audit → adapt → integrate → verify |
| MotionQueryDatabaseEditorUtils.h | To audit → adapt → integrate → verify |
| MotionQueryDatabaseFactory.cpp | To audit → adapt → integrate → verify |
| MotionQueryDatabasePreviewScene.cpp | To audit → adapt → integrate → verify |
| MotionQueryDatabaseViewModel.cpp | To audit → adapt → integrate → verify |
| MotionQueryDatabaseViewportClient.cpp | To audit → adapt → integrate → verify |
| MotionQueryDebugger.cpp | To audit → adapt → integrate → verify |
| MotionQueryDebugger.h | To audit → adapt → integrate → verify |
| MotionQueryDebuggerDatabaseColumns.h | To audit → adapt → integrate → verify |
| MotionQueryDebuggerDatabaseRow.h | To audit → adapt → integrate → verify |
| MotionQueryDebuggerDatabaseRowData.h | To audit → adapt → integrate → verify |
| MotionQueryDebuggerDatabaseView.cpp | To audit → adapt → integrate → verify |
| MotionQueryDebuggerDatabaseView.h | To audit → adapt → integrate → verify |
| MotionQueryDebuggerReflection.h | To audit → adapt → integrate → verify |
| MotionQueryDebuggerSettings.cpp | To audit → adapt → integrate → verify |
| MotionQueryDebuggerSettings.h | To audit → adapt → integrate → verify |
| MotionQueryDebuggerTrackCreator.cpp | To audit → adapt → integrate → verify |
| MotionQueryDebuggerView.cpp | To audit → adapt → integrate → verify |
| MotionQueryDebuggerView.h | To audit → adapt → integrate → verify |
| MotionQueryDebuggerViewModel.cpp | To audit → adapt → integrate → verify |
| MotionQueryDebuggerViewModel.h | To audit → adapt → integrate → verify |
| MotionQueryEditor.cpp | To audit → adapt → integrate → verify |
| MotionQueryInteractionAssetEditor.cpp | To audit → adapt → integrate → verify |
| MotionQueryInteractionAssetFactory.cpp | To audit → adapt → integrate → verify |
| MotionQueryMeshComponent.cpp | To audit → adapt → integrate → verify |
| MotionQueryMeshComponent.h | To audit → adapt → integrate → verify |
| MotionQueryNormalizationSetFactory.cpp | To audit → adapt → integrate → verify |
| MotionQuerySchemaFactory.cpp | To audit → adapt → integrate → verify |
| PoseSearchAssetDefinitions.cpp | To audit → adapt → integrate → verify |
| PoseSearchAssetDefinitions.h | To audit → adapt → integrate → verify |
| SMotionQueryDatabaseViewport.cpp | To audit → adapt → integrate → verify |
| SMotionQueryDatabaseViewportToolbar.cpp | To audit → adapt → integrate → verify |
| Trace_MotionQueryTraceAnalyzer.cpp | To audit → adapt → integrate → verify |
| Trace_MotionQueryTraceAnalyzer.h | To audit → adapt → integrate → verify |
| Trace_MotionQueryTraceModule.cpp | To audit → adapt → integrate → verify |
| Trace_MotionQueryTraceModule.h | To audit → adapt → integrate → verify |
| Trace_MotionQueryTraceProvider.cpp | To audit → adapt → integrate → verify |
| Trace_MotionQueryTraceProvider.h | To audit → adapt → integrate → verify |

### **Editor — Public**
| File | Integration status |
|---|---|
| AnimGraphNode_MotionMatching.h | To audit → adapt → integrate → verify |
| AnimGraphNode_MotionQueryHistoryCollector.h | To audit → adapt → integrate → verify |
| IMultiAnimAssetEditor.h | To audit → adapt → integrate → verify |
| MotionQueryColumnEditor.h | To audit → adapt → integrate → verify |
| MotionQueryCustomization.h | To audit → adapt → integrate → verify |
| MotionQueryDatabaseAssetListItem.h | To audit → adapt → integrate → verify |
| MotionQueryDatabaseAssetTree.h | To audit → adapt → integrate → verify |
| MotionQueryDatabaseAssetTreeNode.h | To audit → adapt → integrate → verify |
| MotionQueryDatabaseDataDetails.h | To audit → adapt → integrate → verify |
| MotionQueryDatabaseEdMode.h | To audit → adapt → integrate → verify |
| MotionQueryDatabaseEditor.h | To audit → adapt → integrate → verify |
| MotionQueryDatabaseEditorCommands.h | To audit → adapt → integrate → verify |
| MotionQueryDatabaseEditorReflection.h | To audit → adapt → integrate → verify |
| MotionQueryDatabaseFactory.h | To audit → adapt → integrate → verify |
| MotionQueryDatabasePreviewScene.h | To audit → adapt → integrate → verify |
| MotionQueryDatabaseViewModel.h | To audit → adapt → integrate → verify |
| MotionQueryDatabaseViewportClient.h | To audit → adapt → integrate → verify |
| MotionQueryDebuggerTrackCreator.h | To audit → adapt → integrate → verify |
| MotionQueryEditor.h | To audit → adapt → integrate → verify |
| MotionQueryInteractionAssetEditor.h | To audit → adapt → integrate → verify |
| MotionQueryInteractionAssetFactory.h | To audit → adapt → integrate → verify |
| MotionQueryNormalizationSetFactory.h | To audit → adapt → integrate → verify |
| MotionQuerySchemaFactory.h | To audit → adapt → integrate → verify |
| SMotionQueryDatabaseViewport.h | To audit → adapt → integrate → verify |
| SMotionQueryDatabaseViewportToolbar.h | To audit → adapt → integrate → verify |

### **Runtime — Private**
| File | Integration status |
|---|---|
| AnimNode_MotionMatching.cpp | To audit → adapt → integrate → verify |
| AnimNode_MotionQueryHistoryCollector.cpp | To audit → adapt → integrate → verify |
| KDTree.cpp | To audit → adapt → integrate → verify |
| MotionMatchingAnimNodeLibrary.cpp | To audit → adapt → integrate → verify |
| MotionQueryAnimNotifies.cpp | To audit → adapt → integrate → verify |
| MotionQueryAssetDefinitions.cpp | To audit → adapt → integrate → verify |
| MotionQueryAssetIndexer.cpp | To audit → adapt → integrate → verify |
| MotionQueryDatabase.cpp | To audit → adapt → integrate → verify |
| MotionQueryDatabaseConfig.cpp | To audit → adapt → integrate → verify |
| MotionQueryDatabaseSampler.cpp | To audit → adapt → integrate → verify |
| MotionQueryDerivedData.cpp | To audit → adapt → integrate → verify |
| MotionQueryFeature_Base.cpp | To audit → adapt → integrate → verify |
| MotionQueryFeature_Body.cpp | To audit → adapt → integrate → verify |
| MotionQueryFeature_Trajectory.cpp | To audit → adapt → integrate → verify |
| MotionQueryHistory.cpp | To audit → adapt → integrate → verify |
| MotionQueryLibrary.cpp | To audit → adapt → integrate → verify |
| MotionQueryModule.cpp | To audit → adapt → integrate → verify |
| MotionQueryNormalization.cpp | To audit → adapt → integrate → verify |
| MotionQuerySchema.cpp | To audit → adapt → integrate → verify |
| MotionQuerySearch.cpp | To audit → adapt → integrate → verify |
| MotionQuerySettings.cpp | To audit → adapt → integrate → verify |
| MotionQueryTrace.cpp | To audit → adapt → integrate → verify |
| PoseSearchAssetSampler.cpp | To audit → adapt → integrate → verify |
| SMotionQueryDatabaseViewport.cpp | To audit → adapt → integrate → verify |

### **Runtime — Public**
| File | Integration status |
|---|---|
| AnimNode_MotionMatching.h | To audit → adapt → integrate → verify |
| AnimNode_MotionQueryHistoryCollector.h | To audit → adapt → integrate → verify |
| KDTree.h | To audit → adapt → integrate → verify |
| MotionMatchingAnimNodeLibrary.h | To audit → adapt → integrate → verify |
| MotionQueryAnimNotifies.h | To audit → adapt → integrate → verify |
| MotionQueryAssetDefinitions.h | To audit → adapt → integrate → verify |
| MotionQueryAssetIndexer.h | To audit → adapt → integrate → verify |
| MotionQueryDatabase.h | To audit → adapt → integrate → verify |
| MotionQueryDatabaseConfig.h | To audit → adapt → integrate → verify |
| MotionQueryDatabaseSampler.h | To audit → adapt → integrate → verify |
| MotionQueryDerivedData.h | To audit → adapt → integrate → verify |
| MotionQueryFeature_Base.h | To audit → adapt → integrate → verify |
| MotionQueryFeature_Body.h | To audit → adapt → integrate → verify |
| MotionQueryFeature_Trajectory.h | To audit → adapt → integrate → verify |
| MotionQueryHistory.h | To audit → adapt → integrate → verify |
| MotionQueryLibrary.h | To audit → adapt → integrate → verify |
| MotionQueryModule.h | To audit → adapt → integrate → verify |
| MotionQueryNormalization.h | To audit → adapt → integrate → verify |
| MotionQuerySchema.h | To audit → adapt → integrate → verify |
| MotionQuerySearch.h | To audit → adapt → integrate → verify |
| MotionQuerySettings.h | To audit → adapt → integrate → verify |
| MotionQueryTrace.h | To audit → adapt → integrate → verify |
| PoseSearchAssetSampler.h | To audit → adapt → integrate → verify |
