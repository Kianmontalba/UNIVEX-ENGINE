#version 330 core
// gizmo_solid.vert
// ---------------------------------------------------------------------------
// The gizmo's filled geometry — arrow cones, scale cubes, plane handles, the
// nav gizmo's balls. Same uOrigin/uScale placement as the line pass so the
// two always agree about where the widget is and how big it is.
// ---------------------------------------------------------------------------

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColorAlpha;

uniform mat4 uViewProj;
uniform vec3 uOrigin;
uniform float uScale;

out vec4 vColorAlpha;

void main() {
    vColorAlpha = aColorAlpha;
    gl_Position = uViewProj * vec4(aPosition * uScale + uOrigin, 1.0);
}
