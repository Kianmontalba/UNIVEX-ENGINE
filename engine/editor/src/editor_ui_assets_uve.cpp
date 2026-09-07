// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/editor/editor_ui_assets_uve.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include <GL/gl.h>

namespace UVE::Editor {

namespace {

constexpr int kLogoWidthUVE = 24;
constexpr int kLogoHeightUVE = 24;
constexpr int kEditorIconWidthUVE = 20;
constexpr int kEditorIconHeightUVE = 20;
constexpr int kContentTypeIconWidthUVE = 64;
constexpr int kContentTypeIconHeightUVE = 64;

#include "univex_logo_uve_display_bytes.inc"
#include "uve_general_icon_bytes.inc"
#include "uve_content_type_icon_bytes.inc"

struct IconSourceUVE final {
    std::string_view key;
    const std::uint8_t* pixels = nullptr;
};

constexpr std::array<IconSourceUVE, 4U> kGeneralIconSourcesUVE{{
    {"environment", uve_general_icon_environment_rgba.data()},
    {"plugin", uve_general_icon_plugin_rgba.data()},
    {"snap", uve_general_icon_snap_rgba.data()},
    {"sun", uve_general_icon_sun_rgba.data()},
}};

// Keyed by the exact GetContentBrowserItemTypeLabelUVE() strings (editor_uve.cpp).
constexpr std::array<IconSourceUVE, 10U> kContentTypeIconSourcesUVE{{
    {"Scene", uve_content_type_icon_scene_content_type_rgba.data()},
    {"Prefab", uve_content_type_icon_prefab_content_type_rgba.data()},
    {"Bundle", uve_content_type_icon_bundle_content_type_rgba.data()},
    {"Mesh", uve_content_type_icon_mesh_content_type_rgba.data()},
    {"Texture", uve_content_type_icon_texture_content_type_rgba.data()},
    {"Shader", uve_content_type_icon_shader_content_type_rgba.data()},
    {"Material", uve_content_type_icon_material_content_type_rgba.data()},
    {"Save", uve_content_type_icon_save_content_type_rgba.data()},
    {"Motion Query", uve_content_type_icon_motion_query_content_type_rgba.data()},
    {"File", uve_content_type_icon_file_content_type_rgba.data()},
}};

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

template <std::size_t Size>
void DeleteTextureSetUVE(std::array<std::uintptr_t, Size>& textureIds) noexcept {
    for (std::uintptr_t& textureId : textureIds) {
        DeleteTextureUVE(textureId);
    }
}

template <std::size_t Size>
[[nodiscard]] std::uintptr_t FindTextureIdUVE(const std::array<IconSourceUVE, Size>& sources,
                                               const std::array<std::uintptr_t, Size>& textureIds,
                                               const std::string_view key) noexcept {
    for (std::size_t index = 0U; index < sources.size(); ++index) {
        if (sources[index].key == key) {
            return textureIds[index];
        }
    }
    return 0U;
}

} // namespace

bool EditorUiAssetsUVE::InitializeUVE() noexcept {
    if (IsReadyUVE()) {
        return true;
    }

    m_logoTextureId = UploadTextureUVE(univex_logo_uve_display_rgba.data(), kLogoWidthUVE, kLogoHeightUVE);
    for (std::size_t index = 0U; index < kGeneralIconSourcesUVE.size(); ++index) {
        m_generalIconTextureIds[index] = UploadTextureUVE(kGeneralIconSourcesUVE[index].pixels,
                                                           kEditorIconWidthUVE, kEditorIconHeightUVE);
    }
    for (std::size_t index = 0U; index < kContentTypeIconSourcesUVE.size(); ++index) {
        m_contentTypeIconTextureIds[index] = UploadTextureUVE(
            kContentTypeIconSourcesUVE[index].pixels, kContentTypeIconWidthUVE, kContentTypeIconHeightUVE);
    }

    if (!IsReadyUVE()) {
        ShutdownUVE();
        return false;
    }
    return true;
}

void EditorUiAssetsUVE::ShutdownUVE() noexcept {
    DeleteTextureUVE(m_logoTextureId);
    DeleteTextureSetUVE(m_generalIconTextureIds);
    DeleteTextureSetUVE(m_contentTypeIconTextureIds);
}

bool EditorUiAssetsUVE::IsReadyUVE() const noexcept {
    return m_logoTextureId != 0U &&
           std::all_of(m_generalIconTextureIds.cbegin(), m_generalIconTextureIds.cend(),
                       [](const std::uintptr_t textureId) { return textureId != 0U; }) &&
           std::all_of(m_contentTypeIconTextureIds.cbegin(), m_contentTypeIconTextureIds.cend(),
                       [](const std::uintptr_t textureId) { return textureId != 0U; });
}

std::uintptr_t EditorUiAssetsUVE::GetLogoTextureIdUVE() const noexcept {
    return m_logoTextureId;
}

std::uintptr_t EditorUiAssetsUVE::GetGeneralIconTextureIdUVE(const std::string_view iconId) const noexcept {
    return FindTextureIdUVE(kGeneralIconSourcesUVE, m_generalIconTextureIds, iconId);
}

std::uintptr_t EditorUiAssetsUVE::GetContentTypeIconTextureIdUVE(const std::string_view typeId) const noexcept {
    return FindTextureIdUVE(kContentTypeIconSourcesUVE, m_contentTypeIconTextureIds, typeId);
}

std::uintptr_t EditorUiAssetsUVE::UploadDynamicTextureUVE(const std::uint8_t* const pixels, const int width,
                                                           const int height) noexcept {
    return UploadTextureUVE(pixels, width, height);
}

void EditorUiAssetsUVE::DeleteDynamicTextureUVE(std::uintptr_t& textureId) noexcept {
    DeleteTextureUVE(textureId);
}

} // namespace UVE::Editor
