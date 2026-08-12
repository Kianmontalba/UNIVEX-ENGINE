//
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.

#include "uve/editor/editor_uve.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <limits>
#include <numbers>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "uve/asset/uve_file_envelope_uve.h"
#include "uve/physics/raycast_query_uve.h"
#include "uve/scene/components/camera_component_uve.h"
#include "uve/scene/components/collider_component_uve.h"
#include "uve/scene/components/hierarchy_component_uve.h"
#include "uve/scene/components/light_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"

namespace UVE::Editor {

namespace {

constexpr float kVectorEpsilonUVE = 0.00001F;
constexpr float kGizmoAxisLengthUVE = 1.25F;
constexpr float kGizmoHandleRadiusPixelsUVE = 12.0F;
constexpr float kMinimumViewportWidthUVE = 64.0F;
constexpr float kMinimumViewportHeightUVE = 64.0F;
constexpr float kAssetsPanelHeightUVE = 150.0F;

[[nodiscard]] bool ContainsCaseInsensitiveUVE(const std::string_view text,
                                               const std::string_view query) noexcept {
    if (query.empty()) {
        return true;
    }

    const auto equalsCaseInsensitive = [](const char lhs, const char rhs) noexcept {
        return std::tolower(static_cast<unsigned char>(lhs)) == std::tolower(static_cast<unsigned char>(rhs));
    };
    return std::search(text.begin(), text.end(), query.begin(), query.end(), equalsCaseInsensitive) != text.end();
}

[[nodiscard]] std::string EntityLabelUVE(const Scene::EntityUVE entity) {
    return "Entity " + std::to_string(entity.index) + ":" + std::to_string(entity.generation);
}

[[nodiscard]] std::filesystem::path MakeRecoveryPathUVE(const std::filesystem::path& scenePath) {
    std::filesystem::path recoveryPath = scenePath;
    recoveryPath += ".editor-recovery";
    return recoveryPath;
}

[[nodiscard]] bool IsFiniteUVE(const float value) noexcept {
    return std::isfinite(value);
}

[[nodiscard]] float Dot2UVE(const Math::Vector2UVE& lhs, const Math::Vector2UVE& rhs) noexcept {
    return lhs.x * rhs.x + lhs.y * rhs.y;
}

[[nodiscard]] float LengthSquared2UVE(const Math::Vector2UVE& vector) noexcept {
    return Dot2UVE(vector, vector);
}

[[nodiscard]] Math::Vector2UVE Scale2UVE(const Math::Vector2UVE& vector, const float scalar) noexcept {
    return Math::Vector2UVE{vector.x * scalar, vector.y * scalar};
}

[[nodiscard]] Math::QuaternionUVE ConjugateUVE(const Math::QuaternionUVE& value) noexcept {
    return Math::QuaternionUVE{-value.x, -value.y, -value.z, value.w};
}

[[nodiscard]] ImU32 GizmoAxisColorUVE(const EditorTranslateAxisUVE axis, const bool active) noexcept {
    const std::uint8_t alpha = active ? 255U : 210U;
    switch (axis) {
        case EditorTranslateAxisUVE::X:
            return IM_COL32(224, 83, 83, alpha);
        case EditorTranslateAxisUVE::Y:
            return IM_COL32(97, 196, 111, alpha);
        case EditorTranslateAxisUVE::Z:
            return IM_COL32(87, 139, 231, alpha);
        case EditorTranslateAxisUVE::None:
            return IM_COL32(190, 190, 190, alpha);
    }
    return IM_COL32(190, 190, 190, alpha);
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
        // Install the backend's chained GLFW callbacks so the interactive overlay receives cursor
        // and pointer-button events while WindowManagerUVE's existing close/resize/focus callbacks
        // remain active. Engine input remains a separate service-level abstraction; overlay clicks
        // consume ImGui pointer state and never leak into runtime action mappings.
        const bool glfwInitialized = ImGui_ImplGlfw_InitForOpenGL(nativeWindow, true);
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
    if (m_gizmoDrag.axis != EditorTranslateAxisUVE::None &&
        (!IsDocumentEntityUVE(m_gizmoDrag.entity) || m_gizmoDrag.entity != m_selectedEntity)) {
        CancelGizmoDragUVE();
    }
}

void EditorUVE::RenderOverlayUVE() {
    if (m_state != EditorStateUVE::Running || !m_uiInitialized) {
        return;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    DrawMenuBarUVE();
    DrawViewportPanelUVE();
    DrawHierarchyPanelUVE();
    DrawInspectorPanelUVE();
    DrawAssetsPanelUVE();

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
    CancelGizmoDragUVE();
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

std::optional<Math::RayUVE> EditorUVE::MakeViewportRayUVE(const EditorViewportRectUVE& viewportRect,
                                                            const Math::Vector2UVE pointerPosition) const {
    if (m_state != EditorStateUVE::Running || !IsViewportRectValidUVE(viewportRect) ||
        !IsFiniteUVE(pointerPosition.x) || !IsFiniteUVE(pointerPosition.y) ||
        pointerPosition.x < viewportRect.origin.x || pointerPosition.y < viewportRect.origin.y ||
        pointerPosition.x > viewportRect.origin.x + viewportRect.size.x ||
        pointerPosition.y > viewportRect.origin.y + viewportRect.size.y) {
        return std::nullopt;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.IsAliveUVE(m_viewportCamera) ||
        !entityManager.HasComponentUVE<Scene::CameraComponentUVE>(m_viewportCamera) ||
        !entityManager.HasComponentUVE<Scene::WorldTransformComponentUVE>(m_viewportCamera)) {
        return std::nullopt;
    }

    const Scene::CameraComponentUVE& camera =
        entityManager.GetComponentUVE<Scene::CameraComponentUVE>(m_viewportCamera);
    const Scene::WorldTransformComponentUVE& cameraWorld =
        entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(m_viewportCamera);
    if (cameraWorld.dirty || !IsFiniteUVE(camera.fieldOfViewDegrees) || camera.fieldOfViewDegrees <= 1.0F ||
        camera.fieldOfViewDegrees >= 179.0F || !IsFiniteUVE(camera.nearPlane) ||
        !IsFiniteUVE(camera.farPlane) || camera.nearPlane <= 0.0F || camera.farPlane <= camera.nearPlane ||
        !IsFiniteVectorUVE(cameraWorld.worldPosition)) {
        return std::nullopt;
    }

    const float relativeX = (pointerPosition.x - viewportRect.origin.x) / viewportRect.size.x;
    const float relativeY = (pointerPosition.y - viewportRect.origin.y) / viewportRect.size.y;
    const float aspectRatio = viewportRect.size.x / viewportRect.size.y;
    const float tanHalfFov = std::tan((camera.fieldOfViewDegrees * std::numbers::pi_v<float>) / 360.0F);
    if (!IsFiniteUVE(aspectRatio) || !IsFiniteUVE(tanHalfFov) || aspectRatio <= kVectorEpsilonUVE ||
        tanHalfFov <= kVectorEpsilonUVE) {
        return std::nullopt;
    }

    const Math::Vector3UVE cameraDirection{
        ((relativeX * 2.0F) - 1.0F) * tanHalfFov * aspectRatio,
        (1.0F - (relativeY * 2.0F)) * tanHalfFov,
        -1.0F,
    };
    if (Math::LengthSquaredUVE(cameraDirection) <= kVectorEpsilonUVE) {
        return std::nullopt;
    }

    const Math::Vector3UVE worldDirection =
        Math::RotateVectorUVE(cameraWorld.worldRotation, Math::NormalizeUVE(cameraDirection));
    if (!IsFiniteVectorUVE(worldDirection) || Math::LengthSquaredUVE(worldDirection) <= kVectorEpsilonUVE) {
        return std::nullopt;
    }
    return Math::RayUVE{cameraWorld.worldPosition, Math::NormalizeUVE(worldDirection)};
}

bool EditorUVE::PickViewportUVE(const EditorViewportRectUVE& viewportRect,
                                 const Math::Vector2UVE pointerPosition) {
    const std::optional<Math::RayUVE> ray = MakeViewportRayUVE(viewportRect, pointerPosition);
    if (!ray.has_value()) {
        return false;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    const Scene::CameraComponentUVE& camera =
        entityManager.GetComponentUVE<Scene::CameraComponentUVE>(m_viewportCamera);
    Physics::RaycastQueryUVE query{};
    query.ray = *ray;
    query.maxDistance = camera.farPlane;
    query.ignoreEntity = m_viewportCamera;

    const std::optional<Physics::RaycastHitUVE> hit =
        m_services->GetRaycastSystemUVE().RaycastUVE(entityManager, query);
    if (hit.has_value() && IsDocumentEntityUVE(hit->entity)) {
        SelectEntityUVE(hit->entity);
        return true;
    }

    ClearSelectionUVE();
    return false;
}

bool EditorUVE::TranslateSelectedAlongAxisUVE(const EditorTranslateAxisUVE axis, const float worldDistance) {
    if (m_state != EditorStateUVE::Running || !IsDocumentEntityUVE(m_selectedEntity) ||
        !IsFiniteUVE(worldDistance) || axis == EditorTranslateAxisUVE::None) {
        return false;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::TransformComponentUVE>(m_selectedEntity)) {
        return false;
    }

    Math::Vector3UVE localDelta{};
    const Math::Vector3UVE worldDelta = GetAxisVectorUVE(axis) * worldDistance;
    if (!ComputeLocalDeltaForWorldDeltaUVE(m_selectedEntity, worldDelta, localDelta)) {
        return false;
    }

    Scene::TransformComponentUVE updated =
        entityManager.GetComponentUVE<Scene::TransformComponentUVE>(m_selectedEntity);
    updated.localPosition += localDelta;
    return SetSelectedLocalTransformUVE(updated);
}

Scene::EntityUVE EditorUVE::CreateDocumentEntityUVE(const EditorEntityKindUVE kind) {
    if (m_state != EditorStateUVE::Running) {
        return Scene::kInvalidEntityUVE;
    }

    switch (kind) {
        case EditorEntityKindUVE::Empty:
        case EditorEntityKindUVE::Camera:
        case EditorEntityKindUVE::DirectionalLight:
        case EditorEntityKindUVE::CollisionBox:
            break;
        default:
            return Scene::kInvalidEntityUVE;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    Scene::ISceneGraphUVE& sceneGraph = m_services->GetSceneGraphUVE();
    const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
    sceneGraph.AttachTransformUVE(entityManager, entity, Scene::TransformComponentUVE{});

    switch (kind) {
        case EditorEntityKindUVE::Empty:
            break;
        case EditorEntityKindUVE::Camera:
            entityManager.AddComponentUVE<Scene::CameraComponentUVE>(entity);
            break;
        case EditorEntityKindUVE::DirectionalLight: {
            Scene::LightComponentUVE light{};
            light.type = Scene::LightTypeUVE::Directional;
            entityManager.AddComponentUVE<Scene::LightComponentUVE>(entity, light);
            break;
        }
        case EditorEntityKindUVE::CollisionBox:
            entityManager.AddComponentUVE<Scene::ColliderComponentUVE>(entity);
            break;
        default:
            return Scene::kInvalidEntityUVE;
    }

    SelectEntityUVE(entity);
    m_sceneDirty = true;
    return entity;
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

const std::optional<Asset::AssetRecordUVE>& EditorUVE::GetSelectedAssetUVE() const noexcept {
    return m_selectedAsset;
}

const std::string& EditorUVE::GetAssetFilterUVE() const noexcept {
    return m_assetFilter;
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

    CancelGizmoDragUVE();
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
    return IsFiniteVectorUVE(transform.localPosition) && IsFiniteUVE(transform.localRotation.x) &&
           IsFiniteUVE(transform.localRotation.y) && IsFiniteUVE(transform.localRotation.z) &&
           IsFiniteUVE(transform.localRotation.w) && IsFiniteVectorUVE(transform.localScale);
}

bool EditorUVE::IsViewportRectValidUVE(const EditorViewportRectUVE& viewportRect) const noexcept {
    return IsFiniteUVE(viewportRect.origin.x) && IsFiniteUVE(viewportRect.origin.y) &&
           IsFiniteUVE(viewportRect.size.x) && IsFiniteUVE(viewportRect.size.y) &&
           viewportRect.size.x >= kMinimumViewportWidthUVE && viewportRect.size.y >= kMinimumViewportHeightUVE;
}

bool EditorUVE::IsFiniteVectorUVE(const Math::Vector3UVE& vector) const noexcept {
    return IsFiniteUVE(vector.x) && IsFiniteUVE(vector.y) && IsFiniteUVE(vector.z);
}

Math::Vector3UVE EditorUVE::GetAxisVectorUVE(const EditorTranslateAxisUVE axis) const noexcept {
    switch (axis) {
        case EditorTranslateAxisUVE::X:
            return Math::Vector3UVE{1.0F, 0.0F, 0.0F};
        case EditorTranslateAxisUVE::Y:
            return Math::Vector3UVE{0.0F, 1.0F, 0.0F};
        case EditorTranslateAxisUVE::Z:
            return Math::Vector3UVE{0.0F, 0.0F, 1.0F};
        case EditorTranslateAxisUVE::None:
            return Math::Vector3UVE{};
    }
    return Math::Vector3UVE{};
}

bool EditorUVE::ProjectWorldPointUVE(const EditorViewportRectUVE& viewportRect,
                                     const Math::Vector3UVE& worldPoint,
                                     Math::Vector2UVE& outScreenPoint) const {
    if (!IsViewportRectValidUVE(viewportRect) || !IsFiniteVectorUVE(worldPoint)) {
        return false;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.IsAliveUVE(m_viewportCamera) ||
        !entityManager.HasComponentUVE<Scene::CameraComponentUVE>(m_viewportCamera) ||
        !entityManager.HasComponentUVE<Scene::WorldTransformComponentUVE>(m_viewportCamera)) {
        return false;
    }

    const Scene::CameraComponentUVE& camera =
        entityManager.GetComponentUVE<Scene::CameraComponentUVE>(m_viewportCamera);
    const Scene::WorldTransformComponentUVE& cameraWorld =
        entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(m_viewportCamera);
    if (cameraWorld.dirty || !IsFiniteUVE(camera.fieldOfViewDegrees) || camera.fieldOfViewDegrees <= 1.0F ||
        camera.fieldOfViewDegrees >= 179.0F || !IsFiniteVectorUVE(cameraWorld.worldPosition)) {
        return false;
    }

    const Math::Vector3UVE cameraSpace =
        Math::RotateVectorUVE(ConjugateUVE(cameraWorld.worldRotation), worldPoint - cameraWorld.worldPosition);
    if (!IsFiniteVectorUVE(cameraSpace) || cameraSpace.z >= -kVectorEpsilonUVE) {
        return false;
    }

    const float aspectRatio = viewportRect.size.x / viewportRect.size.y;
    const float tanHalfFov = std::tan((camera.fieldOfViewDegrees * std::numbers::pi_v<float>) / 360.0F);
    if (!IsFiniteUVE(aspectRatio) || !IsFiniteUVE(tanHalfFov) || aspectRatio <= kVectorEpsilonUVE ||
        tanHalfFov <= kVectorEpsilonUVE) {
        return false;
    }

    const float inverseDepth = 1.0F / -cameraSpace.z;
    const float ndcX = (cameraSpace.x * inverseDepth) / (tanHalfFov * aspectRatio);
    const float ndcY = (cameraSpace.y * inverseDepth) / tanHalfFov;
    if (!IsFiniteUVE(ndcX) || !IsFiniteUVE(ndcY)) {
        return false;
    }

    outScreenPoint.x = viewportRect.origin.x + ((ndcX + 1.0F) * 0.5F * viewportRect.size.x);
    outScreenPoint.y = viewportRect.origin.y + ((1.0F - ndcY) * 0.5F * viewportRect.size.y);
    return IsFiniteUVE(outScreenPoint.x) && IsFiniteUVE(outScreenPoint.y);
}

bool EditorUVE::ComputeLocalDeltaForWorldDeltaUVE(const Scene::EntityUVE entity,
                                                   const Math::Vector3UVE& worldDelta,
                                                   Math::Vector3UVE& outLocalDelta) const {
    if (!IsDocumentEntityUVE(entity) || !IsFiniteVectorUVE(worldDelta)) {
        return false;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::HierarchyComponentUVE>(entity)) {
        return false;
    }

    const Scene::HierarchyComponentUVE& hierarchy =
        entityManager.GetComponentUVE<Scene::HierarchyComponentUVE>(entity);
    if (hierarchy.parent == Scene::kInvalidEntityUVE) {
        outLocalDelta = worldDelta;
        return true;
    }

    if (!entityManager.IsAliveUVE(hierarchy.parent) ||
        !entityManager.HasComponentUVE<Scene::WorldTransformComponentUVE>(hierarchy.parent)) {
        return false;
    }

    const Scene::WorldTransformComponentUVE& parentWorld =
        entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(hierarchy.parent);
    if (parentWorld.dirty || !IsFiniteVectorUVE(parentWorld.worldScale) ||
        std::abs(parentWorld.worldScale.x) <= kVectorEpsilonUVE ||
        std::abs(parentWorld.worldScale.y) <= kVectorEpsilonUVE ||
        std::abs(parentWorld.worldScale.z) <= kVectorEpsilonUVE) {
        return false;
    }

    const Math::Vector3UVE unrotated =
        Math::RotateVectorUVE(ConjugateUVE(parentWorld.worldRotation), worldDelta);
    outLocalDelta = Math::Vector3UVE{
        unrotated.x / parentWorld.worldScale.x,
        unrotated.y / parentWorld.worldScale.y,
        unrotated.z / parentWorld.worldScale.z,
    };
    return IsFiniteVectorUVE(outLocalDelta);
}

bool EditorUVE::BeginGizmoDragUVE(const EditorViewportRectUVE& viewportRect,
                                  const Math::Vector2UVE pointerPosition) {
    if (!IsDocumentEntityUVE(m_selectedEntity) || !IsViewportRectValidUVE(viewportRect)) {
        return false;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::TransformComponentUVE>(m_selectedEntity) ||
        !entityManager.HasComponentUVE<Scene::WorldTransformComponentUVE>(m_selectedEntity)) {
        return false;
    }

    const Scene::WorldTransformComponentUVE& selectedWorld =
        entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(m_selectedEntity);
    if (selectedWorld.dirty || !IsFiniteVectorUVE(selectedWorld.worldPosition)) {
        return false;
    }

    Math::Vector2UVE center{};
    if (!ProjectWorldPointUVE(viewportRect, selectedWorld.worldPosition, center)) {
        return false;
    }

    float bestDistanceSquared = std::numeric_limits<float>::max();
    GizmoDragUVE candidate{};
    constexpr std::array<EditorTranslateAxisUVE, 3> axes{
        EditorTranslateAxisUVE::X,
        EditorTranslateAxisUVE::Y,
        EditorTranslateAxisUVE::Z,
    };
    for (const EditorTranslateAxisUVE axis : axes) {
        Math::Vector2UVE endpoint{};
        const Math::Vector3UVE worldEndpoint = selectedWorld.worldPosition + GetAxisVectorUVE(axis) * kGizmoAxisLengthUVE;
        if (!ProjectWorldPointUVE(viewportRect, worldEndpoint, endpoint)) {
            continue;
        }

        const Math::Vector2UVE screenAxis{endpoint.x - center.x, endpoint.y - center.y};
        const float axisLengthSquared = LengthSquared2UVE(screenAxis);
        if (axisLengthSquared <= kVectorEpsilonUVE) {
            continue;
        }

        const Math::Vector2UVE pointerOffset{pointerPosition.x - center.x, pointerPosition.y - center.y};
        const float along = std::clamp(Dot2UVE(pointerOffset, screenAxis) / axisLengthSquared, 0.0F, 1.0F);
        const Math::Vector2UVE closestPoint{center.x + screenAxis.x * along, center.y + screenAxis.y * along};
        const Math::Vector2UVE distanceVector{pointerPosition.x - closestPoint.x, pointerPosition.y - closestPoint.y};
        const float distanceSquared = LengthSquared2UVE(distanceVector);
        if (distanceSquared > (kGizmoHandleRadiusPixelsUVE * kGizmoHandleRadiusPixelsUVE) ||
            distanceSquared >= bestDistanceSquared) {
            continue;
        }

        const float axisLength = std::sqrt(axisLengthSquared);
        candidate.axis = axis;
        candidate.entity = m_selectedEntity;
        candidate.initialLocalTransform =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(m_selectedEntity);
        candidate.initialPointer = pointerPosition;
        candidate.screenAxisDirection = Scale2UVE(screenAxis, 1.0F / axisLength);
        candidate.pixelsPerWorldUnit = axisLength / kGizmoAxisLengthUVE;
        bestDistanceSquared = distanceSquared;
    }

    if (candidate.axis == EditorTranslateAxisUVE::None ||
        candidate.pixelsPerWorldUnit <= kVectorEpsilonUVE ||
        !IsFiniteUVE(candidate.pixelsPerWorldUnit)) {
        return false;
    }

    m_gizmoDrag = candidate;
    return true;
}

void EditorUVE::UpdateGizmoDragUVE(const Math::Vector2UVE pointerPosition) {
    if (m_gizmoDrag.axis == EditorTranslateAxisUVE::None || !IsDocumentEntityUVE(m_gizmoDrag.entity) ||
        m_gizmoDrag.entity != m_selectedEntity || !IsFiniteUVE(pointerPosition.x) ||
        !IsFiniteUVE(pointerPosition.y)) {
        CancelGizmoDragUVE();
        return;
    }

    const Math::Vector2UVE pointerDelta{
        pointerPosition.x - m_gizmoDrag.initialPointer.x,
        pointerPosition.y - m_gizmoDrag.initialPointer.y,
    };
    const float pixelDistance = Dot2UVE(pointerDelta, m_gizmoDrag.screenAxisDirection);
    const float worldDistance = pixelDistance / m_gizmoDrag.pixelsPerWorldUnit;
    if (!IsFiniteUVE(worldDistance)) {
        return;
    }

    Math::Vector3UVE localDelta{};
    if (!ComputeLocalDeltaForWorldDeltaUVE(
            m_gizmoDrag.entity, GetAxisVectorUVE(m_gizmoDrag.axis) * worldDistance, localDelta)) {
        return;
    }

    Scene::TransformComponentUVE updated = m_gizmoDrag.initialLocalTransform;
    updated.localPosition += localDelta;
    static_cast<void>(SetSelectedLocalTransformUVE(updated));
}

void EditorUVE::CancelGizmoDragUVE() noexcept {
    m_gizmoDrag = GizmoDragUVE{};
}

void EditorUVE::DrawTranslateGizmoUVE(const EditorViewportRectUVE& viewportRect) {
    if (!IsDocumentEntityUVE(m_selectedEntity) || !IsViewportRectValidUVE(viewportRect)) {
        return;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::WorldTransformComponentUVE>(m_selectedEntity)) {
        return;
    }

    const Scene::WorldTransformComponentUVE& selectedWorld =
        entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(m_selectedEntity);
    if (selectedWorld.dirty || !IsFiniteVectorUVE(selectedWorld.worldPosition)) {
        return;
    }

    Math::Vector2UVE center{};
    if (!ProjectWorldPointUVE(viewportRect, selectedWorld.worldPosition, center)) {
        return;
    }

    ImDrawList* const drawList = ImGui::GetForegroundDrawList();
    const ImVec2 centerPoint{center.x, center.y};
    drawList->AddCircle(centerPoint, 7.0F, IM_COL32(235, 235, 235, 220), 16, 1.5F);

    constexpr std::array<EditorTranslateAxisUVE, 3> axes{
        EditorTranslateAxisUVE::X,
        EditorTranslateAxisUVE::Y,
        EditorTranslateAxisUVE::Z,
    };
    for (const EditorTranslateAxisUVE axis : axes) {
        Math::Vector2UVE endpoint{};
        if (!ProjectWorldPointUVE(
                viewportRect, selectedWorld.worldPosition + GetAxisVectorUVE(axis) * kGizmoAxisLengthUVE, endpoint)) {
            continue;
        }
        const bool active = m_gizmoDrag.axis == axis;
        const ImU32 color = GizmoAxisColorUVE(axis, active);
        const ImVec2 endpointPoint{endpoint.x, endpoint.y};
        drawList->AddLine(centerPoint, endpointPoint, color, active ? 4.0F : 2.5F);
        drawList->AddCircleFilled(endpointPoint, active ? 6.5F : 5.0F, color, 12);
    }
}

void EditorUVE::DestroyDocumentSubtreeUVE(const Scene::EntityUVE root) {
    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    const std::vector<Scene::EntityUVE> children =
        m_services->GetSceneGraphUVE().GetChildrenUVE(entityManager, root);
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

void EditorUVE::DrawMenuBarUVE() {
    if (!ImGui::BeginMainMenuBar()) {
        return;
    }

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Save Scene")) {
            static_cast<void>(SaveSceneUVE());
        }
        if (ImGui::MenuItem("Load Scene")) {
            static_cast<void>(LoadSceneUVE());
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Scene")) {
        if (ImGui::MenuItem("Create Empty")) {
            static_cast<void>(CreateDocumentEntityUVE(EditorEntityKindUVE::Empty));
        }
        if (ImGui::MenuItem("Create Camera")) {
            static_cast<void>(CreateDocumentEntityUVE(EditorEntityKindUVE::Camera));
        }
        if (ImGui::MenuItem("Create Directional Light")) {
            static_cast<void>(CreateDocumentEntityUVE(EditorEntityKindUVE::DirectionalLight));
        }
        if (ImGui::MenuItem("Create Collision Box")) {
            static_cast<void>(CreateDocumentEntityUVE(EditorEntityKindUVE::CollisionBox));
        }
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void EditorUVE::DrawHierarchyPanelUVE() {
    const ImGuiViewport* const mainViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2{mainViewport->WorkPos.x, mainViewport->WorkPos.y},
                            ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(
        ImVec2{250.0F, std::max(kMinimumViewportHeightUVE, mainViewport->WorkSize.y - kAssetsPanelHeightUVE)},
        ImGuiCond_FirstUseEver);
    ImGui::Begin("Scene");
    for (const Scene::EntityUVE root : GetDocumentRootsUVE()) {
        DrawHierarchyNodeUVE(root);
    }
    ImGui::End();
}

void EditorUVE::DrawHierarchyNodeUVE(const Scene::EntityUVE entity) {
    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    const std::vector<Scene::EntityUVE> children =
        m_services->GetSceneGraphUVE().GetChildrenUVE(entityManager, entity);
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
    const ImGuiViewport* const mainViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(
        ImVec2{mainViewport->WorkPos.x + mainViewport->WorkSize.x - 330.0F, mainViewport->WorkPos.y},
        ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(
        ImVec2{330.0F, std::max(kMinimumViewportHeightUVE, mainViewport->WorkSize.y - kAssetsPanelHeightUVE)},
        ImGuiCond_FirstUseEver);
    ImGui::Begin("Properties");
    if (!IsDocumentEntityUVE(m_selectedEntity)) {
        ImGui::TextUnformatted("Select an entity in Scene or Viewport.");
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

    Scene::TransformComponentUVE edited =
        entityManager.GetComponentUVE<Scene::TransformComponentUVE>(m_selectedEntity);
    float position[3]{edited.localPosition.x, edited.localPosition.y, edited.localPosition.z};
    float rotation[4]{edited.localRotation.x, edited.localRotation.y, edited.localRotation.z, edited.localRotation.w};
    float scale[3]{edited.localScale.x, edited.localScale.y, edited.localScale.z};

    const bool positionChanged = ImGui::InputFloat3("Local Position", position);
    const bool rotationChanged = ImGui::InputFloat4("Local Rotation (xyzw)", rotation);
    const bool scaleChanged = ImGui::InputFloat3("Local Scale", scale);
    if (positionChanged || rotationChanged || scaleChanged) {
        edited.localPosition = Math::Vector3UVE{position[0], position[1], position[2]};
        edited.localRotation = Math::QuaternionUVE{rotation[0], rotation[1], rotation[2], rotation[3]};
        edited.localScale = Math::Vector3UVE{scale[0], scale[1], scale[2]};
        static_cast<void>(SetSelectedLocalTransformUVE(edited));
    }
    ImGui::End();
}

void EditorUVE::DrawAssetsPanelUVE() {
    const ImGuiViewport* const mainViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(
        ImVec2{mainViewport->WorkPos.x, mainViewport->WorkPos.y + mainViewport->WorkSize.y - kAssetsPanelHeightUVE},
        ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2{mainViewport->WorkSize.x, kAssetsPanelHeightUVE}, ImGuiCond_FirstUseEver);
    ImGui::Begin("Assets");

    std::array<char, 256> filterBuffer{};
    const std::size_t copiedCharacters = std::min(m_assetFilter.size(), filterBuffer.size() - 1U);
    m_assetFilter.copy(filterBuffer.data(), copiedCharacters);
    if (ImGui::InputTextWithHint("Filter", "Filter registered asset paths", filterBuffer.data(), filterBuffer.size())) {
        m_assetFilter = filterBuffer.data();
    }

    const std::vector<Asset::AssetRecordUVE> records = m_services->GetAssetDatabaseUVE().GetRegisteredAssetsUVE();
    if (m_selectedAsset.has_value()) {
        const auto selectedIt = std::find_if(
            records.begin(), records.end(), [this](const Asset::AssetRecordUVE& record) {
                return record.guid == m_selectedAsset->guid;
            });
        if (selectedIt == records.end()) {
            m_selectedAsset.reset();
        } else {
            m_selectedAsset = *selectedIt;
        }
    }

    ImGui::SameLine();
    ImGui::Text("%zu registered", records.size());
    ImGui::BeginChild("##asset-list", ImVec2{0.0F, 64.0F}, true);
    bool displayedAsset = false;
    for (const Asset::AssetRecordUVE& record : records) {
        const std::string pathString = record.path.generic_string();
        if (!ContainsCaseInsensitiveUVE(pathString, m_assetFilter)) {
            continue;
        }

        displayedAsset = true;
        const bool selected = m_selectedAsset.has_value() && m_selectedAsset->guid == record.guid;
        const std::string selectableLabel = pathString + "##asset-" + std::to_string(record.guid.value);
        if (ImGui::Selectable(selectableLabel.c_str(), selected)) {
            m_selectedAsset = record;
        }
    }
    if (!displayedAsset) {
        ImGui::TextUnformatted(records.empty() ? "No registered assets." : "No registered assets match the filter.");
    }
    ImGui::EndChild();

    if (m_selectedAsset.has_value()) {
        const std::string selectedPath = m_selectedAsset->path.generic_string();
        ImGui::TextWrapped("Selected: %s", selectedPath.c_str());
        ImGui::Text("GUID: %016llX", static_cast<unsigned long long>(m_selectedAsset->guid.value));
    } else {
        ImGui::TextUnformatted("Select a registered asset to inspect its registry record.");
    }
    ImGui::End();
}

void EditorUVE::DrawViewportPanelUVE() {
    // Renderer3DUVE still presents to the engine window's default framebuffer. This editor window
    // therefore owns only a transparent interactive input rectangle; it does not duplicate render
    // target, shader, or presentation ownership.
    const ImGuiViewport* const mainViewport = ImGui::GetMainViewport();
    const float leftInset = 250.0F;
    const float rightInset = 330.0F;
    const float bottomInset = kAssetsPanelHeightUVE;
    const ImVec2 desiredPosition{mainViewport->WorkPos.x + leftInset, mainViewport->WorkPos.y};
    const ImVec2 desiredSize{
        std::max(kMinimumViewportWidthUVE, mainViewport->WorkSize.x - leftInset - rightInset),
        std::max(kMinimumViewportHeightUVE, mainViewport->WorkSize.y - bottomInset),
    };
    ImGui::SetNextWindowPos(desiredPosition, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(desiredSize, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.0F);
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                       ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar |
                                       ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::Begin("Viewport", nullptr, flags);

    const ImVec2 contentOrigin = ImGui::GetCursorScreenPos();
    const ImVec2 contentSize = ImGui::GetContentRegionAvail();
    const EditorViewportRectUVE viewportRect{
        Math::Vector2UVE{contentOrigin.x, contentOrigin.y},
        Math::Vector2UVE{contentSize.x, contentSize.y},
    };
    if (IsViewportRectValidUVE(viewportRect)) {
        ImGui::InvisibleButton("##viewport-input", contentSize, ImGuiButtonFlags_MouseButtonLeft);
        const bool viewportHovered = ImGui::IsItemHovered();
        const bool viewportClicked = viewportHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        const ImVec2 mousePosition = ImGui::GetMousePos();
        const Math::Vector2UVE pointerPosition{mousePosition.x, mousePosition.y};

        if (m_gizmoDrag.axis != EditorTranslateAxisUVE::None) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                UpdateGizmoDragUVE(pointerPosition);
            } else {
                CancelGizmoDragUVE();
            }
        } else if (viewportClicked) {
            if (!BeginGizmoDragUVE(viewportRect, pointerPosition)) {
                static_cast<void>(PickViewportUVE(viewportRect, pointerPosition));
            }
        }

        DrawTranslateGizmoUVE(viewportRect);
        ImDrawList* const drawList = ImGui::GetWindowDrawList();
        drawList->AddText(ImVec2{contentOrigin.x + 10.0F, contentOrigin.y + 10.0F}, IM_COL32(230, 230, 230, 220),
                          "Viewport  |  click collider to select  |  drag RGB axis to move");
    } else {
        ImGui::TextUnformatted("Viewport is too small for picking.");
    }
    ImGui::End();
}

} // namespace UVE::Editor
