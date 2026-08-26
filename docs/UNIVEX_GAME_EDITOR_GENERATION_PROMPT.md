# UniVex Engine — Game Editor Generation Prompt

## How to use this prompt

Copy the **Master Prompt** into the image-generation model. It is written to generate a visual reference for the real UniVex Engine editor. The result must look like a working game-editor application, not an infographic or a static feature list. The exact UVE logo and folder icon must be supplied as image references when the generation system supports reference images; they must not be redrawn.

The current engine has many systems, so the prompt asks for a wide primary workspace plus focused companion states. One image cannot prove every runtime behavior. The generated reference is therefore a design target for editor presentation, while implementation must continue to follow the real source contracts in `UNIVEX_EDITOR_FEATURE_INVENTORY.md`.

## Master Prompt — primary editor workspace

```text
Create a high-fidelity visual reference of the native UniVex Engine game editor, a professional desktop 3D game-development workspace in a neutral charcoal theme. This is a functional editor design reference, not a poster, infographic, marketing page, or feature-list card. Show actual visual workflows and controls: hierarchy rows, selectable scene entities, typed inspector fields, a real 3D viewport, transform handles, a real node graph, project content rows, asset states, and diagnostic status. Do not merely write feature names into empty panels.

Use a wide 21:9 desktop application composition at 3440x1440 or a dense 16:9 equivalent. Use restrained typography, compact spacing, thin neutral borders, low-glare charcoal surfaces, clear hover/pressed/selected states, and a strong but professional hierarchy. Use the supplied exact-preservation UniVex/UVE logo in the title chrome and the supplied exact-preservation folder artwork for directory rows. Do not redraw, redesign, recolor, stylize, replace, or invent a new logo or folder icon.

Application shell:
- Native desktop editor window with a narrow title bar, menu row, compact tool row, and docked workspaces.
- Title chrome shows the exact UVE mark, current workspace, saved/unsaved state, selected-entity count, and edit/paused/playing state.
- Menu groups are File, Edit, Assets, GameObject, Component, Window, and Help.
- Main tool row visibly contains Hand/navigation, Scene, Scripting, Move XYZ, Rotate XYZ, Scale XYZ, Global/Local space, Play, Stop, Android status, and Layout. Keep the controls compact and credible; do not make unsupported controls appear active.

Main Scene workspace:
- Left panel is a real scene hierarchy with expandable parent/child rows, entity icons kept simple and semantic, a filter field, an Add Node plus button, and a compact scene-root status.
- The Add Node palette is categorized and shows real UVE node families: Empty, Marker3D, Camera3D, MeshInstance3D, BoxMesh3D, SphereMesh3D, PlaneMesh3D, Light3D, Area3D, RayCast3D, StaticBody3D, AnimatableBody3D, CharacterBody3D, Collider3D, RigidBody3D, NavigationRegion3D, NavigationAgent3D, Skeleton3D, BoneAttachment3D, SpringArm3D, Hitbox3D, Hurtbox3D, Projectile3D, InteractionArea3D, WorldEnvironment3D, ReflectionProbe3D, Decal3D, LODGroup3D, Occluder3D, VisibilityRegion3D, SpawnPoint3D, LevelStreamer3D, WorldPartition3D, AnimationPlayer, AudioSource3D, ParticleEmitter3D, and Script. `AnimationTree` may appear as a registered non-creatable contract, visibly unavailable rather than falsely addable.
- Center panel is a real perspective 3D viewport showing an authored scene, not random demo content: a selected mesh, an authored light, actual material response, real shadows, a floor or mesh only when it belongs to the authored scene, a clean grid, and a small XYZ orientation widget. Show no fake daylight or hidden preview light.
- In the viewport, make the selected object’s active gizmo unmistakably six-axis. Show red X, green Y, and blue Z translation axes and translation planes for Move XYZ, or three colored rotation rings for Rotate XYZ. Include a second visible scale state or compact mode strip for Scale XYZ. The gizmo must look pointer-interactive and attached to the selected entity, not like a decorative symbol.
- Do not show orthographic camera controls as if they are implemented. Keep the current verified viewport representation perspective-oriented.
- Right panel is a real Inspector titled DETAILS SELECTION. Show typed sections for Name, Transform with Position/Rotation/Scale XYZ, World Transform context, Mesh asset reference, Light or Collider properties, and Add Component. Show fields as editable controls with realistic values, not paragraphs of explanatory text.

Bottom dock:
- Left bottom tab is Project / Filesystem. It has a compact header reading `FILESYSTEM | N entries | auto`, breadcrumb buttons, a current-folder filter, a type filter, and a Clear Filters action. Directory rows show the supplied exact-preservation folder artwork followed by a real folder name and `[Folder]`; file rows use semantic type labels such as `[Scene]`, `[Mesh]`, `[Texture]`, `[Shader]`, `[Material]`, `[Audio]`, `[Animation]`, or `[File]`.
- Do not include a large Review Changes section. Do not include a normal manual Refresh button. Show a small Retry action only as a genuine scan-failure state, not in the normal view. The panel should imply automatic watcher-driven refresh through subtle status, not a fake progress animation.
- A selected asset row may show registered GUID, source/derived identity, import queue state, hot-reload state, or a concise diagnostic. Do not invent thumbnails for unsupported formats.
- Right bottom tab is Console with clear, All, search, developer output, and command-entry surfaces. Show authorized console status only; do not imply cloud logging or an online service.
- The editor is supported by real EngineCore foundations—frame/timer update, ECS/entity manager, scene graph, events, memory, thread-pool jobs, configuration, command line, virtual file system, and dependency-safe lifecycle—but these are runtime authorities, not decorative standalone dashboards. Show their effects through hierarchy, play state, project paths, and concise status only; do not invent a memory profiler, task-graph visualizer, or event-bus dashboard.

Scripting workspace indication:
- Include a visible Scripting tab or a secondary synchronized workspace preview. It must show a real native typed Visual Scripting graph with connected nodes, execution pins, data pins, typed Vector3/Boolean/Number/Entity/Transform values, and compact node categories. Use actual node examples such as `flow.branch`, `input.get_action`, `math.vector3.add`, `physics.raycast`, `entity.spawn`, `animation.play`, `audio.play_sound`, `variable.get_vector3`, `debug.print`, `motion.query.search`, and `motion.query.get_best_match`.
- The graph should look editable: node selection, link routing, pin colors by type, a small searchable palette, and a diagnostic indicator tied to a node. Do not show a fake text editor, a generic flowchart, or unsupported arbitrary reflection.

Animation and plugin context:
- Show a compact Animation/Plugin workspace tab or side panel with a real Skeleton3D/Control Rig context only when a mesh and skeleton are present. Use visible Box/Circle/Arrow control shapes, root/spine/head and hand/foot IK control concepts, reset/mirror/bake-sample actions, and revision/status facts. An empty Skeleton3D must not contain invented bones.
- Show Motion Query as a plugin data/editor workflow with clip/database entries, future trajectory samples, pose/match candidates, feature/search status, interaction context, transition state, and optional capsule dimensions shared with collision prediction. Do not present it as a fully implemented direct renderer-owned motion-editing viewport or a complete takedown/ragdoll state machine.

Visual language and interaction truth:
- Neutral charcoal, graphite, muted steel, warm gray text, and controlled red/green/blue transform accents. No neon cyberpunk styling, glossy marketing gradients, excessive rounded cards, or giant hero typography.
- Show compact empty, selected, error, and playing states as real editor states. Empty scene means no authored scene roots, no fake daylight, no random objects, no decorative demo environment, and no hidden scene entities; it can show a professional `NO SCENE OPEN` card with Create Scene, Open Scene, and Import Asset routes.
- The UI must feel like one coherent native game editor. Every button must correspond to a real UVE contract or be visibly disabled/future. Do not show marketplace, online collaboration, cloud accounts, payments, socket servers, package export, executable/APK generation, full terrain/world-streaming tooling, completed post-processing controls, or unsupported importers as active features.
- Avoid third-party engine logos, copied branding, copied layout, or third-party-specific architecture terminology. Create an original UniVex presentation based on practical game-editor workflow conventions.
- Render crisp, aligned UI at native desktop scale. Text should be sparse and legible. Prioritize actual controls, panels, selection feedback, hierarchy, inspector structure, viewport composition, and believable interaction affordances over decorative labels.
```

