================================================================================
                    MESSAGE TO CLAUDE AI — CRITICAL — READ FIRST
================================================================================

Dear Claude,

Before you begin writing a single line of code, please read this message 
carefully and internalize every word. This is not a typical request. This 
is a multi-month, multi-phase game engine development project, and your 
approach to it must reflect that reality.

1. THIS IS A MARATHON, NOT A SPRINT
-------------------------------------
The UniVex Engine is a MASSIVE undertaking. It is comparable in scope to 
the early versions of Godot, Unity, or Unreal Engine. No human team, and 
certainly no single AI session, can build this in one sitting, one day, 
or even one week. The user is FULLY AWARE of this. The user EXPECTS this 
to take many sessions, many weeks, possibly many months.

You must NOT attempt to dump everything into one response. You must NOT 
try to rush through systems just to check boxes. You must NOT cut corners 
or produce placeholder code that "will be fixed later." The user values 
QUALITY over SPEED. A small, working, well-architected system is infinitely 
more valuable than a large, broken, half-finished codebase.

2. THE USER CONTROLS THE PACE
-----------------------------
The user is the project director. YOU are the implementer. The user decides 
which system to build next, which feature to prioritize, and when a session 
ends. Your job is to:
  • Deliver ONE complete, working system per response when possible.
  • Ask clarifying questions when design decisions are ambiguous.
  • Suggest the next logical step but WAIT for user confirmation.
  • Never assume you know better than the user's stated priorities.

If the user says "stop here," you stop. If the user says "let's focus on 
rendering this week," you focus on rendering. You do not push ahead 
unilaterally.

3. MODULAR DELIVERY IS MANDATORY
--------------------------------
Every deliverable must be self-contained and compilable (where applicable). 
Do not deliver:
  ❌ Stub functions with "TODO" comments
  ❌ Half-implemented classes missing critical methods
  ❌ Code that references non-existent systems
  ❌ Headers without implementations

Do deliver:
  ✅ Complete classes with all declared methods implemented
  ✅ Working CMakeLists.txt or build scripts for each module
  ✅ Test cases or usage examples showing the system works
  ✅ Clear separation between engine core, editor, and scripting layers

4. IF YOU HIT A DESIGN AMBIGUITY, ASK
--------------------------------------
This document describes a vision, but visions have gaps. If you encounter 
a design question that isn't answered here — for example:
  • "Should the ECS use archetypes or sparse sets?"
  • "Should the renderer use immediate or retained command buffers?"
  • "What should the default shadow map resolution be?"

...then ASK the user. Do not guess. Do not pick the first option you 
think of. The user has strong opinions and deep knowledge of game 
development. They will tell you exactly what they want.

5. ENGINE FIRST, ECOSYSTEM SECOND
---------------------------------
The priority order is crystal clear:
  Phase 1: C++ Core Engine (rendering, ECS, physics, audio, input)
  Phase 2: Custom File Formats (.uve*) with importers/exporters
  Phase 3: Editor Shell (window, viewport, basic UI)
  Phase 4: Core Nodes (Node3D, Mesh3D, Camera3D, etc.)
  Phase 5: Native C++ Visual Scripting system
  Phase 6: Plugin architecture
  Phase 7: Sample project
  Phase 8: UniVex Hub desktop app
  Phase 9: News website and backend

Do NOT jump to Phase 8 or 9 while Phase 1 is incomplete. The ecosystem 
exists to serve the engine. Without a solid engine, the ecosystem is 
meaningless.

### **Active Execution Roadmaps**

The authoritative active planning entry point is
[`docs/ROADMAP_INDEX.md`](ROADMAP_INDEX.md). It links the complete increment
history and focused Core & Runtime, Rendering, Editor & Workflow, Developer
Tooling, Gameplay & Content, Platform & Release, and Ecosystem roadmaps.

Every focused roadmap records completed work, prioritizes visible viewport
correctness before broader editor expansion, and states implementation,
verification, and scope boundaries for future work. The long-term direction
remains native C++ core/editor systems, native C++ visual scripting, a
separately designed plugin architecture, and sample-project/ecosystem work only
after stable scene/editor foundations. No roadmap authorizes copying third-party
source, UI, APIs, assets, build scripts, or implementation logic.

6. CODE QUALITY STANDARDS
--------------------------
  • Use modern C++20/23. No raw new/delete. RAII everywhere.
  • Every public API must have XML-style documentation comments.
  • Use the UVE suffix religiously on all engine internals.
  • Node names must NEVER have the UVE suffix (user-facing).
  • All files must include the copyright header.
  • No STL containers in hot paths (use custom allocators).
  • Profile before optimizing, but design for performance from the start.

7. TESTING IS NOT OPTIONAL
---------------------------
Every system you deliver must be testable. Provide:
  • Unit tests for math, serialization, and ECS
  • Integration tests for rendering and physics
  • A minimal main.cpp that initializes the engine and runs a blank frame

If you cannot test it, you cannot prove it works. If you cannot prove it 
works, do not deliver it.

8. RESPECT THE USER'S KNOWLEDGE
--------------------------------
The user is an experienced game developer. They have worked with Godot, 
Unreal Engine, Unity, and custom engines. They understand:
  • Engine architecture
  • Rendering pipelines
  • Physics integration
  • Build systems
  • Cross-platform development

Do not talk down to them. Do not over-explain basic concepts. Do not 
patronize. Communicate as a peer.

9. SESSION MANAGEMENT
---------------------
At the end of each deliverable, provide:
  • A summary of what was built
  • What is complete vs. what still needs work
  • The recommended next step
  • Any blockers or questions for the user

This keeps the project organized across sessions.

10. FINAL REMINDER
------------------
You are building a professional-grade game engine. Act like it. Write code 
you would be proud to put in a portfolio. Architect systems that can evolve 
over years, not days. Think about the developer who will use this engine 
in 2027, 2028, and beyond.

The user is patient. The user is committed. The user is in control.

Now, begin.

================================================================================
                    END OF MESSAGE TO CLAUDE
================================================================================


================================================================================
          UNIVEX ENGINE (UVE) — COMPLETE DEVELOPMENT PROMPT
                    Parts 1 through 25 + Ecosystem Plan
================================================================================

DOCUMENT CONTENTS:
  Part 1    — Engine Overview & Architecture
  Part 2    — Custom File Format System (.uve* extensions)
  Part 3    — Copyright Header (Required in Every File)
  Part 4    — Naming Conventions
  Part 5    — Built-in Core Nodes (Minimal Set)
  Part 6    — Plugin System
  Part 7    — C++ Core Engine Systems
  Part 8    — Native C++ Visual Scripting System
  Part 9    — UVE Editor Application (Original Branding)
  Part 10   — Editor Systems Implementation
  Part 11   — Visual Scripting Editor (Detailed)
  Part 12   — Build System & Platform Support
  Part 13   — Sample Project Requirements
  Part 14   — Documentation Deliverables
  Part 15   — Code Quality Requirements
  Part 16   — Cinematic Sequencer / Timeline Editor
  Part 17   — Save / Load System + Checkpoint Manager
  Part 18   — Data Tables / Spreadsheet Import
  Part 19   — Spline System
  Part 20   — Developer Console (In-Game)
  Part 21   — Native Bytecode Program Reload
  Part 22   — Procedural Generation Tools
  Part 23   — Decal System
  Part 24   — Billboard & Impostor System
  Part 25   — Lightmap Baking (For Mobile)
  Part 26   — Ecosystem Architecture Plan
              (UniVex Hub, UniVex News, Account System, Online+Offline)

================================================================================
PART 1 — ENGINE OVERVIEW & ARCHITECTURE
================================================================================

ENGINE NAME: UniVex Engine (UVE)
PRIMARY FOCUS: 3D Game Development
TARGET PLATFORMS: Windows PC, Linux PC, Android Mobile, iOS Mobile
LANGUAGE STACK:
  - Core Engine API: Modern C++ (C++20/23)
  - In-Editor Visual Scripting: Native C++ graph compiler, bytecode VM, and C++ node registry
  - Editor UI: Custom immediate-mode/retained-mode hybrid UI built on engine renderer

CORE PHILOSOPHY:
  - Minimal built-in nodes. Core scene nodes only.
  - Everything else is a Plugin (like Unreal Engine plugins).
  - Editor is independent, responsive, and fits any screen size.
  - Full touch support for Android viewport manipulation.

================================================================================
PART 2 — CUSTOM FILE FORMAT SYSTEM (.uve* extensions)
================================================================================

Every file format must have a binary header starting with magic number: "UVE\0"
Followed by version uint32, then asset type uint32, then compressed payload.

File Extensions:
  .uveproj      — Project file (scenes list, settings, build config)
  .uvescene     — Scene file (entity hierarchy, component data, references)
  .uveprefab    — Prefab file (reusable entity template)
  .uvemodel     — 3D Mesh/Model (vertices, indices, UVs, normals, tangents, bone weights, LODs)
  .uvetex       — Texture (mipmaps, compression format: ASTC/ETC2/BC7/DXT)
  .uvemat       — Material (PBR parameters, shader graph reference, texture slots)
  .uveshader    — Shader source (GLSL for OpenGL/Vulkan, HLSL for DirectX, MSL for Metal)
  .uveanim      — Animation (bone tracks, blend curves, events)
  .uveaudio     — Audio (compressed audio data, loop points, 3D spatial settings)
  .uvescript    — Visual Script Graph (node positions, connections, validated native bytecode program)
  .uveplugin    — Plugin manifest (plugin name, version, dependencies, entry points)
  .uvesettings  — Engine/Editor settings (keybinds, theme, layout, platform prefs)

