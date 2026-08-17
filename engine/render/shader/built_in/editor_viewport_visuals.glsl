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

uniform vec3 uViewportMin;
uniform vec3 uViewportMax;
uniform vec3 uSelectionMin;
uniform vec3 uSelectionMax;
uniform vec3 uCameraForward;
uniform int uSelectionVisible;
uniform int uActiveGizmoAxis;

float RectMaskUVE(vec2 point, vec2 minimum, vec2 maximum) {
    vec2 inside = step(minimum, point) * step(point, maximum);
    return inside.x * inside.y;
}

float RectOutlineUVE(vec2 point, vec2 minimum, vec2 maximum, float width) {
    float outer = RectMaskUVE(point, minimum - vec2(width), maximum + vec2(width));
    float inner = RectMaskUVE(point, minimum + vec2(width), maximum - vec2(width));
    return max(outer - inner, 0.0);
}

float SegmentUVE(vec2 point, vec2 start, vec2 end, float radius) {
    vec2 direction = end - start;
    float parameter = clamp(dot(point - start, direction) / max(dot(direction, direction), 0.000001), 0.0, 1.0);
    return 1.0 - smoothstep(radius, radius * 1.8, length(point - (start + direction * parameter)));
}

void main() {
    vec2 viewportMin = uViewportMin.xy;
    vec2 viewportMax = uViewportMax.xy;
    float inViewport = RectMaskUVE(vTexCoord, viewportMin, viewportMax);
    if (inViewport < 0.5) {
        discard;
    }

    vec2 local = (vTexCoord - viewportMin) / max(viewportMax - viewportMin, vec2(0.0001));
    float verticalGradient = smoothstep(0.0, 1.0, local.y);
    vec3 backgroundTop = vec3(0.060, 0.082, 0.108);
    vec3 backgroundBottom = vec3(0.028, 0.043, 0.060);
    vec3 color = mix(backgroundBottom, backgroundTop, verticalGradient);
    float alpha = 0.30;

    vec2 grid = abs(fract((local - 0.5) * 24.0) - 0.5);
    float fineLine = 1.0 - smoothstep(0.47, 0.50, max(grid.x, grid.y));
    vec2 majorGrid = abs(fract((local - 0.5) * 4.0) - 0.5);
    float majorLine = 1.0 - smoothstep(0.455, 0.50, max(majorGrid.x, majorGrid.y));
    float axisX = 1.0 - smoothstep(0.004, 0.010, abs(local.y - 0.5));
    float axisZ = 1.0 - smoothstep(0.004, 0.010, abs(local.x - 0.5));
    color += vec3(0.16, 0.27, 0.37) * fineLine;
    color += vec3(0.25, 0.43, 0.57) * majorLine;
    color += vec3(0.10, 0.62, 0.91) * axisX;
    color += vec3(0.95, 0.48, 0.12) * axisZ;
    alpha = max(alpha, 0.22 * fineLine + 0.28 * majorLine + 0.42 * max(axisX, axisZ));

    if (uSelectionVisible != 0) {
        float outline = RectOutlineUVE(vTexCoord, uSelectionMin.xy, uSelectionMax.xy, 0.0025);
        color = mix(color, vec3(1.0, 0.82, 0.16), outline);
        alpha = max(alpha, outline * 0.90);
    }

    vec2 widgetCenter = vec2(0.90, 0.88);
    float widget = 1.0 - smoothstep(0.095, 0.105, length(local - widgetCenter));
    if (widget > 0.0) {
        vec2 direction = normalize(vec2(uCameraForward.x, -uCameraForward.y) + vec2(0.0001));
        vec2 xEnd = widgetCenter + vec2(0.055, 0.0);
        vec2 yEnd = widgetCenter + vec2(0.0, 0.055);
        vec2 zEnd = widgetCenter - direction * 0.050;
        float xAxis = SegmentUVE(local, widgetCenter, xEnd, 0.006);
        float yAxis = SegmentUVE(local, widgetCenter, yEnd, 0.006);
        float zAxis = SegmentUVE(local, widgetCenter, zEnd, 0.006);
        color += vec3(0.92, 0.22, 0.24) * xAxis + vec3(0.25, 0.78, 0.38) * yAxis + vec3(0.15, 0.52, 0.95) * zAxis;
        alpha = max(alpha, max(xAxis, max(yAxis, zAxis)) * 0.94 + widget * 0.10);
    }

    vec3 gizmoColor = uActiveGizmoAxis == 1 ? vec3(0.92, 0.22, 0.24) :
                      uActiveGizmoAxis == 2 ? vec3(0.25, 0.78, 0.38) :
                      uActiveGizmoAxis == 3 ? vec3(0.15, 0.52, 0.95) : vec3(0.58, 0.68, 0.76);
    float gizmoMarker = RectMaskUVE(local, vec2(0.025, 0.88), vec2(0.070, 0.925));
    color = mix(color, gizmoColor, gizmoMarker);
    alpha = max(alpha, gizmoMarker * 0.88);

    FragColor = vec4(color, clamp(alpha, 0.0, 0.94));
}
#endif
