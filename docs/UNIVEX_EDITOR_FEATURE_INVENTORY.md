# UniVex Engine — Game Editor Feature Inventory

**Purpose.** This document is the source-of-truth inventory for generating a visual reference of the UniVex game editor. It describes the capabilities that actually exist in the canonical engine source and distinguishes them from UI labels, registered descriptors, metadata contracts, and future work. The prompt built from this inventory must never depict an unsupported system as if it were implemented.

**Audited revision:** `f5979f1ac1ff862144aeaef02265f98510c79ff0` on `main`.

> **Core rule:** The generated editor reference must represent a real native UVE workflow. It must not be a decorative mockup that merely writes feature names into panels. Every visible control must correspond to an implemented engine/editor contract, or be clearly shown as disabled, future, metadata-only, or unavailable.

## 1. Status vocabulary

| Status | Meaning for the generated editor reference |
|---|---|
| **VERIFIED / IMPLEMENTED** | Real source implementation, integration, tests, and current editor/runtime reachability exist. It may be shown as an active editor workflow. |
| **REGISTERED CONTRACT** | A real node/component/descriptor is registered and can be represented by the editor contract, but deeper runtime behavior is bounded or not fully implemented. Show only its contract-level properties and do not imply a complete production subsystem. |
| **METADATA-ONLY** | The engine can validate, store, expose, or transport the information, but it does not yet perform the full runtime/editor behavior. Show status/metadata, not a working authoring workflow. |
| **FUTURE / UNSUPPORTED** | Explicitly outside the current supported boundary. Do not show as a working button, fake dialog, fake preview, or automatic success state. |

The consolidated roadmap states that the broad Core Runtime and supported Editor/workflow boundaries are complete, while also listing deeper exclusions explicitly. The completion count is a planning metric, not permission to depict every future AAA feature as implemented.[1]

## 2. Real editor shell and workflow

The current native editor is a C++/Dear ImGui desktop editor with a GLFW/OpenGL presentation path and a headless-safe lifecycle. Its editor session owns selection, hierarchy interaction, transform tools, layout/session state, document dirty state, and visual presentation; EngineCore owns ECS, runtime services, lifecycle, and frame order.[2] [3]

### 2.1 Shell surfaces that may be shown as active

| Surface | Real behavior | Visual treatment for the generated reference |
|---|---|---|
| Native title chrome | UVE logo asset, workspace label, saved/unsaved state, selected count, edit/paused/playing state, editor version text. | Use the supplied exact-preservation UVE logo. Do not redraw or replace it with a new wordmark. |
| Menu row | File, Edit, Assets, GameObject, Component, Window, and Help entry points exist in the native shell. | Use compact menus with realistic spacing; do not fill them with unsupported commands. |
| Main toolbar | Hand/navigation, Scene, Scripting, Move XYZ, Rotate XYZ, Scale XYZ, Global/local space, Play, Stop, Collaboration/account/layers/Android/Layout status surfaces. | Show Move/Rotate/Scale as explicit XYZ tools. Do not depict a three-axis-only gizmo. |
| Scene workspace | Hierarchy, viewport, inspector, and bottom dock. | This is the primary game-editor screen. |
| Asset workspace | Asset/content presentation is reachable from the editor shell. | Show content and inspector facts, not an imaginary asset-authoring suite. |
| Scripting workspace | Native Visual Scripting graph canvas and its editor backend. | Show an actual node graph with typed pins and bounded graph editing affordances. |
| Debug workspace | Developer console and diagnostic-oriented editor surfaces. | Show logs/status and authorized console state, not a full profiler product. |
| Plugin workspace | Plugin-facing editor/bridge surfaces and registered plugin capabilities. | Show plugin capability/status panels only where the underlying contract exists. |
| Bottom dock | Project/Filesystem and Console tabs are real editor panels. | Keep the dock compact and information-dense. |

### 2.2 Empty-scene truthfulness

When no authored scene roots exist, the viewport must show a clean waiting state with **No Scene Open**, **Create Scene**, **Open Scene**, and **Import Asset** routes. It must not create a random object, fake environment, hidden preview sun, decorative ground plane, or fake daylight indicator. The grid and orientation aid may remain as editor-only aids. When a real authored scene is open, the renderer shows the actual scene and actual authored lights.[4]