Importer Requirements:
  - AssetImporterUVE class handles all imports.
  - FBX, OBJ, GLTF/GLB → .uvemodel
  - PNG, JPG, TGA, HDR → .uvetex
  - WAV, OGG, MP3 → .uveaudio
  - Import settings dialog for each type (e.g., normal map flip, LOD generation count).
  - Async import with progress bar.
  - Auto-reimport on source file change (hot reload).

================================================================================
PART 3 — COPYRIGHT HEADER (REQUIRED IN EVERY FILE)
================================================================================

Every .cpp, .h, .hpp, and .mm file outside `engine/core` MUST start with:

// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

Only files in `engine/core` carry the complete proprietary notice, with the centered UVE / UniVex Engine title and no decorative separator lines.

================================================================================
PART 4 — NAMING CONVENTIONS
================================================================================

C++ ENGINE API (UVE suffix REQUIRED):
  Classes:      RenderSystemUVE, AssetManagerUVE, SceneGraphUVE, PhysicsSystemUVE
  Structs:      Vector3UVE, Matrix4x4UVE, VertexDataUVE, RayUVE
  Enums:        RenderModeUVE, PlatformTypeUVE, TextureFormatUVE, LogLevelUVE
  Functions:    InitializeEngineUVE(), LoadSceneUVE(), RenderFrameUVE(), 
                ImportAssetUVE(), SerializeSceneUVE()
  Macros:       UVE_ASSERT(cond), UVE_LOG(level, msg), UVE_API, UVE_INLINE
  Namespaces:   namespace UVE { ... }
  Files:        render_system_uve.h, asset_manager_uve.cpp

NATIVE C++ VISUAL SCRIPTING (UVE suffix REQUIRED):
  Classes:      ScriptGraphUVE, ScriptNodeUVE, ScriptCompilerUVE
  Methods:      CompileGraphUVE(), ExecuteNodeUVE()
  Events:       OnBeginPlayUVE, OnTickUVE, OnCollisionUVE

EDITOR UI (NO UVE suffix on user-facing node names):
  Keep node names clean and industry-standard:
    Node3D, Mesh3D, Camera3D, Light3D, DirectionalLight3D, PointLight3D,
    SpotLight3D, CollisionShape3D, RigidBody3D, CharacterBody3D,
    StaticBody3D, Area3D, AnimationPlayer3D, AudioSource3D, ParticleSystem3D
  Editor classes use UVE suffix internally:
    EditorUVE, ViewportUVE, InspectorUVE, ContentBrowserUVE

================================================================================
PART 5 — BUILT-IN CORE NODES (Minimal Set)
================================================================================

These are the ONLY nodes built into the engine core. Everything else is a plugin.

SCENE NODES:
  Node3D              — Base 3D node with Transform (Position, Rotation, Scale)
  Mesh3D              — Static mesh renderer (requires MeshFilter3D + Material3D)
  MeshFilter3D        — Holds reference to .uvemodel
  Material3D          — Holds reference to .uvemat
  Camera3D            — Perspective/Orthographic camera with FOV, near/far clip
  Light3D             — Base light (abstract)
    ├─ DirectionalLight3D  — Sun/moon light with shadow cascades
    ├─ PointLight3D        — Omni light with attenuation radius
    └─ SpotLight3D        — Cone light with inner/outer angle
  CollisionShape3D    — Physics collision shape (box, sphere, capsule, mesh)
  RigidBody3D         — Dynamic physics body
  StaticBody3D        — Static physics body
  CharacterBody3D     — Kinematic character controller
  Area3D              — Trigger volume (detects overlaps)
  AnimationPlayer3D   — Plays .uveanim files
  AudioSource3D       — 3D positional audio
  ParticleSystem3D  — GPU particle emitter (basic)
  VisualScript3D      — Attaches a .uvescript graph to this node
  Canvas3D            — 3D UI canvas (for in-world UI)

2D NODES (for UI, NOT gameplay):
  Node2D              — Base 2D node
  Sprite2D            — 2D image
  Label2D             — Text label
  Button2D            — Interactive button
  Panel2D             — Container panel
  ProgressBar2D       — Health/loading bar

NOTE: No UVE suffix on any of these node names. They are user-facing.

================================================================================
PART 6 — PLUGIN SYSTEM (The Rest Is A Plugin)
================================================================================

Plugin Architecture:
  - PluginManagerUVE loads .uveplugin files at engine startup.
  - Each plugin is a native C++ DLL/.so with an explicit UVE plugin manifest.
  - Plugins can register: new node types, new editor windows, new importers,
    new render features, new visual script nodes.

Example Plugins (to be built as reference):
  UVE_TerrainPlugin        — Heightmap terrain, splat mapping, grass system
  UVE_NavMeshPlugin        — AI navigation mesh, A* pathfinding
  UVE_PostProcessPlugin    — Volumetric fog, SSR, raymarched clouds
  UVE_VFXPlugin            — Advanced particle systems (VFX graph style)
  UVE_ShaderGraphPlugin    — Node-based shader editor
  UVE_BehaviorTreePlugin   — AI behavior trees
  UVE_IKPlugin             — Inverse kinematics (foot locking, hand IK)
  UVE_NetworkingPlugin     — Multiplayer networking, replication
  UVE_AnimationGraphPlugin — Animation blend trees, state machines
  UVE_FMODPlugin           — FMOD audio integration (alternative to built-in)

Plugin API:
  - IPluginUVE interface: OnLoadUVE(), OnUnloadUVE(), GetNameUVE(), GetVersionUVE()
  - Plugin can call Engine API through UVE:: namespace.
  - Plugin can register custom nodes via NodeFactoryUVE::RegisterNodeUVE<T>()

================================================================================
PART 7 — C++ CORE ENGINE SYSTEMS
================================================================================

7.1 ENGINE LOOP & FOUNDATION
  EngineCoreUVE           — Main loop: Init → Load → Update → Render → Shutdown
  MemoryManagerUVE        — Custom allocators: PoolAllocatorUVE, StackAllocatorUVE,
                            HeapAllocatorUVE with memory tracking and leak detection.
  ThreadPoolUVE           — Job system with work-stealing queue.
  EventSystemUVE          — Type-safe event bus (pub/sub) for decoupled communication.
  LoggerUVE               — Multi-sink logging: Console, File, DebugOutput, In-Editor Console.
                            Levels: Trace, Debug, Info, Warning, Error, Fatal.
  TimerUVE                — High-resolution timer, delta time, fixed timestep physics.
  ConfigManagerUVE        — Key-value config system (JSON-based .uvesettings).
  CommandLineUVE          — Parse startup arguments.

7.2 RENDERING PIPELINE
  RenderSystemUVE         — Abstract rendering backend.
  Renderer3DUVE           — Forward+ AND Deferred rendering paths (switchable per project).
  ShaderManagerUVE        — Compile GLSL/HLSL/Metal at runtime. Hot-reload on file change.
  MaterialSystemUVE       — PBR workflow: Albedo, Normal, Metallic, Roughness, AO, Emissive.
  MeshRendererUVE         — Submit static and skinned meshes to GPU.
  PostProcessUVE          — Bloom, SSAO, SSR (if plugin), Tone Mapping (ACES), FXAA/TAA.
  ShadowSystemUVE         — CSM for directional lights, cubemap shadows for point lights.
  LightSystemUVE          — Light culling, IBL (diffuse + specular probes).
  CameraSystemUVE         — Camera management, frustum culling, occlusion queries.
  RenderQueueUVE          — Sortable render command buffer (opaque → transparent → UI).
  ComputeSystemUVE        — Compute shader dispatch for GPU particles, skinning.

7.3 SCENE & ECS
  SceneGraphUVE           — Hierarchical transform tree with dirty-flag propagation.
  EntityManagerUVE        — ECS with archetypes and chunk-based memory (DOTS-like).
  ComponentUVE            — Base component. Built-ins: Transform, Mesh, Light, Camera,
                            Collider, RigidBody, AudioSource, Script, ParticleEmitter.
  PrefabSystemUVE         — Instantiate prefabs, nested prefabs, prefab overrides.
  SceneSerializerUVE      — Save/load .uvescene files (binary + JSON metadata).

7.4 ASSET PIPELINE
  AssetManagerUVE         — Async loading, reference counting, garbage collection.
  AssetDatabaseUVE        — GUID-based asset registry. No file path dependencies in scenes.
  AssetImporterUVE        — Import pipeline with settings per asset type.
  HotReloadUVE          — Detect file changes, reload assets without restart.
  AssetBundleUVE        — Pack assets for builds (streaming, DLC).

7.5 PHYSICS
  PhysicsSystemUVE        — Integration with Jolt Physics (preferred) or Bullet.
  CollisionSystemUVE      — Broad phase (BVH) + narrow phase (GJK/EPA).
  RaycastSystemUVE        — Raycast, SphereCast, BoxCast, CapsuleCast.
  PhysicsMaterialUVE      — Friction, restitution, density.

7.6 AUDIO
  AudioSystemUVE          — 3D positional audio. Built-in: OpenAL-soft backend.
  AudioSourceUVE          — Spatial audio with distance attenuation curves.
  AudioMixerUVE           — Channel groups with effects: Reverb, EQ, Compressor, Limiter.
  AudioListenerUVE        — Attached to Camera3D by default.

7.7 INPUT
  InputSystemUVE          — Abstract input: Keyboard, Mouse, Gamepad, Touch.
  InputActionUVE          — Action-based mapping (like Unity Input System).
  InputBindingUVE         — Bind keys/buttons to actions (WASD → Move, Space → Jump).
  TouchInputUVE           — Multi-touch gestures: Pan, Pinch, Rotate, Tap, Swipe.
  GyroscopeUVE            — Mobile gyro input for camera look.

