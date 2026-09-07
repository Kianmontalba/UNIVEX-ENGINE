#include "univex/render/InfiniteGridRenderer.h"

#include <array>
#include <cmath>
#include <utility>

#include "univex/render/GridShaderSources.h"

namespace univex::render {

namespace {

// A single triangle that covers the whole clip-space square. Bigger than
// the viewport on purpose: one triangle rasterises with no diagonal seam
// and no redundant fragments along a quad's shared edge.
constexpr std::array<float, 6> kFullscreenTriangle = {
    -1.f, -1.f,
     3.f, -1.f,
    -1.f,  3.f,
};

} // namespace

InfiniteGridRenderer::~InfiniteGridRenderer() { Destroy(); }

InfiniteGridRenderer::InfiniteGridRenderer(InfiniteGridRenderer&& other) noexcept
    : program_(std::move(other.program_)),
      vao_(std::exchange(other.vao_, 0)),
      vbo_(std::exchange(other.vbo_, 0)),
      settings_(other.settings_) {}

InfiniteGridRenderer& InfiniteGridRenderer::operator=(InfiniteGridRenderer&& other) noexcept {
    if (this != &other) {
        Destroy();
        program_ = std::move(other.program_);
        vao_ = std::exchange(other.vao_, 0);
        vbo_ = std::exchange(other.vbo_, 0);
        settings_ = other.settings_;
    }
    return *this;
}

void InfiniteGridRenderer::Destroy() noexcept {
    if (vbo_ != 0) { glDeleteBuffers(1, &vbo_); vbo_ = 0; }
    if (vao_ != 0) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
}

std::optional<InfiniteGridRenderer> InfiniteGridRenderer::Create(std::string_view vertexSource,
                                                                 std::string_view fragmentSource,
                                                                 std::string& outError) {
    auto program = ShaderProgram::Build(vertexSource, fragmentSource, outError);
    if (!program.has_value()) return std::nullopt;

    InfiniteGridRenderer renderer;
    renderer.program_ = std::move(*program);

    glGenVertexArrays(1, &renderer.vao_);
    glBindVertexArray(renderer.vao_);

    glGenBuffers(1, &renderer.vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, renderer.vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(kFullscreenTriangle.size() * sizeof(float)),
                 kFullscreenTriangle.data(),
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return renderer;
}

std::optional<InfiniteGridRenderer> InfiniteGridRenderer::CreateWithBuiltinShaders(std::string& outError) {
    return Create(shaders::kInfiniteGridVertexSource, shaders::kInfiniteGridFragmentSource, outError);
}

void InfiniteGridRenderer::Draw(const GridFrameParams& frame) const {
    if (!Valid()) return;

    // ---- remember the state we are about to change ------------------------
    const GLboolean hadBlend = glIsEnabled(GL_BLEND);
    const GLboolean hadDepthTest = glIsEnabled(GL_DEPTH_TEST);
    GLboolean depthMask = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
    GLint blendSrcRgb = GL_ONE, blendDstRgb = GL_ZERO, blendSrcAlpha = GL_ONE, blendDstAlpha = GL_ZERO;
    glGetIntegerv(GL_BLEND_SRC_RGB, &blendSrcRgb);
    glGetIntegerv(GL_BLEND_DST_RGB, &blendDstRgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDstAlpha);

    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE); // test against the scene, but never occlude it

    program_.Use();
    program_.SetMat4("uViewProj", frame.viewProjection.Data());
    program_.SetMat4("uInvViewProj", frame.inverseViewProjection.Data());
    program_.SetVec3("uCameraPos", frame.cameraPosition.x, frame.cameraPosition.y, frame.cameraPosition.z);

    program_.SetFloat("uBaseSpacing", settings_.baseSpacing);
    program_.SetFloat("uTargetCellPixels", settings_.targetCellPixels);
    program_.SetFloat("uLineWidthPixels", settings_.lineWidthPixels);
    program_.SetFloat("uAxisWidthPixels", settings_.axisWidthPixels);

    program_.SetVec3("uThinColor", settings_.thinColor.r, settings_.thinColor.g, settings_.thinColor.b);
    program_.SetVec3("uMidColor", settings_.midColor.r, settings_.midColor.g, settings_.midColor.b);
    program_.SetVec3("uThickColor", settings_.thickColor.r, settings_.thickColor.g, settings_.thickColor.b);
    program_.SetFloat("uThinIntensity", settings_.thinIntensity);
    program_.SetFloat("uMidIntensity", settings_.midIntensity);
    program_.SetFloat("uThickIntensity", settings_.thickIntensity);

    program_.SetVec3("uAxisColorX", settings_.axisColorX.r, settings_.axisColorX.g, settings_.axisColorX.b);
    program_.SetVec3("uAxisColorZ", settings_.axisColorZ.r, settings_.axisColorZ.g, settings_.axisColorZ.b);

    const float reference = frame.referenceDistance > 1e-4f ? frame.referenceDistance : 1e-4f;
    program_.SetFloat("uFadeStart", reference * settings_.fadeStartDistanceScale);
    program_.SetFloat("uFadeEnd", reference * settings_.fadeEndDistanceScale);
    program_.SetFloat("uOpacity", settings_.opacity);

    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    // ---- put it back ------------------------------------------------------
    glDepthMask(depthMask);
    if (hadDepthTest == GL_FALSE) glDisable(GL_DEPTH_TEST);
    glBlendFuncSeparate(static_cast<GLenum>(blendSrcRgb), static_cast<GLenum>(blendDstRgb),
                        static_cast<GLenum>(blendSrcAlpha), static_cast<GLenum>(blendDstAlpha));
    if (hadBlend == GL_FALSE) glDisable(GL_BLEND);
}

void InfiniteGridRenderer::Draw(const univex::camera::OrbitCamera& camera,
                                int framebufferWidth,
                                int framebufferHeight) const {
    if (framebufferWidth <= 0 || framebufferHeight <= 0) return;
    const float aspect = static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight);

    GridFrameParams frame;
    frame.viewProjection = camera.ViewProjection(aspect);
    frame.inverseViewProjection = camera.InverseViewProjection(aspect);
    frame.cameraPosition = camera.Eye();
    frame.referenceDistance = camera.Distance();
    Draw(frame);
}

float WorldPerPixelAtPivot(const univex::camera::OrbitCamera& camera, int framebufferHeight) {
    if (framebufferHeight <= 0) return 0.f;
    const float viewportWorldHeight =
        2.f * camera.Distance() * std::tan(camera.Settings().fovYRadians * 0.5f);
    return viewportWorldHeight / static_cast<float>(framebufferHeight);
}

} // namespace univex::render