## Focused companion prompts

### A. Scene, viewport, and six-axis gizmo state

```text
Create a high-fidelity native UniVex Engine Scene Editor screenshot in a neutral charcoal desktop theme. Show a real authored 3D scene in a perspective viewport with grid, XYZ orientation widget, selected MeshInstance3D, real material response, and actual authored light/shadow. Show the complete six-axis transform workflow: a prominent Move XYZ state with red X, green Y, blue Z axes and translation planes, plus a visible toolbar containing Move XYZ, Rotate XYZ, Scale XYZ, and Global/Local. Include a second small in-editor preview state showing three colored Rotate XYZ rings around the same selected object. The right Inspector must contain editable Name, Position XYZ, Rotation XYZ, Scale XYZ, Mesh, Light/Collider, and Add Component sections. The left Hierarchy must show real parent/child selection and a compact Add Node plus button. Do not add fake daylight, random objects, a hidden environment, an orthographic control, or a decorative demo scene. Do not depict the gizmo as a flat three-axis icon; it must be an interactive six-axis transform tool attached to the selected object. Use the supplied exact-preservation UVE logo in the title bar and do not redesign it.
```

### B. Visual Scripting workspace

```text
Create a high-fidelity native UniVex Engine Visual Scripting editor screenshot, integrated into the same neutral charcoal game-editor shell. Show a real editable graph canvas with connected nodes, execution links, typed data links, pin-direction clarity, pin colors by value type, node selection, pan/zoom affordances, and a compact searchable node palette. Use only real UVE node examples: flow.branch, flow.sequence, input.get_action, math.vector3.add, math.transform.translate, physics.raycast, entity.spawn, animation.play, audio.play_sound, variable.get_vector3, debug.print, motion.query.search, and motion.query.get_best_match. Show Vector3, Boolean, Number, Entity, Asset, Component, Rotation, Transform, Array, Map, Set, and Struct as typed concepts, but do not fill the screen with a text list. Include a small compiler/validation diagnostic attached to one node and a bounded graph status, not a fake code editor or generic flowchart. Keep the graph visually connected and usable, not randomly scattered. Do not invent unsupported nodes or imply arbitrary reflection, online collaboration, or a fully separate runtime service.
```

