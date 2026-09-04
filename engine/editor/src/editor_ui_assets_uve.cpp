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
constexpr int kFolderWidthUVE = 18;
constexpr int kFolderHeightUVE = 16;
constexpr int kGizmoIconWidthUVE = 24;
constexpr int kGizmoIconHeightUVE = 24;
constexpr int kEditorIconWidthUVE = 20;
constexpr int kEditorIconHeightUVE = 20;
constexpr int kContentTypeIconWidthUVE = 64;
constexpr int kContentTypeIconHeightUVE = 64;

#include "univex_logo_uve_display_bytes.inc"
#include "uve_folder_icon_display_bytes.inc"
#include "uve_gizmo_icon_display_bytes.inc"
#include "uve_node_icon_bytes.inc"
#include "uve_component_icon_bytes.inc"
#include "uve_general_icon_bytes.inc"
#include "uve_content_type_icon_bytes.inc"

struct IconSourceUVE final {
    std::string_view key;
    const std::uint8_t* pixels = nullptr;
};

constexpr std::array<IconSourceUVE, 41U> kNodeIconSourcesUVE{{
    {"animatable_body_3d", uve_node_icon_animatable_body_3d_node_rgba.data()},
    {"animation_player", uve_node_icon_animation_player_node_rgba.data()},
    {"animation_tree", uve_node_icon_animation_tree_node_rgba.data()},
    {"area_3d", uve_node_icon_area_3d_node_rgba.data()},
    {"audio_source_3d", uve_node_icon_audio_source_3d_node_rgba.data()},
    {"bone_attachment_3d", uve_node_icon_bone_attachment_3d_node_rgba.data()},
    {"box_mesh_3d", uve_node_icon_box_mesh_3d_node_rgba.data()},
    {"camera_3d", uve_node_icon_camera_3d_node_rgba.data()},
    {"character_body_3d", uve_node_icon_character_body_3d_node_rgba.data()},
    {"collision_shape_3d", uve_node_icon_collision_shape_3d_node_rgba.data()},
    {"decal_3d", uve_node_icon_decal_node_rgba.data()},
    {"empty", uve_node_icon_empty_node_rgba.data()},
    {"hitbox_3d", uve_node_icon_hitbox_3d_node_rgba.data()},
    {"hurtbox_3d", uve_node_icon_hurtbox_3d_node_rgba.data()},
    {"interaction_area_3d", uve_node_icon_interaction_area_3d_node_rgba.data()},
    {"level_streamer_3d", uve_node_icon_level_streamer_node_rgba.data()},
    {"light_3d", uve_node_icon_light_3d_node_rgba.data()},
    {"lod_group_3d", uve_node_icon_lod_group_node_rgba.data()},
    {"marker_3d", uve_node_icon_marker_3d_node_rgba.data()},
    {"mesh_instance_3d", uve_node_icon_mesh_instance_3d_node_rgba.data()},
    {"navigation_agent_3d", uve_node_icon_navigation_agent_3d_node_rgba.data()},
    {"navigation_region_3d", uve_node_icon_navigation_region_3d_node_rgba.data()},
    {"occluder_3d", uve_node_icon_occluder_3d_node_rgba.data()},
    {"particle_emitter_3d", uve_node_icon_particle_emitter_3d_node_rgba.data()},
    {"plane_mesh_3d", uve_node_icon_plane_mesh_3d_node_rgba.data()},
    {"projectile_3d", uve_node_icon_projectile_3d_node_rgba.data()},
    {"ray_cast_3d", uve_node_icon_ray_cast_3d_node_rgba.data()},
    {"reflection_probe_3d", uve_node_icon_reflection_probe_node_rgba.data()},
    {"rigid_body_3d", uve_node_icon_rigid_body_3d_node_rgba.data()},
    {"scene", uve_node_icon_scene_node_rgba.data()},
    {"scene_node_registry", uve_node_icon_scene_node_registry_rgba.data()},
    {"script", uve_node_icon_script_node_rgba.data()},
    {"skeleton_3d", uve_node_icon_skeleton_3d_node_rgba.data()},
    {"spawn_point_3d", uve_node_icon_spawn_point_3d_node_rgba.data()},
    {"sphere_mesh_3d", uve_node_icon_sphere_mesh_3d_node_rgba.data()},
    {"spring_arm_3d", uve_node_icon_spring_arm_3d_node_rgba.data()},
    {"static_body_3d", uve_node_icon_static_body_3d_node_rgba.data()},
    {"transform", uve_node_icon_transform_node_rgba.data()},
    {"visibility_region_3d", uve_node_icon_visibility_region_node_rgba.data()},
    {"world_environment_3d", uve_node_icon_world_environment_node_rgba.data()},
    {"world_partition_3d", uve_node_icon_world_partition_node_rgba.data()},
}};
static_assert(kNodeIconSourcesUVE.size() == 41U, "Node icon source/storage cardinality must match the supplied catalog.");

constexpr std::array<IconSourceUVE, 6U> kComponentIconSourcesUVE{{
    {"collider", uve_component_icon_collider_component_rgba.data()},
    {"hierarchy", uve_component_icon_hierarchy_component_rgba.data()},
    {"name", uve_component_icon_name_component_rgba.data()},
    {"prefab_instance", uve_component_icon_prefab_instance_component_rgba.data()},
    {"primitive_mesh", uve_component_icon_primitive_mesh_component_rgba.data()},
    {"world_transform", uve_component_icon_world_transform_component_rgba.data()},
}};

