# UNIVEX — Editor Viewport (C++20 / OpenGL)

A real editor viewport: perspective/orthographic camera, an infinite ground
grid that adjusts its own spacing as you zoom, all four transform gizmos, a
draggable orientation gizmo, and the display-mode set an editor viewport
menu normally exposes. Built as drop-in modules plus a runnable demo.

Everything here has been compiled, linked, run, and photographed by a GL
driver — the screenshots in this repo came out of the same render path the
demo runs.

---

## What it actually does

**Truly infinite, not "a very big quad".** There is no ground mesh. One
fullscreen triangle is rasterised; each pixel rebuilds its own world-space
view ray (by unprojecting the near and far plane with the inverse
view-projection) and intersects it with the plane `y = 0`. Pixels whose ray
never meets the plane in front of the camera are discarded, so the grid
ends at the true mathematical horizon. There is no edge to hide, no quad to
re-centre, and no maximum extent to tune.

**The grid adjusts itself.** It never draws a fixed 1-unit cell. The shader
measures how much world space one pixel covers (screen-space derivatives)
and picks the decade of spacing that keeps cells near `targetCellPixels` on
screen. Four decades are drawn simultaneously and cross-faded by the
fractional part of the LOD, so zooming slides the tiers continuously —
1 m → 10 m → 100 m → 1 km — with no popping at the transitions. Zoom in far
enough and it stops at `baseSpacing` rather than subdividing forever.

A useful side effect: because spacing grows with distance, the argument to
`fract()` stays in a small numeric range even a long way from the origin,
which is what stops fp32 precision from making distant lines wobble.

**Correct depth.** The ray hit point is re-projected and written to
`gl_FragDepth`, so the grid depth-tests against scene geometry properly.
The renderer draws it depth-tested but with depth writes off: solid objects
in front of the ground hide the grid, and the grid never occludes anything
drawn after it. The demo includes a cube sitting on the plane specifically
so this is visible rather than asserted.

**Resizable.** The shader works entirely in clip space, so it is
resolution-independent by construction. The demo handles
`glfwSetFramebufferSizeCallback` and re-renders inside the callback, so a
live resize drag keeps painting instead of showing a stretched or blank
framebuffer between events.

**Dynamic clip planes.** Near and far scale with the orbit distance instead
of being fixed. A fixed `near=0.05 / far=20000` pair looks fine at one zoom
and falls apart at the others — at 4 km out the depth buffer has almost no
precision left. Scaling both keeps the near:far ratio constant across the
whole range, which is what makes a grid spanning centimetres to kilometres
usable.

---

## The gizmos

All four transform gizmos plus the orientation gizmo are in the box.

**Every stroke is triangles.** Core-profile GL only guarantees 1 px
`GL_LINES`, so nothing here uses them. Straight strokes are expanded in the
vertex shader into screen-space quads carrying a distance-from-centreline
value, which the fragment shader turns into analytic anti-aliasing — the
line is exactly as many pixels wide as you asked for, with clean edges,
with or without MSAA. Curved strokes (the rotation rings) are solid annuli
instead: stroking a circle from independent per-segment quads leaves
gear-tooth notches wherever the chord gets shorter than the stroke is wide,
which is exactly what happens on a small ring.

**Constant screen size.** The gizmo is authored in abstract units and
scaled every frame so it stays `gizmoPixelRadius` pixels across, whether
the camera is 20 cm or 2 km away.

**Anti-aliasing, on both passes.** The two passes get it differently and
both are needed. The line pass computes each fragment's distance from the
stroke's centreline in pixels and feathers coverage over exactly one pixel
— that is why a 2.3 px stroke is 2.3 px with clean edges regardless of the
framebuffer. The solid pass (cones, cubes, ring annuli, nav balls) writes
flat colour and has no such trick available, so it relies on multisampling:
the demo requests 8 samples and enables `GL_MULTISAMPLE`, and the capture
tool renders into a multisampled FBO and resolves it with a blit. Without
that resolve the solid silhouettes come out hard-edged, which is worth
knowing if you ever render this into your own FBO — the earlier captures in
this project looked far rougher than the demo did for exactly that reason.