### C. Filesystem and asset workflow

```text
Create a high-fidelity native UniVex Engine Project/Filesystem editor screenshot in the neutral charcoal theme. Show a compact bottom Project dock with the exact header `FILESYSTEM | N entries | auto`, breadcrumb navigation, folder filter, type filter, Clear Filters, and a folder-first project tree. Directory rows must display the supplied exact-preservation UVE folder artwork; file rows must use semantic labels such as [Scene], [Mesh], [Texture], [Shader], [Material], [Audio], [Animation], or [File]. Show one selected asset with real facts: relative path, registered GUID when available, source/derived identity, import queue state, or hot-reload state. Do not show a large Review Changes area, a normal manual Refresh button, fake thumbnails for unsupported formats, fake importer success, or a fake operating-system file dialog. Show only a small Retry control if the screenshot is deliberately depicting a genuine scan failure. Use the supplied exact-preservation UVE logo in the editor title and do not alter either artwork.
```

### D. Animation, Control Rig, Motion Query, and diagnostics

```text
Create a high-fidelity native UniVex Engine Animation and Plugin workspace reference in the neutral charcoal game-editor shell. Show an authored character mesh with a real Skeleton3D context, visible animator control shapes using Box, Circle, and Arrow forms, root/spine/head controls, hand and foot IK/pole controls, reset and side-mirror actions, bake-sample status, and a revisioned viewport-ready snapshot. Beside it, show a Motion Query data workspace with clip/database entries, future trajectory samples, pose/match candidates, feature/search status, transition state, interaction context, and optional capsule dimensions shared with collision prediction. Include a compact validation/console area with bounded diagnostics. Do not put invented bones into an empty Skeleton3D, do not show full automatic takedown/ragdoll state-machine behavior, do not show arbitrary raw FBX decoding, and do not present Motion Query as a completed direct renderer-owned editing viewport. Use original UVE presentation, not copied third-party branding or layout.
```

## Negative prompt / rejection conditions

```text
Reject the result if it is a poster, infographic, landing page, dashboard-only mockup, or text-heavy feature list. Reject giant labels that replace real controls. Reject fake daylight, random startup geometry, hidden preview lights, decorative demo assets presented as project content, fake importer success, fake Review Changes UI, a normal manual Refresh button in the normal Filesystem state, fake OS dialogs, fake online users, fake cloud accounts, fake marketplace/payment panels, fake socket/network panels, fake package/export buttons, fake EXE/APK generation, unsupported terrain/world streaming, unsupported post-processing controls, unsupported orthographic controls, a flat three-axis-only gizmo, a disconnected random node graph, invented bones, distorted logo artwork, redesigned folder artwork, third-party engine branding, copied third-party layout, neon cyberpunk styling, excessive gradients, excessive rounded cards, or unreadable microtext.
```

## Implementation handoff rules

The generated image is a visual target only. The implementation must remain native to UniVex Engine and must connect each visual state to a real engine contract. A panel is not complete because its label exists. A button is not complete because it is visible. The implementation must provide the underlying state, command, service call, error path, persistence behavior, and regression coverage where the current engine supports that boundary.

The highest-priority visual states to implement from the generated reference are:

| Priority | Editor state | Required truth |
|---:|---|---|
| 1 | Scene viewport | Authored ECS scene, authored lighting, perspective viewport, grid, selection, and six-axis Move/Rotate/Scale gizmos. |
| 2 | Filesystem | Automatic watcher-driven index refresh, compact panel, exact logo/folder artwork, real failure/retry behavior. |
| 3 | Inspector | Typed component drawers for real scene components, transform editing, Add Component, undo/redo, and Play-mode guards. |
| 4 | Visual Scripting | Native typed graph editor backed by the real registry, validation, compiler/VM, persistence, debugger, and hot reload contracts. |
| 5 | Animation/plugins | Real Control Rig and Motion Query data/authoring contracts without inventing unsupported direct viewport or playback behavior. |
| 6 | Diagnostics | Developer Console authorization, project checks, import/hot-reload status, bridge state, concise errors, and truthful runtime/service status. |

The exact feature inventory and source references are in [`UNIVEX_EDITOR_FEATURE_INVENTORY.md`](UNIVEX_EDITOR_FEATURE_INVENTORY.md).
