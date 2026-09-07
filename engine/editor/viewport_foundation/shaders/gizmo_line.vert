#version 330 core
// gizmo_line.vert
// ---------------------------------------------------------------------------
// Expands each gizmo line segment into a screen-space quad — two real
// triangles — instead of using GL_LINES.
//
// Two reasons. Core-profile GL only guarantees 1 px wide GL_LINES, so a line
// primitive cannot give a gizmo the deliberate weight it needs; and a quad
// carries a distance-from-centreline value into the fragment shader, which is
// what lets the edges be analytically anti-aliased without depending on MSAA
// being switched on.
//
// The gizmo is authored around the origin in abstract units and placed by
// uOrigin/uScale, so the same mesh serves any pivot at any zoom, and uScale is
// chosen per frame to keep the widget a constant size on screen.
// ---------------------------------------------------------------------------

layout(location = 0) in vec3 aPosCurrent;  // this end of the segment
layout(location = 1) in vec3 aPosOther;    // the far end, for the direction
layout(location = 2) in vec3 aColor;
layout(location = 3) in vec2 aSideWidth;   // x: which side (-1/+1), y: width in px

uniform mat4 uViewProj;
uniform vec2 uViewportSize;
uniform vec3 uOrigin;
uniform float uScale;

out vec3 vColor;
out float vDistPx;       // signed distance from the centreline, in pixels
out float vHalfWidthPx;

void main() {
    vec3 worldCurrent = aPosCurrent * uScale + uOrigin;
    vec3 worldOther   = aPosOther   * uScale + uOrigin;

    vec4 clipCurrent = uViewProj * vec4(worldCurrent, 1.0);
    vec4 clipOther   = uViewProj * vec4(worldOther,   1.0);

    // Guard against a zero/negative w: a segment endpoint exactly on or behind
    // the eye would otherwise produce an infinite screen position and smear the
    // quad across the whole viewport.
    float wCurrent = (abs(clipCurrent.w) < 1e-6) ? 1e-6 : clipCurrent.w;
    float wOther   = (abs(clipOther.w)   < 1e-6) ? 1e-6 : clipOther.w;

    vec2 halfViewport = uViewportSize * 0.5;
    vec2 screenCurrent = (clipCurrent.xy / wCurrent) * halfViewport;
    vec2 screenOther   = (clipOther.xy   / wOther)   * halfViewport;

    vec2 delta = screenOther - screenCurrent;
    float len = length(delta);
    vec2 direction = (len > 1e-6) ? delta / len : vec2(1.0, 0.0);
    vec2 normal = vec2(-direction.y, direction.x);

    // Half the line width plus one pixel of feather for the anti-aliased edge.
    float expandPx = aSideWidth.y * 0.5 + 1.0;
    vec2 offsetPx = normal * aSideWidth.x * expandPx;

    gl_Position = clipCurrent;
    gl_Position.xy += (offsetPx / halfViewport) * clipCurrent.w;

    vColor = aColor;
    vDistPx = aSideWidth.x * expandPx;
    vHalfWidthPx = aSideWidth.y * 0.5;
}
