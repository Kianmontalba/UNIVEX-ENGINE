<div align="center">

<h1><strong>UNIVEX ENGINE — COMPLETE DEVELOPMENT ROADMAP</strong></h1>

<strong>Native C++ Engine and First-Generation Scene Editor</strong><br/>
<strong>Last updated: August 2026</strong>

</div>

> **Status rule:** `COMPLETED` means the increment was delivered as a finished, verified milestone. `PARTIAL` means the milestone is planned or incomplete and must not be described as a shipped capability until implementation and verification are evidenced. An increment that affects the viewport must include a real desktop/OpenGL visual check, not only a headless unit test.

| Column | Meaning |
|---|---|
| **Status** | Delivery state of the increment. |
| **Increment** | Stable sequential milestone number. |
| **Outcome** | The lasting engine or editor capability added by the milestone. |
| **Verification / boundary** | Evidence or intentionally excluded scope that keeps the milestone honest. |

<div align="center">

<h2><strong>COMPLETED — CORE, SCENE, AND ASSET FOUNDATIONS</strong></h2>

</div>

| Status | Increment | Outcome | Verification / boundary |
|---|---:|---|---|
| **COMPLETED** | **1** | Foundation layer: `LoggerUVE`, `TimerUVE`, `EventSystemUVE`, and `EngineCoreUVE` frame lifecycle. | Established the engine startup, update, render, and shutdown foundation. |
| **COMPLETED** | **2** | Memory manager plus pool, stack, and heap allocators. | Native RAII-oriented memory foundation. |
| **COMPLETED** | **3** | Thread-pool job system with work-stealing queue. | Core asynchronous-work foundation. |
| **COMPLETED** | **4** | `ConfigManagerUVE` and command-line parsing. | Versioned configuration and executable-option foundation. |
| **COMPLETED** | **5** | Scene/ECS core: entity manager and scene graph. | Real scene ownership; no duplicate editor-only scene system. |
| **COMPLETED** | **6** | Prefab system and scene serializer. | Persistent scene/prefab foundation through `.uve*` envelopes. |
| **COMPLETED** | **7** | Asset manager, generic importer, hot reload, and asset bundle foundation. | Asset lifecycle and loading boundary. |
| **COMPLETED** | **8** | Virtual file system. | Mount-based path resolution foundation. |

<div align="center">

<h2><strong>COMPLETED — RENDERING, PHYSICS, AND RUNTIME SYSTEMS</strong></h2>

</div>

| Status | Increment | Outcome | Verification / boundary |
|---|---:|---|---|
| **COMPLETED** | **9** | Rendering math foundations. | Matrices, vectors, quaternions, bounds, and frustum utilities. |
| **COMPLETED** | **10** | Render Hardware Interface foundation. | Backend-agnostic render-device and command-buffer contracts. |
| **COMPLETED** | **11** | Camera system with view/projection matrices and frustum culling. | Real camera/culling foundation. |
| **COMPLETED** | **12** | Rendering-facing asset types. | Typed rendering data boundary. |
| **COMPLETED** | **13** | Mesh renderer and render queue. | Scene-to-render submission foundation. |
| **COMPLETED** | **14** | `Renderer3DUVE` frame orchestration. | Centralized scene-rendering pipeline. |
| **COMPLETED** | **15** | Physics and collision systems. | Core collision simulation foundation. |
| **COMPLETED** | **16** | Raycast system and physics material support. | Queryable collider interaction foundation. |
| **COMPLETED** | **17** | Input system foundations. | Native input actions and bindings. |
| **COMPLETED** | **18** | Audio system foundations. | Device-independent audio baseline. |
| **COMPLETED** | **19** | Save game and checkpoint foundations. | Persistent runtime-state baseline. |
| **COMPLETED** | **20** | GLFW window manager and real OpenGL render device. | Native desktop window and OpenGL execution path. |
| **COMPLETED** | **21** | Shader manager with loading, hot reload, and uniform reflection. | Runtime shader lifecycle foundation. |
| **COMPLETED** | **22** | Unlit textured materials. | First material path through `Renderer3DUVE`. |
| **COMPLETED** | **23** | Basic directional lighting. | Lit 3D scene foundation. |
| **COMPLETED** | **24** | Metallic/specular PBR uniform plumbing. | Material parameter expansion without a second renderer. |
| **COMPLETED** | **25** | Point, spot, and multi-light support. | Multi-light scene lighting foundation. |
| **COMPLETED** | **26** | Directional-light shadow mapping. | First real shadow pipeline. |

