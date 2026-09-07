// univex/render/InfiniteGridRenderer.h
// -----------------------------------------------------------------------
// Draws the infinite ground grid: one fullscreen triangle, with the plane
// intersection, the auto-adjusting decade LOD and the depth write all
// happening per-pixel in infinite_grid.frag.
//
// Draw order: this belongs AFTER opaque scene geometry. It depth-tests
// (so solid objects in front of the ground correctly hide it) but does
// not write depth (so a semi-transparent grid line never occludes
// anything drawn later). The renderer sets that state itself and puts
// back what it found, so it can be dropped into an existing frame graph
// without leaving blend/depth state changed behind it.
// -----------------------------------------------------------------------
#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "univex/camera/OrbitCamera.h"
#include "univex/math/Mat4.h"
#include "univex/math/Vec.h"
#include "univex/render/GlApi.h"
#include "univex/render/GridSettings.h"
#include "univex/render/ShaderProgram.h"

namespace univex::render {

using univex::math::Mat4;
using univex::math::Vec3;

// Everything the grid needs to know about the current frame. Kept
// separate from OrbitCamera so an engine with its own camera type can
// drive the grid without adopting this one.
struct GridFrameParams {
    Mat4 viewProjection = Mat4::Identity();
    Mat4 inverseViewProjection = Mat4::Identity();
    Vec3 cameraPosition{};
    // Reference distance used for the horizon fade — the orbit distance
    // for an orbit camera, or roughly the camera's height above the
    // ground for a free-fly one.
    float referenceDistance = 10.f;
};

class InfiniteGridRenderer {
public:
    InfiniteGridRenderer() = default;
    ~InfiniteGridRenderer();

    InfiniteGridRenderer(const InfiniteGridRenderer&) = delete;
    InfiniteGridRenderer& operator=(const InfiniteGridRenderer&) = delete;
    InfiniteGridRenderer(InfiniteGridRenderer&& other) noexcept;
    InfiniteGridRenderer& operator=(InfiniteGridRenderer&& other) noexcept;

    // Builds the program and the fullscreen-triangle VAO. Requires a
    // current GL context with a loaded function pointer table. Returns
    // nullopt and fills `outError` if the shaders fail to build.
    [[nodiscard]] static std::optional<InfiniteGridRenderer> Create(std::string_view vertexSource,
                                                                    std::string_view fragmentSource,
                                                                    std::string& outError);

    // Uses the shader sources embedded at build time from
    // shaders/infinite_grid.{vert,frag}.
    [[nodiscard]] static std::optional<InfiniteGridRenderer> CreateWithBuiltinShaders(std::string& outError);

    [[nodiscard]] bool Valid() const { return vao_ != 0 && program_.Valid(); }

    [[nodiscard]] GridSettings& Settings() { return settings_; }
    [[nodiscard]] const GridSettings& Settings() const { return settings_; }

    void Draw(const GridFrameParams& frame) const;

    // Convenience for the OrbitCamera in this module.
    void Draw(const univex::camera::OrbitCamera& camera, int framebufferWidth, int framebufferHeight) const;

private:
    void Destroy() noexcept;

    ShaderProgram program_;
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GridSettings settings_{};
};

// World units covered by one pixel at the pivot's depth — what the
// viewport HUD should feed to ComputeDisplayGridSpacing() so the readout
// matches what the user sees around the centre of the screen.
[[nodiscard]] float WorldPerPixelAtPivot(const univex::camera::OrbitCamera& camera, int framebufferHeight);

} // namespace univex::render