## 3. Scene editing and node authoring

### 3.1 Verified scene/editor operations

The native editor has real entity selection, multi-selection, hierarchy traversal, hierarchy filtering, rename and reparent flows, inspector drawers, Add Component presentation, transform editing, undo/redo history, scene dirty state, scene load/save boundaries, Play/Pause/Resume/Step/Stop session control, and failure-atomic authoring guards. The editor viewport supports perspective camera navigation, grid/orientation presentation, selection bounds, and pointer-driven transform interaction.[2]

| Operation | Status | What the reference may show |
|---|---|---|
| Create an entity/node | **VERIFIED / IMPLEMENTED** | Add Node workflow with real registered scene descriptors. |
| Select, multi-select, rename | **VERIFIED / IMPLEMENTED** | Hierarchy selection and inspector selection state. |
| Reparent while preserving or changing transform mode | **VERIFIED / IMPLEMENTED** | Hierarchy drag/drop and transform-aware command state. |
| Transform position/rotation/scale | **VERIFIED / IMPLEMENTED** | Inspector fields and viewport gizmos. |
| Undo/redo | **VERIFIED / IMPLEMENTED** | Visible only as a real enabled editor command/state. |
| Scene save/load | **VERIFIED / IMPLEMENTED** | `.uvescene` document workflow; no fake instant success. |
| Play mode | **VERIFIED / IMPLEMENTED** | Play, pause, resume, single-step, stop, snapshot restore, selection-intent restore. |
| Prefab save/instantiate/refresh | **VERIFIED / IMPLEMENTED** | `.uveprefab` source/revision/conflict workflow with explicit Merge Required semantics. |
| Native file picker | **FUTURE / UNSUPPORTED** | Do not draw a fake operating-system dialog. Route to the real content/browser boundary or mark unavailable. |
| Arbitrary reflection inspector | **FUTURE / UNSUPPORTED** | Use registered component drawers and typed properties only. |

### 3.2 Scene node registry: 38 descriptors

The actual registry contains 38 scene descriptors. **37 are marked library-creatable; `AnimationTree` is deliberately not library-creatable.** The registry carries a type ID, display name, category, runtime owner, authored contracts, and creatable flag.[5]

| Category | Library-creatable node descriptors |
|---|---|
| Scene | `empty`, `marker_3d` |
| Physics | `area_3d`, `ray_cast_3d`, `static_body_3d`, `animatable_body_3d`, `character_body_3d`, `collider_3d`, `rigid_body_3d` |
| Navigation | `navigation_region_3d`, `navigation_agent_3d` |
| Animation | `skeleton_3d`, `bone_attachment_3d`, `animation_player`; `animation_tree` is registered but **not** library-creatable |
| Camera | `spring_arm_3d` |
| Combat | `hitbox_3d`, `hurtbox_3d`, `projectile_3d` |
| Gameplay | `interaction_area_3d`, `spawn_point_3d` |
| Rendering | `world_environment_3d`, `reflection_probe_3d`, `decal_3d`, `camera_3d`, `mesh_instance_3d`, `box_mesh_3d`, `sphere_mesh_3d`, `plane_mesh_3d`, `light_3d` |
| Optimization | `lod_group_3d`, `occluder_3d`, `visibility_region_3d` |
| World | `level_streamer_3d`, `world_partition_3d` |
| Audio | `audio_source_3d` |
| VFX | `particle_emitter_3d` |
| Logic | `script` |

The descriptors are real editor contracts, but a visual prompt must not imply that every descriptor has the same runtime depth. For example, a registered `NavigationRegion3D` or `WorldPartition3D` may be shown in an Add Node catalog and inspector contract; do not show a completed navigation solver, streaming backend, or world-partition runtime unless source evidence is added later.

### 3.3 Component inspector families

The current source contains component contracts for the following authoring families. The editor may show typed inspector sections for these families where the corresponding component is attached:

| Component family | Real editor/runtime boundary |
|---|---|
| Name and hierarchy | Entity naming, root/child relationships, hierarchy traversal. |
| Transform/world transform | Position, rotation, scale, parent-relative and world-space relationships. |
| Camera | Camera component and EngineCore camera-system reachability. |
| Mesh and primitive mesh | Mesh component, GUID-backed asset reference, Box/Sphere/Plane primitive authoring, renderer reachability. |
| Light | Authored light component; actual scene lighting and shadows come from ECS-authored lights only. |
| Collider/rigid body | Collider shape/material and rigid-body authoring contracts; fixed-step physics integration. |
| Area/raycast/character/interaction/combat | Registered expanded 3D node component contracts and bounded collision/query ownership. |
| Audio source | Audio source component, source-system reachability, bounded PCM/gain/attenuation contracts. |
| Particle emitter | Particle emitter component and EngineCore particle-runtime/render handoff. |
| Script | Script component and Visual Scripting/runtime attachment boundary. |
| AnimationPlayer | Validated project-relative authored clip identity and deterministic playback settings. |
| Skeleton/bone attachment | Registered skeleton/attachment contracts and plugin-facing animation foundations; do not imply full raw animation decode. |
| Prefab instance | Source envelope revision, clean-instance refresh, local-override conflict facts, merge/force-refresh decisions. |

## 4. Viewport and rendering capabilities

### 4.1 Active viewport features

The current renderer path is OpenGL plus a null/headless render device. The render stack includes render device, command buffer, shader manager, render system, camera system, mesh renderer, light system, Renderer3D, RenderGraph/Queue/System contracts, primitive geometry, texture/material/shader handles, PBR/material/texture/shader foundations, and shadow/cascade foundations.[1] [6]

The generated reference may show:

1. A perspective 3D viewport with a neutral charcoal editor theme.
2. An editor grid and orientation widget.
3. Actual authored mesh geometry and materials.
4. Actual authored directional/point/spot-style light data only if those light types are in the current authored scene fixture.
5. Real shadows where the scene and renderer path provide them.
6. Selection bounds and the active transform gizmo.
7. A clean dark empty scene when no authored lights or scene objects exist.
8. A small empty-state card instead of fake scene content.

### 4.2 Six-axis transform gizmo contract

The gizmo is not merely three visible labels. The editor has separate Move XYZ, Rotate XYZ, and Scale XYZ modes with pointer/ray hit testing, axis selection, drag updates, commit/cancel behavior, command history, and undo coverage. Translation, rotation, and scale each operate over X/Y/Z axes.[2]

| Mode | Visible handles | Required visual truth |
|---|---|---|
| Move XYZ | X, Y, Z axes and translation planes where applicable | Use red/green/blue axis convention and show the active axis/plane state. |
| Rotate XYZ | X, Y, Z rotation rings | Show three rings around the selected object; do not collapse to one generic rotate icon. |
| Scale XYZ | X, Y, Z scale handles | Show axis scale interaction and preserve uniform/axis semantics only if the current control exposes them. |

The current viewport is perspective-oriented. **Orthographic camera behavior is not a verified current editor capability and must not be presented as an active control.**

### 4.3 Rendering capabilities that must not be invented

Do not depict active Bloom, ambient occlusion, temporal anti-aliasing, exposure controls, GPU-driven submission, deferred/Forward+ switching, advanced skinning, occlusion culling, dynamic resolution, baked lightmaps, decals/impostor production pipelines, Vulkan/DirectX/Metal runtime backends, or mobile visual quality tiers as completed features. The roadmap explicitly records these as future work beyond the current supported boundary.[1]

## 5. Filesystem, assets, import, and project workflow

### 5.1 Filesystem panel

The Filesystem panel is a real project-content index backed by `ProjectFileIndexUVE` and an engine-owned `ProjectChangeWatcherUVE`. The canonical editor now consumes the watcher snapshot after EngineCore polling, automatically refreshes the read-only index on first initialization or a new watcher sequence, acknowledges ordinary journal entries only after a successful index replacement, and exposes only truthful failure/retry/rescan state.[7]

The visual reference should show:

- A compact `FILESYSTEM | N entries | auto` header.
- No large **Review Changes** section.
- No normal manual **Refresh** button.
- A small **Retry** action only in a genuine scan-failure state.
- Folder rows with the supplied exact-preservation folder artwork.
- Textual semantic type labels for files unless a real asset preview exists.
- Breadcrumb navigation, current-folder filtering, type filtering, clear filters, selected-entry facts, registered GUID information, and folder-first browsing.