<div align="center">

<h2><strong>COMPLETED — RENDERING MATURITY AND DEVELOPER PLATFORM</strong></h2>

</div>

| Status | Increment | Outcome | Verification / boundary |
|---|---:|---|---|
| **COMPLETED** | **27** | Shadow-aware canonical material shader. | Unified lit/shadowed material foundation. |
| **COMPLETED** | **28** | Bounded PCF soft shadows. | Improved shadow filtering without uncontrolled sampling cost. |
| **COMPLETED** | **29** | Camera-fitted directional shadow frustum. | Better shadow coverage aligned to the camera. |
| **COMPLETED** | **30** | Cascaded directional shadows. | Larger-scale directional-light shadow coverage. |
| **COMPLETED** | **31** | Shadow cascade transition blending and stabilization. | Reduced cascade transition/shimmer artifacts. |
| **COMPLETED** | **32** | Tangent-space normal mapping. | Surface-normal detail path. |
| **COMPLETED** | **33** | Separate-stage material shader management. | Better shader-stage composition boundary. |
| **COMPLETED** | **34** | GGX Smith direct-lighting path. | More physically grounded direct-light response. |
| **COMPLETED** | **35** | Deterministic render-graph foundation. | Explicit render-pass dependency/order foundation. |
| **COMPLETED** | **36** | Fullscreen tone-mapping pass. | Renderer-owned final color transform. |
| **COMPLETED** | **37** | GCC CI verification and GLFW/X11 build coverage. | Repeatable pull-request build/test enforcement. |

<div align="center">

<h2><strong>COMPLETED — FIRST-GENERATION SCENE EDITOR</strong></h2>

</div>

| Status | Increment | Outcome | Verification / boundary |
|---|---:|---|---|
| **COMPLETED** | **38** | Editor Foundation v1 workflow. | Native editor shell, workspace composition, and real engine lifecycle integration. |
| **COMPLETED** | **39** | Viewport picking and Translate gizmo. | Collider-backed scene selection and transform mutation path. |
| **COMPLETED** | **40** | Content Browser and scene-entity creation. | Real scene creation controls and initial content tooling. |
| **COMPLETED** | **41** | Persistent entity naming. | Human-readable names survive scene save/load. |
| **COMPLETED** | **42** | Undo/Redo history. | Authoring commands gain bounded reversible transactions. |
| **COMPLETED** | **43** | Editor camera navigation. | Orbit, pan, zoom, and focus without document mutation. |
| **COMPLETED** | **44** | Entity duplicate and delete with Undo/Redo. | Scene lifecycle commands with history restoration. |
| **COMPLETED** | **45** | Hierarchy reparenting with Undo/Redo. | Real parent-child authoring workflow. |
| **COMPLETED** | **46** | Rotate gizmo. | World-axis rotational authoring path. |
| **COMPLETED** | **47** | Scale gizmo. | Positive local-scale authoring path. |
| **COMPLETED** | **48** | Transform snapping. | Deterministic translate/rotate/scale increments. |
| **COMPLETED** | **49** | Viewport selection bounds. | Read-only visible selection feedback. |
| **COMPLETED** | **50** | Play Mode Sandbox. | Transient Play/Pause/Step/Stop document isolation. |
| **COMPLETED** | **51** | Local-axis and plane gizmos. | Parent-aware local manipulation and plane translation. |
| **COMPLETED** | **52** | Hierarchy search and inline rename. | Usable real scene-outliner search/name workflow. |
| **COMPLETED** | **53** | Keep-world reparenting. | Explicit world-transform-preserving hierarchy option. |
| **COMPLETED** | **54** | Uniform scale offset gizmo. | Safe shared local-scale editing behavior. |
| **COMPLETED** | **55** | Free-rotation trackball gizmo. | Camera-oriented arbitrary-axis rotation interaction. |
| **COMPLETED** | **56** | Multi-selection foundation. | Ordered selection with a single active entity. |
| **COMPLETED** | **57** | Fixed UniVex workspace layout. | Clear hierarchy/viewport/Inspector/lower-dock organization. |
| **COMPLETED** | **58** | Viewport scene rendering with built-in primitives. | Cube, UV Sphere, Plane, grid feedback, primitive Inspector controls, and renderer extraction foundation. |
| **COMPLETED** | **59** | Cached project asset browser. | Deterministic read-only `ProjectFileIndexUVE` and AssetDatabase correlation. |
| **COMPLETED** | **60** | Import work queue and derived-artifact cache. | Deterministic main-thread jobs, fingerprints, diagnostics, and cache metadata. |
| **COMPLETED** | **61** | Project Change Watch and targeted reload. | Portable polling, copied bounded journal, explicit refresh acknowledgement, and stale-cache marking. |
| **COMPLETED** | **62** | Inspector Drawer Registry v1. | Deterministic Name/Transform/Primitive drawer seam while preserving existing command/history ownership. |

