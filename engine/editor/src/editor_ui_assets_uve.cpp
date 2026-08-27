// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/editor/editor_ui_assets_uve.h"

#include <array>
#include <cstddef>
#include <cstdint>

#include <GL/gl.h>

namespace UVE::Editor {

namespace {

constexpr int kLogoWidthUVE = 24;
constexpr int kLogoHeightUVE = 24;
constexpr int kFolderWidthUVE = 18;
constexpr int kFolderHeightUVE = 16;
constexpr int kGizmoIconWidthUVE = 24;
constexpr int kGizmoIconHeightUVE = 24;

#include "univex_logo_uve_display_bytes.inc"
#include "uve_folder_icon_display_bytes.inc"
#include "uve_gizmo_icon_display_bytes.inc"

[[nodiscard]] std::uintptr_t UploadTextureUVE(const std::uint8_t* const pixels, const int width,
                                              const int height) noexcept {
    if (pixels == nullptr || width <= 0 || height <= 0) {
        return 0U;
    }

    GLint previousTexture = 0;
    GLint previousUnpackAlignment = 4;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);

    GLuint texture = 0U;
    glGenTextures(1, &texture);
    if (texture == 0U) {
        return 0U;
    }

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    const GLenum uploadError = glGetError();
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
    glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);
    if (uploadError != GL_NO_ERROR) {
        glDeleteTextures(1, &texture);
        return 0U;
    }

    return static_cast<std::uintptr_t>(texture);
}

void DeleteTextureUVE(std::uintptr_t& textureId) noexcept {
    if (textureId == 0U) {
        return;
    }
    const GLuint texture = static_cast<GLuint>(textureId);
    glDeleteTextures(1, &texture);
    textureId = 0U;
}

} // namespace

bool EditorUiAssetsUVE::InitializeUVE() noexcept {
    if (IsReadyUVE()) {
        return true;
    }

    m_logoTextureId = UploadTextureUVE(univex_logo_uve_display_rgba.data(), kLogoWidthUVE, kLogoHeightUVE);
    m_folderTextureId = UploadTextureUVE(uve_folder_icon_display_rgba.data(), kFolderWidthUVE, kFolderHeightUVE);
    m_moveGizmoTextureId = UploadTextureUVE(uve_gizmo_move_rgba.data(), kGizmoIconWidthUVE, kGizmoIconHeightUVE);
    m_rotateGizmoTextureId = UploadTextureUVE(uve_gizmo_rotate_rgba.data(), kGizmoIconWidthUVE, kGizmoIconHeightUVE);
    m_scaleGizmoTextureId = UploadTextureUVE(uve_gizmo_scale_rgba.data(), kGizmoIconWidthUVE, kGizmoIconHeightUVE);
    m_universalGizmoTextureId = UploadTextureUVE(uve_gizmo_universal_rgba.data(), kGizmoIconWidthUVE, kGizmoIconHeightUVE);
    m_viewNavigationTextureId = UploadTextureUVE(uve_gizmo_viewport_nav_gizmo_rgba.data(), kGizmoIconWidthUVE, kGizmoIconHeightUVE);
    if (!IsReadyUVE()) {
            DeleteTextureUVE(m_logoTextureId);
    DeleteTextureUVE(m_folderTextureId);
    DeleteTextureUVE(m_moveGizmoTextureId);
    DeleteTextureUVE(m_rotateGizmoTextureId);
    DeleteTextureUVE(m_scaleGizmoTextureId);
    DeleteTextureUVE(m_universalGizmoTextureId);
    DeleteTextureUVE(m_viewNavigationTextureId);
    return false;

    }
    return true;
}

void EditorUiAssetsUVE::ShutdownUVE() noexcept {
    DeleteTextureUVE(m_logoTextureId);
    DeleteTextureUVE(m_folderTextureId);
    DeleteTextureUVE(m_moveGizmoTextureId);
    DeleteTextureUVE(m_rotateGizmoTextureId);
    DeleteTextureUVE(m_scaleGizmoTextureId);
    DeleteTextureUVE(m_universalGizmoTextureId);
    DeleteTextureUVE(m_viewNavigationTextureId);
}

bool EditorUiAssetsUVE::IsReadyUVE() const noexcept {
    return m_logoTextureId != 0U && m_folderTextureId != 0U;
}

std::uintptr_t EditorUiAssetsUVE::GetLogoTextureIdUVE() const noexcept {
    return m_logoTextureId;
}

std::uintptr_t EditorUiAssetsUVE::GetFolderTextureIdUVE() const noexcept {
    return m_folderTextureId;
}

std::uintptr_t EditorUiAssetsUVE::GetGizmoTextureIdUVE(const EditorGizmoIconUVE icon) const noexcept {
    switch (icon) {
        case EditorGizmoIconUVE::Move:
            return m_moveGizmoTextureId;
        case EditorGizmoIconUVE::Rotate:
            return m_rotateGizmoTextureId;
        case EditorGizmoIconUVE::Scale:
            return m_scaleGizmoTextureId;
        case EditorGizmoIconUVE::Universal:
            return m_universalGizmoTextureId;
        case EditorGizmoIconUVE::ViewNavigation:
            return m_viewNavigationTextureId;
    }
    return 0U;
}

} // namespace UVE::Editor
