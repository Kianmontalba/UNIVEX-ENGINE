#version 330 core
// gizmo_line.frag
// ---------------------------------------------------------------------------
// Analytic anti-aliasing: coverage falls off over exactly one pixel either
// side of the line's nominal width, so a 2.3 px line looks 2.3 px wide with
// clean edges whether or not the framebuffer is multisampled.
// ---------------------------------------------------------------------------

in vec3 vColor;
in float vDistPx;
in float vHalfWidthPx;

uniform float uOpacity;

out vec4 fragColor;

void main() {
    float coverage = clamp(vHalfWidthPx + 0.5 - abs(vDistPx), 0.0, 1.0);
    if (coverage <= 0.0) discard;
    fragColor = vec4(vColor, coverage * uOpacity);
}