7.8 PLATFORM ABSTRACTION
  PlatformUVE             — OS abstraction.
  WindowManagerUVE        — GLFW3 or SDL3 for window creation.
  FileSystemUVE           — Cross-platform file I/O, virtual file system (VFS).
  PlatformMobileUVE       — Android/iOS specific: touch, sensors, battery, safe areas,
                            notch handling, back button, app lifecycle (pause/resume).
  PlatformPCUVE           — Multi-monitor, raw input, windowed/borderless/fullscreen.
  PowerManagerUVE         — Battery-aware frame rate throttling on mobile.

7.9 MOBILE OPTIMIZATIONS
  LODGeneratorUVE         — Auto-generate LOD meshes (simplify geometry).
  OcclusionSystemUVE      — GPU occlusion culling (HZB-based).
  TextureCompressorUVE    — Compress to ASTC (Android), ETC2 (older Android), BC7 (PC).
  DynamicResolutionUVE    — Scale render target based on GPU frame time.
  UIMobileAdapterUVE      — Auto-scale UI for different screen densities.
  AdaptiveQualityUVE      — Adjust shadow quality, particle count, view distance dynamically.

7.10 NETWORKING (Optional Core Module)
  NetworkSystemUVE        — UDP-based networking with reliable channels.
  ReplicationUVE          — Automatic state replication for multiplayer games.
  NetworkPredictionUVE  — Client-side prediction + server reconciliation.

================================================================================
PART 8 — NATIVE C++ VISUAL SCRIPTING SYSTEM
================================================================================

8.1 VISUAL SCRIPTING EDITOR (Inside UVE Editor)
  VisualScriptEditorUVE   — Node-based graph editor (similar to UE Blueprints).
  ScriptGraphUVE          — Authored typed execution graph with nodes, pins, links, and user-facing layout metadata.
  ScriptNodeUVE           — Base graph-node description; native handlers are registered in C++.
  VisualScriptCompilerUVE — Validates graph control/data flow and lowers it to a versioned `.uvescript` bytecode program.
  VisualScriptVMUVE       — Native C++ bytecode executor with bounded instruction dispatch and explicit engine-call bindings.
  ScriptNodeRegistryUVE   — Registers built-in and plugin-provided C++ node handlers and pin contracts.
  ScriptDebuggerUVE       — Bytecode breakpoints, step-through, variable watch, call stack, and source-node mapping.

8.2 BUILT-IN VISUAL SCRIPT NODES

FLOW CONTROL:
  EventBeginPlay, EventTick, EventCollisionEnter, EventCollisionExit,
  EventInputAction, Branch, Sequence, ForLoop, WhileLoop, Delay, Gate,
  DoOnce, DoN, FlipFlop, Multigate

MATH:
  Add, Subtract, Multiply, Divide, Modulo, Power, Sqrt, Sine, Cosine,
  Tangent, ArcSine, ArcCosine, ArcTangent, Lerp, Clamp, Min, Max, Abs,
  Floor, Ceil, Round, RandomFloat, RandomInt, RandomInRange

VECTOR & TRANSFORM:
  Vector3, Vector2, MakeVector, BreakVector, Normalize, DotProduct,
  CrossProduct, Distance, Length, GetPosition, SetPosition, GetRotation,
  SetRotation, GetScale, SetScale, LookAt, ForwardVector, RightVector, UpVector

GAMEPLAY:
  SpawnNode3D, DestroyNode3D, GetComponent, SetComponent, CastTo,
  IsValid, GetNodeByName, GetNodeByTag, GetAllNodesOfClass

INPUT:
  GetAxisValue, GetButtonPressed, GetButtonReleased, GetTouchPosition,
  GetTouchCount, GetGyroscopeRotation

RENDERING:
  SetMaterial, SetVisibility, PlayAnimation, StopAnimation,
  SetAnimationSpeed, CrossFadeAnimation

AUDIO:
  PlaySoundAtLocation, StopSound, SetVolume, SetPitch, FadeIn, FadeOut

PHYSICS:
  AddForce, AddImpulse, AddTorque, SetVelocity, GetVelocity,
  SetGravityEnabled, OnOverlapBegin, OnOverlapEnd

UI:
  SetText, SetProgressBarValue, SetButtonEnabled, SetPanelVisibility,
  PlayUIAnimation

NOTE: Visual script node DISPLAY names have NO UVE suffix (user-friendly).
      Internal native C++ classes use the UVE suffix (e.g., ScriptNodeAddUVE).

8.3 NATIVE C++ RUNTIME BOUNDARY
  ScriptValueUVE          — Tagged, serializable bytecode value model for booleans, numbers, vectors, entities, and assets.
  ScriptBindingRegistryUVE — Explicit native C++ engine-call bindings for Transform, Physics, Render, Audio, and Input operations.
  VisualScriptVMUVE       — Executes validated bytecode without an embedded managed runtime or native/managed bridge.
  ScriptStateUVE          — Versioned script-instance state used for future transactional bytecode reload and migration.

================================================================================
PART 9 — UVE EDITOR APPLICATION (ORIGINAL BRANDING, UNREAL-INSPIRED LAYOUT)
================================================================================

