// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/editor/mesh_thumbnail_renderer_uve.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

// See gl_functions_uve.h (engine/render/src/) for why glext.h is safe to include here: it only
// supplies Khronos PFNGL*PROC typedefs and GL_* constants, never a loader library. Module-private
// (engine/editor/src/ only) - this class's public header never includes a GL header, matching
// this codebase's established GL-header-confinement discipline.
#include <GL/gl.h>
#include <GL/glext.h>
#include <GLFW/glfw3.h>

#include "uve/asset/mesh_asset_uve.h"
#include "uve/math/aabb_uve.h"
#include "uve/math/matrix4x4_uve.h"
#include "uve/math/quaternion_uve.h"
#include "uve/math/vector3_uve.h"

namespace UVE::Editor {

namespace {

constexpr float kFieldOfViewYRadiansUVE = 0.6981317F; // 40 degrees
constexpr float kCameraDistanceMarginUVE = 1.35F;
constexpr float kMinimumBoundingRadiusUVE = 0.25F;
constexpr float kNearPlaneUVE = 0.05F;

// Fixed default viewing angle (a gentle 3/4 elevated view) and a single fixed key light -
// deliberately not derived from the mesh's own assigned material (there may not be one; see this
// class's header comment), matching a generic asset-browser preview rather than an accurate
// material render.
constexpr Math::Vector3UVE kBaseColorUVE{0.65F, 0.68F, 0.72F};

constexpr const char* kVertexShaderSourceUVE = R"GLSL(#version 450 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
uniform mat4 uViewProjection;
out vec3 vNormal;
void main() {
    vNormal = aNormal;
    gl_Position = uViewProjection * vec4(aPosition, 1.0);
}
)GLSL";

constexpr const char* kFragmentShaderSourceUVE = R"GLSL(#version 450 core
in vec3 vNormal;
out vec4 FragColor;
uniform vec3 uLightDirection;
uniform vec3 uBaseColor;
void main() {
    vec3 normal = normalize(vNormal);
    float diffuse = max(dot(normal, -uLightDirection), 0.0);
    FragColor = vec4(uBaseColor * (0.35 + 0.65 * diffuse), 1.0);
}
)GLSL";

template <typename TFunctionPointer>
[[nodiscard]] TFunctionPointer LoadOneUVE(const char* const name) noexcept {
    return reinterpret_cast<TFunctionPointer>(reinterpret_cast<void*>(glfwGetProcAddress(name)));
}

void SetGlCapabilityUVE(const GLenum capability, const GLboolean enabled) noexcept {
    if (enabled == GL_TRUE) {
        glEnable(capability);
    } else {
        glDisable(capability);
    }
}

} // namespace

/// The GL function pointers, compiled shader program, and scratch render target this renderer
/// needs, kept out of the public header (see the header's class doc comment). Hand-rolled loader
/// scoped to exactly what a single-mesh, no-material preview render needs - mirrors
/// engine/render/src/gl_functions_uve.h's precedent rather than depending on it, since that header
/// is private to engine/render and this class is deliberately independent of engine/render.
struct MeshThumbnailRendererUVE::GlStateUVE {
    PFNGLGENBUFFERSPROC glGenBuffers = nullptr;
    PFNGLDELETEBUFFERSPROC glDeleteBuffers = nullptr;
    PFNGLBINDBUFFERPROC glBindBuffer = nullptr;
    PFNGLBUFFERDATAPROC glBufferData = nullptr;

    PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = nullptr;
    PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = nullptr;
    PFNGLBINDVERTEXARRAYPROC glBindVertexArray = nullptr;
    PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = nullptr;
    PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = nullptr;

    PFNGLCREATESHADERPROC glCreateShader = nullptr;
    PFNGLDELETESHADERPROC glDeleteShader = nullptr;
    PFNGLSHADERSOURCEPROC glShaderSource = nullptr;
    PFNGLCOMPILESHADERPROC glCompileShader = nullptr;
    PFNGLGETSHADERIVPROC glGetShaderiv = nullptr;
    PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = nullptr;

