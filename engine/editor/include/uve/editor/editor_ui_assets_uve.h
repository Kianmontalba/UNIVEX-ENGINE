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
};

} // namespace UVE::Editor
