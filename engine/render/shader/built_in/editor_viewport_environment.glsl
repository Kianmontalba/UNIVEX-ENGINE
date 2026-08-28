#version 330 core

#ifdef VERTEX_SHADER
void main() {
    vec2 position = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(position * 2.0 - 1.0, 0.0, 1.0);
}
#endif

#ifdef FRAGMENT_SHADER
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
uniform int uProjectionMode;
uniform float uOrthographicScale;
uniform int uEnvironmentPreviewEnabled;
uniform int uSunPreviewEnabled;

float GridLineCoverageUVE(vec2 coordinate, float spacing) {
    vec2 scaled = coordinate / max(spacing, 0.000001);
    vec2 distanceToLine = abs(fract(scaled + 0.5) - 0.5);
    vec2 antiAlias = clamp(fwidth(scaled) * 0.35, vec2(0.004), vec2(0.055));
    vec2 line = 1.0 - smoothstep(vec2(0.0), antiAlias, distanceToLine);
    return max(line.x, line.y);
}

float SafeLog10UVE(float value) {
    return log(max(value, 0.000001)) / log(10.0);
}

void main() {
    vec2 surfaceUv = gl_FragCoord.xy / max(uSurfaceSize.xy, vec2(1.0));
    vec2 viewportExtent = max(uViewportMax.xy - uViewportMin.xy, vec2(0.001));
    vec2 viewportUv = clamp((surfaceUv - uViewportMin.xy) / viewportExtent, vec2(0.0), vec2(1.0));
    vec2 ndc = vec2(viewportUv.x * 2.0 - 1.0, viewportUv.y * 2.0 - 1.0);

    float tanHalfFov = max(uCameraTanHalfFov, 0.0001);
    float forwardLength = length(uCameraForward);
    vec3 forward = forwardLength > 0.0001 ? uCameraForward / forwardLength : vec3(0.0, 0.0, -1.0);
    float rightLength = length(uCameraRight);
    vec3 right = rightLength > 0.0001 ? uCameraRight / rightLength : vec3(1.0, 0.0, 0.0);
    float upLength = length(uCameraUp);
    vec3 up = upLength > 0.0001 ? uCameraUp / upLength : vec3(0.0, 1.0, 0.0);

    vec3 rayOrigin = uCameraPosition;
    vec3 rayDirection = forward;
    float orthographicScale = max(uOrthographicScale, 0.0001);
    if (uProjectionMode != 0) {
        rayOrigin += right * (ndc.x * uViewportAspect * orthographicScale) + up * (ndc.y * orthographicScale);
    } else {
        rayDirection = normalize(forward + right * (ndc.x * uViewportAspect * tanHalfFov) + up * (ndc.y * tanHalfFov));
    }

    const vec3 skyColor = vec3(0.246, 0.282, 0.322);       // #3F4852
    const vec3 groundColor = vec3(0.145, 0.165, 0.184);    // #252A2F
    const vec3 horizonColor = vec3(0.282, 0.322, 0.361);   // #48525C
    const vec3 gridColor = vec3(0.349, 0.388, 0.427);      // #59636D
    const vec3 axisXColor = vec3(1.0, 0.365, 0.365);       // Navigation Gizmo X #FF5D5D
    const vec3 axisYColor = vec3(0.290, 0.871, 0.502);      // Navigation Gizmo Y #4ADE80
    const vec3 axisZColor = vec3(0.231, 0.612, 1.0);       // Navigation Gizmo Z #3B9CFF

    float skyHeight = clamp(rayDirection.y, -1.0, 1.0);
    float horizonBlend = smoothstep(-0.16, 0.18, skyHeight);
    vec3 color = vec3(0.355, 0.365, 0.380); // neutral preview-off gray
    if (uEnvironmentPreviewEnabled != 0) {
        color = mix(groundColor, horizonColor, smoothstep(0.0, 0.35, skyHeight));
        color = mix(color, skyColor, smoothstep(0.18, 0.72, skyHeight));
    }
    if (uSunPreviewEnabled != 0) {
        const vec3 sunDirection = normalize(vec3(-0.38, 0.82, 0.30));
        float sunDisc = pow(max(dot(rayDirection, sunDirection), 0.0), 256.0);
        color += vec3(1.0, 0.83, 0.55) * sunDisc * 0.82;
    }

    float groundDenominator = rayDirection.y;
    if (groundDenominator < -0.0001) {
        float groundDistance = -rayOrigin.y / groundDenominator;
        if (groundDistance > 0.0 && groundDistance < 1000000.0) {
            vec3 groundHit = rayOrigin + rayDirection * groundDistance;
            vec2 relative = groundHit.xz - uGridOrigin.xz;

            float viewportPixelHeight = max(viewportExtent.y * uSurfaceSize.y, 1.0);
            float worldUnitsPerPixel = uProjectionMode != 0
                                           ? (2.0 * orthographicScale) / viewportPixelHeight
                                           : (groundDistance * 2.0 * tanHalfFov) / viewportPixelHeight;
            worldUnitsPerPixel = max(worldUnitsPerPixel, 0.000001);

            // Select the decade whose major cells remain comfortably readable. The 1/10 minor
            // level is about eight pixels at the crossover, and the logarithmic fraction blends
            // adjacent major levels continuously while zooming.
            float baseSpacing = max(uGridSpacing, 0.000001);
            float desiredMajorSpacing = max(worldUnitsPerPixel * 80.0, baseSpacing * 0.01);
            float decade = floor(SafeLog10UVE(desiredMajorSpacing / baseSpacing));
            float decadeFraction = fract(SafeLog10UVE(desiredMajorSpacing / baseSpacing));
            float majorSpacing = baseSpacing * pow(10.0, decade);
            float nextMajorSpacing = majorSpacing * 10.0;
            float majorTransition = smoothstep(0.18, 0.82, decadeFraction);

            float currentMinor = GridLineCoverageUVE(relative, majorSpacing * 0.1);
            float currentMajor = GridLineCoverageUVE(relative, majorSpacing);
            float nextMajor = GridLineCoverageUVE(relative, nextMajorSpacing);
            float minorWeight = (1.0 - majorTransition) * 0.42;
            float majorWeight = (1.0 - majorTransition) * 0.74 + majorTransition * 0.66;
            float gridFade = 1.0 - smoothstep(majorSpacing * 12.0, majorSpacing * 240.0, groundDistance);
            float gridCoverage = (currentMinor * minorWeight + currentMajor * majorWeight +
                                  nextMajor * majorTransition * 0.74) * gridFade;
            color = mix(color, gridColor, clamp(gridCoverage * 0.52, 0.0, 0.52));

            // X and Z are world-space ground axes and therefore must pass through the real origin.
            // The small width is antialiased in world units, so the axes stay stronger than the
            // grid without turning into a thick overlay.
            float axisWidth = clamp(max(worldUnitsPerPixel * 1.6, majorSpacing * 0.008),
                                    majorSpacing * 0.006, majorSpacing * 0.045);
            float axisX = 1.0 - smoothstep(axisWidth, axisWidth * 2.0, abs(relative.y));
            float axisZ = 1.0 - smoothstep(axisWidth, axisWidth * 2.0, abs(relative.x));
            float axisFade = gridFade * (0.62 + 0.38 * (1.0 - majorTransition));
            color = mix(color, axisXColor, clamp(axisX * axisFade * 0.72, 0.0, 0.78));
            color = mix(color, axisZColor, clamp(axisZ * axisFade * 0.72, 0.0, 0.78));

            // Project the world Y axis into the same camera ray. This is a line in XZ at the
            // origin, not a screen-space decoration, and naturally disappears when edge-on.
            vec2 horizontalRay = rayDirection.xz;
            float horizontalLengthSquared = dot(horizontalRay, horizontalRay);
            if (horizontalLengthSquared > 0.000001) {
                float axisParameter = -dot(rayOrigin.xz - uGridOrigin.xz, horizontalRay) /
                                      horizontalLengthSquared;
                if (axisParameter > 0.0) {
                    vec3 closestAxisPoint = rayOrigin + rayDirection * axisParameter;
                    float yAxisDistance = length(closestAxisPoint.xz - uGridOrigin.xz);
                    float yAxis = 1.0 - smoothstep(axisWidth, axisWidth * 2.0, yAxisDistance);
                    color = mix(color, axisYColor, clamp(yAxis * axisFade * 0.72, 0.0, 0.78));
                }
            }
        }
    }

    // Keep the transition continuous at grazing angles instead of producing a hard horizon strip.
    color = mix(color, horizonColor, (1.0 - horizonBlend) * 0.08);
    FragColor = vec4(color, 1.0);
}
#endif