    PFNGLCREATEPROGRAMPROC glCreateProgram = nullptr;
    PFNGLDELETEPROGRAMPROC glDeleteProgram = nullptr;
    PFNGLATTACHSHADERPROC glAttachShader = nullptr;
    PFNGLLINKPROGRAMPROC glLinkProgram = nullptr;
    PFNGLGETPROGRAMIVPROC glGetProgramiv = nullptr;
    PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = nullptr;
    PFNGLUSEPROGRAMPROC glUseProgram = nullptr;

    PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = nullptr;
    PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers = nullptr;
    PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = nullptr;
    PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = nullptr;
    PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer = nullptr;
    PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus = nullptr;

    PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers = nullptr;
    PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers = nullptr;
    PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer = nullptr;
    PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage = nullptr;

    PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = nullptr;
    PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = nullptr;
    PFNGLUNIFORM3FVPROC glUniform3fv = nullptr;

    bool functionsLoaded = false;

    GLuint shaderProgram = 0U;
    GLint viewProjectionUniform = -1;
    GLint lightDirectionUniform = -1;
    GLint baseColorUniform = -1;
    bool programLinked = false;

    GLuint scratchFramebuffer = 0U;
    GLuint scratchDepthRenderbuffer = 0U;
    int scratchWidth = 0;
    int scratchHeight = 0;

    [[nodiscard]] bool IsCompleteUVE() const noexcept {
        return glGenBuffers != nullptr && glDeleteBuffers != nullptr && glBindBuffer != nullptr &&
               glBufferData != nullptr && glGenVertexArrays != nullptr && glDeleteVertexArrays != nullptr &&
               glBindVertexArray != nullptr && glVertexAttribPointer != nullptr &&
               glEnableVertexAttribArray != nullptr && glCreateShader != nullptr && glDeleteShader != nullptr &&
               glShaderSource != nullptr && glCompileShader != nullptr && glGetShaderiv != nullptr &&
               glGetShaderInfoLog != nullptr && glCreateProgram != nullptr && glDeleteProgram != nullptr &&
               glAttachShader != nullptr && glLinkProgram != nullptr && glGetProgramiv != nullptr &&
               glGetProgramInfoLog != nullptr && glUseProgram != nullptr && glGenFramebuffers != nullptr &&
               glDeleteFramebuffers != nullptr && glBindFramebuffer != nullptr && glFramebufferTexture2D != nullptr &&
               glFramebufferRenderbuffer != nullptr && glCheckFramebufferStatus != nullptr &&
               glGenRenderbuffers != nullptr && glDeleteRenderbuffers != nullptr && glBindRenderbuffer != nullptr &&
               glRenderbufferStorage != nullptr && glGetUniformLocation != nullptr && glUniformMatrix4fv != nullptr &&
               glUniform3fv != nullptr;
    }
};

namespace {

[[nodiscard]] bool CompileShaderStageUVE(MeshThumbnailRendererUVE::GlStateUVE& gl, const GLenum stage,
                                          const char* const source, GLuint& outShader) noexcept {
    outShader = gl.glCreateShader(stage);
    if (outShader == 0U) {
        return false;
    }
    gl.glShaderSource(outShader, 1, &source, nullptr);
    gl.glCompileShader(outShader);
    GLint compiled = GL_FALSE;
    gl.glGetShaderiv(outShader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
        gl.glDeleteShader(outShader);
        outShader = 0U;
        return false;
    }
    return true;
}

/// Creates (once) and resizes (as needed) the scratch framebuffer + depth-renderbuffer that every
/// RenderThumbnailUVE() call reuses. The depth renderbuffer is attached exactly once, right after
/// creation - resizing only calls glRenderbufferStorage, which preserves the object's id and thus
/// its existing attachment. Leaves GL_FRAMEBUFFER/GL_RENDERBUFFER bound to the scratch objects;
/// the caller is responsible for saving/restoring those bindings around the whole render.
[[nodiscard]] bool EnsureScratchTargetUVE(MeshThumbnailRendererUVE::GlStateUVE& gl, const int width,
                                           const int height) noexcept {
    const bool firstTime = gl.scratchFramebuffer == 0U;
    if (firstTime) {
        gl.glGenFramebuffers(1, &gl.scratchFramebuffer);
        gl.glGenRenderbuffers(1, &gl.scratchDepthRenderbuffer);
        if (gl.scratchFramebuffer == 0U || gl.scratchDepthRenderbuffer == 0U) {
            return false;
        }
    }
    if (firstTime || gl.scratchWidth != width || gl.scratchHeight != height) {
        gl.glBindRenderbuffer(GL_RENDERBUFFER, gl.scratchDepthRenderbuffer);
        gl.glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
        gl.scratchWidth = width;
        gl.scratchHeight = height;
    }
    if (firstTime) {
        gl.glBindFramebuffer(GL_FRAMEBUFFER, gl.scratchFramebuffer);
        gl.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                                      gl.scratchDepthRenderbuffer);
    }
    return true;
}

} // namespace