The tracked SVG assets are preserved unchanged. They are embedded-image wrappers whose exact payloads were verified against the supplied source pixels. The native editor uses display-fit derivatives generated from those exact pixels; this is a display adaptation, not a redesign. The prompt must say **use the supplied exact-preservation UVE logo and folder SVG artwork; do not redraw, recolor, stylize, or replace them**.

### 5.2 Asset identity and import

The supported asset boundary includes:

| Capability | Supported current boundary |
|---|---|
| Virtual paths and project root | Yes; normalized, read-only project indexing and virtual-file conventions. |
| GUID-backed identity | Yes; AssetDatabase correlation and registered-entry presentation. |
| Typed envelopes | Mesh, Texture, Material, Shader, Audio, and Animation envelope boundaries. |
| Model/image import bridges | Bounded BMP, PNG, TGA, JPEG, OBJ, glTF, and MTL paths. |
| Shader import | Bounded shader bridge and typed shader envelope. |
| Audio import | Bounded WAV/PCM16 metadata/decoder boundary. |
| Animation asset | `.uveanim` JSON clip serialization and validated project-relative clip identity. |
| Import queue | Explicit queue with deterministic bounded progression; editor may show queued/running/succeeded/failed state. |
| Hot reload | Existing loaded-asset hot reload foundation; do not imply arbitrary live code reload. |
| Derived data | Generation-aware cache validation and stale-artifact invalidation. |
| Project checks | Deterministic project health validation. |
| Bundle format | Asset bundle pack/unpack/scan boundary. |

Do not show raw FBX/DAE codecs, full skeletal animation decoding/retargeting from arbitrary source formats, broad glTF scene/material/image conversion, GPU cooking/compression, thumbnails for every format, marketplace downloads, or automatic importer success as current features.[1]

### 5.3 Project descriptor and release boundary

The `.uveditor` file is a schema-versioned metadata/reference descriptor containing bounded project identity, engine version, revision, and normalized relative references to content root, asset database, and settings authorities. It is not a packed project, does not embed scenes/assets, does not authenticate users, and does not generate `.exe`, `.apk`, installers, or platform packages.[1]

## 6. Visual Scripting editor and runtime

Native C++ Visual Scripting is a completed current family. It has a node registry, typed graph validation, bounded IR/bytecode, deterministic dependency scheduling, bounded VM dispatch, per-entity typed runtime ticks, persistence, debugger, hot reload, editor/canvas contracts, and typed value families.[1] [8]

### 6.1 Graph editor contract

The graph editor may show:

- A searchable categorized node palette.
- Nodes with display names, type IDs, categories, icons, input/output directions, execution pins, data pins, and typed defaults.
- Valid links and bounded diagnostics.
- Selection, movement, panning, zoom, link creation, and persistence state.
- Compiler/runtime diagnostics tied to node/pin/source context.
- Debugger/trace state where the current debugger contract is represented.
- A native canvas that remains distinct from the 3D scene viewport.

The actual graph bounds are **64 nodes** and **256 links** per graph. The real value families are **Execution, Boolean, Number, Vector2, Vector3, Entity, Asset, Component, Rotation, Transform, Array, Map, Set, and Struct**.[8]

### 6.2 Complete built-in node catalog by category

The current registry contains **171 built-in node definitions**. The following IDs are the real source catalog and may be used for palette/search content. The generated visual reference does not need to show all 171 simultaneously; it must show category names and a representative graph without inventing other node types.

