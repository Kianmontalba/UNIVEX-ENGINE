# UVE Editor Senior Redesign Audit

## Supplied brief

The supplied `UVE_Editor_Senior_Redesign_Prompt.docx` requires continuation of the existing native editor rather than a mockup or rewrite. It requires preserving real scene, node, component, inspector, filesystem, asset, rendering, input, and editor systems. The central viewport must remain a genuine camera-aware 3D editor environment with procedural/infinite ground and grid, no flat sky card, finite plane, hard seam, or fake scene lighting. The palette specified is sky `#3F4852`, ground `#252A2F`, transition `#48525C`, grid `#59636D`, and selection/gizmo `#4DA3FF`.

The layout requirements are a central viewport priority, Scene/Hierarchy on the left, Inspector/Import/Signals on the right, and a collapsible/resizable Filesystem/Contents bottom dock. Play and Stop must remain in a stable visible runtime-control location. Hierarchy and filesystem names must be clean user names with type metadata separate; strings such as `test [Folder]`, `Player [node]`, and `Node3 [scene]` are forbidden. Existing real watcher/index, context-menu, drag/drop, and plugin behavior must be preserved. Existing node/component icons should be reused, with no emoji, generic circles, or incompatible second icon system.

## Uploaded icon package

`uve_node_icons.zip` contains 51 SVG assets: 41 in `3d_node_set/`, six in `component_set/`, and four in `general/`. The node set includes scene-node keys such as `camera_3d`, `mesh_instance_3d`, `world_environment_3d`, `skeleton_3d`, `character_body_3d`, and `collision_shape_3d`; the component set includes `collider`, `hierarchy`, `name`, `prefab_instance`, `primitive_mesh`, and `world_transform`; the general set includes `environment`, `plugin`, `snap`, and `sun`. SVG assets are unchanged source files and are rasterized to deterministic 20x20 RGBA embedded arrays for native OpenGL use.

## Current implementation decisions

The editor has no WebView/CEF runtime, so supplied HTML gizmo documents remain design/reference sources; native C++/OpenGL/ImGui remains the runtime. The current branch already contains the native infinite viewport environment and gizmo-family implementation. The present increment extends the native icon service with keyed node/component/general catalogs, adds clean-name presentation fixes, and uses truthful ECS component-backed fallback mapping for hierarchy icons because the current entity model does not retain a dedicated authored node-kind field.

## Adaptive 3D axis grid increment

The editor viewport environment now uses a procedural infinite world grid rather than a fixed grid texture or finite plane. Grid spacing is selected from world-units-per-pixel using logarithmic decade levels; current and next major levels cross-fade smoothly, minor subdivisions are derivative-antialiased with bounded widths, and line intensity fades with ground distance. X and Z axes intersect at the fixed world origin and use the exact Navigation Gizmo palette (`#FF5D5D`, `#4ADE80`, `#3B9CFF`); the projected world-Y axis uses the same green. The renderer contract includes both perspective and orthographic ray construction using the same camera basis/world coordinates, while the current editor toolbar remains truthfully perspective-only because the existing camera/editor interaction system has not yet exposed an end-to-end orthographic toggle.

The final native verification captures `out/final-native-editor-adaptive-grid.png` and `out/final-native-editor-add-node-icons.png` were produced from the rebuilt editor under isolated Xvfb/OpenGL 3.3 after the catalog cardinality fix. They show the real ImGui editor overlay, clean empty-scene state, live filesystem/content panes, native gizmo toolbar, procedural grid, and the actual Add Node popup with supplied node icons. Earlier zoom-in/zoom-out frames showed finer subdivision and coarser/faded structure without a finite rectangle or overlay plane. The current editor UI remains perspective-only; the renderer shader contract has a forward-compatible orthographic branch, but end-to-end camera/editor orthographic interaction is not yet claimed.