MeshThumbnailRendererUVE::MeshThumbnailRendererUVE() = default;
MeshThumbnailRendererUVE::~MeshThumbnailRendererUVE() = default;

void MeshThumbnailRendererUVE::InitializeUVE() {
    m_gl = std::make_unique<GlStateUVE>();
    GlStateUVE& gl = *m_gl;

    gl.glGenBuffers = LoadOneUVE<PFNGLGENBUFFERSPROC>("glGenBuffers");
    gl.glDeleteBuffers = LoadOneUVE<PFNGLDELETEBUFFERSPROC>("glDeleteBuffers");
    gl.glBindBuffer = LoadOneUVE<PFNGLBINDBUFFERPROC>("glBindBuffer");
    gl.glBufferData = LoadOneUVE<PFNGLBUFFERDATAPROC>("glBufferData");

    gl.glGenVertexArrays = LoadOneUVE<PFNGLGENVERTEXARRAYSPROC>("glGenVertexArrays");
    gl.glDeleteVertexArrays = LoadOneUVE<PFNGLDELETEVERTEXARRAYSPROC>("glDeleteVertexArrays");
    gl.glBindVertexArray = LoadOneUVE<PFNGLBINDVERTEXARRAYPROC>("glBindVertexArray");
    gl.glVertexAttribPointer = LoadOneUVE<PFNGLVERTEXATTRIBPOINTERPROC>("glVertexAttribPointer");
    gl.glEnableVertexAttribArray = LoadOneUVE<PFNGLENABLEVERTEXATTRIBARRAYPROC>("glEnableVertexAttribArray");

    gl.glCreateShader = LoadOneUVE<PFNGLCREATESHADERPROC>("glCreateShader");
    gl.glDeleteShader = LoadOneUVE<PFNGLDELETESHADERPROC>("glDeleteShader");
    gl.glShaderSource = LoadOneUVE<PFNGLSHADERSOURCEPROC>("glShaderSource");
    gl.glCompileShader = LoadOneUVE<PFNGLCOMPILESHADERPROC>("glCompileShader");
    gl.glGetShaderiv = LoadOneUVE<PFNGLGETSHADERIVPROC>("glGetShaderiv");
    gl.glGetShaderInfoLog = LoadOneUVE<PFNGLGETSHADERINFOLOGPROC>("glGetShaderInfoLog");

    gl.glCreateProgram = LoadOneUVE<PFNGLCREATEPROGRAMPROC>("glCreateProgram");
    gl.glDeleteProgram = LoadOneUVE<PFNGLDELETEPROGRAMPROC>("glDeleteProgram");
    gl.glAttachShader = LoadOneUVE<PFNGLATTACHSHADERPROC>("glAttachShader");
    gl.glLinkProgram = LoadOneUVE<PFNGLLINKPROGRAMPROC>("glLinkProgram");
    gl.glGetProgramiv = LoadOneUVE<PFNGLGETPROGRAMIVPROC>("glGetProgramiv");
    gl.glGetProgramInfoLog = LoadOneUVE<PFNGLGETPROGRAMINFOLOGPROC>("glGetProgramInfoLog");
    gl.glUseProgram = LoadOneUVE<PFNGLUSEPROGRAMPROC>("glUseProgram");

    gl.glGenFramebuffers = LoadOneUVE<PFNGLGENFRAMEBUFFERSPROC>("glGenFramebuffers");
    gl.glDeleteFramebuffers = LoadOneUVE<PFNGLDELETEFRAMEBUFFERSPROC>("glDeleteFramebuffers");
    gl.glBindFramebuffer = LoadOneUVE<PFNGLBINDFRAMEBUFFERPROC>("glBindFramebuffer");
    gl.glFramebufferTexture2D = LoadOneUVE<PFNGLFRAMEBUFFERTEXTURE2DPROC>("glFramebufferTexture2D");
    gl.glFramebufferRenderbuffer = LoadOneUVE<PFNGLFRAMEBUFFERRENDERBUFFERPROC>("glFramebufferRenderbuffer");
    gl.glCheckFramebufferStatus = LoadOneUVE<PFNGLCHECKFRAMEBUFFERSTATUSPROC>("glCheckFramebufferStatus");

    gl.glGenRenderbuffers = LoadOneUVE<PFNGLGENRENDERBUFFERSPROC>("glGenRenderbuffers");
    gl.glDeleteRenderbuffers = LoadOneUVE<PFNGLDELETERENDERBUFFERSPROC>("glDeleteRenderbuffers");
    gl.glBindRenderbuffer = LoadOneUVE<PFNGLBINDRENDERBUFFERPROC>("glBindRenderbuffer");
    gl.glRenderbufferStorage = LoadOneUVE<PFNGLRENDERBUFFERSTORAGEPROC>("glRenderbufferStorage");

    gl.glGetUniformLocation = LoadOneUVE<PFNGLGETUNIFORMLOCATIONPROC>("glGetUniformLocation");
    gl.glUniformMatrix4fv = LoadOneUVE<PFNGLUNIFORMMATRIX4FVPROC>("glUniformMatrix4fv");
    gl.glUniform3fv = LoadOneUVE<PFNGLUNIFORM3FVPROC>("glUniform3fv");

    gl.functionsLoaded = gl.IsCompleteUVE();
    if (!gl.functionsLoaded) {
        return;
    }

    GLuint vertexShader = 0U;
    GLuint fragmentShader = 0U;
    const bool vertexCompiled = CompileShaderStageUVE(gl, GL_VERTEX_SHADER, kVertexShaderSourceUVE, vertexShader);
    const bool fragmentCompiled =
        vertexCompiled && CompileShaderStageUVE(gl, GL_FRAGMENT_SHADER, kFragmentShaderSourceUVE, fragmentShader);
    if (!fragmentCompiled) {
        if (vertexShader != 0U) {
            gl.glDeleteShader(vertexShader);
        }
        return;
    }

    gl.shaderProgram = gl.glCreateProgram();
    gl.glAttachShader(gl.shaderProgram, vertexShader);
    gl.glAttachShader(gl.shaderProgram, fragmentShader);
    gl.glLinkProgram(gl.shaderProgram);
    GLint linked = GL_FALSE;
    gl.glGetProgramiv(gl.shaderProgram, GL_LINK_STATUS, &linked);
    gl.glDeleteShader(vertexShader);
    gl.glDeleteShader(fragmentShader);
    if (linked == GL_FALSE) {
        gl.glDeleteProgram(gl.shaderProgram);
        gl.shaderProgram = 0U;
        return;
    }

    gl.viewProjectionUniform = gl.glGetUniformLocation(gl.shaderProgram, "uViewProjection");
    gl.lightDirectionUniform = gl.glGetUniformLocation(gl.shaderProgram, "uLightDirection");
    gl.baseColorUniform = gl.glGetUniformLocation(gl.shaderProgram, "uBaseColor");
    gl.programLinked = true;
}