| Category | Count | Real node IDs |
|---|---:|---|
| Animation | 10 | `animation.play`, `animation.stop`, `animation.pause`, `animation.blend`, `animation.blend_space`, `animation.set_speed`, `animation.set_weight`, `animation.montage`, `animation.get_current_animation`, `animation.is_playing` |
| Audio | 7 | `audio.set_volume`, `audio.set_pitch`, `audio.set_3d_position`, `audio.play_sound`, `audio.stop_sound`, `audio.is_playing`, `audio.set_attenuation` |
| Camera | 7 | `camera.get_camera`, `camera.set_position`, `camera.set_rotation`, `camera.look_at`, `camera.set_fov`, `camera.shake`, `camera.set_active` |
| Conversion | 4 | `convert.number_to_boolean`, `convert.boolean_to_number`, `convert.vector2_to_vector3`, `convert.vector3_to_vector2` |
| Debug | 3 | `debug.print`, `debug.warning`, `debug.error` |
| Engine | 2 | `engine.log`, `engine.get_time` |
| Entity | 6 | `entity.spawn`, `entity.destroy`, `entity.find`, `entity.get_entity`, `entity.add_component`, `entity.remove_component` |
| Entity Query | 2 | `query.entity.has_component`, `query.entity.get_component` |
| Flow | 11 | `flow.sequence`, `flow.branch`, `flow.return`, `flow.do_once`, `flow.gate`, `flow.switch`, `flow.event`, `flow.loop`, `flow.for_loop`, `flow.while_loop`, `flow.delay` |
| Input | 8 | `input.key_pressed`, `input.key_released`, `input.key_down`, `input.mouse_position`, `input.mouse_button`, `input.gamepad_button`, `input.get_axis`, `input.get_action` |
| Logic | 10 | `logic.boolean.not`, `logic.boolean.and`, `logic.boolean.or`, `logic.boolean.xor`, `logic.boolean.equal`, `logic.boolean.not_equal`, `logic.boolean.greater`, `logic.boolean.less`, `logic.boolean.greater_equal`, `logic.boolean.less_equal` |
| Math | 39 | `math.float.add`, `math.float.subtract`, `math.float.multiply`, `math.float.divide`, `math.float.modulo`, `math.float.abs`, `math.float.min`, `math.float.max`, `math.float.clamp`, `math.float.power`, `math.float.lerp`, `math.float.remap`, `math.float.sin`, `math.float.cos`, `math.float.tan`, `math.float.sqrt`, `math.float.random`, `math.float.random_range`, `math.vector2.make`, `math.vector2.add`, `math.vector2.subtract`, `math.vector2.multiply`, `math.vector2.length`, `math.vector2.normalize`, `math.vector2.dot`, `math.vector2.distance`, `math.vector2.direction`, `math.vector2.lerp`, `math.vector3.make`, `math.vector3.add`, `math.vector3.subtract`, `math.vector3.multiply`, `math.vector3.dot`, `math.vector3.cross`, `math.vector3.length`, `math.vector3.normalize`, `math.vector3.distance`, `math.vector3.direction`, `math.vector3.lerp` |
| Motion Query | 10 | `motion.query.build`, `motion.query.search`, `motion.query.get_best_match`, `motion.query.set_trajectory`, `motion.query.set_pose`, `motion.query.set_velocity`, `motion.query.set_facing`, `motion.query.set_yaw`, `motion.query.transition`, `motion.query.motion_warp` |
| Physics | 11 | `physics.raycast`, `physics.sphere_cast`, `physics.box_cast`, `physics.capsule_cast`, `physics.overlap`, `physics.apply_force`, `physics.apply_impulse`, `physics.set_velocity`, `physics.get_velocity`, `physics.enable_gravity`, `physics.is_colliding` |
| Rotation | 9 | `math.rotation.make`, `math.rotation.break`, `math.rotation.degrees`, `math.rotation.radians`, `math.rotation.euler`, `math.rotation.quaternion`, `math.rotation.look_at`, `math.rotation.slerp`, `math.rotation.rotate` |
| Transform | 11 | `math.transform.make`, `math.transform.break`, `math.transform.get_position`, `math.transform.set_position`, `math.transform.get_rotation`, `math.transform.set_rotation`, `math.transform.get_scale`, `math.transform.set_scale`, `math.transform.translate`, `math.transform.rotate`, `math.transform.transform_point` |
| Variable | 21 | `variable.make_number`, `variable.get_number`, `variable.set_number`, `variable.make_boolean`, `variable.get_boolean`, `variable.set_boolean`, `variable.make_vector3`, `variable.get_vector3`, `variable.set_vector3`, `variable.make_array`, `variable.get_array`, `variable.set_array`, `variable.make_map`, `variable.get_map`, `variable.set_map`, `variable.make_set`, `variable.get_set`, `variable.set_set`, `variable.make_struct`, `variable.get_struct`, `variable.set_struct` |

The registry’s source implementation is the authority for this count and catalog.[9]

## 7. Physics, collision, and gameplay foundations

### 7.1 Current runtime/editor-visible foundations

The engine exposes collision system, physics system, physics queries, raycast system, bounded physics constraints, area-overlap tracking, character-controller façade, sphere/box/capsule casts, and trajectory collision prediction contracts through EngineServices.[3] The editor may show:

- Collider shape and material properties.
- Rigid-body and constraint properties where the component is attached.
- Raycast/cast/overlap nodes in Visual Scripting.
- CharacterBody3D and interaction/combat node contracts.
- Collision queries and hit results as real runtime concepts.

### 7.2 Explicit non-claims

Do not show a complete replacement physics backend, arbitrary joint graph authoring, full navigation/AI product, replication, prediction, reconciliation, interpolation, or online multiplayer transport as active. Networking currently stops at bounded reliable wire codecs, fragmentation/reassembly, acknowledgement windows, retry backoff, and retransmission policy; it has no sockets, peers, replication, RPC, authentication, or dedicated-server architecture.[1]

## 8. Animation, Motion Query, and Control Rig

Motion Query and Control Rig are completed UVE-native plugin families at their current bounded boundaries, but their editor visualization depth must be represented accurately.

### 8.1 Motion Query current boundary

The plugin includes validated clip-to-motion-library preprocessing, future trajectories, multi-candidate database construction, feature channels, search index/nearest-match behavior, transitions, interaction runtime, runtime telemetry, history, LOD, schema compatibility, asset ingestion, bridge snapshots, and shared trajectory data. Increment 653 adds a time-sampled trajectory stream with broad animation context metadata and optional capsule dimensions shared with physics collision prediction. The current boundary explicitly supports contexts such as locomotion, turns, hop, slide, jump, fall, light/heavy landings, takedown, ragdoll, combat, interaction, and custom contexts, but does **not** claim playback orchestration, full ragdoll/takedown state machines, raw animation-format import, or direct renderer-owned viewport drawing.[1]

For a generated reference, show Motion Query as a plugin/editor data workflow with database entries, trajectories, pose/match facts, and validation status. Do not show it as a fully separate production viewport unless a later increment implements direct visualization.

### 8.2 Control Rig current boundary

Control Rig includes deterministic mapped-skeleton autorig generation; root/spine/head, hand IK/pole, and foot IK/pole controls; animator-visible Box/Circle/Arrow control shapes; role bindings; bounded validation; solver integration; reset and side-mirror operations; skeleton-matched bake transfer; native authoring selection/tool/transform sessions; chronological bake-sample capture; revisioned viewport-ready snapshots; and native/stdio bridge exposure.[1]

For a generated reference, show control shapes and rig facts only when attached to a real skeleton/rig authoring context. Do not place bones inside an empty `Skeleton3D` node or imply that a mesh automatically creates a skeleton. A skeleton without mesh/bone data remains empty.

## 9. Audio, input, save, window, and platform services

| Family | Current editor-facing truth |
|---|---|
| Audio | Audio device/system/source services, bounded PCM16 refill planning, gain-effect scheduling, source handles, WAV metadata/decoder boundary. No claim of complete platform audio backend or mixer-production authoring UI. |
| Desktop input | Input/action/binding layer with key/mouse/action concepts. |
| Gamepad/mobile input | EngineServices exposes bounded gamepad/mobile snapshots and gesture adapter contracts; do not show OS permission/lifecycle/haptics backends as complete. |
| Save/checkpoint | SaveGameSystem framing, metadata validation, bounded compression, migration dispatch, metadata-only reads, scratch-scene rollback, atomic publication, numbered/autosave/manual slots, and checkpoint policy. |
| Windowing | GLFW desktop window/context, close/resize/focus/input polling, VSync, fullscreen validation, monitor enumeration, native diagnostics, and a tested headless/null substitution. |
| `.uveditor` release metadata | Project identity/revision/content/settings/database references and fail-closed atomic save/load; no binary cooking or packaging. |

## 10. Foundational engine services behind the editor

The editor is backed by real EngineCore composition for frame lifecycle, timer/fixed-step update, ECS entity management, scene graph, event dispatch, memory management, thread-pool/job ownership, command-line/configuration services, virtual file system, and dependency-safe service shutdown. These are runtime foundations rather than decorative editor panels. A generated reference may show concise project/runtime status, frame or play-state indicators, and command/configuration entry points, but it must not invent a memory profiler, job-graph visualizer, event-bus dashboard, or runtime service panel unless a later editor product implements one.

