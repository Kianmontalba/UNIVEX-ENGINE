#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace UVE::Editor {

enum class EditorGizmoIconUVE : std::uint8_t {
    Move,
    Rotate,
    Scale,
    Universal,
    ViewNavigation,
};

/// Owns the exact-preservation raster assets used only by the native editor chrome.
/// The OpenGL texture implementation remains private to this editor-owned service.
class EditorUiAssetsUVE final {
public:
    EditorUiAssetsUVE() = default;
    EditorUiAssetsUVE(const EditorUiAssetsUVE&) = delete;
    EditorUiAssetsUVE& operator=(const EditorUiAssetsUVE&) = delete;
    EditorUiAssetsUVE(EditorUiAssetsUVE&&) = delete;
    EditorUiAssetsUVE& operator=(EditorUiAssetsUVE&&) = delete;
    ~EditorUiAssetsUVE() = default;

    [[nodiscard]] bool InitializeUVE() noexcept;
    void ShutdownUVE() noexcept;

    [[nodiscard]] bool IsReadyUVE() const noexcept;
    [[nodiscard]] std::uintptr_t GetLogoTextureIdUVE() const noexcept;
    [[nodiscard]] std::uintptr_t GetFolderTextureIdUVE() const noexcept;
    [[nodiscard]] std::uintptr_t GetGizmoTextureIdUVE(EditorGizmoIconUVE icon) const noexcept;
    [[nodiscard]] std::uintptr_t GetNodeIconTextureIdUVE(std::string_view typeId) const noexcept;
    [[nodiscard]] std::uintptr_t GetComponentIconTextureIdUVE(std::string_view componentId) const noexcept;
    [[nodiscard]] std::uintptr_t GetGeneralIconTextureIdUVE(std::string_view iconId) const noexcept;
    /// Looks up a Content Browser per-type badge icon by its ContentBrowserItemTypeUVE label
    /// (editor_uve.h), e.g. "Mesh" or "Material". Returns 0 for "Folder" (which uses
    /// GetFolderTextureIdUVE() instead) or any unrecognized key.
    [[nodiscard]] std::uintptr_t GetContentTypeIconTextureIdUVE(std::string_view typeId) const noexcept;

    /// Uploads an arbitrary RGBA8 image (e.g. a decoded Asset::TextureAssetUVE) as a standalone GL
    /// texture, for editor-owned dynamic content the fixed baked-icon set above doesn't cover
    /// (e.g. Content Browser thumbnails). Returns 0 on failure. Does not depend on instance state:
    /// callers manage the returned id's lifetime themselves via DeleteDynamicTextureUVE().
    [[nodiscard]] static std::uintptr_t UploadDynamicTextureUVE(const std::uint8_t* pixels, int width,
                                                                 int height) noexcept;
    /// Releases a texture previously returned by UploadDynamicTextureUVE() and zeroes it. Safe to
    /// call with an already-zero id.
    static void DeleteDynamicTextureUVE(std::uintptr_t& textureId) noexcept;

private:
    std::uintptr_t m_logoTextureId = 0U;
    std::uintptr_t m_folderTextureId = 0U;
    std::uintptr_t m_moveGizmoTextureId = 0U;
    std::uintptr_t m_rotateGizmoTextureId = 0U;
    std::uintptr_t m_scaleGizmoTextureId = 0U;
    std::uintptr_t m_universalGizmoTextureId = 0U;
    std::uintptr_t m_viewNavigationTextureId = 0U;
    std::array<std::uintptr_t, 41U> m_nodeIconTextureIds{};
    std::array<std::uintptr_t, 6U> m_componentIconTextureIds{};
    std::array<std::uintptr_t, 4U> m_generalIconTextureIds{};
    std::array<std::uintptr_t, 10U> m_contentTypeIconTextureIds{};
};

} // namespace UVE::Editor