<div align="center">

<h2><strong>COMPLETED — MULTI-LANGUAGE EDITOR FOUNDATIONS</strong></h2>

</div>

| Status | Increment | Outcome | Required proof before status becomes COMPLETED |
|---|---:|---|---|
| **COMPLETED** | **63** | **Viewport Presentation & Render Verification v1**: fixed actual primitive presentation, added an intentional blue-gray empty-scene environment, renderer evidence diagnostics, and real final-back-buffer verification. | Real-GL/Xvfb tests prove red Cube, green Plane, and blue UV Sphere pixels after tone mapping; GCC, Clang, CI, headless, and desktop smoke checks passed. |
| **COMPLETED** | **64** | **Editor Tool Sessions v1**: explicit begin/preview/commit/cancel ownership separates gizmo pointer solving from transform transaction state. | Session tests prove strict re-entrant Begin rejection, no-second-write commit, zero-delta completion, cancellation handoff, and restore-failure evidence; existing editor parity tests plus GCC, Clang, CI, and desktop smoke checks passed. |
| **COMPLETED** | **65** | **Scene Outliner & Inspector Workflow v2**: deterministic type/context metadata, read-only ancestry, and a command-backed Hierarchy Inspector section extend the existing hierarchy/reparent workflow. | Fixed tag priority, ancestry/candidate safety, drawer registration, existing reparent/history parity, GCC, Clang, CI, and desktop smoke checks passed. |
| **COMPLETED** | **66** | **Content Browser Workflow v2**: cached folder navigation, clickable breadcrumbs, deterministic extension type tags, independent Registered badges, and persistent focused filtering extend the read-only FileSystem dock. | Project index/watcher trailing-root regressions, Content Browser tag/navigation regressions, GCC, Clang, CI/Ninja, and desktop smoke checks passed. |
| **COMPLETED** | **67** | **Editor Session Settings & Layout v1**: versioned editor-only preferences, validated visibility state, viewport preferences, and fixed approved layout presets. | Explicit-save migration, invalid-field fallback, preset/non-scene-mutation regression, GCC, Clang, CI/Ninja, and desktop smoke checks passed. |
| **COMPLETED** | **68** | **Project Health & Headless Automation v1**: read-only `uve_project_check` validates registry snapshots and supported universal asset envelopes with deterministic text/JSON diagnostics. | Per-file failure isolation, dependency-aware multi-diagnostic aggregation, read-only fixtures, documented exit mapping, GCC, Clang, CI/Ninja, and headless CLI checks passed. |
| **COMPLETED** | **69** | **Editor Bridge Contract v1**: versioned C++ bridge DTOs, copied revisioned snapshots, capability discovery, stable diagnostics, and stale mutation protection over existing editor commands. | Native-ImGui-change observation, stale mutation rejection, create/name/Undo/Redo routing, protocol/entity safety, GCC, Clang, CI/Ninja, and desktop smoke checks passed. The framed host transport is delivered by Increment 70. |
| **COMPLETED** | **70** | **C# Editor Host Foundation v1**: optional .NET 8/Avalonia connection shell, typed copied bridge client, local framed JSON-RPC over stdio, protocol handshake/capability display, and headless C++ bridge-server mode. | Bridge stdio is mutually exclusive with native ImGui per process; stdout is protocol-only; malformed frames/JSON and incompatible protocol are contained; crash/EOF never auto-restarts; fresh-session and dirty-close loss acknowledgements are explicit. Native transport, managed unit, real C#→C++ probe, GCC, Clang, CI/Ninja, and desktop smoke validation are required. |
| **COMPLETED** | **71** | **C# Editor Shell & Docking v1**: resizable fixed-region Avalonia shell with menu/workspace strips; Scene, Viewport, Inspector/Import/Signals, and lower dock tabs; honest deferred panel cards; and managed-only layout persistence. | No floating docks, C# authoring, native viewport surface, or GL ownership. A recognized v0 layout migrates in memory to v1; corrupt/unrecognized/future files retain safe defaults without overwrite. Only **Save Shell Layout** writes preferences; close/reconnect/crash never autosave. Layout/migration/capability regressions, managed Xvfb no-autosave smoke, bridge probe, GCC, Clang, CI/Ninja, and native desktop smoke are required. |
| **COMPLETED** | **72** | **C# Hierarchy, Inspector & Content Browser v1**: real Avalonia Scene, Inspector, and FileSystem presentation over strictly parsed immutable C++ bridge snapshots; named C++ command actions for selection, name edit, hierarchy filter, and cached content navigation/filter/focus/refresh/selection. | C++ continues to own ECS/entity lifetime, selection authority, drawer eligibility, command/history/dirty/Play guards, project index/cache, breadcrumbs/type/asset registration facts, filesystem validation, backend process, renderer, and GL. Snapshot rows/text are bounded with explicit truncation facts; all request mutations are revision-protected. Native bridge, stdio transport, managed parser/dispatch, GCC, Clang, CI/Ninja, real probe, Xvfb host, and desktop smoke validation are required. |
| **COMPLETED** | **73** | **GLSL Viewport Visuals v1**: renderer-owned analytic viewport grid, major-axis accents, active-selection outline, axis widget, and gizmo-axis visual state through an additive alpha-composited GLSL pass. | C++ owns the copied visual state, renderer pass ordering, RHI blend state, and ImGui input/gizmo semantics; GLSL owns only bounded visual composition. Runtime rendering is unchanged when editor visuals are disabled. Shader parity, GCC, Clang, CI/Ninja, real OpenGL/Xvfb desktop smoke, managed host smoke, and full native CTest validation passed. |
| **COMPLETED** | **74** | **Managed Viewport Surface Lifecycle v1**: typed C++↔C# surface-state contract, generation/dimension metadata, explicit unavailable/detached/native-owned states, and managed shell lifecycle presentation. | Headless bridge sessions report `Unavailable` with native renderer ownership and `managedAttachAllowed=false`; no raw window handle, GL context, texture, picking pass, or input-forwarding path crosses into C#. Native bridge/stdio, managed parser/dispatch, GCC/CI, managed 36-test suite, real bridge probe, native Xvfb, and managed no-autosave smoke passed. |
| **COMPLETED** | **75** | **Native C++ Visual Scripting Core v1**: stable node registry descriptors, typed pins, authored graph nodes/links, structural mutation guards, and deterministic validation diagnostics. | C++ owns the graph model and registry; exact typed connections, direction, unknown-node/pin, duplicate-link, self-link, and missing-endpoint cases are validated without a managed runtime, compiler invocation, bytecode VM, or C# graph UI. Native focused regression suite passed 9/9; full GCC/Clang/CI validation is required before merge. |
| **COMPLETED** | **76** | **Visual Scripting Compiler IR v1**: validation-first graph lowering to a versioned descriptive IR with deterministic node/link ordering and source-node mapping. | Invalid graphs produce diagnostics and no partial program. The IR is native and non-executable; bytecode, VM, engine bindings, debugger, hot reload, and managed graph presentation remain separate increments. Focused native scripting suite passed 11/11; full GCC/Clang/CI validation is required before merge. |
| **COMPLETED** | **77** | **Versioned Visual Script Bytecode v1**: bounded `UVES` bytecode envelope, explicit version/instruction count, deterministic encoding, and strict decode diagnostics. | Magic, version, truncation, instruction-cap, opcode, and trailing-byte failures publish diagnostics without partial programs. Bytecode remains non-executing and has no implicit engine-call bindings. Focused IR/bytecode suite passed 5/5; full GCC/Clang/CI validation is required before merge. |
| **COMPLETED** | **78** | **Bounded Native Visual Script VM v1**: native bytecode dispatch with explicit instruction budget, completion status, and deterministic execution diagnostics. | Unsupported versions, invalid instruction kinds, and budget exhaustion halt safely with instruction indices; no ECS, renderer, input, or engine-call binding is implicit. Focused IR/bytecode/VM suite passed 8/8; full GCC/Clang/CI validation is required before merge. |