| Foundation | How it may appear in the editor |
|---|---|
| Frame lifecycle and timer | Play/pause/step state, frame-safe runtime status, or concise simulation state. |
| ECS/entity manager and scene graph | Hierarchy, selection, parenting, transform, component attachment, and runtime scene ownership. |
| Events and input/action layer | Script event/input nodes and gameplay interaction state. |
| Memory manager and thread pool | Stability/runtime status only; no fake profiler or task graph. |
| Configuration and command line | Project settings/status and launch/runtime mode facts. |
| Virtual file system and file system service | Filesystem/content browser and normalized project paths. |

## 11. Diagnostics, security, plugins, and managed bridge

The editor may show:

- Developer Console authorization states: Denied, ReadOnly, and Full.
- Irreversible Shipping denial and caller-labeled principal/session audit records.
- Plugin manifest validation, protocol negotiation, capability policy, generation-checked registration scopes, and busy-safe unregister.
- Project health validation and contract-inventory status.
- Native/stdio/managed bridge transport status.
- Headless/editor-host probe status.

Do not show external identity resolution, audit persistence, dynamic library discovery/loading/unloading, cloud accounts, collaboration servers, marketplace services, or payments as implemented engine features. Those belong to the separate ecosystem/release track.[1]

## 12. What the generated editor reference must not contain

The generated image or design must not contain fake daylight, random startup geometry, hidden scene entities, decorative demo assets presented as user content, fake importer success, fake dialogs, fake sockets/online users, fake asset thumbnails for unsupported formats, fake orthographic controls, fake post-processing controls, fake terrain/world-streaming production workflows, fake package/export buttons, `.exe`/`.apk` generation claims, or third-party engine branding/structure.

The reference should use the neutral-charcoal UVE visual language, restrained text, compact status rows, strong selection states, realistic spacing, and native game-editor hierarchy. It should show the engine’s actual capabilities as working interaction surfaces rather than placing every capability name into a static sidebar.

## References

[1]: https://github.com/Kianmontalba/UNIVEX-ENGINE/blob/f5979f1ac1ff862144aeaef02265f98510c79ff0/docs/ROADMAP_INDEX.md "UniVex Engine consolidated roadmap and supported boundaries"
[2]: https://github.com/Kianmontalba/UNIVEX-ENGINE/blob/f5979f1ac1ff862144aeaef02265f98510c79ff0/engine/editor/include/uve/editor/editor_uve.h "Native EditorUVE public and internal editor contracts"
[3]: https://github.com/Kianmontalba/UNIVEX-ENGINE/blob/f5979f1ac1ff862144aeaef02265f98510c79ff0/engine/core/include/uve/core/engine_services_uve.h "EngineServicesUVE runtime authority accessors"
[4]: https://github.com/Kianmontalba/UNIVEX-ENGINE/blob/f5979f1ac1ff862144aeaef02265f98510c79ff0/engine/editor/src/editor_uve.cpp "Native editor viewport, shell, Filesystem, and truthful empty-state implementation"
[5]: https://github.com/Kianmontalba/UNIVEX-ENGINE/blob/f5979f1ac1ff862144aeaef02265f98510c79ff0/engine/scene/src/nodes/scene_node_registry_uve.cpp "UVE scene-node descriptor registry"
[6]: https://github.com/Kianmontalba/UNIVEX-ENGINE/blob/f5979f1ac1ff862144aeaef02265f98510c79ff0/engine/render/include/uve/render/i_renderer_3d_uve.h "Renderer3DUVE editor/runtime rendering contract"
[7]: https://github.com/Kianmontalba/UNIVEX-ENGINE/blob/f5979f1ac1ff862144aeaef02265f98510c79ff0/engine/asset/include/uve/asset/i_project_change_watcher_uve.h "ProjectChangeWatcherUVE automatic observation and acknowledgment contract"
[8]: https://github.com/Kianmontalba/UNIVEX-ENGINE/blob/f5979f1ac1ff862144aeaef02265f98510c79ff0/engine/scripting/include/uve/scripting/script_graph_uve.h "Typed Visual Scripting graph, pin, value, validation, and bounds contract"
[9]: https://github.com/Kianmontalba/UNIVEX-ENGINE/blob/f5979f1ac1ff862144aeaef02265f98510c79ff0/engine/scripting/src/script_builtin_nodes_uve.cpp "Complete registered UVE Visual Scripting node catalog"
