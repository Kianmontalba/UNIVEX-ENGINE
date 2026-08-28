# HTML Gizmo and Viewport Environment Audit

## Reference and scope

The supplied `gizmo_toolbar_demo.html` is a self-contained HTML5 canvas reference for Move, Rotate, Scale, Universal, and six-direction viewport navigation presentation. Its interaction examples include orbit and preset camera behavior, ray-plane interaction, and demo target geometry. The demo targets are reference content only; they are not inserted into UniVex’s default scene.

The supplied requirements specify separate HTML documents for each gizmo and a genuine camera-aware infinite viewport environment. The required environment palette is sky `#3F4852`, ground `#252A2F`, transition `#48525C`, and grid `#59636D`. The implementation must not use a finite ground plane, a 2D background card, a visible seam or horizon strip, or a grid with a terminal boundary.

## Runtime boundary

The native UniVex editor currently uses GLFW, OpenGL, and Dear ImGui. The repository and native build do not contain a WebView, CEF, WebKit, Ultralight, or equivalent HTML runtime. Consequently, the six HTML documents under `engine/editor/assets/gizmos/html/` are canonical visual and interaction design sources; they are not falsely presented as HTML rendered inside `uve_editor`. A future HTML runtime would require an explicit platform, build, input, security, and lifecycle design before these sources could replace native presentation.

## Native behavior preserved

The shipped editor behavior remains native and authoritative. ECS transforms, selection and raycasting, Move/Rotate/Scale/Universal drag sessions, local/world coordinate resolution, snapping, and history Undo/Redo stay in the existing C++ editor and renderer path. The dedicated native `GizmoSystemUVE` and `ViewportNavGizmoUVE` modules provide the behavior-family mapping required by the reference without introducing demo-only scene content or an unrelated duplicate Global/Universal control.

## Infinite environment implementation

The old flat UV visual overlay was replaced by a renderer-owned `EditorViewportEnvironment` pass. The pass reconstructs a camera ray from the editor’s published camera position, forward, right, and up vectors; remaps authoritative framebuffer coordinates into the editor viewport; intersects the ray with the world ground plane; and evaluates an analytic, camera-relative grid with distance fading. Its color transition is continuous and uses the required palette. The environment runs before `MainColor`, while scene geometry and actual scene lighting remain responsible for scene content and shadows.

Native OpenGL verification was performed on a clean Xvfb display using the rebuilt `engine/app/uve_editor` executable with OpenGL 3.3 and a warmed frame run. The final capture showed a non-flat camera-aware environment in the real editor window; no random mesh, fake scene light, or finite ground mesh was introduced. The earlier uniform captures were traced to incorrect presentation/viewport Y orientation and stale legacy program naming, both corrected before validation.