<div align="center">

<h2><strong>PARTIAL — INCREMENT 74+ ENGINE AND TOOLING BACKLOG</strong></h2>

</div>

| Status | Roadmap area | Intended direction | Entry condition |
|---|---|---|---|
| **PARTIAL** | Native C++ Visual Scripting | Continue the native C++ node registry, graph compiler, and bytecode/VM direction. | Stable scene/editor command and property contracts. |
| **PARTIAL** | Plugin architecture | Design a native extension contract with lifecycle, ownership, ABI/versioning, and safety rules. | Stable editor and scripting foundations. |
| **PARTIAL** | Asset pipeline depth | Add format-specific model, texture, audio, material, and thumbnail workflows only after format/licensing/derived-data contracts are selected. | Project Health and Content Browser foundations. |
| **PARTIAL** | Advanced editor tools | Animation, terrain, world streaming, profiling, cinematics, source control, and collaboration. | Proven viewport, scene, asset, and session foundations. |
| **PARTIAL** | Platform and shipping workflow | Cooking, packaging, deployment, and sample-project growth. | Stable asset pipeline, project validation, and renderer presentation. |

<div align="center">

<h2><strong>FIRST-GENERATION SCENE EDITOR — COMPLETION GATE</strong></h2>

</div>

| Area | Required condition |
|---|---|
| **Viewport** | Real geometry is visible and usable; an empty scene is intentionally presented rather than plain black. |
| **Navigation** | Orbit, pan, zoom, and focus operate on the editor camera without scene mutation. |
| **Hierarchy** | The Outliner displays real parent-child data and selection stays synchronized with viewport/Inspector. |
| **Inspector** | Every editable property is backed by a validated engine command and actual scene state. |
| **Transform tools** | Select, Move, Rotate, Scale, snapping, cancellation, and Undo/Redo are real testable behaviors. |
| **Content Browser** | Displays deterministic real project content, not mock folders or fake asset cards. |
| **Toolbar and menus** | Every displayed action is implemented; unavailable work remains disabled, hidden, or explicitly future scope. |
| **Evidence** | Progress reports distinguish **[DESIGNED]**, **[IMPLEMENTED]**, **[COMPILED]**, **[TESTED]**, **[PROFILED]**, and **[HARDENED]**. |

> **Technology direction:** UniVex remains a native **C++20** engine/editor with CMake and engine-owned scene, history, renderer, command, project-index, and OpenGL authority. GLSL belongs to viewport, grid, selection, and gizmo visuals. JSON/TOML-style data belongs to versioned editor settings and presets. SVG/PNG/WebP belong to future icon/resource pipelines. Python is optional only for offline asset processing or build automation. The optional **C#/.NET host, fixed shell, and Hierarchy/Inspector/FileSystem snapshot presentation are COMPLETED** as a separate headless-bridge client; full scene authoring, native viewport hosting, import review, and later workflows remain **PARTIAL**.