constexpr std::array<IconSourceUVE, 4U> kGeneralIconSourcesUVE{{
    {"environment", uve_general_icon_environment_rgba.data()},
    {"plugin", uve_general_icon_plugin_rgba.data()},
    {"snap", uve_general_icon_snap_rgba.data()},
    {"sun", uve_general_icon_sun_rgba.data()},
}};

// Keyed by the exact GetContentBrowserItemTypeLabelUVE() strings (editor_uve.cpp), excluding
// "Folder" (which uses GetFolderTextureIdUVE() instead).
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
    m_folderTextureId = UploadTextureUVE(uve_folder_icon_display_rgba.data(), kFolderWidthUVE, kFolderHeightUVE);
    m_moveGizmoTextureId = UploadTextureUVE(uve_gizmo_move_rgba.data(), kGizmoIconWidthUVE, kGizmoIconHeightUVE);
    m_rotateGizmoTextureId = UploadTextureUVE(uve_gizmo_rotate_rgba.data(), kGizmoIconWidthUVE, kGizmoIconHeightUVE);
    m_scaleGizmoTextureId = UploadTextureUVE(uve_gizmo_scale_rgba.data(), kGizmoIconWidthUVE, kGizmoIconHeightUVE);
    m_universalGizmoTextureId = UploadTextureUVE(uve_gizmo_universal_rgba.data(), kGizmoIconWidthUVE, kGizmoIconHeightUVE);
    m_viewNavigationTextureId = UploadTextureUVE(uve_gizmo_viewport_nav_gizmo_rgba.data(), kGizmoIconWidthUVE, kGizmoIconHeightUVE);
    for (std::size_t index = 0U; index < kNodeIconSourcesUVE.size(); ++index) {
        m_nodeIconTextureIds[index] = UploadTextureUVE(kNodeIconSourcesUVE[index].pixels,
                                                        kEditorIconWidthUVE, kEditorIconHeightUVE);
    }
    for (std::size_t index = 0U; index < kComponentIconSourcesUVE.size(); ++index) {
        m_componentIconTextureIds[index] = UploadTextureUVE(kComponentIconSourcesUVE[index].pixels,
                                                             kEditorIconWidthUVE, kEditorIconHeightUVE);
    }
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
    DeleteTextureUVE(m_folderTextureId);
    DeleteTextureUVE(m_moveGizmoTextureId);
    DeleteTextureUVE(m_rotateGizmoTextureId);
    DeleteTextureUVE(m_scaleGizmoTextureId);
    DeleteTextureUVE(m_universalGizmoTextureId);
    DeleteTextureUVE(m_viewNavigationTextureId);
    DeleteTextureSetUVE(m_nodeIconTextureIds);
    DeleteTextureSetUVE(m_componentIconTextureIds);
    DeleteTextureSetUVE(m_generalIconTextureIds);
    DeleteTextureSetUVE(m_contentTypeIconTextureIds);
}

bool EditorUiAssetsUVE::IsReadyUVE() const noexcept {
    return m_logoTextureId != 0U && m_folderTextureId != 0U && m_moveGizmoTextureId != 0U &&
           m_rotateGizmoTextureId != 0U && m_scaleGizmoTextureId != 0U && m_universalGizmoTextureId != 0U &&
           m_viewNavigationTextureId != 0U &&
           std::all_of(m_nodeIconTextureIds.cbegin(), m_nodeIconTextureIds.cend(),
                       [](const std::uintptr_t textureId) { return textureId != 0U; }) &&
           std::all_of(m_componentIconTextureIds.cbegin(), m_componentIconTextureIds.cend(),
                       [](const std::uintptr_t textureId) { return textureId != 0U; }) &&
           std::all_of(m_generalIconTextureIds.cbegin(), m_generalIconTextureIds.cend(),
                       [](const std::uintptr_t textureId) { return textureId != 0U; }) &&
           std::all_of(m_contentTypeIconTextureIds.cbegin(), m_contentTypeIconTextureIds.cend(),
                       [](const std::uintptr_t textureId) { return textureId != 0U; });
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

std::uintptr_t EditorUiAssetsUVE::GetNodeIconTextureIdUVE(const std::string_view typeId) const noexcept {
    return FindTextureIdUVE(kNodeIconSourcesUVE, m_nodeIconTextureIds, typeId);
}

std::uintptr_t EditorUiAssetsUVE::GetComponentIconTextureIdUVE(const std::string_view componentId) const noexcept {
    return FindTextureIdUVE(kComponentIconSourcesUVE, m_componentIconTextureIds, componentId);
}

std::uintptr_t EditorUiAssetsUVE::GetGeneralIconTextureIdUVE(const std::string_view iconId) const noexcept {
    return FindTextureIdUVE(kGeneralIconSourcesUVE, m_generalIconTextureIds, iconId);
}

std::uintptr_t EditorUiAssetsUVE::GetContentTypeIconTextureIdUVE(const std::string_view typeId) const noexcept {
    return FindTextureIdUVE(kContentTypeIconSourcesUVE, m_contentTypeIconTextureIds, typeId);
}

} // namespace UVE::Editor
