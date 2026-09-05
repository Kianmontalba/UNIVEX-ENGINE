// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstdint>
#include <memory>

namespace UVE::Asset {
struct MeshAssetUVE;
}

namespace UVE::Editor {

/// Renders a small preview image of a mesh asset's own geometry - fixed default camera angle
/// auto-framed to the mesh's bounding box, flat directional-key-light shading, no material -
/// directly into a standalone GL texture, for Content Browser mesh thumbnails.
///
/// Mirrors EditorUiAssetsUVE's precedent of talking to raw OpenGL directly from the editor module
/// rather than going through IRenderDeviceUVE: Render::TextureHandleUVE is fully opaque with no
/// accessor to its backing GLuint (by design - see gl_render_device_uve.h's header comment on
/// confining GL types to engine/render/src/), and the ECS mesh-rendering path
/// (Scene::MeshComponentUVE/Render::MeshRendererUVE) requires a valid material reference with no
/// "no material" fallback, so neither can be reused for an isolated single-mesh, no-material
/// preview render. This class therefore owns its own minimal GL function-pointer loader, shader
/// program, and scratch framebuffer entirely independent of engine/render.
///
/// Not thread-safe: must be driven from the same thread that owns the editor's GL context, and
/// only after that context has been made current (matching EditorUiAssetsUVE::InitializeUVE()'s
/// contract).
class MeshThumbnailRendererUVE final {
public:
    MeshThumbnailRendererUVE();
    MeshThumbnailRendererUVE(const MeshThumbnailRendererUVE&) = delete;
    MeshThumbnailRendererUVE& operator=(const MeshThumbnailRendererUVE&) = delete;
    MeshThumbnailRendererUVE(MeshThumbnailRendererUVE&&) = delete;
    MeshThumbnailRendererUVE& operator=(MeshThumbnailRendererUVE&&) = delete;
    ~MeshThumbnailRendererUVE();

    /// Loads the GL function pointers this renderer needs and compiles its shader program. A
    /// failure here (missing function pointers, shader compile/link failure) leaves the renderer
    /// permanently inert: RenderThumbnailUVE() returns 0 for every call without touching the GPU
    /// further.
    void InitializeUVE();

    /// Releases the shader program and the scratch framebuffer/depth-renderbuffer created lazily
    /// by RenderThumbnailUVE(). Does not release any texture id previously returned by
    /// RenderThumbnailUVE() - callers own those and must delete them independently (e.g. via
    /// EditorUiAssetsUVE::DeleteDynamicTextureUVE()).
    void ShutdownUVE() noexcept;

    /// Renders `mesh` from a fixed default viewing angle chosen to frame its own local-space
    /// bounding box into a new `width` x `height` RGBA8 texture, ready for direct use as an ImGui
    /// image id (exactly like EditorUiAssetsUVE::UploadDynamicTextureUVE()'s return value).
    /// Returns 0 if `mesh` has no vertices/indices, `width`/`height` is not positive, InitializeUVE()
    /// previously failed, or any GL step fails. The caller owns the returned texture's lifetime.
    [[nodiscard]] std::uintptr_t RenderThumbnailUVE(const Asset::MeshAssetUVE& mesh, int width, int height);

    /// Opaque - defined only in mesh_thumbnail_renderer_uve.cpp, which is the sole translation
    /// unit allowed to name its members. Public (rather than private) purely so that file's own
    /// free helper functions can take a `GlStateUVE&` parameter; nothing outside that .cpp can
    /// form more than a pointer/reference to this incomplete type. Keeps every GL type
    /// (function-pointer typedefs, GLuint) out of this public header, matching this codebase's
    /// established GL-header-confinement discipline (see gl_functions_uve.h).
    struct GlStateUVE;

private:
    std::unique_ptr<GlStateUVE> m_gl;
};

} // namespace UVE::Editor
