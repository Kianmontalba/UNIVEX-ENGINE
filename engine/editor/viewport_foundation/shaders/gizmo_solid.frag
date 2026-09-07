#version 330 core
// gizmo_solid.frag
// ---------------------------------------------------------------------------
// Flat colour, no lighting: a gizmo is a UI element that happens to live in
// world space, and shading it would make the axis colours read differently
// depending on which way the object happens to be facing.
// ---------------------------------------------------------------------------

in vec4 vColorAlpha;

uniform float uOpacity;

out vec4 fragColor;

void main() {
    fragColor = vec4(vColorAlpha.rgb, vColorAlpha.a * uOpacity);
}
