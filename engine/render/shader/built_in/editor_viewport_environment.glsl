#version 330 core

#ifdef VERTEX_SHADER
out vec2 vTexCoord;

void main() {
    vec2 position = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    vTexCoord = position * 0.5;
    gl_Position = vec4(position * 2.0 - 1.0, 0.0, 1.0);
}
#endif

#ifdef FRAGMENT_SHADER
in vec2 vTexCoord;
out vec4 FragColor;

uniform vec3 uCameraPosition;
uniform vec3 uCameraForward;
uniform vec3 uCameraRight;
uniform vec3 uCameraUp;
uniform vec3 uViewportMin;
uniform vec3 uViewportMax;
uniform vec3 uSurfaceSize;
uniform vec3 uGridOrigin;
uniform float uCameraTanHalfFov;
uniform float uViewportAspect;
uniform float uGridSpacing;

float GridLineUVE(vec2 coordinate, float spacing, float width) {
    vec2 scaled = coordinate / max(spacing, 0.001);
    vec2 cellDistance = min(fract(scaled), 1.0 - fract(scaled));
    vec2 lineWidth = vec2(clamp(0.012 * max(width, 0.5), 0.006, 0.030));
    vec2 line = 1.0 - smoothstep(lineWidth, lineWidth + vec2(0.010), cellDistance);
    return max(line.x, line.y);
}

void main() {
    vec2 surfaceUv = gl_FragCoord.xy / max(uSurfaceSize.xy, vec2(1.0));
    vec2 viewportExtent = max(uViewportMax.xy - uViewportMin.xy, vec2(0.001));
    vec2 viewportUv = clamp((surfaceUv - uViewportMin.xy) / viewportExtent, vec2(0.0), vec2(1.0));
    vec2 ndc = vec2(viewportUv.x * 2.0 - 1.0, viewportUv.y * 2.0 - 1.0);
    float tanHalfFov = max(uCameraTanHalfFov, 0.57735026);
    float forwardLength = length(uCameraForward);
    vec3 forward = forwardLength > 0.0001 ? uCameraForward / forwardLength : vec3(0.0, 0.0, -1.0);
    float rightLength = length(uCameraRight);
    vec3 right = rightLength > 0.0001 ? uCameraRight / rightLength : vec3(1.0, 0.0, 0.0);
    float upLength = length(uCameraUp);
    vec3 up = upLength > 0.0001 ? uCameraUp / upLength : vec3(0.0, 1.0, 0.0);
    vec3 rayDirection = normalize(forward + right * (ndc.x * uViewportAspect * tanHalfFov) +
                                  up * (ndc.y * tanHalfFov));

    const vec3 skyColor = vec3(0.246, 0.282, 0.322);       // #3F4852
    const vec3 groundColor = vec3(0.145, 0.165, 0.184);    // #252A2F
    const vec3 horizonColor = vec3(0.282, 0.322, 0.361);   // #48525C
    const vec3 gridColor = vec3(0.349, 0.388, 0.427);      // #59636D

    float skyHeight = clamp(rayDirection.y, -1.0, 1.0);
    float horizonBlend = smoothstep(-0.16, 0.18, skyHeight);
    vec3 color = mix(groundColor, horizonColor, smoothstep(0.0, 0.35, skyHeight));
    color = mix(color, skyColor, smoothstep(0.18, 0.72, skyHeight));

    float groundDenominator = rayDirection.y;
    if (groundDenominator < -0.0001) {
        float groundDistance = -uCameraPosition.y / groundDenominator;
        if (groundDistance > 0.0 && groundDistance < 1000000.0) {
            vec3 groundHit = uCameraPosition + rayDirection * groundDistance;
            vec2 relative = groundHit.xz - uGridOrigin.xz;
            float spacing = max(uGridSpacing, 0.01);
            float fineGrid = GridLineUVE(relative, spacing * 0.1, 0.72);
            float majorGrid = GridLineUVE(relative, spacing, 0.80);
            float distanceFade = 1.0 - smoothstep(spacing * 12.0, spacing * 180.0, groundDistance);
            float groundHorizon = 1.0 - smoothstep(spacing * 12.0, spacing * 90.0, groundDistance);
            color = mix(groundColor, horizonColor, groundHorizon * 0.70);
            color += gridColor * (fineGrid * 0.34 + majorGrid * 0.72) * distanceFade;
            float axisX = 1.0 - smoothstep(0.018, 0.030, abs(relative.y));
            float axisZ = 1.0 - smoothstep(0.018, 0.030, abs(relative.x));
            color += vec3(0.80, 0.28, 0.26) * axisX * majorGrid * distanceFade;
            color += vec3(0.24, 0.55, 0.92) * axisZ * majorGrid * distanceFade;
        }
    }

    // Keep the transition continuous at grazing angles instead of producing a hard horizon strip.
    color = mix(color, horizonColor, (1.0 - horizonBlend) * 0.08);
    FragColor = vec4(color, 1.0);
}
#endif