void MeshThumbnailRendererUVE::ShutdownUVE() noexcept {
    if (!m_gl) {
        return;
    }
    GlStateUVE& gl = *m_gl;
    if (gl.functionsLoaded) {
        if (gl.scratchDepthRenderbuffer != 0U) {
            gl.glDeleteRenderbuffers(1, &gl.scratchDepthRenderbuffer);
            gl.scratchDepthRenderbuffer = 0U;
        }
        if (gl.scratchFramebuffer != 0U) {
            gl.glDeleteFramebuffers(1, &gl.scratchFramebuffer);
            gl.scratchFramebuffer = 0U;
        }
        if (gl.shaderProgram != 0U) {
            gl.glDeleteProgram(gl.shaderProgram);
            gl.shaderProgram = 0U;
        }
    }
    gl.programLinked = false;
    m_gl.reset();
}

std::uintptr_t MeshThumbnailRendererUVE::RenderThumbnailUVE(const Asset::MeshAssetUVE& mesh, const int width,
                                                             const int height) {
    if (!m_gl || !m_gl->functionsLoaded || !m_gl->programLinked) {
        return 0U;
    }
    if (mesh.vertices.empty() || mesh.indices.empty() || width <= 0 || height <= 0) {
        return 0U;
    }
    GlStateUVE& gl = *m_gl;

    const Math::Vector3UVE center = mesh.localBounds.GetCenterUVE();
    const Math::Vector3UVE extents = mesh.localBounds.GetExtentsUVE();
    const float boundingRadius =
        std::max(kMinimumBoundingRadiusUVE,
                 std::sqrt(extents.x * extents.x + extents.y * extents.y + extents.z * extents.z));
    const float distance = (boundingRadius / std::sin(kFieldOfViewYRadiansUVE * 0.5F)) * kCameraDistanceMarginUVE;

    const Math::Vector3UVE viewDirection = Math::NormalizeUVE(Math::Vector3UVE{1.0F, 0.85F, 1.0F});
    const Math::Vector3UVE cameraPosition = center + viewDirection * distance;

    Math::QuaternionUVE cameraRotation{};
    if (!Math::TryMakeLookAtUVE(cameraPosition - center, Math::Vector3UVE{0.0F, 1.0F, 0.0F}, cameraRotation)) {
        return 0U;
    }

    const Math::Matrix4x4UVE view =
        Math::Matrix4x4UVE::ViewFromPositionAndRotationUVE(cameraPosition, cameraRotation);
    const float farPlane = distance + boundingRadius * 2.0F + kNearPlaneUVE;
    const Math::Matrix4x4UVE projection = Math::Matrix4x4UVE::PerspectiveUVE(
        kFieldOfViewYRadiansUVE, static_cast<float>(width) / static_cast<float>(height), kNearPlaneUVE, farPlane);
    const Math::Matrix4x4UVE viewProjection = projection * view;
    const Math::Vector3UVE lightDirection = Math::NormalizeUVE(Math::Vector3UVE{-0.5F, -1.0F, -0.35F});

    GLint previousFramebuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
    GLint previousRenderbuffer = 0;
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &previousRenderbuffer);
    GLint previousViewport[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_VIEWPORT, previousViewport);
    GLint previousProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    GLint previousVao = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVao);
    GLint previousArrayBuffer = 0;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousArrayBuffer);
    GLint previousElementArrayBuffer = 0;
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &previousElementArrayBuffer);
    GLint previousTexture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
    const GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean cullFaceWasEnabled = glIsEnabled(GL_CULL_FACE);
    const GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    const GLboolean scissorWasEnabled = glIsEnabled(GL_SCISSOR_TEST);
    GLboolean previousDepthMask = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthMask);
    GLfloat previousClearColor[4] = {0.0F, 0.0F, 0.0F, 0.0F};
    glGetFloatv(GL_COLOR_CLEAR_VALUE, previousClearColor);

    bool succeeded = EnsureScratchTargetUVE(gl, width, height);

    GLuint colorTexture = 0U;
    if (succeeded) {
        glGenTextures(1, &colorTexture);
        succeeded = colorTexture != 0U;
    }
    if (succeeded) {
        glBindTexture(GL_TEXTURE_2D, colorTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        gl.glBindFramebuffer(GL_FRAMEBUFFER, gl.scratchFramebuffer);
        gl.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
        succeeded = gl.glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    }

    GLuint vao = 0U;
    GLuint vertexBuffer = 0U;
    GLuint indexBuffer = 0U;
    if (succeeded) {
        glViewport(0, 0, width, height);
        glClearColor(0.0F, 0.0F, 0.0F, 0.0F);
        SetGlCapabilityUVE(GL_DEPTH_TEST, GL_TRUE);
        glDepthMask(GL_TRUE);
        SetGlCapabilityUVE(GL_CULL_FACE, GL_FALSE);
        SetGlCapabilityUVE(GL_BLEND, GL_FALSE);
        SetGlCapabilityUVE(GL_SCISSOR_TEST, GL_FALSE);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        gl.glGenVertexArrays(1, &vao);
        gl.glBindVertexArray(vao);
        gl.glGenBuffers(1, &vertexBuffer);
        gl.glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        gl.glBufferData(GL_ARRAY_BUFFER,
                         static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(Asset::MeshVertexUVE)),
                         mesh.vertices.data(), GL_STREAM_DRAW);
        gl.glGenBuffers(1, &indexBuffer);
        gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
        gl.glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(std::uint32_t)), mesh.indices.data(),
                         GL_STREAM_DRAW);
        gl.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Asset::MeshVertexUVE),
                                  reinterpret_cast<const void*>(offsetof(Asset::MeshVertexUVE, position)));
        gl.glEnableVertexAttribArray(0);
        gl.glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Asset::MeshVertexUVE),
                                  reinterpret_cast<const void*>(offsetof(Asset::MeshVertexUVE, normal)));
        gl.glEnableVertexAttribArray(1);

        gl.glUseProgram(gl.shaderProgram);
        gl.glUniformMatrix4fv(gl.viewProjectionUniform, 1, GL_TRUE, &viewProjection.m[0][0]);
        gl.glUniform3fv(gl.lightDirectionUniform, 1, &lightDirection.x);
        gl.glUniform3fv(gl.baseColorUniform, 1, &kBaseColorUVE.x);

        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indices.size()), GL_UNSIGNED_INT, nullptr);
        succeeded = glGetError() == GL_NO_ERROR;

        gl.glBindVertexArray(0);
    }

    if (vertexBuffer != 0U) {
        gl.glDeleteBuffers(1, &vertexBuffer);
    }
    if (indexBuffer != 0U) {
        gl.glDeleteBuffers(1, &indexBuffer);
    }
    if (vao != 0U) {
        gl.glDeleteVertexArrays(1, &vao);
    }
    if (colorTexture != 0U) {
        gl.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    }

    gl.glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(previousFramebuffer));
    gl.glBindRenderbuffer(GL_RENDERBUFFER, static_cast<GLuint>(previousRenderbuffer));
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
    gl.glUseProgram(static_cast<GLuint>(previousProgram));
    gl.glBindVertexArray(static_cast<GLuint>(previousVao));
    gl.glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(previousArrayBuffer));
    gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(previousElementArrayBuffer));
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
    SetGlCapabilityUVE(GL_DEPTH_TEST, depthTestWasEnabled);
    SetGlCapabilityUVE(GL_CULL_FACE, cullFaceWasEnabled);
    SetGlCapabilityUVE(GL_BLEND, blendWasEnabled);
    SetGlCapabilityUVE(GL_SCISSOR_TEST, scissorWasEnabled);
    glDepthMask(previousDepthMask);
    glClearColor(previousClearColor[0], previousClearColor[1], previousClearColor[2], previousClearColor[3]);

    if (!succeeded) {
        if (colorTexture != 0U) {
            glDeleteTextures(1, &colorTexture);
        }
        return 0U;
    }
    return static_cast<std::uintptr_t>(colorTexture);
}

} // namespace UVE::Editor