**Axis letters.** X, Y and Z sit on the positive nav-gizmo balls, drawn as
vector strokes rather than from a font: three glyphs are not worth a
texture atlas or a font dependency, and routing them through the line pass
means they inherit its anti-aliasing. They are built on a screen-aligned
basis, so they stay upright however the view is orbited, with a fallback
reference axis for the degenerate straight-up and straight-down cases.

**It occludes itself but not the scene.** The pass clears depth first, then
draws depth-tested: the widget composites over the scene (a handle hidden
inside the object it moves is useless) while its own solid cubes still hide
their own back faces.

**Universal layout.** The all-in-one gizmo keeps its three tools clearly
apart along each axis — rotate ring innermost at 0.72, move arrow reaching
out to a tip at 1.54, scale cube starting at 1.78 with a deliberate gap in
between. `tests/ViewportTests.cpp` asserts that ordering and that gap, so a
future tweak cannot quietly collapse them back into one another.

**Orientation gizmo.** Six balls on three axis stubs in its own square
corner viewport, always orthographic so near balls are not drawn bigger
than far ones. Positive ends are solid, negative ends hollow. It answers
two gestures: **drag it** to orbit freely, exactly like dragging the scene,
or **click a ball** to ease-snap to that axis. Which one happened is
decided at release by whether the pointer moved past a few pixels, so a
click never jerks the view and a drag never snaps at the end.

## View modes

The mode set an editor viewport menu usually carries, all live:

| Mode | Key |
|---|---|
| Select / Move / Rotate / Scale / Universal | `Q` `W` `E` `R` `T` |
| Top / Bottom | `7` / `Ctrl+7` |
| Front / Rear | `1` / `Ctrl+1` |
| Right / Left | `3` / `Ctrl+3` |
| Perspective ↔ Orthographic | `5` |
| Normal → Wireframe → Unshaded | `Z` |
| Grid · orientation gizmo · transform gizmo | `G` · `N` · `H` |
| Environment · scene geometry | `B` · `M` |
| Focus origin · focus selection · reset view | `O` · `F` · `Home` |

Snapping to an axis view switches to orthographic on its own and orbiting
away switches back (`autoOrthogonal`), because an axis view in perspective
is almost never what "Front" means.

