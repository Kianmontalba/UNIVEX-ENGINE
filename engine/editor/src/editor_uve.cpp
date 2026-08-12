//                                UniVex Engine
//
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.

#include "uve/editor/editor_uve.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>
#include <system_error>
#include <utility>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "uve/asset/uve_file_envelope_uve.h"
#include "uve/scene/components/camera_component_uve.h"

namespace UVE::Editor {

namespace {

[[nodiscard]] std::string EntityLabelUVE(const Scene::EntityUVE entity) {
    return "Entity " + std::to_string(entity.index) + ":" + std::to_string(entity.generation);
}

[[nodiscard]] std::filesystem::path MakeRecoveryPathUVE(const std::filesystem::path& scenePath) {
    std::filesystem::path recoveryPath = scenePath;
    recoveryPath += ".editor-recovery";
    return recoveryPath;
}

} // namespace

EditorUVE::EditorUVE(Core::EngineServicesUVE& services, std::filesystem::path activeScenePath)
    : m_services(&services), m_activeScenePath(std::move(activeScenePath)) {}

EditorUVE::~EditorUVE() {
    ShutdownUVE();
}

void EditorUVE::InitUVE() {
    if (m_state != EditorStateUVE::Uninitialized) {
        return;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    Scene::ISceneGraphUVE& sceneGraph = m_services->GetSceneGraphUVE();
    m_viewportCamera = entityManager.CreateEntityUVE();

    Scene::TransformComponentUVE cameraTransform{};
    cameraTransform.localPosition = Math::Vector3UVE{0.0F, 1.5F, 6.0F};
    sceneGraph.AttachTransformUVE(entityManager, m_viewportCamera, cameraTransform);
    entityManager.AddComponentUVE<Scene::CameraComponentUVE>(m_viewportCamera);

    Window::IWindowManagerUVE& windowManager = m_services->GetWindowManagerUVE();
    if (windowManager.IsValidUVE() && windowManager.GetNativeWindowHandleUVE() != nullptr) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        auto* const nativeWindow = static_cast<GLFWwindow*>(windowManager.GetNativeWindowHandleUVE());
        const bool glfwInitialized = ImGui_ImplGlfw_InitForOpenGL(nativeWindow, false);
        const bool openglInitialized = glfwInitialized && ImGui_ImplOpenGL3_Init("#version 330 core");
        if (openglInitialized) {
            m_uiInitialized = true;
        } else {
            if (glfwInitialized) {
                ImGui_ImplGlfw_Shutdown();
            }
            ImGui::DestroyContext();
        }
    }

    m_state = EditorStateUVE::Running;
}

void EditorUVE::TickUVE() {
    if (m_state != EditorStateUVE::Running) {
        return;
    }

    if (m_selectedEntity != Scene::kInvalidEntityUVE &&
        !m_services->GetEntityManagerUVE().IsAliveUVE(m_selectedEntity)) {
        ClearSelectionUVE();
    }
}

void EditorUVE::RenderOverlayUVE() {
    if (m_state != EditorStateUVE::Running || !m_uiInitialized) {
        return;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Draw the transparent scene viewport first; hierarchy and inspector panels are then layered
    // on top and remain the only interactive editor controls in this foundation increment.
    DrawViewportPanelUVE();
    DrawHierarchyPanelUVE();
    DrawInspectorPanelUVE();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool EditorUVE::SaveSceneUVE() {
    if (m_state != EditorStateUVE::Running || m_activeScenePath.empty()) {
        return false;
    }

    const std::vector<Scene::EntityUVE> roots = GetDocumentRootsUVE();
    const bool saved = m_services->GetSceneSerializerUVE().SaveUVE(
        m_services->GetEntityManagerUVE(), roots, m_activeScenePath, Asset::AssetKindUVE::Scene);
    if (saved) {
        m_sceneDirty = false;
    }
    return saved;
}

bool EditorUVE::LoadSceneUVE() {
    if (m_state != EditorStateUVE::Running || m_activeScenePath.empty() ||
        !std::filesystem::exists(m_activeScenePath)) {
        return false;
    }

    const std::filesystem::path recoveryPath = MakeRecoveryPathUVE(m_activeScenePath);
    std::error_code error;
    std::filesystem::remove(recoveryPath, error);

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    const std::vector<Scene::EntityUVE> documentRoots = GetDocumentRootsUVE();
    if (!m_services->GetSceneSerializerUVE().SaveUVE(
            entityManager, documentRoots, recoveryPath, Asset::AssetKindUVE::Scene)) {
        return false;
    }

    ClearDocumentSceneUVE();
    const std::vector<Scene::EntityUVE> loadedRoots =
        m_services->GetSceneSerializerUVE().LoadUVE(entityManager, m_activeScenePath);
    if (loadedRoots.empty()) {
        // Treat an empty result as a failed load in this foundation increment. This preserves the
        // current editable document rather than allowing a malformed file to erase it; support for
        // intentionally empty documents can be added once the serializer exposes a success status.
        // A serializer failure can have produced partially constructed ECS entities before it
        // reported failure. Clear them before restoring the known-good recovery document.
        ClearDocumentSceneUVE();
        static_cast<void>(m_services->GetSceneSerializerUVE().LoadUVE(entityManager, recoveryPath));
        std::filesystem::remove(recoveryPath, error);
        return false;
    }

    std::filesystem::remove(recoveryPath, error);
    ClearSelectionUVE();
    m_sceneDirty = false;
    return true;
}

void EditorUVE::SelectEntityUVE(const Scene::EntityUVE entity) noexcept {
    if (entity != Scene::kInvalidEntityUVE && IsDocumentEntityUVE(entity)) {
        m_selectedEntity = entity;
        return;
    }
    ClearSelectionUVE();
}

void EditorUVE::ClearSelectionUVE() noexcept {
    m_selectedEntity = Scene::kInvalidEntityUVE;
}

bool EditorUVE::SetSelectedLocalTransformUVE(const Scene::TransformComponentUVE& transform) {
    if (m_state != EditorStateUVE::Running || !IsDocumentEntityUVE(m_selectedEntity) ||
        !IsTransformFiniteUVE(transform)) {
        return false;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::TransformComponentUVE>(m_selectedEntity)) {
        return false;
    }

    m_services->GetSceneGraphUVE().SetLocalTransformUVE(entityManager, m_selectedEntity, transform);
    m_sceneDirty = true;
    return true;
}

std::vector<Scene::EntityUVE> EditorUVE::GetDocumentRootsUVE() {
    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    std::vector<Scene::EntityUVE> roots =
        m_services->GetSceneGraphUVE().GetChildrenUVE(entityManager, Scene::kInvalidEntityUVE);
    roots.erase(std::remove(roots.begin(), roots.end(), m_viewportCamera), roots.end());
    return roots;
}

EditorStateUVE EditorUVE::GetStateUVE() const noexcept {
    return m_state;
}

Scene::EntityUVE EditorUVE::GetSelectedEntityUVE() const noexcept {
    return m_selectedEntity;
}

Scene::EntityUVE EditorUVE::GetViewportCameraUVE() const noexcept {
    return m_viewportCamera;
}

bool EditorUVE::IsSceneDirtyUVE() const noexcept {
    return m_sceneDirty;
}

const std::filesystem::path& EditorUVE::GetActiveScenePathUVE() const noexcept {
    return m_activeScenePath;
}

void EditorUVE::SetActiveScenePathUVE(std::filesystem::path path) {
    if (!path.empty()) {
        m_activeScenePath = std::move(path);
    }
}

void EditorUVE::ShutdownUVE() {
    if (m_state == EditorStateUVE::Shutdown || m_state == EditorStateUVE::Uninitialized) {
        return;
    }

    if (m_uiInitialized) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        m_uiInitialized = false;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (m_viewportCamera != Scene::kInvalidEntityUVE && entityManager.IsAliveUVE(m_viewportCamera)) {
        entityManager.DestroyEntityUVE(m_viewportCamera);
    }
    m_viewportCamera = Scene::kInvalidEntityUVE;
    ClearSelectionUVE();
    m_state = EditorStateUVE::Shutdown;
}

bool EditorUVE::IsDocumentEntityUVE(const Scene::EntityUVE entity) const noexcept {
    return entity != Scene::kInvalidEntityUVE && entity != m_viewportCamera &&
           m_services->GetEntityManagerUVE().IsAliveUVE(entity);
}

bool EditorUVE::IsTransformFiniteUVE(const Scene::TransformComponentUVE& transform) const noexcept {
    return std::isfinite(transform.localPosition.x) && std::isfinite(transform.localPosition.y) &&
           std::isfinite(transform.localPosition.z) && std::isfinite(transform.localRotation.x) &&
           std::isfinite(transform.localRotation.y) && std::isfinite(transform.localRotation.z) &&
           std::isfinite(transform.localRotation.w) && std::isfinite(transform.localScale.x) &&
           std::isfinite(transform.localScale.y) && std::isfinite(transform.localScale.z);
}

void EditorUVE::DestroyDocumentSubtreeUVE(const Scene::EntityUVE root) {
    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    const std::vector<Scene::EntityUVE> children = m_services->GetSceneGraphUVE().GetChildrenUVE(entityManager, root);
    for (const Scene::EntityUVE child : children) {
        DestroyDocumentSubtreeUVE(child);
    }
    if (entityManager.IsAliveUVE(root)) {
        entityManager.DestroyEntityUVE(root);
    }
}

void EditorUVE::ClearDocumentSceneUVE() {
    const std::vector<Scene::EntityUVE> roots = GetDocumentRootsUVE();
    for (const Scene::EntityUVE root : roots) {
        DestroyDocumentSubtreeUVE(root);
    }
    ClearSelectionUVE();
}

void EditorUVE::DrawHierarchyPanelUVE() {
    ImGui::Begin("Scene Hierarchy");
    for (const Scene::EntityUVE root : GetDocumentRootsUVE()) {
        DrawHierarchyNodeUVE(root);
    }
    ImGui::End();
}

void EditorUVE::DrawHierarchyNodeUVE(const Scene::EntityUVE entity) {
    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    const std::vector<Scene::EntityUVE> children = m_services->GetSceneGraphUVE().GetChildrenUVE(entityManager, entity);
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (children.empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }
    if (entity == m_selectedEntity) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    const bool open = ImGui::TreeNodeEx(EntityLabelUVE(entity).c_str(), flags);
    if (ImGui::IsItemClicked()) {
        SelectEntityUVE(entity);
    }
    if (open) {
        for (const Scene::EntityUVE child : children) {
            DrawHierarchyNodeUVE(child);
        }
        ImGui::TreePop();
    }
}

void EditorUVE::DrawInspectorPanelUVE() {
    ImGui::Begin("Inspector");
    if (!IsDocumentEntityUVE(m_selectedEntity)) {
        ImGui::TextUnformatted("Select an entity in the Scene Hierarchy.");
        ImGui::End();
        return;
    }

    ImGui::Text("%s", EntityLabelUVE(m_selectedEntity).c_str());
    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::TransformComponentUVE>(m_selectedEntity)) {
        ImGui::TextUnformatted("No local Transform component.");
        ImGui::End();
        return;
    }

    Scene::TransformComponentUVE edited = entityManager.GetComponentUVE<Scene::TransformComponentUVE>(m_selectedEntity);
    float position[3]{edited.localPosition.x, edited.localPosition.y, edited.localPosition.z};
    float rotation[4]{edited.localRotation.x, edited.localRotation.y, edited.localRotation.z, edited.localRotation.w};
    float scale[3]{edited.localScale.x, edited.localScale.y, edited.localScale.z};

    const bool positionChanged = ImGui::InputFloat3("Local Position", position);
    const bool rotationChanged = ImGui::InputFloat4("Local Rotation (xyzw)", rotation);
    const bool scaleChanged = ImGui::InputFloat3("Local Scale", scale);
    const bool changed = positionChanged || rotationChanged || scaleChanged;
    if (changed) {
        edited.localPosition = Math::Vector3UVE{position[0], position[1], position[2]};
        edited.localRotation = Math::QuaternionUVE{rotation[0], rotation[1], rotation[2], rotation[3]};
        edited.localScale = Math::Vector3UVE{scale[0], scale[1], scale[2]};
        static_cast<void>(SetSelectedLocalTransformUVE(edited));
    }
    ImGui::End();
}

void EditorUVE::DrawViewportPanelUVE() {
    // The renderer currently presents to the engine window's default framebuffer rather than an
    // editor-owned texture. A full-window transparent, non-interactive region is therefore the
    // safe first viewport: the actual HDR scene/tone-mapped output remains visible underneath the
    // editor panels without duplicating renderer or render-target ownership.
    const ImGuiViewport* const viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowBgAlpha(0.0F);
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                       ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoInputs;
    ImGui::Begin("Viewport", nullptr, flags);
    ImGui::TextUnformatted("Game viewport: engine HDR MainColor -> ToneMapping presentation.");
    ImGui::Text("Editor camera: %s", EntityLabelUVE(m_viewportCamera).c_str());
    ImGui::TextUnformatted("Texture compositing, picking, gizmos, and play mode are deferred.");
    ImGui::End();
}

} // namespace UVE::Editor