9.1 EDITOR DESIGN PRINCIPLES
  - Branding: "UniVex" / "UVE" everywhere. NO Unreal Engine assets/colors.
  - Color Scheme: Dark theme (#0D0D0D background, #00D4FF accent, #1A1A1A panels).
  - Typography: Clean sans-serif (Inter or Roboto equivalent).
  - Icons: Custom icon set (NOT Unreal's). Flat, minimal, cyan-accented.
  - Layout: Viewport-centric. Unreal-inspired panel arrangement but original styling.

9.2 EDITOR LAYOUT (Responsive & Compact)

The editor MUST be responsive and fit any screen size:
  - PC (1920x1080+): Full layout with all panels visible.
  - Laptop (1366x768): Auto-collapse side panels to icons, smaller fonts.
  - Tablet (1024x768): Collapse left/right to flyout drawers.
  - Mobile Editor (Android): Touch-optimized, gesture-based, floating toolbars.

MAIN PANELS:

[TOP BAR]
  - UVE Logo (left)
  - Menu: File, Edit, View, Assets, GameObject, Component, Tools, Build, Help
  - Play Controls: ▶ Play, ⏸ Pause, ⏹ Stop, ⏵ Step
  - Platform Toggle: 🖥 PC | 📱 Mobile (switches viewport preview)
  - Build Button: Package for current platform
  - Search Bar: Global asset/node search

[LEFT PANEL — Outliner]
  - Tab 1: "Scene Outliner" — Hierarchical tree of all Node3Ds in scene.
    Features: Search filter, type filter, lock/hide toggles, multi-select,
    drag-and-drop parenting, right-click context menu.
  - Tab 2: "Layers" — Layer organization (like Photoshop layers for scene objects).
  - Collapsible to icon-only mode on small screens.

[CENTER — Viewport]
  - Large 3D viewport (primary workspace).
  - Viewport toolbar (top of viewport):
      * Shading modes: Lit, Unlit, Wireframe, UV, Normal, Roughness
      * Camera speed slider
      * Grid toggle, Gizmo size slider
      * Screenshot button
  - Gizmo toolbar (bottom of viewport): Select, Translate, Rotate, Scale, Local/World toggle
  - Orientation gizmo (top-right corner of viewport): XYZ axis indicator
  - Stats overlay (top-left): FPS, Draw Calls, Tri Count, VRAM usage
  - Game Preview toggle: Split viewport or docked floating window
  - On Android: Full touch support (see Section 9.4)

[RIGHT PANEL — Inspector]
  - Tab 1: "Inspector" — Properties of selected node.
    * Collapsible component cards (Transform, Mesh, Material, Physics, etc.)
    * Each card has header with component name and enable/disable toggle.
    * Property fields: Float sliders, color pickers, dropdowns, file pickers,
      vector3 inputs (X/Y/Z with colored labels), checkboxes.
    * "+ Add Component" button at bottom.
  - Tab 2: "Visual Script" — Opens graph editor for selected node's script.
  - Tab 3: "Node" — Node-specific settings (name, tag, layer, visibility).

[BOTTOM PANEL — Content Browser + Output]
  - Left half: "Content Browser"
    * Folder tree (res://, assets://)
    * Thumbnail grid view for assets (.uvemodel, .uvetex, .uvemat, etc.)
    * List view option
    * Search bar with filters (type, date, size)
    * Right-click: Import, Reimport, Show in Explorer, Delete, Rename
    * Drag-and-drop into viewport to instantiate
  - Right half: "Output Log / Console"
    * Filterable by log level (Trace, Debug, Info, Warning, Error, Fatal)
    * Searchable text
    * Clear and Copy buttons
    * Click error → jump to source line
  - Bottom tabs also include: "Animation", "Profiler", "Shader Editor"

[STATUS BAR]
  - Bottom-most strip showing:
    * Current platform target (PC/Android/iOS)
    * Build status
    * Git branch / version control status
    * Memory usage
    * Current tool mode (Select/Translate/Rotate/Scale)

9.3 EDITOR WINDOWS (Pop-out / Dockable)
  All panels can be:
  - Dragged to dock left/right/top/bottom/center
  - Popped out as floating windows
  - Tabbed together
  - Saved/loaded as custom layouts (.uvelayout files)

Additional Editor Windows:
  - Material EditorUVE — Node-based material graph (when ShaderGraphPlugin active)
  - Animation EditorUVE — Timeline with keyframes, curves, events
  - ProfilerUVE — CPU/GPU timeline, memory profiler, draw call breakdown
  - Visual Script EditorUVE — Full-screen graph editing
  - Settings EditorUVE — Project settings, editor preferences, keybinds
  - Build SettingsUVE — Platform selection, build options, packaging

9.4 ANDROID TOUCH SUPPORT (CRITICAL)

The viewport MUST fully support Android touch gestures:

  Single Finger:
    - Drag on empty space = Orbit camera (rotate around pivot)
    - Drag on selected object = Move object (if translate mode)
    - Tap = Select object (raycast from touch position)
    - Double-tap = Focus camera on object
    - Long press = Context menu

  Two Fingers:
    - Pinch = Zoom camera (dolly in/out)
    - Spread = Zoom out
    - Rotate two fingers = Orbit camera (alternative to single-finger orbit)
    - Drag two fingers = Pan camera (move pivot)

  Three Fingers:
    - Drag = Pan camera (alternative)
    - Tap = Reset camera to default view

  Gizmo Interaction:
    - Tap on gizmo arrow = Select axis
    - Drag on gizmo arrow = Move along that axis only
    - Tap on gizmo box (center) = Free move in view plane
    - Visual feedback: Highlighted axis turns bright cyan when touched

  UI Adaptations for Mobile:
    - All buttons minimum 44x44dp touch target
    - Increase spacing between controls
    - Bottom toolbar floats and can be moved
    - Side panels become slide-out drawers (swipe from edge)
    - Virtual joysticks appear in Game Preview mode
    - On-screen keyboard for text input
    - Haptic feedback on selection

  Performance:
    - Touch input runs on separate thread to avoid frame drops
    - Gesture recognition uses velocity-based smoothing
    - 120Hz touch sampling on supported devices

9.5 EDITOR THEME & STYLING (ORIGINAL)

  Background:      #0D0D0D (near black)
  Panel BG:        #141414
  Panel Border:    #2A2A2A
  Accent Color:    #00D4FF (cyan)
  Accent Hover:    #33DDFF
  Text Primary:    #E0E0E0
  Text Secondary:  #888888
  Text Disabled:   #555555
  Success:         #27C93F
  Warning:         #FFBD2E
  Error:           #FF5F56
  Selection:       #00D4FF33 (cyan with transparency)
  Grid Line:       #1A1A1A
  Grid Axis X:     #FF4444 (red)
  Grid Axis Z:     #4444FF (blue)
  Grid Axis Y:     #44FF44 (green)

  Font: System sans-serif, 13px default, 11px for compact mode
  Border Radius: 6px for panels, 4px for buttons
  Shadows: Subtle drop shadows on floating panels
  Animations: Smooth panel transitions (200ms ease)

================================================================================
PART 10 — EDITOR SYSTEMS IMPLEMENTATION
================================================================================

10.1 EditorUVE — Main Editor Class
  - Initialize all editor subsystems.
  - Load user layout preferences.
  - Manage editor state: Edit Mode, Play Mode, Pause Mode.
  - Play Mode: Spawn separate game process or run in same process with 
    scene duplication (sandboxed).

10.2 ViewportUVE — 3D Viewport
  - Render scene with editor camera (separate from game camera).
  - Draw grid, gizmos, selection outlines, bounding boxes.
  - Support multiple viewports (split screen: perspective + top + front + side).
  - Camera controls: WASD + mouse (PC), touch gestures (Android).
  - Picking: Raycast from mouse/touch to select objects.

10.3 GizmoSystemUVE
  - Translate gizmo: XYZ arrows + plane handles.
  - Rotate gizmo: Circle rings per axis + trackball.
  - Scale gizmo: XYZ boxes + uniform scale center box.
  - Snapping: Grid snap, angle snap, scale snap (configurable).
  - Local vs World space toggle.

10.4 InspectorUVE
  - Auto-generate UI from component reflection metadata.
  - Support custom property drawers per type.
  - Undo/Redo integration (every property change is a command).
  - Multi-object editing: Show common properties when multiple nodes selected.

10.5 ContentBrowserUVE
  - Virtual file system view.
  - Thumbnail generation (async, cached).
  - Drag-and-drop from browser to viewport = instantiate prefab/model.
  - Drag-and-drop from browser to inspector = assign material/texture.
  - Folder colors, favorites, recent files.

10.6 UndoSystemUVE
  - Command pattern: every editor action is a Command.
  - Undo stack with configurable limit (default 100).
  - Grouped commands (e.g., moving object = one undo step even if dragged).
  - Ctrl+Z / Ctrl+Y shortcuts.

10.7 SerializationUVE
  - Save editor layout (.uvelayout).
  - Save editor preferences (.uvesettings).
  - Scene auto-save (configurable interval).

================================================================================
PART 11 — VISUAL SCRIPTING EDITOR (DETAILED)
================================================================================

11.1 Graph Editor UI
  - Infinite canvas with pan/zoom (mouse wheel or pinch).
  - Grid background with snap.
  - Node palette on left: searchable, categorized list of all nodes.
  - Right-click context menu: Add node, Break links, Delete.
  - Connection lines: Bezier curves, color-coded by type (exec=white, float=green,
    vector=yellow, object=blue, bool=red).
  - Node selection: Box select, Ctrl+click multi-select.
  - Node groups: Colored comment boxes around related nodes.

11.2 Node Types
  - Event Nodes (red header): Trigger execution (BeginPlay, Tick, Collision, Input).
  - Function Nodes (blue header): Call engine functions.
  - Variable Nodes (green header): Get/Set variables.
  - Math Nodes (purple header): Math operations.
  - Flow Control Nodes (gray header): Branch, Loop, etc.
  - Custom Nodes: User-defined reusable graphs (macro/function).

11.3 Compilation
  - Real-time validation: Red error lines on invalid connections.
  - Compile button: Validate graph and lower it to a versioned native `.uvescript` bytecode program.
  - Generated bytecode remains inspectable through source-node mapping and the script debugger.
  - Debug: Breakpoints on nodes, step execution, variable watch.

================================================================================
PART 12 — BUILD SYSTEM & PLATFORM SUPPORT
================================================================================

12.1 Build PipelineUVE
  - CMake-based build system.
  - Supported platforms: Windows (MSVC), Linux (GCC/Clang), Android (NDK), iOS (Xcode).
  - Debug / Development / Release / Shipping configurations.
  - Asset cooking: Convert editor assets to platform-optimized runtime formats.
  - Shader compilation: Pre-compile shaders for target platform to avoid runtime stutter.

12.2 PC Build
  - Executable + asset bundle.
  - Optional: Steam integration hooks.

12.3 Android Build
  - APK/AAB output.
  - Gradle integration.
  - ARM64 support required, ARM32 optional.
  - Vulkan primary, OpenGL ES 3.2 fallback.
  - Touch input auto-mapped.

12.4 iOS Build
  - Xcode project generation.
  - Metal backend.
  - App Store compliance (64-bit only).

================================================================================
PART 13 — SAMPLE PROJECT REQUIREMENTS
================================================================================

Deliver a complete sample project demonstrating:
  1. A 3D scene with: DirectionalLight3D, Camera3D, Mesh3D (cube + floor),
     RigidBody3D with CollisionShape3D (physics boxes falling).
  2. A player CharacterBody3D with VisualScript3D attached:
     - WASD movement (PC)
     - Virtual joystick movement (Android)
     - Jump on Space / Screen tap
  3. Audio: Background music (.uveaudio) + jump sound effect.
  4. UI: Health bar (ProgressBar2D) + Score label (Label2D).
  5. Visual Script: Simple collectible system (coin pickup, score increment).
  6. Build configs for both PC and Android.

================================================================================
PART 14 — DOCUMENTATION DELIVERABLES
================================================================================

Provide documentation for:
  1. Engine API Reference (all UVE-suffixed classes and functions).
  2. Visual Scripting Node Reference (all built-in nodes with descriptions).
  3. Custom File Format Specification (binary layout of all .uve* files).
  4. Plugin Development Guide (how to create a plugin from scratch).
  5. Editor User Manual (layout, shortcuts, Android touch controls).
  6. Build & Deployment Guide (CMake setup, platform-specific steps).

================================================================================
PART 15 — CODE QUALITY REQUIREMENTS
================================================================================

  - Modern C++: Use smart pointers, RAII, constexpr, concepts where applicable.
  - No raw new/delete in engine code (use MemoryManagerUVE allocators).
  - Thread-safety: Document thread affinity for all systems.
  - Error handling: Use UVE_ASSERT for debug, UVE_LOG for runtime errors.
  - Performance: Profile-guided optimizations. No STL in hot paths.
  - Comments: XML-style comments for all public APIs (for auto-doc generation).
  - Testing: Unit tests for core systems (Math, Serialization, ECS).


================================================================================
================================================================================
                    ADDITIONAL FEATURES — PARTS 16 TO 25
================================================================================
================================================================================

The following features extend the base engine with advanced systems.
They are prioritized but should only be implemented AFTER the core engine
(Parts 1-15) is stable and functional.


================================================================================
PART 16 — CINEMATIC SEQUENCER / TIMELINE EDITOR
================================================================================

PURPOSE:
  Create cutscenes, camera animations, and scripted events visually
  without writing code.

COMPONENTS:
  SequencerEditorUVE      — Main timeline window in the editor.
  SequenceAssetUVE        — .uvesequence file format.
  TimelineTrackUVE        — Base class for all track types.

TRACK TYPES:
  CameraTrackUVE          — Animate camera position, rotation, FOV.
  AnimationTrackUVE       — Play .uveanim clips with blending.
  AudioTrackUVE           — Sync sound effects and music to timeline.
  EventTrackUVE           — Fire custom events (spawn enemy, trigger UI, etc.).
  TransformTrackUVE       — Animate any Node3D transform directly.
  LightTrackUVE           — Animate light color, intensity, range.
  ParticleTrackUVE        — Trigger particle bursts at specific times.
  FadeTrackUVE            — Screen fade in/out (black/white/color).

KEY FEATURES:
  - Scrubbing: Drag playhead to preview any frame.
  - Keyframes: Add/remove/edit keyframes with curve interpolation.
  - Looping: Loop sections of sequence.
  - Blending: Crossfade between camera shots.
  - Trigger zones: Auto-play sequence when player enters area.
  - Cinematic camera: Smooth dolly, crane, shake effects.
  - Export: Render sequence to video file (MP4/AVI) for trailers.

USE CASES:
  - Opening cutscene of game.
  - Boss entrance animation.
  - Ending credits sequence.
  - Tutorial camera flythrough.

================================================================================
PART 17 — SAVE / LOAD SYSTEM + CHECKPOINT MANAGER
================================================================================

PURPOSE:
  Built-in serialization of game state. Cross-platform save files.

COMPONENTS:
  SaveGameSystemUVE       — Core save/load manager.
  CheckpointManagerUVE    — Auto-save and manual checkpoint system.
  GameStateUVE            — Snapshot of entire game world.
  PersistentDataUVE       — Data that persists across sessions.

SAVE FILE FORMAT:
  .uvesave — Binary format with compression (LZ4).
  Structure: Header + Metadata (timestamp, version, playtime) +
             World State + Player State + Inventory + Quest Progress

FEATURES:
  - Slot-based saves: Up to 99 save slots per user.
  - Auto-save: Configurable interval (e.g., every 5 minutes).
  - Checkpoint: Manual save points (e.g., before boss fight).
  - Cloud sync: Hooks for Google Play Games, iCloud, Steam Cloud.
  - Screenshot: Auto-capture thumbnail per save slot.
  - Migration: Auto-upgrade old save files to new game version.
  - Encryption: Optional AES-256 encryption for save files.

SAVE DATA INCLUDES:
  - All Node3D transforms, health, state
  - Inventory items, currency, stats
  - Quest progress, dialogue flags
  - World time, weather state
  - Unlocked levels, achievements

CROSS-PLATFORM:
  - PC: Save to %APPDATA%/UniVex/Saves/
  - Android: Save to internal storage / cloud
  - iOS: Save to Documents directory / iCloud

================================================================================
PART 18 — DATA TABLES / SPREADSHEET IMPORT
================================================================================

PURPOSE:
  Game designers can edit game data in Excel/Google Sheets and import
  directly into the engine. No coding needed to tweak numbers.

COMPONENTS:
  DataTableUVE            — Runtime data table asset.
  DataTableImporterUVE    — Import CSV, JSON, TSV, Excel (.xlsx).
  DataTableRowUVE         — Single row of data.
  DataTableEditorUVE      — In-editor spreadsheet view.

SUPPORTED IMPORT FORMATS:
  .csv  — Comma-separated values
  .json — JSON array of objects
  .tsv  — Tab-separated values
  .xlsx — Excel (via lightweight parser)

EXAMPLE USE CASES:
  WEAPON TABLE:
    Name        | Damage | FireRate | Recoil | Ammo | UnlockLevel
    Pistol      | 25     | 0.3      | 2.0    | 12   | 1
    Rifle       | 45     | 0.1      | 5.5    | 30   | 5
    Sniper      | 120    | 1.5      | 12.0   | 5    | 10

  ENEMY TABLE:
    Name        | HP   | Speed | Damage | XP_Drop | LootTable
    Goblin      | 50   | 5.0   | 10     | 15      | goblin_loot
    Orc         | 150  | 3.5   | 25     | 40      | orc_loot
    Dragon      | 2000 | 8.0   | 100    | 500     | dragon_loot

  DIALOGUE TABLE:
    ID | Speaker | Text                    | NextID | Condition
    1  | NPC     | "Hello, traveler?"      | 2      | none
    2  | Player  | "Who are you?"          | 3      | none
    3  | NPC     | "I am the guardian."    | 4      | none

FEATURES:
  - Hot reload: Change CSV → auto-update in editor without restart.
  - Type validation: Auto-detect int, float, bool, string, Vector3.
  - Reference validation: Check if referenced assets exist.
  - Search & filter: Find rows by column value.
  - Export: Export runtime-modified tables back to CSV.

VISUAL SCRIPTING INTEGRATION:
  - GetDataTableRowUVE node — fetch row by ID.
  - GetDataTableValueUVE node — fetch specific column value.
  - ForEachRowUVE node — loop through all rows.

================================================================================
PART 19 — SPLINE SYSTEM
================================================================================

PURPOSE:
  Create smooth curves for roads, rails, camera paths, AI patrol routes,
  rivers, power lines, and rope physics.

COMPONENTS:
  SplineComponentUVE      — Attach to Node3D to define a curve path.
  SplinePointUVE          — Control point (position + tangent + up vector).
  SplineMeshGeneratorUVE  — Extrude mesh along spline (roads, pipes, cables).
  SplineFollowerUVE       — Move object along spline at constant speed.

SPLINE TYPES:
  - Linear: Straight lines between points.
  - Bezier: Cubic Bezier curves (smooth, C1 continuous).
  - Catmull-Rom: Smooth curves passing through all points.
  - B-Spline: Uniform B-spline for extra smoothness.

FEATURES:
  - Visual editing: Drag control points in viewport. Tangents show as handles.
  - Subdivision: Auto-add points for smoother curves.
  - Closed loop: Connect first and last point (race track, patrol loop).
  - Roll banking: Tilt along curve (roller coaster effect).
  - Speed control: Variable speed along spline (slow on curves, fast on straight).
  - Event triggers: Fire events at specific spline positions.

USE CASES:
  - Racing game: AI follows road spline.
  - Camera: Cinematic dolly along spline.
  - Train/tram: Follow rail spline with physics.
  - River: Flow particles along spline path.
  - Rope/Chain: Physics-based spline simulation.

================================================================================
PART 20 — DEVELOPER CONSOLE (In-Game)
================================================================================

PURPOSE:
  Command-line interface inside the game for debugging, cheats, and testing.
  Like Source engine console or Quake console.

COMPONENTS:
  DeveloperConsoleUVE     — Overlay console UI (tilde ` key to open).
  ConsoleCommandUVE         — Base class for commands.
  CVARUVE                   — Console variables (runtime tweakable).

BUILT-IN COMMANDS:
  show_fps 1              — Show FPS counter.
  show_collision 1        — Draw collision wireframes.
  god_mode 1              — Invincible player.
  noclip 1                — Fly through walls.
  spawn Mesh3D cube       — Spawn object at cursor.
  destroy [name]          — Destroy object by name.
  set_time 12:00          — Set day/night time.
  teleport 100 0 50       — Teleport player to coordinates.
  give_item health_potion 5 — Add items to inventory.
  load_level level_02     — Load scene by name.
  screenshot              — Capture screenshot.
  profile_start           — Start CPU/GPU profiling.
  mem_report              — Show memory usage breakdown.

CVAR EXAMPLES:
  r_shadow_quality 2      — Shadow map resolution (0-4).
  r_draw_distance 500     — Max render distance.
  s_master_volume 0.7     — Master audio volume.
  g_difficulty 2          — Game difficulty (0-3).

FEATURES:
  - Auto-complete: Tab completion for commands and CVARs.
  - History: Arrow up/down to recall previous commands.
  - Color-coded output: White=info, Yellow=warning, Red=error.
  - Scrollable log: View full command history.
  - Scriptable: Register custom commands from native C++ visual scripts through explicit bindings.
  - Conditional compilation: Stripped out in shipping builds.

================================================================================
PART 21 — NATIVE BYTECODE PROGRAM RELOAD
================================================================================

PURPOSE:
  Recompile a visual-script graph to native `.uvescript` bytecode and reload it without restarting the editor. The initial implementation is deliberately future work; this section defines its safe C++ architecture.

COMPONENTS:
  ScriptHotReloadUVE       — Detects `.uvescript` or authored graph changes and schedules recompilation.
  BytecodeProgramReloaderUVE — Validates a replacement program before atomically replacing the active bytecode handle.
  ScriptStatePreserverUVE  — Preserves versioned script-instance values across compatible program reloads.

HOW IT WORKS:
  1. User edits a graph in the native visual-script editor.
  2. The graph compiler validates types, links, and control flow, then emits a candidate bytecode program.
  3. The runtime validates bytecode version, instruction bounds, node-binding compatibility, and state schema.
  4. If validation succeeds:
     - Pause game (if in play mode).
     - Serialize compatible script-instance state.
     - Atomically replace the old bytecode program.
     - Restore compatible state and resume game.
  5. If validation fails: show diagnostics and retain the last known-good bytecode program.

FEATURES:
  - Preserve compatible variables, transforms, and component data.
  - Graph edits can recompile without a managed runtime or external C++ compiler invocation.
  - Source-node mapping keeps breakpoints and diagnostics associated with graph nodes.
  - Reload history can retain prior validated bytecode programs for future rollback tooling.

LIMITATIONS (documented):
  - Incompatible pin-type or state-schema changes may require explicit migration.
  - Removed variables can cause intentional state loss when no migration rule exists.
  - Native engine-call binding signature changes require bytecode revalidation.

================================================================================
PART 22 — PROCEDURAL GENERATION TOOLS
================================================================================

PURPOSE:
  Generate content algorithmically — terrain, caves, dungeons, and vegetation.

COMPONENTS:
  NoiseGeneratorUVE       — Perlin, Simplex, Value, Worley (cellular) noise.
  ProceduralMeshUVE       — Generate meshes from noise (terrain, asteroids).
  DungeonGeneratorUVE     — Room-and-corridor dungeon layout.
  VoronoiDiagramUVE       — Cell-based maps (territories, biomes).

NOISE FEATURES:
  - 1D, 2D, 3D, 4D noise support.
  - Octaves, persistence, lacunarity (fractal Brownian motion).
  - Domain warping: Distort noise coordinates for organic shapes.
  - Ridged multifractal: Sharp ridges for mountains.
  - Billow: Smooth rounded shapes for clouds.

TERRAIN GENERATION:
  - Heightmap from 2D noise.
  - Erosion simulation (thermal, hydraulic).
  - Biome mapping: Temperature + humidity noise → biome (desert, forest, snow).
  - River carving: Pathfinding from high to low elevation.

DUNGEON GENERATION:
  - BSP (Binary Space Partitioning) rooms.
  - Random walk (drunkard's walk) caves.
  - Cellular automata smoothing.
  - Room connection via minimum spanning tree.

USE CASES:
  - Open world terrain: Infinite procedural world.
  - Roguelike: Random dungeon every run.
  - Asteroid fields: Procedural asteroid shapes.
  - Clouds: Volumetric cloud noise.

================================================================================
PART 23 — DECAL SYSTEM
================================================================================

PURPOSE:
  Project 2D textures onto 3D surfaces — bullet holes, blood, graffiti,
  road markings, scorch marks, and wet patches.

COMPONENTS:
  DecalComponentUVE       — Attach to Node3D, project decal onto geometry.
  DecalRendererUVE        — Render all decals in forward pass.
  DecalAtlasUVE           — Texture atlas for multiple decals (performance).

FEATURES:
  - Projection: Orthographic projection box onto mesh surface.
  - Normal blending: Decal normals blend with surface normals.
  - Fade over distance: Decals fade out for LOD.
  - Lifetime: Auto-destroy after X seconds (bullet holes disappear).
  - Pooling: Reuse decal objects instead of create/destroy.
  - Atlas support: Pack multiple decals into one texture (1 draw call).
  - Deferred + Forward compatible.

DECAL TYPES:
  - Standard: Albedo + Normal only.
  - PBR: Albedo + Normal + Roughness + Metallic.
  - Emissive: Glow decals (neon graffiti, energy marks).

USE CASES:
  - FPS: Bullet holes on walls.
  - Horror: Blood splatters.
  - Racing: Tire skid marks.
  - RPG: Spell impact marks on ground.

================================================================================
PART 24 — BILLBOARD & IMPOSTOR SYSTEM
================================================================================

PURPOSE:
  Render distant objects as 2D sprites for massive performance boost.
  Critical for open world + mobile.

COMPONENTS:
  BillboardComponentUVE   — 2D sprite always facing camera.
  ImpostorSystemUVE         — Auto-generate impostors from 3D models.
  LODImpostorUVE            — Switch to impostor at far distances.

BILLBOARD:
  - Always faces camera (Y-axis rotation only or full billboard).
  - Used for: Trees, grass, clouds, distant crowds, particles.
  - Supports: Alpha testing, alpha blending, wind animation.

IMPOSTOR:
  - Pre-render 3D model from multiple angles (8, 16, or 32 views).
  - Store in texture atlas.
  - At runtime: Pick closest angle based on camera direction.
  - Looks almost identical to 3D model from far away.
  - Switch distance: Configurable per object (e.g., 100m for trees, 500m for buildings).

FEATURES:
  - Auto-generation: Editor generates impostor atlas automatically.
  - Parallax: Slight parallax offset so it does not look flat.
  - Lighting: Normal map from impostor so it is affected by lights.
  - Cross-fade: Smooth transition between mesh and impostor.
  - Shadow casting: Impostors can cast simple blob shadows.

PERFORMANCE:
  - Billboard: 2 triangles vs thousands. 1000x faster.
  - Impostor: 2 triangles + 1 texture sample. Near-zero cost.
  - Mobile critical: Enables forests with 10,000+ trees at 60 FPS.

================================================================================
PART 25 — LIGHTMAP BAKING (FOR MOBILE)
================================================================================

PURPOSE:
  Pre-calculate global illumination for static scenes.
  Real-time GI on PC, baked GI on mobile.

COMPONENTS:
  LightmapBakerUVE        — Bake lightmaps in editor.
  UVUnwrapperUVE          — Generate lightmap UVs (auto or manual).
  BakedLightingUVE        — Apply baked lightmaps at runtime.

BAKING PROCESS:
  1. Mark objects as "Static" (will not move at runtime).
  2. Place light sources (Directional, Point, Spot).
  3. Configure bake settings: Resolution, quality, bounce count.
  4. Click "Bake" → Engine computes radiosity / path tracing.
  5. Output: .uvetex lightmap textures assigned to static meshes.

FEATURES:
  - Direct lighting: Sun/shadows baked into texture.
  - Indirect lighting: Bounced light (color bleeding) baked.
  - Ambient occlusion: Contact shadows baked.
  - Emissive surfaces: Glowing objects illuminate nearby surfaces.
  - Multiple bounces: 1-4 bounces (tradeoff quality vs bake time).
  - Denoising: AI-based denoiser for smooth results.
  - Progressive baking: See preview while baking (like progressive JPEG).

RUNTIME:
  - PC: Can mix baked GI + real-time lights.
  - Mobile: Use baked GI only (disable real-time shadows for static objects).
  - Dynamic objects: Use light probes (spherical harmonics) for indirect light.

QUALITY SETTINGS:
  - Low: 1 bounce, 1 texel per meter, no AO.
  - Medium: 2 bounces, 2 texels per meter, AO enabled.
  - High: 4 bounces, 4 texels per meter, AO + emissive.
  - Ultra: Path traced, 8 bounces, 8 texels per meter, AI denoiser.


================================================================================
================================================================================
                    PART 26 — ECOSYSTEM ARCHITECTURE PLAN
              UniVex Hub + UniVex News + Account System + Backend
================================================================================
================================================================================

26.1 EXECUTIVE SUMMARY
----------------------
The UniVex Engine ecosystem consists of TWO interconnected platforms that
share a single account system:

  PLATFORM A — UniVex Hub (Dashboard Application)
    → The central launcher and project manager for developers.
    → Users log in, download the engine, and manage projects.
    → Similar to Unity Hub or Epic Games Launcher.
    → Works both online and offline.
    → Users CANNOT create new projects unless the engine is installed.

  PLATFORM B — UniVex News (Public Website)
    → The public-facing website for news, updates, tutorials, and community.
    → Similar to Unreal Engine's official website or Unity Blog.
    → Anyone can access without an account.
    → Links to UniVex Hub for downloads.

Both platforms share a single account system. One login for everything.

26.2 DESIGN PRINCIPLES FOR BOTH PLATFORMS
-----------------------------------------
  - Color Theme: Black (#0A0A0A) + Orange (#FF6B00) + Glass effects
  - Fluid glass effect on every panel/card: backdrop-filter: blur(40px),
    rgba(255,255,255,0.03) background, 1px rgba(255,255,255,0.08) border
  - Animated orange glow orbs in background (subtle, not distracting)
  - Smooth page transitions: fade + slide on every route change
  - Hover effects: cards lift up, borders glow orange, subtle shadow increase
  - Typography: Inter or similar clean sans-serif
  - All interactive elements have 44px minimum touch target on mobile

26.3 UNIVEX HUB (Dashboard App — Concept C Style)
--------------------------------------------------

PURPOSE:
  The central launcher. Users open this to manage everything related to
  their UniVex development workflow.

LAYOUT:
  ┌──────────────────────────────────────────────────────────────────────────┐
  │  [U] UniVex                                    [🔍] [🔔] [👤 Account]     │
  ├──────┬───────────────────────────────────────────────────────────────────┤
  │      │                                                                   │
  │  🏠  │  DASHBOARD                                         [+ New Proj] │
  │  📁  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐             │
  │  📦  │  │ 12 Projects │  │ v1.0.0-beta │  │  2.4 GB     │             │
  │  🔌  │  │  [▓▓▓▓▓░░] │  │  [Update]   │  │  [▓▓▓▓▓▓░░] │             │
  │  📊  │  └─────────────┘  └─────────────┘  └─────────────┘             │
  │  ⚙️  │                                                                   │
  │      │  RECENT PROJECTS                                                    │
  │  ─── │  ┌───────────────────────────────────────────────────────────────┐  │
  │  👤  │  │ 🎮 FPS_Shooter     │ .uveproj │ 2 hrs ago │ [Open] [☁️]    │  │
  │  🚪  │  │ 🏎️ Racing_Game    │ .uveproj │ 1 day ago │ [Open] [☁️]    │  │
  │      │  │ 🧟 Zombie_Survival │ .uveproj │ 3 days    │ [Open] [☁️]    │  │
  │      │  └───────────────────────────────────────────────────────────────┘  │
  │      │                                                                   │
  │      │  ENGINE STATUS                                                      │
  │      │  ┌───────────────────────────────────────────────────────────────┐  │
  │      │  │ ✅ v1.0.0-beta installed  │  📁 C:/UniVex/Engine/              │  │
  │      │  │ 🔄 v1.0.1 available       │  [Update] [Verify] [Uninstall]    │  │
  │      │  └───────────────────────────────────────────────────────────────┘  │
  │      │                                                                   │
  └──────┴───────────────────────────────────────────────────────────────────┘

SIDEBAR NAVIGATION:
  🏠 Dashboard        — Overview, stats, recent activity
  📁 Projects         — All local + cloud projects
  📦 Asset Store      — Browse/download assets, plugins, templates
  🔌 Plugins          — Manage installed plugins
  📊 Analytics        — Build stats, crash reports, player data
  ⚙️  Settings        — Engine prefs, keybinds, theme, account
  ─────────────────────────────────────────
  👤 Profile          — Account info, subscription, billing
  🚪 Logout

PROJECT MANAGER SCREEN:
  - Grid/List view toggle
  - Sort: Name, Date Modified, Engine Version, Platform
  - Filter: PC, Android, iOS, All
  - Search bar
  - Right-click context menu:
      Open in Editor | Show in Explorer | Duplicate | Rename | Delete |
      Backup to Cloud | Share (generate link)

NEW PROJECT DIALOG — THE GATE:
  ⚠️ IF engine is NOT installed:
    "Engine Required — Download UniVex Engine to create projects"
    Button: [Download Engine] → opens Engine Download page
    The "Create Project" button is DISABLED and grayed out.

  IF engine IS installed:
    Fields:
      • Project Name: [________________]
      • Template: [🎮 3D FPS] [🏎️ 3D Racing] [🏗️ Empty 3D]
                  [📱 Mobile Empty] [🎲 RPG Starter] [🧩 Puzzle]
      • Location: [C:/UniVex/Projects/____]
      • Engine Version: [v1.0.0-beta ▼]
      • Target Platforms: [☑ PC] [☑ Android] [☐ iOS]
    Buttons: [Cancel] [Create Project]

ENGINE DOWNLOAD PAGE:
  - Auto-detect OS (Windows / Linux / Mac)
  - Show: download size, version, release notes, system requirements
  - Download progress bar with resume support
  - Install wizard (choose path, components to install)
  - "Launch Engine" button after install completes
  - Offline installer option (for studios with no internet)

26.4 UNIVEX NEWS (Public Website — Concept D Style)
---------------------------------------------------

PURPOSE:
  The public face of UniVex. New users discover the engine here.

PAGES:

  HOME / LANDING:
    - Hero section with large "UniVex Engine" headline
    - Animated orange glow orb behind text
    - Tagline: "Build stunning 3D games for PC and mobile"
    - [Download Free] button + [Watch Trailer] button
    - Feature showcase grid (6 cards with glass effect):
        🎮 3D Rendering | 📱 Mobile Export | 🧩 Visual Scripts
        ⚡ Hot Reload    | 🔌 Plugin System | 🌐 Multiplayer
    - Latest News section (3 most recent articles)
    - Featured Showcase (games made with UniVex, with screenshots)
    - Final CTA: "Download UniVex Engine — Free"

  NEWS / BLOG:
    - Article list with category filters
    - Categories: Releases, Tutorials, Community, Engine Dev, Plugins
    - Search bar, pagination or infinite scroll
    - Each article: thumbnail, title, excerpt, date, author, read time

  DOCUMENTATION:
    - Sidebar navigation (like ReadTheDocs/Docusaurus)
    - Searchable
    - Dark mode default
    - Code blocks with syntax highlighting

  COMMUNITY:
    - Forum links
    - Discord invite button
    - GitHub repo link
    - Showcase submission form

  DOWNLOAD:
    - Auto-detect OS
    - Show system requirements
    - Download buttons per platform
    - Release notes
    - Previous versions archive
    - Two options:
        [Download Engine Only]     → Direct installer
        [Download Hub + Engine]    → Recommended (downloads Hub)

LINK TO HUB:
  Fixed floating button at bottom-right of every News page:
    ┌─────────────────┐
    │  [📱 Open Hub]   │  → Opens UniVex Hub app (or prompts download)
    └─────────────────┘

FLUID GLASS EFFECT SPECIFICATION:
  Every card, panel, and modal must use:
    background: rgba(255, 255, 255, 0.03);
    backdrop-filter: blur(40px);
    -webkit-backdrop-filter: blur(40px);
    border: 1px solid rgba(255, 255, 255, 0.08);
    border-radius: 16px;
    box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);

  On hover:
    border-color: rgba(255, 107, 0, 0.3);
    box-shadow: 0 12px 48px rgba(0, 0, 0, 0.4), 0 0 40px rgba(255, 107, 0, 0.05);
    transform: translateY(-4px);
    transition: all 0.4s cubic-bezier(0.4, 0, 0.2, 1);

  Top accent line on cards:
    ::before { height: 2px; background: linear-gradient(90deg, transparent, #FF6B00, transparent); }

PAGE TRANSITIONS:
  Every page open/change must animate:
    - Exit: opacity 1→0, translateY(0→-20px), duration 200ms, ease-out
    - Enter: opacity 0→1, translateY(20px→0), duration 300ms, ease-out
    - Background mesh orbs drift slowly (20s infinite animation)

26.5 ACCOUNT SYSTEM & AUTHENTICATION
------------------------------------

REGISTRATION FLOW:
  Step 1: Email + Password + Username
  Step 2: Verify email (6-digit code sent via email)
  Step 3: Profile setup (display name, country, developer type)
  Step 4: Optional: Link GitHub, Google, or Discord for SSO

LOGIN METHODS:
  • Email + Password
  • Google OAuth 2.0
  • GitHub OAuth
  • Discord OAuth
  • "Remember Me" checkbox (30-day persistent session)
  • 2FA optional (TOTP via authenticator app like Google Authenticator)

ACCOUNT TIERS:
  ┌─────────────────┬─────────────────┬─────────────────┐
  │   FREE          │   INDIE         │   STUDIO        │
  ├─────────────────┼─────────────────┼─────────────────┤
  │ $0/month        │ $9/month        │ $29/month       │
  │                 │                 │                 │
  │ • Engine access │ • Everything    │ • Everything    │
  │ • 3 projects    │   in Free       │   in Indie      │
  │ • Community     │ • 10 projects   │ • Unlimited     │
  │   support       │ • Cloud backup  │   projects      │
  │                 │ • Priority      │ • Cloud backup  │
  │                 │   support       │ • Team collab   │
  │                 │ • Analytics     │ • Analytics     │
  │                 │                 │ • Custom plugins│
  │                 │                 │ • Dedicated     │
  │                 │                 │   support       │
  └─────────────────┴─────────────────┴─────────────────┘

26.6 PRIVACY & ADMIN ACCESS RULES
----------------------------------

WHAT THE ADMIN (YOU, THE OWNER) CAN SEE:
  ✅ User list: username, email, account tier, join date, last login date,
     total projects count, total storage used, country
  ✅ Aggregate analytics: total active users, total projects, download counts,
     most popular assets, crash report summaries (anonymized)
  ✅ Asset store submissions: pending approvals, reports, reviews
  ✅ Payment/subscription status (via Stripe dashboard, not raw card data)
  ✅ Account status: active, suspended, banned, ban reason

WHAT THE ADMIN CANNOT SEE:
  ❌ User passwords (stored as bcrypt hashes, irreversible)
  ❌ Project file contents (encrypted per-user with AES-256)
  ❌ Source code inside .uveproj files
  ❌ Private asset uploads
  ❌ Payment card details (handled entirely by Stripe)
  ❌ 2FA secrets (encrypted)
  ❌ Direct messages between users

WHAT THE ADMIN CAN DO:
  ⚠️ Suspend or ban accounts for Terms of Service violations
  ⚠️ View individual crash reports IF the user has opted in to data sharing
  ⚠️ Approve or reject asset store submissions
  ⚠️ Issue refunds (via Stripe)
  ⚠️ Send system-wide announcements

GDPR / PRIVACY COMPLIANCE:
  • Users can request full data export (JSON download)
  • Users can request account deletion (30-day grace period)
  • Privacy policy page on News website
  • Cookie consent banner
  • Analytics opt-out toggle in settings

26.7 ONLINE + OFFLINE DEVELOPMENT WORKFLOW
------------------------------------------

ONLINE MODE (Internet Available):
  [User opens UniVex Hub] → Auto-login via cached JWT token
  ├── Check for engine updates → notify if available
  ├── Sync cloud projects → download latest versions
  ├── Check asset store for owned asset updates
  ├── Upload queued crash reports & analytics
  └── Open Editor → Full features available

OFFLINE MODE (No Internet):
  [User opens UniVex Hub] → "Working Offline" badge shown
  ├── Use cached login token (valid for 7 days offline)
  ├── Access local projects only
  ├── Cannot download new assets or plugins
  ├── Cannot sync to cloud
  ├── Editor works 100% — ALL features functional
  └── Changes are queued for sync when back online

SYNC MECHANISM (When back online):
  1. Detect internet connection restored
  2. Auto-sync queued changes (upload/download)
  3. Conflict resolution:
       - Detect if same file modified locally and in cloud
       - Show conflict dialog:
         ┌─────────────────────────────────────────┐
         │  Sync Conflict Detected                 │
         │                                         │
         │  File: player_controller.uvescript      │
         │  Local: Modified today at 14:30        │
         │  Cloud: Modified yesterday at 22:15    │
         │                                         │
         │  [Keep Local] [Keep Cloud] [Merge]      │
         └─────────────────────────────────────────┘
  4. Upload new projects/assets
  5. Download updates from cloud

26.8 DATABASE SCHEMA
--------------------

USERS TABLE:
  user_id (UUID, PRIMARY KEY)
  email (VARCHAR, UNIQUE, INDEXED)
  username (VARCHAR, UNIQUE, INDEXED)
  password_hash (VARCHAR, bcrypt)
  display_name (VARCHAR)
  avatar_url (VARCHAR, nullable)
  country (VARCHAR, 2-letter code)
  account_tier (ENUM: free, indie, studio)
  created_at (TIMESTAMP)
  last_login_at (TIMESTAMP)
  email_verified (BOOLEAN)
  two_factor_enabled (BOOLEAN)
  two_factor_secret (VARCHAR, encrypted)
  is_active (BOOLEAN)
  is_banned (BOOLEAN)
  ban_reason (TEXT, nullable)

PROJECTS TABLE:
  project_id (UUID, PRIMARY KEY)
  user_id (UUID, FOREIGN KEY → users)
  project_name (VARCHAR)
  project_slug (VARCHAR)
  description (TEXT, nullable)
  engine_version (VARCHAR)
  target_platforms (JSON ARRAY: ["pc", "android"])
  local_path (VARCHAR)
  cloud_path (VARCHAR, nullable)
  is_cloud_synced (BOOLEAN)
  is_shared (BOOLEAN)
  share_token (VARCHAR, UNIQUE, nullable)
  thumbnail_url (VARCHAR, nullable)
  file_size_bytes (BIGINT)
  created_at (TIMESTAMP)
  updated_at (TIMESTAMP)
  last_opened_at (TIMESTAMP)

ENGINE_VERSIONS TABLE:
  version_id (UUID, PRIMARY KEY)
  version_string (VARCHAR, e.g., "1.0.0-beta")
  version_type (ENUM: stable, beta, nightly)
  release_date (DATE)
  download_url_windows (VARCHAR)
  download_url_linux (VARCHAR)
  download_url_mac (VARCHAR)
  changelog (TEXT)
  is_latest (BOOLEAN)
  is_deprecated (BOOLEAN)

ASSETS TABLE (Asset Store):
  asset_id (UUID, PRIMARY KEY)
  uploader_id (UUID, FOREIGN KEY → users)
  asset_name (VARCHAR)
  asset_type (ENUM: plugin, model, texture, audio, script, template, material)
  description (TEXT)
  price (DECIMAL, 0.00 for free)
  file_url (VARCHAR)
  thumbnail_url (VARCHAR)
  rating_avg (DECIMAL, 1-5)
  download_count (INTEGER)
  is_approved (BOOLEAN)
  created_at (TIMESTAMP)

SESSIONS TABLE:
  session_id (UUID, PRIMARY KEY)
  user_id (UUID, FOREIGN KEY → users)
  token (VARCHAR, JWT)
  device_info (VARCHAR)
  ip_address (VARCHAR)
  created_at (TIMESTAMP)
  expires_at (TIMESTAMP)
  is_valid (BOOLEAN)

26.9 API ENDPOINTS
------------------

BASE URL: https://api.univexengine.com/v1/

AUTH ENDPOINTS:
  POST   /auth/register
  POST   /auth/login
  POST   /auth/logout
  POST   /auth/refresh
  POST   /auth/forgot-password
  POST   /auth/reset-password
  POST   /auth/verify-email

USER ENDPOINTS:
  GET    /user/profile
  PATCH  /user/profile
  GET    /user/projects
  DELETE /user/account

PROJECT ENDPOINTS:
  POST   /projects              → Create new project (requires engine installed)
  GET    /projects/:id
  PATCH  /projects/:id
  DELETE /projects/:id
  POST   /projects/:id/sync      → Upload/download sync
  POST   /projects/:id/share     → Generate share link
  GET    /projects/:id/collaborators

ENGINE ENDPOINTS:
  GET    /engine/versions        → List all versions
  GET    /engine/versions/latest
  GET    /engine/download/:version/:platform
  POST   /engine/verify          → Verify installed engine checksum

ASSET STORE ENDPOINTS:
  GET    /assets                 → Browse with filters
  GET    /assets/:id
  POST   /assets/:id/download
  POST   /assets                 → Upload (approved users only)
  GET    /assets/categories

ANALYTICS ENDPOINTS:
  POST   /analytics/crash        → Submit crash report
  POST   /analytics/session      → Track editor session
  GET    /analytics/project/:id  → Get project analytics (Studio tier)

ADMIN ENDPOINTS (Protected, admin-only):
  GET    /admin/users            → List all users (no passwords)
  GET    /admin/users/:id        → User details
  PATCH  /admin/users/:id/ban    → Ban/unban user
  GET    /admin/analytics        → Global stats
  GET    /admin/assets/pending   → Assets awaiting approval

26.10 TECH STACK RECOMMENDATIONS
--------------------------------

FRONTEND (News Website + Hub Dashboard UI):
  Framework:    Next.js 15 (React) — SSR for SEO, SPA for Hub
  Styling:      Tailwind CSS + custom glassmorphism utilities
  Animation:    Framer Motion (page transitions, hover effects, scroll animations)
  Icons:        Lucide React
  State:        Zustand (lightweight global state)
  Auth:         NextAuth.js (OAuth providers)

DESKTOP APP (UniVex Hub):
  Framework:    Tauri (Rust + WebView frontend)
  Why Tauri:     Small bundle size (~3MB vs ~150MB Electron), native performance,
                 Rust security, auto-updater built-in
  Alternative:   Electron (if Tauri limitations are hit)

BACKEND:
  API Server:     Node.js + Fastify (faster than Express) OR Go (compiled, fast)
  Database:       PostgreSQL (relational data) + Redis (sessions, rate limiting, cache)
  File Storage:   Cloudflare R2 (S3-compatible, cheaper) OR AWS S3
  CDN:            Cloudflare

AUTHENTICATION:
  Service:        Clerk (modern, handles OAuth, MFA, sessions) OR self-hosted
  2FA:            TOTP via speakeasy (Node.js)

PAYMENTS:
  Service:        Stripe (subscriptions, one-time purchases)
  Webhooks:       Stripe webhooks for subscription events

ANALYTICS:
  Product:        PostHog (open-source, self-hostable) OR Plausible (privacy-friendly)
  Crash Reports:  Sentry (self-hosted or cloud)

HOSTING:
  News Website:   Vercel (Next.js native, edge functions, global CDN)
  API Server:     Railway / Render / Fly.io
  Database:       Supabase (PostgreSQL) OR self-hosted on DigitalOcean
  Domain:         Cloudflare (DNS + DDoS protection + SSL)

26.11 IMPLEMENTATION ROADMAP
----------------------------

PHASE 1 — FOUNDATION (Weeks 1-4)
  ☐ Set up domain + hosting + SSL
  ☐ Design and implement database schema
  ☐ Set up authentication (register, login, OAuth, email verification)
  ☐ Build basic API (auth + user profile endpoints)
  ☐ Create News Website (static pages: Home, News, Docs, Community, Download)
  ☐ Design UniVex Hub UI mockups and prototype

PHASE 2 — HUB APP (Weeks 5-8)
  ☐ Build UniVex Hub desktop app (Tauri)
  ☐ Implement engine download system (auto-detect OS, progress, install)
  ☐ Build project manager (local projects only)
  ☐ Implement "Engine Gate" (block project creation if no engine)
  ☐ Connect Hub to API (login, fetch user data)

PHASE 3 — CLOUD SYNC (Weeks 9-12)
  ☐ Implement cloud project storage (upload/download)
  ☐ Build sync mechanism with conflict resolution
  ☐ Offline mode support with change queuing
  ☐ Asset Store backend (browse, purchase, download)
  ☐ Asset Store frontend in Hub

PHASE 4 — POLISH (Weeks 13-16)
  ☐ Payment integration (Stripe for Indie/Studio tiers)
  ☐ Account tier enforcement (project limits, feature gates)
  ☐ Analytics dashboard in Hub
  ☐ Community features (forums, showcase)
  ☐ Mobile-responsive News site
  ☐ Performance optimization (lazy loading, image optimization)
  ☐ Security audit (penetration testing, dependency scanning)

PHASE 5 — LAUNCH (Week 17+)
  ☐ Closed beta testing (invite-only)
  ☐ Bug fixes and polish
  ☐ Documentation (user guide, API docs, plugin dev guide)
  ☐ Public launch
  ☐ Marketing push (social media, dev communities, YouTube)

26.12 NOTES FOR CLAUDE (ENGINE DEVELOPER)
------------------------------------------

WHAT CLAUDE NEEDS TO BUILD (ENGINE FOCUS):
  The ecosystem (Hub, News, backend) is SEPARATE from the engine.
  Claude's primary responsibility is the ENGINE itself.

  Priority order for Claude:
    1. C++ Core Engine (Parts 1-15)
    2. Native C++ Visual Scripting (Part 8)
    3. Editor Application (Parts 9-11)
    4. Custom File Formats (Part 2)
    5. Build System (Part 12)
    6. Sample Project (Part 13)
    7. Additional Features (Parts 16-25)

  The ecosystem (Part 26) can be built by another developer or team
  while Claude works on the engine. However, the engine must expose:
    • Engine version string (for Hub compatibility check)
    • Command-line interface for Hub to open projects
    • Build/export commands callable from Hub
    • Crash log path (for Hub to upload)

  ENGINE ↔ HUB INTEGRATION POINTS:
    • Hub calls engine executable with --project <path> to open project
    • Hub calls engine with --build <platform> to trigger build
    • Hub reads engine's stdout for build progress
    • Engine writes crash logs to known path for Hub to find
    • Hub passes --server flag for multiplayer testing

================================================================================
                          END OF MASTER DOCUMENT
                    UniVex Engine — Complete Specification
                         Parts 1 through 26
================================================================================