What is *not* here is the dropdown widget itself — drawing menus needs a UI
toolkit (ImGui, or UNIVEX's own), which this module deliberately does not
pull in. The modes are all real state on `ViewportSettings`; wiring them to
a menu is the host application's job. The demo reports the live state in
its window title.

## Layout

| Path | What it is |
|---|---|
| `include/univex/math/Vec.h` | Vec2/3/4, constexpr, dependency-free |
| `include/univex/math/Mat4.h`, `src/Mat4.cpp` | Column-major Mat4: `Perspective`, `LookAt`, `Multiply`, general `Inverse` |
| `include/univex/camera/OrbitCamera.h`, `src/OrbitCamera.cpp` | Orbit / pan / dolly, dynamic clip planes, view-projection and its inverse |
| `include/univex/render/GridSettings.h` | Every tunable, plus the CPU mirror of the shader's LOD (`ComputeGridLod`, `ComputeDisplayGridSpacing`) |
| `include/univex/render/ShaderProgram.h`, `src/ShaderProgram.cpp` | Move-only RAII program; compile/link failures return the driver's info log |
| `include/univex/render/InfiniteGridRenderer.h`, `src/InfiniteGridRenderer.cpp` | The renderer: owns the program + triangle, sets uniforms, saves and restores GL state |
| `include/univex/render/GlApi.h` | Single GL-loader include point (GLEW by default, swappable via `UNIVEX_GL_LOADER_HEADER`) |
| `include/univex/gizmo/GizmoStyle.h` | Every gizmo tunable — line weights in pixels, handle offsets in gizmo units |
| `include/univex/gizmo/GizmoGeometry.h`, `src/GizmoGeometry.cpp` | Move / Rotate / Scale / Universal geometry |
| `include/univex/gizmo/NavGizmo.h`, `src/NavGizmo.cpp` | Orientation gizmo geometry + ball picking |
| `include/univex/render/GizmoRenderer.h`, `src/GizmoRenderer.cpp` | Screen-space line quads + solid pass; serves both gizmos |
| `include/univex/viewport/ViewportSettings.h` | Projection / display / overlay state and the standard views |
| `shaders/infinite_grid.vert` / `.frag` | The grid GLSL, embedded at configure time |
| `shaders/gizmo_line.{vert,frag}` / `gizmo_solid.{vert,frag}` | The gizmo GLSL, embedded the same way |
| `app/` | `ReferenceScene` (the cube), `ViewportRenderPass` (one frame), `main.cpp` (the GLFW demo) |
| `tools/headless_capture.cpp` | Renders off-screen into an FBO and writes a PPM — how the screenshots below were produced |
| `tests/ViewportTests.cpp` | CPU-side checks; no GPU required |

The library splits in two deliberately: `univex_viewport_core` is pure CPU
(math, camera, LOD) and links nothing, while `univex_viewport_gl` adds the
GL renderer. Tests link only the core, so they run anywhere.

---

## Build and run

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

./build/univex_viewport_demo          # interactive window
./build/univex_viewport_tests         # CPU checks
./build/univex_headless_capture --out frame.ppm --width 1600 --height 900 --dist 40
```

Requires OpenGL 3.3 core, GLEW and GLFW (`libglfw3-dev libglew-dev
libgl1-mesa-dev` on Debian/Ubuntu). GLFW is needed only for the demo and
the capture tool — the module itself needs no windowing library.

Demo controls: **left drag** orbit · **middle/right drag** pan · **scroll**
dolly · drag or click the **orientation gizmo** · plus the mode keys in the
table above. The title bar shows the framebuffer size, gizmo mode,
projection, shading and the grid spacing the LOD has settled on — scroll
out and watch the spacing step.

The capture tool takes the same modes, which is how the screenshots were
made:

```sh
./build/univex_headless_capture --out frame.ppm --gizmo universal \
    --width 1600 --height 900 [--ortho] [--display wireframe] [--no-grid] \
    [--samples 8] [--nav-size 620]
```

### Dropping it into UNIVEX

```cpp
std::string error;
auto grid = univex::render::InfiniteGridRenderer::CreateWithBuiltinShaders(error);
// ... once per frame, after opaque geometry:
univex::render::GridFrameParams frame;
frame.viewProjection        = myCamera.ViewProj();
frame.inverseViewProjection = Invert(myCamera.ViewProj());
frame.cameraPosition        = myCamera.Position();
frame.referenceDistance     = myCamera.DistanceToPivot();
grid->Draw(frame);
```

`GridFrameParams` is plain data on purpose — the grid does not require you
to adopt this module's `OrbitCamera`. If UNIVEX already initialises GLAD,
define `UNIVEX_GL_LOADER_HEADER="glad/gl.h"` and `GlApi.h` follows.

---

## What was actually verified

Everything below was run, not asserted. Environment: GCC 13.3.0, C++20,
`-Wall -Wextra -Wpedantic`, Mesa 25.2.8 llvmpipe reporting an OpenGL 4.5
core context under Xvfb.

**Builds clean.** All five targets compile and link with zero warnings.

**CPU tests: 33/33 pass** (`./build/univex_viewport_tests`). Notable ones:
`Perspective`/`LookAt` match the earlier WebGL reference build
element-for-element; `M · M⁻¹` returns the identity to 1.5e-5; the
ray-reconstruction section replays the shader's own geometry and every
sample pixel lands on `y = 0` and re-projects to within 2.4e-5 of the
pixel it came from.

**It renders.** `headless_capture` draws through the same
`ViewportRenderPass` the demo uses, into an FBO, and reads the pixels
back — zero GL errors at every size and zoom tried.

**The grid really does auto-adjust.** Captured at a fixed camera angle
while dollying out:

| orbit distance | world per pixel | LOD level | grid spacing | lit pixels |
|---|---|---|---|---|
| 50 m | 0.075 | 0.26 | **1 m** | 4.47 % |
| 500 m | 0.752 | 1.26 | **10 m** | 4.47 % |
| 5000 m | 7.521 | 2.26 | **100 m** | 4.47 % |

The spacing steps by decades while the fraction of lit pixels stays
identical to three significant figures — constant on-screen density
across a 100× zoom range, which is the entire point of the mechanism.

**The decade crossfade does not pop.** Twenty-one frames were captured
across the 1 m → 10 m boundary and their mean brightness compared. The
change at the exact frame where the tier flips is **0.052**, against a
mean of **0.025** and a maximum of **0.064** for ordinary adjacent
frames. The transition is smaller than the largest normal frame-to-frame
variation — there is no discontinuity to see.

**Depth compositing is exact.** In the default capture, every row of
every visible cube face was scanned: **0 interrupted pixels** across 409
rows. No grid line penetrates the cube. (A first pass reported 1027 —
that turned out to be five stray pixels elsewhere in the frame whose
colour coincidentally equalled a face colour, stretching the scanline
spans. Restricted to the cube, the count is zero.)

**Resizes correctly.** Rendered at 320×240, 1920×1080, 600×1200 (portrait),
2560×720 (ultrawide) and 97×421 (a sliver): correct aspect throughout, no
stretching, grid squares stay square.

**The interactive demo runs.** Launched under Xvfb, it creates its GL 4.5
core context, builds all five programs and renders continuously; killed by
timeout rather than by any fault.

**Every gizmo and view mode renders.** All four transform gizmos, the
orientation gizmo, orthographic, wireframe, unshaded, and the Top and Front
standard views were each captured through the FBO path with zero GL errors.

**Captures are multisampled.** The capture path renders into a
multisampled FBO and resolves by blit; llvmpipe reports `GL_MAX_SAMPLES` 4,
so the screenshots here are 4x MSAA and the code clamps to whatever the
driver offers (it asks for 8).

**Nav-gizmo picking is exact.** Each of the six balls is picked at its own
projected position; a press on empty space inside the widget picks nothing;
and with the camera looking straight down +Y — where the +Y and -Y balls
project on top of each other — the nearer one wins.

### What is *not* verified

**Hardware.** This has only run on Mesa's llvmpipe software rasteriser,
never on real GPU hardware or a non-Mesa driver. The GLSL is plain
`#version 330 core` with no vendor extensions, so it should be portable,
but "should" is the honest word.

**The mouse gestures themselves.** The nav gizmo's picking math and the
camera orbit it drives are both unit-tested, and the click-versus-drag rule
is a few lines of threshold logic — but no automated test actually presses
and drags a mouse over the widget, because the headless path has no input
device. That interaction has been reasoned about, not exercised.

**The grid in an exactly edge-on view.** Front / Rear / Left / Right look
along the ground plane, so an infinite plane viewed exactly edge-on is a
line, and the grid all but vanishes. That is geometrically correct rather
than a bug, but editors usually paper over it by swinging a grid up to face
the view in axis views; this one does not.

## Technique credit

The building blocks — a ray-cast infinite ground plane, procedural lines
via `fract()` with derivative-based anti-aliasing, and decade LOD with
cross-fade — are the standard approaches, described in public shader
literature and used across many engines and toolkits. This is an
implementation of those ideas, not a copy of any single engine's source.
