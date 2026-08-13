// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/editor/editor_uve.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <limits>
#include <numbers>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>

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
#include "uve/scene/components/name_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"

namespace UVE::Editor {

namespace {

constexpr float kVectorEpsilonUVE = 0.00001F;
constexpr float kMinimumLocalScaleUVE = 0.001F;
constexpr float kGizmoAxisLengthUVE = 1.25F;
constexpr float kGizmoHandleRadiusPixelsUVE = 12.0F;
constexpr float kTrackballRadiusPixelsUVE = 42.0F;
constexpr float kTrackballAntipodalDotThresholdUVE = -0.999F;
constexpr float kMinimumViewportWidthUVE = 64.0F;
constexpr float kMinimumViewportHeightUVE = 64.0F;
constexpr float kAssetsPanelHeightUVE = 200.0F;
constexpr float kBottomDockTabHeightUVE = 30.0F;
constexpr std::size_t kMaximumEntityNameBytesUVE = 96U;
constexpr float kMinimumViewportDistanceUVE = 0.5F;
constexpr float kMaximumViewportDistanceUVE = 500.0F;
constexpr float kMaximumViewportPitchRadiansUVE = 1.4835299F; // 85 degrees.
constexpr float kViewportOrbitRadiansPerPixelUVE = 0.008F;
constexpr float kViewportZoomExponentPerWheelUnitUVE = 0.16F;
constexpr const char* kHierarchyEntityPayloadUVE = "UVE_SCENE_HIERARCHY_ENTITY";

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

[[nodiscard]] bool IsWhitespaceOnlyUVE(const std::string_view value) noexcept {
    return std::all_of(value.begin(), value.end(), [](const char character) noexcept {
        return std::isspace(static_cast<unsigned char>(character)) != 0;
    });
}

[[nodiscard]] bool AreTransformsEqualUVE(const Scene::TransformComponentUVE& lhs,
                                         const Scene::TransformComponentUVE& rhs) noexcept {
    return lhs.localPosition.x == rhs.localPosition.x && lhs.localPosition.y == rhs.localPosition.y &&
           lhs.localPosition.z == rhs.localPosition.z && lhs.localRotation.x == rhs.localRotation.x &&
           lhs.localRotation.y == rhs.localRotation.y && lhs.localRotation.z == rhs.localRotation.z &&
           lhs.localRotation.w == rhs.localRotation.w && lhs.localScale.x == rhs.localScale.x &&
           lhs.localScale.y == rhs.localScale.y && lhs.localScale.z == rhs.localScale.z;
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

[[nodiscard]] Math::QuaternionUVE MakeViewportOrientationUVE(const float yawRadians,
                                                              const float pitchRadians) noexcept {
    const float halfYaw = yawRadians * 0.5F;
    const float halfPitch = pitchRadians * 0.5F;
    const Math::QuaternionUVE yaw{0.0F, std::sin(halfYaw), 0.0F, std::cos(halfYaw)};
    const Math::QuaternionUVE pitch{std::sin(halfPitch), 0.0F, 0.0F, std::cos(halfPitch)};
    return Math::MultiplyUVE(yaw, pitch);
}

[[nodiscard]] Math::Vector3UVE MakeViewportForwardUVE(const float yawRadians,
                                                        const float pitchRadians) noexcept {
    const float cosinePitch = std::cos(pitchRadians);
    return Math::Vector3UVE{-std::sin(yawRadians) * cosinePitch,
                            std::sin(pitchRadians),
                            -std::cos(yawRadians) * cosinePitch};
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

[[nodiscard]] bool GetRingBasisUVE(const EditorTranslateAxisUVE axis, Math::Vector3UVE& outFirst,
                                   Math::Vector3UVE& outSecond) noexcept {
    switch (axis) {
        case EditorTranslateAxisUVE::X:
            outFirst = Math::Vector3UVE{0.0F, 1.0F, 0.0F};
            outSecond = Math::Vector3UVE{0.0F, 0.0F, 1.0F};
            return true;
        case EditorTranslateAxisUVE::Y:
            outFirst = Math::Vector3UVE{1.0F, 0.0F, 0.0F};
            outSecond = Math::Vector3UVE{0.0F, 0.0F, 1.0F};
            return true;
        case EditorTranslateAxisUVE::Z:
            outFirst = Math::Vector3UVE{1.0F, 0.0F, 0.0F};
            outSecond = Math::Vector3UVE{0.0F, 1.0F, 0.0F};
            return true;
        case EditorTranslateAxisUVE::None:
            return false;
    }
    return false;
}

[[nodiscard]] Math::Vector3UVE MakeRingPointUVE(const Math::Vector3UVE& center,
                                                 const Math::Vector3UVE& first,
                                                 const Math::Vector3UVE& second,
                                                 const float parameterRadians) noexcept {
    return center + (first * (std::cos(parameterRadians) * kGizmoAxisLengthUVE)) +
           (second * (std::sin(parameterRadians) * kGizmoAxisLengthUVE));
}

} // namespace

EditorUVE::EditorUVE(Core::EngineServicesUVE& services, std::filesystem::path activeScenePath,
                     const std::size_t historyCapacity, Core::ISimulationControlUVE* const simulationControl)
    : m_services(&services),
      m_simulationControl(simulationControl),
      m_activeScenePath(std::move(activeScenePath)),
      m_historyCapacity(std::max<std::size_t>(std::size_t{1U}, historyCapacity)) {}

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

    PruneSelectionUVE();
    if (m_gizmoDrag.axis != EditorTranslateAxisUVE::None &&
        (!HasSingleDocumentSelectionUVE() || !IsDocumentEntityUVE(m_gizmoDrag.entity) ||
         m_gizmoDrag.entity != m_selectedEntity)) {
        CancelGizmoDragUVE();
    }
    if (m_hierarchyRenameEntity != Scene::kInvalidEntityUVE &&
        (!IsAuthoringCommandAllowedUVE() || !HasSingleDocumentSelectionUVE() ||
         !IsDocumentEntityUVE(m_hierarchyRenameEntity) || m_hierarchyRenameEntity != m_selectedEntity ||
         m_gizmoDrag.axis != EditorTranslateAxisUVE::None ||
         m_viewportNavigationMode != EditorViewportNavigationModeUVE::None)) {
        CancelHierarchyRenameUVE();
    }
}

bool EditorUVE::EnterPlayModeUVE() {
    if (m_state != EditorStateUVE::Running || m_playModeState != EditorPlayModeStateUVE::Edit ||
        m_simulationControl == nullptr || !IsAuthoringCommandAllowedUVE() ||
        m_gizmoDrag.axis != EditorTranslateAxisUVE::None ||
        m_viewportNavigationMode != EditorViewportNavigationModeUVE::None) {
        return false;
    }

    const std::vector<Scene::EntityUVE> roots = GetDocumentRootsUVE();
    PlayModeSessionUVE session{};
    session.capturedEmptyDocument = roots.empty();
    if (!session.capturedEmptyDocument) {
        const std::optional<Scene::SceneSnapshotUVE> snapshot =
            m_services->GetSceneSerializerUVE().CaptureUVE(
                m_services->GetEntityManagerUVE(), roots, Asset::AssetKindUVE::Scene);
        if (!snapshot.has_value()) {
            return false;
        }
        session.documentSnapshot = *snapshot;
    }
    session.dirtyBefore = m_sceneDirty;
    session.selectionBefore = CaptureSelectionPathsUVE(roots);

    if (!m_simulationControl->SetTransientSimulationSessionActiveUVE(true)) {
        return false;
    }
    if (!m_simulationControl->SetSimulationExecutionModeUVE(Core::SimulationExecutionModeUVE::Running)) {
        static_cast<void>(m_simulationControl->SetTransientSimulationSessionActiveUVE(false));
        return false;
    }

    m_playModeSession = std::move(session);
    m_playModeState = EditorPlayModeStateUVE::Playing;
    return true;
}

bool EditorUVE::PausePlayModeUVE() {
    if (m_state != EditorStateUVE::Running || m_playModeState != EditorPlayModeStateUVE::Playing ||
        m_simulationControl == nullptr ||
        !m_simulationControl->SetSimulationExecutionModeUVE(Core::SimulationExecutionModeUVE::Paused)) {
        return false;
    }
    m_playModeState = EditorPlayModeStateUVE::Paused;
    return true;
}

bool EditorUVE::ResumePlayModeUVE() {
    if (m_state != EditorStateUVE::Running || m_playModeState != EditorPlayModeStateUVE::Paused ||
        m_simulationControl == nullptr ||
        !m_simulationControl->SetSimulationExecutionModeUVE(Core::SimulationExecutionModeUVE::Running)) {
        return false;
    }
    m_playModeState = EditorPlayModeStateUVE::Playing;
    return true;
}

bool EditorUVE::StepPlayModeUVE() {
    return m_state == EditorStateUVE::Running && m_playModeState == EditorPlayModeStateUVE::Paused &&
           m_simulationControl != nullptr && m_simulationControl->RequestSingleSimulationStepUVE();
}

bool EditorUVE::StopPlayModeUVE() {
    if (m_state != EditorStateUVE::Running || m_playModeState == EditorPlayModeStateUVE::Edit ||
        !m_playModeSession.has_value() || m_simulationControl == nullptr ||
        !m_simulationControl->SetSimulationExecutionModeUVE(Core::SimulationExecutionModeUVE::Paused)) {
        return false;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    const std::vector<Scene::EntityUVE> transientRoots = GetDocumentRootsUVE();
    std::optional<Scene::SceneSnapshotUVE> transientSnapshot;
    if (!transientRoots.empty()) {
        transientSnapshot = m_services->GetSceneSerializerUVE().CaptureUVE(
            entityManager, transientRoots, Asset::AssetKindUVE::Scene);
        if (!transientSnapshot.has_value()) {
            return false;
        }
    }

    const PlayModeSessionUVE& session = *m_playModeSession;
    ClearDocumentSceneUVE();
    std::vector<Scene::EntityUVE> restoredRoots;
    if (!session.capturedEmptyDocument) {
        restoredRoots = m_services->GetSceneSerializerUVE().RestoreUVE(entityManager, session.documentSnapshot);
        if (restoredRoots.empty()) {
            ClearDocumentSceneUVE();
            if (transientSnapshot.has_value()) {
                static_cast<void>(m_services->GetSceneSerializerUVE().RestoreUVE(entityManager, *transientSnapshot));
            }
            return false;
        }
    }

    RestoreSelectionUVE(ResolveSelectionPathsUVE(session.selectionBefore, restoredRoots));
    m_sceneDirty = session.dirtyBefore;
    if (!m_simulationControl->SetSimulationExecutionModeUVE(Core::SimulationExecutionModeUVE::Running) ||
        !m_simulationControl->SetTransientSimulationSessionActiveUVE(false)) {
        return false;
    }

    m_playModeSession.reset();
    m_playModeState = EditorPlayModeStateUVE::Edit;
    return true;
}

EditorPlayModeStateUVE EditorUVE::GetPlayModeStateUVE() const noexcept {
    return m_playModeState;
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
    DrawBottomDockContentUVE();
    DrawBottomDockUVE();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool EditorUVE::SaveSceneUVE() {
    if (!IsAuthoringCommandAllowedUVE() || m_activeScenePath.empty()) {
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
    if (!IsAuthoringCommandAllowedUVE() || m_activeScenePath.empty() ||
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

    ClearHistoryUVE();
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
    ClearHistoryUVE();
    m_sceneDirty = false;
    InvalidateHierarchyFilterCacheUVE();
    return true;
}

void EditorUVE::SelectEntityUVE(const Scene::EntityUVE entity) noexcept {
    if (!IsAuthoringCommandAllowedUVE()) {
        return;
    }
    if (!IsDocumentEntityUVE(entity)) {
        ClearSelectionUVE();
        return;
    }
    RestoreSelectionUVE(EditorSelectionSnapshotUVE{{entity}, entity});
}

void EditorUVE::ToggleEntitySelectionUVE(const Scene::EntityUVE entity) noexcept {
    if (!IsAuthoringCommandAllowedUVE() || !IsDocumentEntityUVE(entity)) {
        return;
    }

    const auto selectedIt = std::find(m_selectedEntities.begin(), m_selectedEntities.end(), entity);
    if (selectedIt == m_selectedEntities.end()) {
        m_selectedEntities.push_back(entity);
        m_selectedEntity = entity;
        CancelHierarchyRenameUVE();
        CancelGizmoDragUVE();
        return;
    }

    const bool removedActive = entity == m_selectedEntity;
    m_selectedEntities.erase(selectedIt);
    if (m_selectedEntities.empty()) {
        m_selectedEntity = Scene::kInvalidEntityUVE;
    } else if (removedActive) {
        m_selectedEntity = m_selectedEntities.back();
    }
    CancelHierarchyRenameUVE();
    CancelGizmoDragUVE();
}

void EditorUVE::ClearSelectionUVE() noexcept {
    m_selectedEntities.clear();
    m_selectedEntity = Scene::kInvalidEntityUVE;
    CancelHierarchyRenameUVE();
    CancelGizmoDragUVE();
}

const std::vector<Scene::EntityUVE>& EditorUVE::GetSelectedEntitiesUVE() const noexcept {
    return m_selectedEntities;
}

bool EditorUVE::HasSingleDocumentSelectionUVE() const noexcept {
    return m_selectedEntities.size() == 1U && m_selectedEntities.front() == m_selectedEntity &&
           IsDocumentEntityUVE(m_selectedEntity);
}

bool EditorUVE::SetSelectedLocalTransformUVE(const Scene::TransformComponentUVE& transform) {
    if (!IsAuthoringCommandAllowedUVE() || !HasSingleDocumentSelectionUVE() ||
        !IsTransformFiniteUVE(transform)) {
        return false;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::TransformComponentUVE>(m_selectedEntity)) {
        return false;
    }

    const EditorSelectionSnapshotUVE selectionBefore = CaptureSelectionSnapshotUVE();
    const Scene::TransformComponentUVE before =
        entityManager.GetComponentUVE<Scene::TransformComponentUVE>(m_selectedEntity);
    if (AreTransformsEqualUVE(before, transform)) {
        return false;
    }

    const bool dirtyBefore = m_sceneDirty;
    if (!ApplyLocalTransformUVE(m_selectedEntity, transform)) {
        return false;
    }

    m_sceneDirty = true;
    RecordHistoryUVE(TransformHistoryEntryUVE{
        m_selectedEntity, before, transform, selectionBefore, CaptureSelectionSnapshotUVE(), dirtyBefore, true});
    return true;
}

bool EditorUVE::SetSelectedEntityNameUVE(std::string name) {
    if (!IsAuthoringCommandAllowedUVE() || !HasSingleDocumentSelectionUVE() ||
        !IsEntityNameValidUVE(name)) {
        return false;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    std::optional<std::string> beforeName;
    if (entityManager.HasComponentUVE<Scene::NameComponentUVE>(m_selectedEntity)) {
        beforeName = entityManager.GetComponentUVE<Scene::NameComponentUVE>(m_selectedEntity).name;
        if (*beforeName == name) {
            return false;
        }
    }

    const EditorSelectionSnapshotUVE selectionBefore = CaptureSelectionSnapshotUVE();
    const bool dirtyBefore = m_sceneDirty;
    const std::optional<std::string> afterName{std::move(name)};
    if (!ApplyEntityNameStateUVE(m_selectedEntity, afterName)) {
        return false;
    }

    m_sceneDirty = true;
    RecordHistoryUVE(NameHistoryEntryUVE{
        m_selectedEntity, beforeName, afterName, selectionBefore, CaptureSelectionSnapshotUVE(), dirtyBefore, true});
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
                                 const Math::Vector2UVE pointerPosition,
                                 const bool toggleSelection) {
    if (!IsAuthoringCommandAllowedUVE()) {
        return false;
    }
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
        if (toggleSelection) {
            ToggleEntitySelectionUVE(hit->entity);
        } else {
            SelectEntityUVE(hit->entity);
        }
        return true;
    }

    if (!toggleSelection) {
        ClearSelectionUVE();
    }
    return false;
}

bool EditorUVE::TranslateSelectedAlongAxisUVE(const EditorTranslateAxisUVE axis, const float worldDistance) {
    if (!IsAuthoringCommandAllowedUVE() || !HasSingleDocumentSelectionUVE() ||
        !IsFiniteUVE(worldDistance) || axis == EditorTranslateAxisUVE::None) {
        return false;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::TransformComponentUVE>(m_selectedEntity)) {
        return false;
    }

    const float snappedDistance = m_transformSnappingSettings.enabled
                                      ? SnapScalarUVE(worldDistance, m_transformSnappingSettings.translateStep)
                                      : worldDistance;
    Math::Vector3UVE localDelta{};
    const Math::Vector3UVE worldDelta = GetAxisVectorUVE(axis) * snappedDistance;
    if (!ComputeLocalDeltaForWorldDeltaUVE(m_selectedEntity, worldDelta, localDelta)) {
        return false;
    }

    Scene::TransformComponentUVE updated =
        entityManager.GetComponentUVE<Scene::TransformComponentUVE>(m_selectedEntity);
    updated.localPosition += localDelta;
    return SetSelectedLocalTransformUVE(updated);
}

bool EditorUVE::RotateSelectedAroundWorldAxisUVE(const EditorTranslateAxisUVE axis, const float radians) {
    if (!IsAuthoringCommandAllowedUVE() || !HasSingleDocumentSelectionUVE() ||
        !IsFiniteUVE(radians) || axis == EditorTranslateAxisUVE::None ||
        m_gizmoDrag.axis != EditorTranslateAxisUVE::None ||
        m_viewportNavigationMode != EditorViewportNavigationModeUVE::None) {
        return false;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::TransformComponentUVE>(m_selectedEntity)) {
        return false;
    }

    const float rotateStepRadians =
        (m_transformSnappingSettings.rotateStepDegrees * std::numbers::pi_v<float>) / 180.0F;
    const float snappedRadians = m_transformSnappingSettings.enabled
                                     ? SnapScalarUVE(radians, rotateStepRadians)
                                     : radians;
    Scene::TransformComponentUVE updated =
        entityManager.GetComponentUVE<Scene::TransformComponentUVE>(m_selectedEntity);
    Math::QuaternionUVE localRotation{};
    if (!ComputeLocalRotationForWorldAxisUVE(m_selectedEntity, updated.localRotation, GetAxisVectorUVE(axis),
                                             snappedRadians, localRotation)) {
        return false;
    }
    updated.localRotation = localRotation;
    return SetSelectedLocalTransformUVE(updated);
}

bool EditorUVE::ScaleSelectedAlongAxisUVE(const EditorTranslateAxisUVE axis,
                                           const float localScaleDelta) {
    if (!IsAuthoringCommandAllowedUVE() || !HasSingleDocumentSelectionUVE() ||
        !IsFiniteUVE(localScaleDelta) || axis == EditorTranslateAxisUVE::None ||
        m_gizmoDrag.axis != EditorTranslateAxisUVE::None ||
        m_viewportNavigationMode != EditorViewportNavigationModeUVE::None) {
        return false;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::TransformComponentUVE>(m_selectedEntity)) {
        return false;
    }

    Scene::TransformComponentUVE updated =
        entityManager.GetComponentUVE<Scene::TransformComponentUVE>(m_selectedEntity);
    float* component = nullptr;
    switch (axis) {
        case EditorTranslateAxisUVE::X:
            component = &updated.localScale.x;
            break;
        case EditorTranslateAxisUVE::Y:
            component = &updated.localScale.y;
            break;
        case EditorTranslateAxisUVE::Z:
            component = &updated.localScale.z;
            break;
        case EditorTranslateAxisUVE::None:
            return false;
    }
    const float snappedScaleDelta = m_transformSnappingSettings.enabled
                                        ? SnapScalarUVE(localScaleDelta, m_transformSnappingSettings.scaleStep)
                                        : localScaleDelta;
    *component += snappedScaleDelta;
    if (!IsFiniteUVE(*component) || *component < kMinimumLocalScaleUVE) {
        return false;
    }
    return SetSelectedLocalTransformUVE(updated);
}

bool EditorUVE::ScaleSelectedUniformlyUVE(const float localScaleOffset) {
    if (!IsAuthoringCommandAllowedUVE() || !HasSingleDocumentSelectionUVE() ||
        !IsFiniteUVE(localScaleOffset) || m_gizmoDrag.axis != EditorTranslateAxisUVE::None ||
        m_viewportNavigationMode != EditorViewportNavigationModeUVE::None) {
        return false;
    }
    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::TransformComponentUVE>(m_selectedEntity)) {
        return false;
    }
    const float snappedOffset = m_transformSnappingSettings.enabled
                                    ? SnapScalarUVE(localScaleOffset, m_transformSnappingSettings.scaleStep)
                                    : localScaleOffset;
    Scene::TransformComponentUVE updated =
        entityManager.GetComponentUVE<Scene::TransformComponentUVE>(m_selectedEntity);
    updated.localScale.x += snappedOffset;
    updated.localScale.y += snappedOffset;
    updated.localScale.z += snappedOffset;
    if (!IsFiniteUVE(snappedOffset) || !IsFiniteUVE(updated.localScale.x) ||
        !IsFiniteUVE(updated.localScale.y) || !IsFiniteUVE(updated.localScale.z) ||
        updated.localScale.x < kMinimumLocalScaleUVE || updated.localScale.y < kMinimumLocalScaleUVE ||
        updated.localScale.z < kMinimumLocalScaleUVE) {
        return false;
    }
    return SetSelectedLocalTransformUVE(updated);
}

void EditorUVE::SetGizmoModeUVE(const EditorGizmoModeUVE mode) noexcept {
    if (!IsAuthoringCommandAllowedUVE() || m_gizmoDrag.axis != EditorTranslateAxisUVE::None ||
        m_viewportNavigationMode != EditorViewportNavigationModeUVE::None) {
        return;
    }
    m_gizmoMode = mode;
}

EditorGizmoModeUVE EditorUVE::GetGizmoModeUVE() const noexcept {
    return m_gizmoMode;
}

bool EditorUVE::SetGizmoCoordinateSpaceUVE(const EditorGizmoCoordinateSpaceUVE coordinateSpace) {
    if (!IsAuthoringCommandAllowedUVE() || m_gizmoDrag.axis != EditorTranslateAxisUVE::None ||
        m_viewportNavigationMode != EditorViewportNavigationModeUVE::None) {
        return false;
    }
    m_gizmoCoordinateSpace = coordinateSpace;
    return true;
}

EditorGizmoCoordinateSpaceUVE EditorUVE::GetGizmoCoordinateSpaceUVE() const noexcept {
    return m_gizmoCoordinateSpace;
}

bool EditorUVE::IsReparentModeChangeAllowedUVE() const noexcept {
    return IsAuthoringCommandAllowedUVE() && m_gizmoDrag.axis == EditorTranslateAxisUVE::None &&
           m_viewportNavigationMode == EditorViewportNavigationModeUVE::None;
}

bool EditorUVE::SetReparentTransformModeUVE(const EditorReparentTransformModeUVE mode) {
    if (!IsReparentModeChangeAllowedUVE()) {
        return false;
    }
    m_reparentTransformMode = mode;
    return true;
}

EditorReparentTransformModeUVE EditorUVE::GetReparentTransformModeUVE() const noexcept {
    return m_reparentTransformMode;
}

bool EditorUVE::SetTransformSnappingSettingsUVE(const EditorTransformSnappingSettingsUVE& settings) {
    if (!IsAuthoringCommandAllowedUVE() || m_gizmoDrag.axis != EditorTranslateAxisUVE::None ||
        m_viewportNavigationMode != EditorViewportNavigationModeUVE::None ||
        !AreTransformSnappingSettingsValidUVE(settings)) {
        return false;
    }
    m_transformSnappingSettings = settings;
    return true;
}

const EditorTransformSnappingSettingsUVE& EditorUVE::GetTransformSnappingSettingsUVE() const noexcept {
    return m_transformSnappingSettings;
}

std::optional<EditorSelectionBoundsUVE> EditorUVE::TryGetSelectedBoundsUVE() const {
    return TryGetEntityBoundsUVE(m_selectedEntity);
}

std::optional<EditorSelectionBoundsUVE> EditorUVE::TryGetEntityBoundsUVE(const Scene::EntityUVE entity) const {
    if (m_state != EditorStateUVE::Running || !IsDocumentEntityUVE(entity)) {
        return std::nullopt;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::TransformComponentUVE>(entity) ||
        !entityManager.HasComponentUVE<Scene::WorldTransformComponentUVE>(entity) ||
        !entityManager.HasComponentUVE<Scene::ColliderComponentUVE>(entity)) {
        return std::nullopt;
    }

    const Scene::WorldTransformComponentUVE& worldTransform =
        entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(entity);
    const Scene::ColliderComponentUVE& collider =
        entityManager.GetComponentUVE<Scene::ColliderComponentUVE>(entity);
    if (worldTransform.dirty || !IsFiniteVectorUVE(worldTransform.worldPosition) ||
        !IsFiniteVectorUVE(worldTransform.worldScale) || !IsFiniteVectorUVE(collider.halfExtents) ||
        collider.halfExtents.x <= kVectorEpsilonUVE || collider.halfExtents.y <= kVectorEpsilonUVE ||
        collider.halfExtents.z <= kVectorEpsilonUVE ||
        std::abs(worldTransform.worldScale.x) <= kVectorEpsilonUVE ||
        std::abs(worldTransform.worldScale.y) <= kVectorEpsilonUVE ||
        std::abs(worldTransform.worldScale.z) <= kVectorEpsilonUVE) {
        return std::nullopt;
    }

    Math::QuaternionUVE normalizedRotation{};
    if (!Math::TryNormalizeUVE(worldTransform.worldRotation, normalizedRotation)) {
        return std::nullopt;
    }

    constexpr std::array<Math::Vector3UVE, 8> kCornerSignsUVE{
        Math::Vector3UVE{-1.0F, -1.0F, -1.0F},
        Math::Vector3UVE{1.0F, -1.0F, -1.0F},
        Math::Vector3UVE{1.0F, 1.0F, -1.0F},
        Math::Vector3UVE{-1.0F, 1.0F, -1.0F},
        Math::Vector3UVE{-1.0F, -1.0F, 1.0F},
        Math::Vector3UVE{1.0F, -1.0F, 1.0F},
        Math::Vector3UVE{1.0F, 1.0F, 1.0F},
        Math::Vector3UVE{-1.0F, 1.0F, 1.0F},
    };

    EditorSelectionBoundsUVE bounds{};
    bounds.worldCenter = worldTransform.worldPosition;
    for (std::size_t index = 0U; index < kCornerSignsUVE.size(); ++index) {
        const Math::Vector3UVE localCorner = kCornerSignsUVE[index] * collider.halfExtents;
        const Math::Vector3UVE scaledCorner = localCorner * worldTransform.worldScale;
        bounds.worldCorners[index] = worldTransform.worldPosition +
                                     Math::RotateVectorUVE(normalizedRotation, scaledCorner);
        if (!IsFiniteVectorUVE(bounds.worldCorners[index])) {
            return std::nullopt;
        }
    }
    return bounds;
}

bool EditorUVE::FocusSelectedEntityUVE() {
    if (m_state != EditorStateUVE::Running || !IsDocumentEntityUVE(m_selectedEntity)) {
        return false;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::WorldTransformComponentUVE>(m_selectedEntity)) {
        return false;
    }

    const Scene::WorldTransformComponentUVE& selectedWorld =
        entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(m_selectedEntity);
    if (selectedWorld.dirty || !IsFiniteVectorUVE(selectedWorld.worldPosition)) {
        return false;
    }

    const Math::Vector3UVE previousFocus = m_viewportFocusPoint;
    const float previousDistance = m_viewportDistance;
    m_viewportFocusPoint = selectedWorld.worldPosition;
    m_viewportDistance = std::clamp(m_viewportDistance, kMinimumViewportDistanceUVE, kMaximumViewportDistanceUVE);
    if (ApplyViewportCameraUVE()) {
        return true;
    }

    m_viewportFocusPoint = previousFocus;
    m_viewportDistance = previousDistance;
    return false;
}

bool EditorUVE::OrbitViewportUVE(const float yawDeltaRadians, const float pitchDeltaRadians) {
    if (m_state != EditorStateUVE::Running || !IsFiniteUVE(yawDeltaRadians) || !IsFiniteUVE(pitchDeltaRadians)) {
        return false;
    }

    const float previousYaw = m_viewportYawRadians;
    const float previousPitch = m_viewportPitchRadians;
    m_viewportYawRadians += yawDeltaRadians;
    m_viewportPitchRadians = std::clamp(m_viewportPitchRadians + pitchDeltaRadians,
                                        -kMaximumViewportPitchRadiansUVE,
                                        kMaximumViewportPitchRadiansUVE);
    if (ApplyViewportCameraUVE()) {
        return true;
    }

    m_viewportYawRadians = previousYaw;
    m_viewportPitchRadians = previousPitch;
    return false;
}

bool EditorUVE::PanViewportUVE(const Math::Vector2UVE pixelDelta, const EditorViewportRectUVE& viewportRect) {
    if (m_state != EditorStateUVE::Running || !IsFiniteUVE(pixelDelta.x) || !IsFiniteUVE(pixelDelta.y) ||
        !IsViewportRectValidUVE(viewportRect) || !IsViewportNavigationFiniteUVE()) {
        return false;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.IsAliveUVE(m_viewportCamera) ||
        !entityManager.HasComponentUVE<Scene::CameraComponentUVE>(m_viewportCamera)) {
        return false;
    }

    const Scene::CameraComponentUVE& camera =
        entityManager.GetComponentUVE<Scene::CameraComponentUVE>(m_viewportCamera);
    const float tanHalfFov = std::tan((camera.fieldOfViewDegrees * std::numbers::pi_v<float>) / 360.0F);
    if (!IsFiniteUVE(camera.fieldOfViewDegrees) || !IsFiniteUVE(tanHalfFov) || tanHalfFov <= kVectorEpsilonUVE) {
        return false;
    }

    const float worldUnitsPerPixel = (2.0F * m_viewportDistance * tanHalfFov) / viewportRect.size.y;
    const Math::QuaternionUVE orientation =
        MakeViewportOrientationUVE(m_viewportYawRadians, m_viewportPitchRadians);
    const Math::Vector3UVE right = Math::RotateVectorUVE(orientation, Math::Vector3UVE{1.0F, 0.0F, 0.0F});
    const Math::Vector3UVE up = Math::RotateVectorUVE(orientation, Math::Vector3UVE{0.0F, 1.0F, 0.0F});
    if (!IsFiniteUVE(worldUnitsPerPixel) || !IsFiniteVectorUVE(right) || !IsFiniteVectorUVE(up)) {
        return false;
    }

    const Math::Vector3UVE previousFocus = m_viewportFocusPoint;
    m_viewportFocusPoint += right * (-pixelDelta.x * worldUnitsPerPixel);
    m_viewportFocusPoint += up * (pixelDelta.y * worldUnitsPerPixel);
    if (ApplyViewportCameraUVE()) {
        return true;
    }

    m_viewportFocusPoint = previousFocus;
    return false;
}

bool EditorUVE::ZoomViewportUVE(const float wheelDelta) {
    if (m_state != EditorStateUVE::Running || !IsFiniteUVE(wheelDelta) || !IsViewportNavigationFiniteUVE()) {
        return false;
    }

    const float previousDistance = m_viewportDistance;
    const float zoomFactor = std::exp(-wheelDelta * kViewportZoomExponentPerWheelUnitUVE);
    if (!IsFiniteUVE(zoomFactor) || zoomFactor <= 0.0F) {
        return false;
    }

    m_viewportDistance = std::clamp(m_viewportDistance * zoomFactor,
                                    kMinimumViewportDistanceUVE,
                                    kMaximumViewportDistanceUVE);
    if (m_viewportDistance == previousDistance) {
        return false;
    }
    if (ApplyViewportCameraUVE()) {
        return true;
    }

    m_viewportDistance = previousDistance;
    return false;
}

Scene::EntityUVE EditorUVE::CreateDocumentEntityUVE(const EditorEntityKindUVE kind) {
    if (!IsAuthoringCommandAllowedUVE()) {
        return Scene::kInvalidEntityUVE;
    }

    const EditorSelectionSnapshotUVE selectionBefore = CaptureSelectionSnapshotUVE();
    const bool dirtyBefore = m_sceneDirty;
    const Scene::EntityUVE entity = CreateDocumentEntityInternalUVE(kind, std::nullopt);
    if (entity == Scene::kInvalidEntityUVE) {
        return Scene::kInvalidEntityUVE;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    const std::string createdName = entityManager.GetComponentUVE<Scene::NameComponentUVE>(entity).name;
    SelectEntityUVE(entity);
    m_sceneDirty = true;
    RecordHistoryUVE(CreationHistoryEntryUVE{
        kind, createdName, entity, selectionBefore, CaptureSelectionSnapshotUVE(), dirtyBefore, true});
    return entity;
}

Scene::EntityUVE EditorUVE::DuplicateSelectedEntityUVE() {
    if (!IsLifecycleCommandAllowedUVE() || !IsDocumentEntityUVE(m_selectedEntity)) {
        return Scene::kInvalidEntityUVE;
    }

    const Scene::EntityUVE source = m_selectedEntity;
    const std::optional<Scene::SceneSnapshotUVE> snapshot = CaptureSubtreeUVE(source);
    if (!snapshot.has_value()) {
        return Scene::kInvalidEntityUVE;
    }

    Scene::EntityUVE originalParent = Scene::kInvalidEntityUVE;
    if (!TryGetDocumentParentUVE(source, originalParent)) {
        return Scene::kInvalidEntityUVE;
    }

    const Scene::EntityUVE duplicate = RestoreSubtreeUnderParentUVE(*snapshot, originalParent);
    if (duplicate == Scene::kInvalidEntityUVE) {
        return Scene::kInvalidEntityUVE;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    std::optional<std::string> duplicateRootName;
    if (entityManager.HasComponentUVE<Scene::NameComponentUVE>(source)) {
        const std::string& sourceName = entityManager.GetComponentUVE<Scene::NameComponentUVE>(source).name;
        if (IsEntityNameValidUVE(sourceName)) {
            duplicateRootName = MakeUniqueDocumentEntityNameUVE(sourceName);
            if (!ApplyEntityNameStateUVE(duplicate, duplicateRootName)) {
                DestroyDocumentSubtreeUVE(duplicate);
                return Scene::kInvalidEntityUVE;
            }
        }
    }

    const EditorSelectionSnapshotUVE selectionBefore = CaptureSelectionSnapshotUVE();
    const bool dirtyBefore = m_sceneDirty;
    SelectEntityUVE(duplicate);
    m_sceneDirty = true;
    InvalidateHierarchyFilterCacheUVE();
    RecordHistoryUVE(DuplicationHistoryEntryUVE{
        std::move(*snapshot), originalParent, duplicate, std::move(duplicateRootName), selectionBefore,
        CaptureSelectionSnapshotUVE(), dirtyBefore, true});
    return duplicate;
}

bool EditorUVE::DeleteSelectedEntityUVE() {
    if (!IsLifecycleCommandAllowedUVE() || !IsDocumentEntityUVE(m_selectedEntity)) {
        return false;
    }

    const Scene::EntityUVE target = m_selectedEntity;
    const std::optional<Scene::SceneSnapshotUVE> snapshot = CaptureSubtreeUVE(target);
    if (!snapshot.has_value()) {
        return false;
    }

    Scene::EntityUVE originalParent = Scene::kInvalidEntityUVE;
    if (!TryGetDocumentParentUVE(target, originalParent)) {
        return false;
    }

    const EditorSelectionSnapshotUVE selectionBefore = CaptureSelectionSnapshotUVE();
    const bool dirtyBefore = m_sceneDirty;
    DestroyDocumentSubtreeUVE(target);
    const Scene::EntityUVE selectionAfter = IsDocumentEntityUVE(originalParent)
                                                ? originalParent
                                                : Scene::kInvalidEntityUVE;
    RestoreSelectionUVE(EditorSelectionSnapshotUVE{
        selectionAfter == Scene::kInvalidEntityUVE ? std::vector<Scene::EntityUVE>{}
                                                    : std::vector<Scene::EntityUVE>{selectionAfter},
        selectionAfter});
    m_sceneDirty = true;
    InvalidateHierarchyFilterCacheUVE();
    RecordHistoryUVE(DeletionHistoryEntryUVE{
        std::move(*snapshot), originalParent, target, selectionBefore, CaptureSelectionSnapshotUVE(), dirtyBefore, true});
    return true;
}

bool EditorUVE::ReparentSelectedEntityUVE(const Scene::EntityUVE newParent) {
    return ReparentDocumentEntityUVE(m_selectedEntity, newParent);
}

bool EditorUVE::ComputeKeepWorldLocalTransformUVE(const Scene::EntityUVE entity,
                                                    const Scene::EntityUVE newParent,
                                                    Scene::TransformComponentUVE& outTransform) const {
    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!IsDocumentEntityUVE(entity) || !entityManager.HasComponentUVE<Scene::WorldTransformComponentUVE>(entity)) {
        return false;
    }
    const Scene::WorldTransformComponentUVE& sourceWorld =
        entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(entity);
    Math::QuaternionUVE sourceRotation{};
    if (sourceWorld.dirty || !IsFiniteVectorUVE(sourceWorld.worldPosition) ||
        !IsFiniteVectorUVE(sourceWorld.worldScale) || !Math::TryNormalizeUVE(sourceWorld.worldRotation, sourceRotation)) {
        return false;
    }

    Math::Vector3UVE parentPosition{};
    Math::Vector3UVE parentScale{1.0F, 1.0F, 1.0F};
    Math::QuaternionUVE parentRotation{0.0F, 0.0F, 0.0F, 1.0F};
    if (newParent != Scene::kInvalidEntityUVE) {
        if (!IsDocumentEntityUVE(newParent) ||
            !entityManager.HasComponentUVE<Scene::WorldTransformComponentUVE>(newParent)) {
            return false;
        }
        const Scene::WorldTransformComponentUVE& parentWorld =
            entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(newParent);
        if (parentWorld.dirty || !IsFiniteVectorUVE(parentWorld.worldPosition) ||
            !IsFiniteVectorUVE(parentWorld.worldScale) ||
            !Math::TryNormalizeUVE(parentWorld.worldRotation, parentRotation) ||
            parentWorld.worldScale.x < kMinimumLocalScaleUVE ||
            parentWorld.worldScale.y < kMinimumLocalScaleUVE ||
            parentWorld.worldScale.z < kMinimumLocalScaleUVE) {
            return false;
        }
        const bool nonUniform = std::abs(parentWorld.worldScale.x - parentWorld.worldScale.y) > kVectorEpsilonUVE ||
                                std::abs(parentWorld.worldScale.x - parentWorld.worldScale.z) > kVectorEpsilonUVE ||
                                std::abs(parentWorld.worldScale.y - parentWorld.worldScale.z) > kVectorEpsilonUVE;
        const bool rotated = std::abs(parentRotation.x) > kVectorEpsilonUVE ||
                             std::abs(parentRotation.y) > kVectorEpsilonUVE ||
                             std::abs(parentRotation.z) > kVectorEpsilonUVE ||
                             std::abs(std::abs(parentRotation.w) - 1.0F) > kVectorEpsilonUVE;
        if (nonUniform && rotated) {
            return false;
        }
        parentPosition = parentWorld.worldPosition;
        parentScale = parentWorld.worldScale;
    }

    Math::QuaternionUVE parentInverse{};
    if (!Math::TryInverseUVE(parentRotation, parentInverse)) {
        return false;
    }
    const Math::Vector3UVE unrotated = Math::RotateVectorUVE(
        parentInverse, sourceWorld.worldPosition - parentPosition);
    outTransform.localPosition = Math::Vector3UVE{
        unrotated.x / parentScale.x, unrotated.y / parentScale.y, unrotated.z / parentScale.z};
    if (!Math::TryNormalizeUVE(Math::MultiplyUVE(parentInverse, sourceRotation), outTransform.localRotation)) {
        return false;
    }
    outTransform.localScale = Math::Vector3UVE{
        sourceWorld.worldScale.x / parentScale.x, sourceWorld.worldScale.y / parentScale.y,
        sourceWorld.worldScale.z / parentScale.z};
    return IsTransformFiniteUVE(outTransform) && outTransform.localScale.x >= kMinimumLocalScaleUVE &&
           outTransform.localScale.y >= kMinimumLocalScaleUVE && outTransform.localScale.z >= kMinimumLocalScaleUVE;
}

bool EditorUVE::ReparentDocumentEntityUVE(const Scene::EntityUVE entity, const Scene::EntityUVE newParent) {
    if (!IsLifecycleCommandAllowedUVE() || !HasSceneGraphNodeUVE(entity) || !IsDocumentSubtreeUVE(entity) ||
        (newParent != Scene::kInvalidEntityUVE && !HasSceneGraphNodeUVE(newParent)) ||
        entity == newParent || DoesSubtreeContainEntityUVE(entity, newParent)) {
        return false;
    }
    Scene::EntityUVE parentBefore = Scene::kInvalidEntityUVE;
    if (!TryGetDocumentParentUVE(entity, parentBefore) || parentBefore == newParent) {
        return false;
    }
    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::TransformComponentUVE>(entity)) {
        return false;
    }
    const Scene::TransformComponentUVE localBefore =
        entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity);
    Scene::TransformComponentUVE localAfter = localBefore;
    if (m_reparentTransformMode == EditorReparentTransformModeUVE::KeepWorld &&
        !ComputeKeepWorldLocalTransformUVE(entity, newParent, localAfter)) {
        return false;
    }
    const EditorSelectionSnapshotUVE selectionBefore = CaptureSelectionSnapshotUVE();
    const bool dirtyBefore = m_sceneDirty;
    m_services->GetSceneGraphUVE().SetParentUVE(entityManager, entity, newParent);
    if (!ApplyLocalTransformUVE(entity, localAfter)) {
        m_services->GetSceneGraphUVE().SetParentUVE(entityManager, entity, parentBefore);
        static_cast<void>(ApplyLocalTransformUVE(entity, localBefore));
        return false;
    }
    RestoreSelectionUVE(EditorSelectionSnapshotUVE{{entity}, entity});
    m_sceneDirty = true;
    InvalidateHierarchyFilterCacheUVE();
    RecordHistoryUVE(ReparentHistoryEntryUVE{
        entity, parentBefore, newParent, localBefore, localAfter, selectionBefore, CaptureSelectionSnapshotUVE(),
        dirtyBefore, true});
    return true;
}

bool EditorUVE::UndoUVE() {
    if (!IsAuthoringCommandAllowedUVE() || m_undoHistory.empty()) {
        return false;
    }

    HistoryEntryUVE entry = std::move(m_undoHistory.back());
    m_undoHistory.pop_back();
    if (!UndoHistoryEntryUVE(entry)) {
        ClearHistoryUVE();
        return false;
    }

    m_redoHistory.push_back(std::move(entry));
    return true;
}

bool EditorUVE::RedoUVE() {
    if (!IsAuthoringCommandAllowedUVE() || m_redoHistory.empty()) {
        return false;
    }

    HistoryEntryUVE entry = std::move(m_redoHistory.back());
    m_redoHistory.pop_back();
    if (!RedoHistoryEntryUVE(entry)) {
        ClearHistoryUVE();
        return false;
    }

    m_undoHistory.push_back(std::move(entry));
    return true;
}

bool EditorUVE::CanUndoUVE() const noexcept {
    return IsAuthoringCommandAllowedUVE() && !m_undoHistory.empty();
}

bool EditorUVE::CanRedoUVE() const noexcept {
    return IsAuthoringCommandAllowedUVE() && !m_redoHistory.empty();
}

bool EditorUVE::ApplyLocalTransformUVE(const Scene::EntityUVE entity,
                                       const Scene::TransformComponentUVE& transform) {
    if (!IsDocumentEntityUVE(entity) || !IsTransformFiniteUVE(transform)) {
        return false;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::TransformComponentUVE>(entity)) {
        return false;
    }

    m_services->GetSceneGraphUVE().SetLocalTransformUVE(entityManager, entity, transform);
    return true;
}

bool EditorUVE::ApplyEntityNameStateUVE(const Scene::EntityUVE entity,
                                        const std::optional<std::string>& name) {
    if (!IsDocumentEntityUVE(entity) || (name.has_value() && !IsEntityNameValidUVE(*name))) {
        return false;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    const bool hasName = entityManager.HasComponentUVE<Scene::NameComponentUVE>(entity);
    if (!name.has_value()) {
        if (!hasName) {
            return false;
        }
        entityManager.RemoveComponentUVE<Scene::NameComponentUVE>(entity);
        InvalidateHierarchyFilterCacheUVE();
        return true;
    }

    if (hasName) {
        entityManager.GetComponentUVE<Scene::NameComponentUVE>(entity).name = *name;
    } else {
        entityManager.AddComponentUVE<Scene::NameComponentUVE>(entity, Scene::NameComponentUVE{*name});
    }
    InvalidateHierarchyFilterCacheUVE();
    return true;
}

bool EditorUVE::IsDocumentSubtreeUVE(const Scene::EntityUVE root) const {
    if (!IsDocumentEntityUVE(root)) {
        return false;
    }

    // A document subtree must never absorb the editor-owned viewport camera, even if a caller
    // externally attempts an invalid reparent. Detect malformed cycles before any traversal caller
    // can act on the subtree.
    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    std::vector<Scene::EntityUVE> pending{root};
    std::vector<Scene::EntityUVE> visited;
    while (!pending.empty()) {
        const Scene::EntityUVE current = pending.back();
        pending.pop_back();
        if (!IsDocumentEntityUVE(current) ||
            std::find(visited.begin(), visited.end(), current) != visited.end()) {
            return false;
        }
        visited.push_back(current);
        const std::vector<Scene::EntityUVE> children =
            m_services->GetSceneGraphUVE().GetChildrenUVE(entityManager, current);
        pending.insert(pending.end(), children.begin(), children.end());
    }
    return true;
}

bool EditorUVE::DoesSubtreeContainEntityUVE(const Scene::EntityUVE root,
                                            const Scene::EntityUVE candidate) const {
    if (candidate == Scene::kInvalidEntityUVE || !IsDocumentSubtreeUVE(root)) {
        return false;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    std::vector<Scene::EntityUVE> pending{root};
    while (!pending.empty()) {
        const Scene::EntityUVE current = pending.back();
        pending.pop_back();
        if (current == candidate) {
            return true;
        }
        const std::vector<Scene::EntityUVE> children =
            m_services->GetSceneGraphUVE().GetChildrenUVE(entityManager, current);
        pending.insert(pending.end(), children.begin(), children.end());
    }
    return false;
}

std::optional<Scene::SceneSnapshotUVE> EditorUVE::CaptureSubtreeUVE(const Scene::EntityUVE root) {
    if (!IsDocumentSubtreeUVE(root)) {
        return std::nullopt;
    }

    return m_services->GetSceneSerializerUVE().CaptureUVE(
        m_services->GetEntityManagerUVE(), {root}, Asset::AssetKindUVE::Scene);
}

Scene::EntityUVE EditorUVE::RestoreSubtreeUnderParentUVE(const Scene::SceneSnapshotUVE& snapshot,
                                                         const Scene::EntityUVE parent) {
    if (parent != Scene::kInvalidEntityUVE && !IsDocumentEntityUVE(parent)) {
        return Scene::kInvalidEntityUVE;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    std::vector<Scene::EntityUVE> restoredRoots = m_services->GetSceneSerializerUVE().RestoreUVE(entityManager, snapshot);
    if (restoredRoots.size() != 1U || !IsDocumentEntityUVE(restoredRoots.front())) {
        for (const Scene::EntityUVE restoredRoot : restoredRoots) {
            if (IsDocumentEntityUVE(restoredRoot)) {
                DestroyDocumentSubtreeUVE(restoredRoot);
            }
        }
        return Scene::kInvalidEntityUVE;
    }

    const Scene::EntityUVE restoredRoot = restoredRoots.front();
    if (parent != Scene::kInvalidEntityUVE) {
        if (!entityManager.HasComponentUVE<Scene::HierarchyComponentUVE>(restoredRoot)) {
            DestroyDocumentSubtreeUVE(restoredRoot);
            return Scene::kInvalidEntityUVE;
        }
        m_services->GetSceneGraphUVE().SetParentUVE(entityManager, restoredRoot, parent);
    }
    return restoredRoot;
}

bool EditorUVE::TryGetDocumentParentUVE(const Scene::EntityUVE entity, Scene::EntityUVE& outParent) const {
    outParent = Scene::kInvalidEntityUVE;
    if (!IsDocumentEntityUVE(entity)) {
        return false;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::HierarchyComponentUVE>(entity)) {
        return true;
    }
    const Scene::EntityUVE parent = entityManager.GetComponentUVE<Scene::HierarchyComponentUVE>(entity).parent;
    if (parent == Scene::kInvalidEntityUVE) {
        return true;
    }
    if (!IsDocumentEntityUVE(parent)) {
        return false;
    }
    outParent = parent;
    return true;
}

bool EditorUVE::IsLifecycleCommandAllowedUVE() const noexcept {
    return IsAuthoringCommandAllowedUVE() && HasSingleDocumentSelectionUVE() &&
           m_gizmoDrag.axis == EditorTranslateAxisUVE::None &&
           m_viewportNavigationMode == EditorViewportNavigationModeUVE::None;
}

bool EditorUVE::IsAuthoringCommandAllowedUVE() const noexcept {
    return m_state == EditorStateUVE::Running && m_playModeState == EditorPlayModeStateUVE::Edit;
}

EditorUVE::EditorSelectionSnapshotUVE EditorUVE::CaptureSelectionSnapshotUVE() const {
    EditorSelectionSnapshotUVE selection{};
    for (const Scene::EntityUVE entity : m_selectedEntities) {
        if (IsDocumentEntityUVE(entity) &&
            std::find(selection.entities.begin(), selection.entities.end(), entity) == selection.entities.end()) {
            selection.entities.push_back(entity);
        }
    }
    if (IsDocumentEntityUVE(m_selectedEntity) &&
        std::find(selection.entities.begin(), selection.entities.end(), m_selectedEntity) != selection.entities.end()) {
        selection.activeEntity = m_selectedEntity;
    } else if (!selection.entities.empty()) {
        selection.activeEntity = selection.entities.back();
    }
    return selection;
}

void EditorUVE::RestoreSelectionUVE(EditorSelectionSnapshotUVE selection) noexcept {
    std::vector<Scene::EntityUVE> restored;
    restored.reserve(selection.entities.size());
    for (const Scene::EntityUVE entity : selection.entities) {
        if (IsDocumentEntityUVE(entity) && std::find(restored.begin(), restored.end(), entity) == restored.end()) {
            restored.push_back(entity);
        }
    }

    const bool activeValid = IsDocumentEntityUVE(selection.activeEntity) &&
                             std::find(restored.begin(), restored.end(), selection.activeEntity) != restored.end();
    const Scene::EntityUVE restoredActive = activeValid
                                                ? selection.activeEntity
                                                : (restored.empty() ? Scene::kInvalidEntityUVE : restored.back());
    const bool changed = restored != m_selectedEntities || restoredActive != m_selectedEntity;
    m_selectedEntities = std::move(restored);
    m_selectedEntity = restoredActive;
    if (changed) {
        CancelHierarchyRenameUVE();
        CancelGizmoDragUVE();
    }
}

void EditorUVE::PruneSelectionUVE() noexcept {
    RestoreSelectionUVE(CaptureSelectionSnapshotUVE());
}

bool EditorUVE::IsEntitySelectedUVE(const Scene::EntityUVE entity) const noexcept {
    return std::find(m_selectedEntities.begin(), m_selectedEntities.end(), entity) != m_selectedEntities.end();
}

EditorUVE::EditorSelectionPathsUVE EditorUVE::CaptureSelectionPathsUVE(
    const std::vector<Scene::EntityUVE>& roots) const {
    EditorSelectionPathsUVE paths{};
    const EditorSelectionSnapshotUVE selection = CaptureSelectionSnapshotUVE();
    const auto capturePath = [this, &roots](const Scene::EntityUVE entity) -> std::optional<EditorSelectionPathUVE> {
        for (std::size_t rootIndex = 0U; rootIndex < roots.size(); ++rootIndex) {
            EditorSelectionPathUVE path{};
            path.rootIndex = rootIndex;
            if (FindSelectionPathUVE(roots[rootIndex], entity, path.childIndices)) {
                return path;
            }
        }
        return std::nullopt;
    };

    for (const Scene::EntityUVE entity : selection.entities) {
        if (const std::optional<EditorSelectionPathUVE> path = capturePath(entity); path.has_value()) {
            paths.entityPaths.push_back(*path);
        }
    }
    paths.activePath = capturePath(selection.activeEntity);
    return paths;
}

EditorUVE::EditorSelectionSnapshotUVE EditorUVE::ResolveSelectionPathsUVE(
    const EditorSelectionPathsUVE& paths, const std::vector<Scene::EntityUVE>& roots) const {
    EditorSelectionSnapshotUVE selection{};
    for (const EditorSelectionPathUVE& path : paths.entityPaths) {
        const Scene::EntityUVE entity = ResolveSelectionPathUVE(path, roots);
        if (IsDocumentEntityUVE(entity) &&
            std::find(selection.entities.begin(), selection.entities.end(), entity) == selection.entities.end()) {
            selection.entities.push_back(entity);
        }
    }
    if (paths.activePath.has_value()) {
        const Scene::EntityUVE active = ResolveSelectionPathUVE(*paths.activePath, roots);
        if (std::find(selection.entities.begin(), selection.entities.end(), active) != selection.entities.end()) {
            selection.activeEntity = active;
        }
    }
    if (selection.activeEntity == Scene::kInvalidEntityUVE && !selection.entities.empty()) {
        selection.activeEntity = selection.entities.back();
    }
    return selection;
}

Scene::EntityUVE EditorUVE::ResolveSelectionPathUVE(const EditorSelectionPathUVE& path,
                                                     const std::vector<Scene::EntityUVE>& roots) const {
    if (path.rootIndex >= roots.size() || !IsDocumentEntityUVE(roots[path.rootIndex])) {
        return Scene::kInvalidEntityUVE;
    }

    Scene::EntityUVE resolved = roots[path.rootIndex];
    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    for (const std::size_t childIndex : path.childIndices) {
        const std::vector<Scene::EntityUVE> children =
            m_services->GetSceneGraphUVE().GetChildrenUVE(entityManager, resolved);
        if (childIndex >= children.size() || !IsDocumentEntityUVE(children[childIndex])) {
            return Scene::kInvalidEntityUVE;
        }
        resolved = children[childIndex];
    }
    return resolved;
}

bool EditorUVE::FindSelectionPathUVE(const Scene::EntityUVE current, const Scene::EntityUVE target,
                                     std::vector<std::size_t>& inOutChildIndices) const {
    if (current == target) {
        return true;
    }
    if (!IsDocumentEntityUVE(current)) {
        return false;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    const std::vector<Scene::EntityUVE> children =
        m_services->GetSceneGraphUVE().GetChildrenUVE(entityManager, current);
    for (std::size_t childIndex = 0U; childIndex < children.size(); ++childIndex) {
        inOutChildIndices.push_back(childIndex);
        if (FindSelectionPathUVE(children[childIndex], target, inOutChildIndices)) {
            return true;
        }
        inOutChildIndices.pop_back();
    }
    return false;
}

Scene::EntityUVE EditorUVE::CreateDocumentEntityInternalUVE(
    const EditorEntityKindUVE kind, const std::optional<std::string>& explicitName) {
    switch (kind) {
        case EditorEntityKindUVE::Empty:
        case EditorEntityKindUVE::Camera:
        case EditorEntityKindUVE::DirectionalLight:
        case EditorEntityKindUVE::CollisionBox:
            break;
        default:
            return Scene::kInvalidEntityUVE;
    }

    const std::string defaultName = GetDefaultEntityNameUVE(kind);
    const std::string name = explicitName.has_value() ? *explicitName : MakeUniqueDocumentEntityNameUVE(defaultName);
    if (defaultName.empty() || !IsEntityNameValidUVE(name)) {
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

    entityManager.AddComponentUVE<Scene::NameComponentUVE>(entity, Scene::NameComponentUVE{name});
    InvalidateHierarchyFilterCacheUVE();
    return entity;
}

void EditorUVE::RecordHistoryUVE(HistoryEntryUVE entry) {
    m_redoHistory.clear();
    if (m_undoHistory.size() >= m_historyCapacity) {
        m_undoHistory.pop_front();
    }
    m_undoHistory.push_back(std::move(entry));
}

void EditorUVE::ClearHistoryUVE() noexcept {
    m_undoHistory.clear();
    m_redoHistory.clear();
}

bool EditorUVE::UndoHistoryEntryUVE(HistoryEntryUVE& entry) {
    return std::visit(
        [this](auto& typedEntry) -> bool {
            using EntryType = std::decay_t<decltype(typedEntry)>;
            if constexpr (std::is_same_v<EntryType, TransformHistoryEntryUVE>) {
                if (!ApplyLocalTransformUVE(typedEntry.entity, typedEntry.before)) {
                    return false;
                }
                RestoreSelectionUVE(typedEntry.selectionBefore);
                m_sceneDirty = typedEntry.dirtyBefore;
                return true;
            } else if constexpr (std::is_same_v<EntryType, NameHistoryEntryUVE>) {
                if (!ApplyEntityNameStateUVE(typedEntry.entity, typedEntry.beforeName)) {
                    return false;
                }
                RestoreSelectionUVE(typedEntry.selectionBefore);
                m_sceneDirty = typedEntry.dirtyBefore;
                return true;
            } else if constexpr (std::is_same_v<EntryType, CreationHistoryEntryUVE>) {
                if (!IsDocumentEntityUVE(typedEntry.activeEntity)) {
                    return false;
                }
                DestroyDocumentSubtreeUVE(typedEntry.activeEntity);
                typedEntry.activeEntity = Scene::kInvalidEntityUVE;
                RestoreSelectionUVE(typedEntry.selectionBefore);
                m_sceneDirty = typedEntry.dirtyBefore;
                return true;
            } else if constexpr (std::is_same_v<EntryType, DuplicationHistoryEntryUVE>) {
                if (!IsDocumentEntityUVE(typedEntry.activeEntity)) {
                    return false;
                }
                DestroyDocumentSubtreeUVE(typedEntry.activeEntity);
                typedEntry.activeEntity = Scene::kInvalidEntityUVE;
                RestoreSelectionUVE(typedEntry.selectionBefore);
                m_sceneDirty = typedEntry.dirtyBefore;
                return true;
            } else if constexpr (std::is_same_v<EntryType, DeletionHistoryEntryUVE>) {
                if (typedEntry.activeEntity != Scene::kInvalidEntityUVE &&
                    m_services->GetEntityManagerUVE().IsAliveUVE(typedEntry.activeEntity)) {
                    return false;
                }
                const Scene::EntityUVE restored = RestoreSubtreeUnderParentUVE(typedEntry.snapshot, typedEntry.originalParent);
                if (restored == Scene::kInvalidEntityUVE) {
                    return false;
                }
                typedEntry.activeEntity = restored;
                typedEntry.selectionBefore = EditorSelectionSnapshotUVE{{restored}, restored};
                RestoreSelectionUVE(typedEntry.selectionBefore);
                m_sceneDirty = typedEntry.dirtyBefore;
                return true;
            } else {
                if (!HasSceneGraphNodeUVE(typedEntry.entity) ||
                    (typedEntry.parentBefore != Scene::kInvalidEntityUVE &&
                     !HasSceneGraphNodeUVE(typedEntry.parentBefore)) ||
                    DoesSubtreeContainEntityUVE(typedEntry.entity, typedEntry.parentBefore)) {
                    return false;
                }
                m_services->GetSceneGraphUVE().SetParentUVE(
                    m_services->GetEntityManagerUVE(), typedEntry.entity, typedEntry.parentBefore);
                if (!ApplyLocalTransformUVE(typedEntry.entity, typedEntry.localTransformBefore)) {
                    return false;
                }
                RestoreSelectionUVE(typedEntry.selectionBefore);
                m_sceneDirty = typedEntry.dirtyBefore;
                InvalidateHierarchyFilterCacheUVE();
                return true;
            }
        },
        entry);
}

bool EditorUVE::RedoHistoryEntryUVE(HistoryEntryUVE& entry) {
    return std::visit(
        [this](auto& typedEntry) -> bool {
            using EntryType = std::decay_t<decltype(typedEntry)>;
            if constexpr (std::is_same_v<EntryType, TransformHistoryEntryUVE>) {
                if (!ApplyLocalTransformUVE(typedEntry.entity, typedEntry.after)) {
                    return false;
                }
                RestoreSelectionUVE(typedEntry.selectionAfter);
                m_sceneDirty = typedEntry.dirtyAfter;
                return true;
            } else if constexpr (std::is_same_v<EntryType, NameHistoryEntryUVE>) {
                if (!ApplyEntityNameStateUVE(typedEntry.entity, typedEntry.afterName)) {
                    return false;
                }
                RestoreSelectionUVE(typedEntry.selectionAfter);
                m_sceneDirty = typedEntry.dirtyAfter;
                return true;
            } else if constexpr (std::is_same_v<EntryType, CreationHistoryEntryUVE>) {
                if (typedEntry.activeEntity != Scene::kInvalidEntityUVE &&
                    m_services->GetEntityManagerUVE().IsAliveUVE(typedEntry.activeEntity)) {
                    return false;
                }
                const Scene::EntityUVE recreated =
                    CreateDocumentEntityInternalUVE(typedEntry.kind, std::optional<std::string>{typedEntry.name});
                if (recreated == Scene::kInvalidEntityUVE) {
                    return false;
                }
                typedEntry.activeEntity = recreated;
                typedEntry.selectionAfter = EditorSelectionSnapshotUVE{{recreated}, recreated};
                RestoreSelectionUVE(typedEntry.selectionAfter);
                m_sceneDirty = typedEntry.dirtyAfter;
                return true;
            } else if constexpr (std::is_same_v<EntryType, DuplicationHistoryEntryUVE>) {
                if (typedEntry.activeEntity != Scene::kInvalidEntityUVE &&
                    m_services->GetEntityManagerUVE().IsAliveUVE(typedEntry.activeEntity)) {
                    return false;
                }
                const Scene::EntityUVE restored = RestoreSubtreeUnderParentUVE(typedEntry.snapshot, typedEntry.originalParent);
                if (restored == Scene::kInvalidEntityUVE) {
                    return false;
                }
                if (typedEntry.duplicateRootName.has_value() &&
                    !ApplyEntityNameStateUVE(restored, typedEntry.duplicateRootName)) {
                    DestroyDocumentSubtreeUVE(restored);
                    return false;
                }
                typedEntry.activeEntity = restored;
                typedEntry.selectionAfter = EditorSelectionSnapshotUVE{{restored}, restored};
                RestoreSelectionUVE(typedEntry.selectionAfter);
                m_sceneDirty = typedEntry.dirtyAfter;
                return true;
            } else if constexpr (std::is_same_v<EntryType, DeletionHistoryEntryUVE>) {
                if (!IsDocumentEntityUVE(typedEntry.activeEntity)) {
                    return false;
                }
                DestroyDocumentSubtreeUVE(typedEntry.activeEntity);
                typedEntry.activeEntity = Scene::kInvalidEntityUVE;
                RestoreSelectionUVE(typedEntry.selectionAfter);
                m_sceneDirty = typedEntry.dirtyAfter;
                return true;
            } else {
                if (!HasSceneGraphNodeUVE(typedEntry.entity) ||
                    (typedEntry.parentAfter != Scene::kInvalidEntityUVE &&
                     !HasSceneGraphNodeUVE(typedEntry.parentAfter)) ||
                    DoesSubtreeContainEntityUVE(typedEntry.entity, typedEntry.parentAfter)) {
                    return false;
                }
                m_services->GetSceneGraphUVE().SetParentUVE(
                    m_services->GetEntityManagerUVE(), typedEntry.entity, typedEntry.parentAfter);
                if (!ApplyLocalTransformUVE(typedEntry.entity, typedEntry.localTransformAfter)) {
                    return false;
                }
                RestoreSelectionUVE(typedEntry.selectionAfter);
                m_sceneDirty = typedEntry.dirtyAfter;
                InvalidateHierarchyFilterCacheUVE();
                return true;
            }
        },
        entry);
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

Math::Vector3UVE EditorUVE::GetViewportFocusPointUVE() const noexcept {
    return m_viewportFocusPoint;
}

float EditorUVE::GetViewportDistanceUVE() const noexcept {
    return m_viewportDistance;
}

EditorViewportNavigationModeUVE EditorUVE::GetViewportNavigationModeUVE() const noexcept {
    return m_viewportNavigationMode;
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
    CancelViewportNavigationUVE();
    if (m_playModeState != EditorPlayModeStateUVE::Edit) {
        if (!StopPlayModeUVE() && m_simulationControl != nullptr) {
            static_cast<void>(m_simulationControl->SetSimulationExecutionModeUVE(
                Core::SimulationExecutionModeUVE::Running));
            static_cast<void>(m_simulationControl->SetTransientSimulationSessionActiveUVE(false));
            m_playModeSession.reset();
            m_playModeState = EditorPlayModeStateUVE::Edit;
        }
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
    ClearHistoryUVE();
    m_state = EditorStateUVE::Shutdown;
}

bool EditorUVE::IsDocumentEntityUVE(const Scene::EntityUVE entity) const noexcept {
    return entity != Scene::kInvalidEntityUVE && entity != m_viewportCamera &&
           m_services->GetEntityManagerUVE().IsAliveUVE(entity);
}

bool EditorUVE::HasSceneGraphNodeUVE(const Scene::EntityUVE entity) const noexcept {
    if (!IsDocumentEntityUVE(entity)) {
        return false;
    }

    const Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    return entityManager.HasComponentUVE<Scene::TransformComponentUVE>(entity) &&
           entityManager.HasComponentUVE<Scene::HierarchyComponentUVE>(entity) &&
           entityManager.HasComponentUVE<Scene::WorldTransformComponentUVE>(entity);
}

bool EditorUVE::IsEntityNameValidUVE(const std::string_view name) const noexcept {
    return !name.empty() && name.size() <= kMaximumEntityNameBytesUVE && !IsWhitespaceOnlyUVE(name);
}

std::string EditorUVE::GetEntityDisplayLabelUVE(const Scene::EntityUVE entity) const {
    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (entityManager.IsAliveUVE(entity) && entityManager.HasComponentUVE<Scene::NameComponentUVE>(entity)) {
        const std::string& name = entityManager.GetComponentUVE<Scene::NameComponentUVE>(entity).name;
        if (!name.empty()) {
            return name;
        }
    }
    return EntityLabelUVE(entity);
}

std::string EditorUVE::GetDefaultEntityNameUVE(const EditorEntityKindUVE kind) const {
    switch (kind) {
        case EditorEntityKindUVE::Empty:
            return "Empty";
        case EditorEntityKindUVE::Camera:
            return "Camera";
        case EditorEntityKindUVE::DirectionalLight:
            return "Directional Light";
        case EditorEntityKindUVE::CollisionBox:
            return "Collision Box";
    }
    return {};
}

std::string EditorUVE::MakeUniqueDocumentEntityNameUVE(const std::string_view baseName) const {
    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    std::vector<std::string> names;
    entityManager.ForEachUVE<Scene::NameComponentUVE>(
        [this, &names](const Scene::EntityUVE entity, Scene::NameComponentUVE& component) {
            if (IsDocumentEntityUVE(entity)) {
                names.push_back(component.name);
            }
        });

    const auto isUsed = [&names](const std::string_view candidate) {
        return std::any_of(names.begin(), names.end(), [candidate](const std::string& name) {
            return name == candidate;
        });
    };
    if (!isUsed(baseName)) {
        return std::string{baseName};
    }

    for (std::size_t suffix = 2U;; ++suffix) {
        const std::string candidate = std::string{baseName} + " " + std::to_string(suffix);
        if (!isUsed(candidate)) {
            return candidate;
        }
    }
}

bool EditorUVE::IsTransformFiniteUVE(const Scene::TransformComponentUVE& transform) const noexcept {
    return IsFiniteVectorUVE(transform.localPosition) && IsFiniteUVE(transform.localRotation.x) &&
           IsFiniteUVE(transform.localRotation.y) && IsFiniteUVE(transform.localRotation.z) &&
           IsFiniteUVE(transform.localRotation.w) && IsFiniteVectorUVE(transform.localScale);
}

bool EditorUVE::IsQuaternionFiniteUVE(const Math::QuaternionUVE& quaternion) const noexcept {
    return Math::IsFiniteUVE(quaternion);
}

bool EditorUVE::AreTransformSnappingSettingsValidUVE(
    const EditorTransformSnappingSettingsUVE& settings) const noexcept {
    return IsFiniteUVE(settings.translateStep) && settings.translateStep > kVectorEpsilonUVE &&
           IsFiniteUVE(settings.rotateStepDegrees) && settings.rotateStepDegrees > kVectorEpsilonUVE &&
           IsFiniteUVE(settings.scaleStep) && settings.scaleStep > kVectorEpsilonUVE;
}

float EditorUVE::SnapScalarUVE(const float value, const float increment) const noexcept {
    if (!IsFiniteUVE(value) || !IsFiniteUVE(increment) || increment <= kVectorEpsilonUVE) {
        return value;
    }
    const float snapped = std::round(value / increment) * increment;
    return IsFiniteUVE(snapped) ? snapped : value;
}

bool EditorUVE::IsViewportRectValidUVE(const EditorViewportRectUVE& viewportRect) const noexcept {
    return IsFiniteUVE(viewportRect.origin.x) && IsFiniteUVE(viewportRect.origin.y) &&
           IsFiniteUVE(viewportRect.size.x) && IsFiniteUVE(viewportRect.size.y) &&
           viewportRect.size.x >= kMinimumViewportWidthUVE && viewportRect.size.y >= kMinimumViewportHeightUVE;
}

bool EditorUVE::IsViewportNavigationFiniteUVE() const noexcept {
    return IsFiniteVectorUVE(m_viewportFocusPoint) && IsFiniteUVE(m_viewportYawRadians) &&
           IsFiniteUVE(m_viewportPitchRadians) && IsFiniteUVE(m_viewportDistance) &&
           m_viewportDistance >= kMinimumViewportDistanceUVE &&
           m_viewportDistance <= kMaximumViewportDistanceUVE;
}

bool EditorUVE::ApplyViewportCameraUVE() {
    if (m_state != EditorStateUVE::Running || !IsViewportNavigationFiniteUVE()) {
        return false;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.IsAliveUVE(m_viewportCamera) ||
        !entityManager.HasComponentUVE<Scene::TransformComponentUVE>(m_viewportCamera)) {
        return false;
    }

    const Math::Vector3UVE forward = MakeViewportForwardUVE(m_viewportYawRadians, m_viewportPitchRadians);
    const Math::QuaternionUVE orientation =
        MakeViewportOrientationUVE(m_viewportYawRadians, m_viewportPitchRadians);
    if (!IsFiniteVectorUVE(forward) || !IsFiniteUVE(orientation.x) || !IsFiniteUVE(orientation.y) ||
        !IsFiniteUVE(orientation.z) || !IsFiniteUVE(orientation.w)) {
        return false;
    }

    Scene::TransformComponentUVE transform =
        entityManager.GetComponentUVE<Scene::TransformComponentUVE>(m_viewportCamera);
    transform.localPosition = m_viewportFocusPoint - (forward * m_viewportDistance);
    transform.localRotation = orientation;
    if (!IsTransformFiniteUVE(transform)) {
        return false;
    }

    m_services->GetSceneGraphUVE().SetLocalTransformUVE(entityManager, m_viewportCamera, transform);
    return true;
}

void EditorUVE::CancelViewportNavigationUVE() noexcept {
    m_viewportNavigationMode = EditorViewportNavigationModeUVE::None;
}

bool EditorUVE::IsFiniteVectorUVE(const Math::Vector3UVE& vector) const noexcept {
    return IsFiniteUVE(vector.x) && IsFiniteUVE(vector.y) && IsFiniteUVE(vector.z);
}

bool EditorUVE::GetGizmoAxisWorldVectorUVE(const Scene::EntityUVE entity, const EditorTranslateAxisUVE axis,
                                            Math::Vector3UVE& outAxis) const {
    outAxis = GetAxisVectorUVE(axis);
    if (axis == EditorTranslateAxisUVE::None || m_gizmoCoordinateSpace == EditorGizmoCoordinateSpaceUVE::World) {
        return axis != EditorTranslateAxisUVE::None;
    }
    if (!IsDocumentEntityUVE(entity)) {
        return false;
    }
    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::WorldTransformComponentUVE>(entity)) {
        return false;
    }
    const Scene::WorldTransformComponentUVE& world =
        entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(entity);
    Math::QuaternionUVE rotation{};
    if (world.dirty || !Math::TryNormalizeUVE(world.worldRotation, rotation)) {
        return false;
    }
    outAxis = Math::RotateVectorUVE(rotation, outAxis);
    return IsFiniteVectorUVE(outAxis) && Math::LengthSquaredUVE(outAxis) > kVectorEpsilonUVE;
}

bool EditorUVE::GetPlaneAxesUVE(const EditorTranslatePlaneUVE plane, Math::Vector3UVE& outAxisA,
                                 Math::Vector3UVE& outAxisB) const noexcept {
    switch (plane) {
        case EditorTranslatePlaneUVE::XY: outAxisA = {1.0F, 0.0F, 0.0F}; outAxisB = {0.0F, 1.0F, 0.0F}; return true;
        case EditorTranslatePlaneUVE::XZ: outAxisA = {1.0F, 0.0F, 0.0F}; outAxisB = {0.0F, 0.0F, 1.0F}; return true;
        case EditorTranslatePlaneUVE::YZ: outAxisA = {0.0F, 1.0F, 0.0F}; outAxisB = {0.0F, 0.0F, 1.0F}; return true;
        case EditorTranslatePlaneUVE::None: return false;
    }
    return false;
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

bool EditorUVE::ComputeLocalRotationForWorldAxisUVE(const Scene::EntityUVE entity,
                                                        const Math::QuaternionUVE& initialLocalRotation,
                                                        const Math::Vector3UVE& worldAxis, const float radians,
                                                        Math::QuaternionUVE& outLocalRotation) const {
    if (!IsDocumentEntityUVE(entity) || !IsQuaternionFiniteUVE(initialLocalRotation) ||
        !IsFiniteUVE(radians) || !IsFiniteVectorUVE(worldAxis) ||
        Math::LengthSquaredUVE(worldAxis) <= kVectorEpsilonUVE) {
        return false;
    }

    Math::QuaternionUVE initialNormalized{};
    Math::QuaternionUVE worldDelta{};
    if (!Math::TryNormalizeUVE(initialLocalRotation, initialNormalized) ||
        !Math::TryMakeAxisAngleUVE(worldAxis, radians, worldDelta)) {
        return false;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::HierarchyComponentUVE>(entity)) {
        return false;
    }

    const Scene::HierarchyComponentUVE& hierarchy =
        entityManager.GetComponentUVE<Scene::HierarchyComponentUVE>(entity);
    Math::QuaternionUVE localDelta = worldDelta;
    if (hierarchy.parent != Scene::kInvalidEntityUVE) {
        if (!entityManager.IsAliveUVE(hierarchy.parent) ||
            !entityManager.HasComponentUVE<Scene::WorldTransformComponentUVE>(hierarchy.parent)) {
            return false;
        }

        const Scene::WorldTransformComponentUVE& parentWorld =
            entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(hierarchy.parent);
        Math::QuaternionUVE parentNormalized{};
        Math::QuaternionUVE parentInverse{};
        if (parentWorld.dirty || !Math::TryNormalizeUVE(parentWorld.worldRotation, parentNormalized) ||
            !Math::TryInverseUVE(parentNormalized, parentInverse)) {
            return false;
        }
        localDelta = Math::MultiplyUVE(
            Math::MultiplyUVE(parentInverse, worldDelta), parentNormalized);
    }

    return Math::TryNormalizeUVE(Math::MultiplyUVE(localDelta, initialNormalized), outLocalRotation);
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
    if (!IsAuthoringCommandAllowedUVE()) {
        return false;
    }
    if (m_gizmoMode == EditorGizmoModeUVE::Rotate) {
        return BeginRotateGizmoDragUVE(viewportRect, pointerPosition);
    }
    if (!HasSingleDocumentSelectionUVE() || !IsViewportRectValidUVE(viewportRect)) {
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
    float uniformPixelsPerWorldUnitSum = 0.0F;
    std::size_t uniformPixelsPerWorldUnitCount = 0U;
    GizmoDragUVE candidate{};
    constexpr std::array<EditorTranslateAxisUVE, 3> axes{
        EditorTranslateAxisUVE::X,
        EditorTranslateAxisUVE::Y,
        EditorTranslateAxisUVE::Z,
    };
    for (const EditorTranslateAxisUVE axis : axes) {
        Math::Vector3UVE worldAxis{};
        if (!GetGizmoAxisWorldVectorUVE(m_selectedEntity, axis, worldAxis)) {
            continue;
        }
        Math::Vector2UVE endpoint{};
        const Math::Vector3UVE worldEndpoint = selectedWorld.worldPosition + worldAxis * kGizmoAxisLengthUVE;
        if (!ProjectWorldPointUVE(viewportRect, worldEndpoint, endpoint)) {
            continue;
        }

        const Math::Vector2UVE screenAxis{endpoint.x - center.x, endpoint.y - center.y};
        const float axisLengthSquared = LengthSquared2UVE(screenAxis);
        if (axisLengthSquared <= kVectorEpsilonUVE) {
            continue;
        }

        const float axisLength = std::sqrt(axisLengthSquared);
        uniformPixelsPerWorldUnitSum += axisLength / kGizmoAxisLengthUVE;
        ++uniformPixelsPerWorldUnitCount;
        const Math::Vector2UVE pointerOffset{pointerPosition.x - center.x, pointerPosition.y - center.y};
        const float along = std::clamp(Dot2UVE(pointerOffset, screenAxis) / axisLengthSquared, 0.0F, 1.0F);
        const Math::Vector2UVE closestPoint = m_gizmoMode == EditorGizmoModeUVE::Scale
                                                  ? endpoint
                                                  : Math::Vector2UVE{center.x + screenAxis.x * along,
                                                                     center.y + screenAxis.y * along};
        const Math::Vector2UVE distanceVector{pointerPosition.x - closestPoint.x, pointerPosition.y - closestPoint.y};
        const float distanceSquared = LengthSquared2UVE(distanceVector);
        if (distanceSquared > (kGizmoHandleRadiusPixelsUVE * kGizmoHandleRadiusPixelsUVE) ||
            distanceSquared >= bestDistanceSquared) {
            continue;
        }
        candidate.mode = m_gizmoMode;
        candidate.axis = axis;
        candidate.entity = m_selectedEntity;
        candidate.initialLocalTransform =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(m_selectedEntity);
        candidate.initialPointer = pointerPosition;
        candidate.worldAxisA = worldAxis;
        candidate.screenAxisDirection = Scale2UVE(screenAxis, 1.0F / axisLength);
        candidate.pixelsPerWorldUnit = axisLength / kGizmoAxisLengthUVE;
        candidate.initialDirty = m_sceneDirty;
        bestDistanceSquared = distanceSquared;
    }

    // Axis/endpoint candidates win deterministically. The Scale center is a fallback only after every axis hit.
    if (candidate.axis == EditorTranslateAxisUVE::None && m_gizmoMode == EditorGizmoModeUVE::Scale &&
        uniformPixelsPerWorldUnitCount > 0U) {
        const Math::Vector2UVE centerOffset{pointerPosition.x - center.x, pointerPosition.y - center.y};
        const float centerDistanceSquared = LengthSquared2UVE(centerOffset);
        if (centerDistanceSquared <= (kGizmoHandleRadiusPixelsUVE * kGizmoHandleRadiusPixelsUVE)) {
            candidate.mode = EditorGizmoModeUVE::Scale;
            candidate.handleKind = GizmoHandleKindUVE::UniformScaleOffset;
            candidate.axis = EditorTranslateAxisUVE::X;
            candidate.entity = m_selectedEntity;
            candidate.initialLocalTransform = entityManager.GetComponentUVE<Scene::TransformComponentUVE>(m_selectedEntity);
            candidate.initialPointer = pointerPosition;
            candidate.screenCenter = center;
            candidate.pixelsPerWorldUnit =
                uniformPixelsPerWorldUnitSum / static_cast<float>(uniformPixelsPerWorldUnitCount);
            candidate.initialDirty = m_sceneDirty;
        }
    }

    // Axis/endpoint candidates win deterministically. Plane handles are considered only when no axis hit exists.
    if (candidate.axis == EditorTranslateAxisUVE::None && candidate.handleKind != GizmoHandleKindUVE::UniformScaleOffset &&
        m_gizmoMode == EditorGizmoModeUVE::Translate) {
        constexpr std::array<EditorTranslatePlaneUVE, 3> planes{
            EditorTranslatePlaneUVE::XY, EditorTranslatePlaneUVE::XZ, EditorTranslatePlaneUVE::YZ};
        for (const EditorTranslatePlaneUVE plane : planes) {
            Math::Vector3UVE localAxisA{};
            Math::Vector3UVE localAxisB{};
            if (!GetPlaneAxesUVE(plane, localAxisA, localAxisB)) {
                continue;
            }
            const EditorTranslateAxisUVE axisA = plane == EditorTranslatePlaneUVE::YZ ? EditorTranslateAxisUVE::Y : EditorTranslateAxisUVE::X;
            const EditorTranslateAxisUVE axisB = plane == EditorTranslatePlaneUVE::XY ? EditorTranslateAxisUVE::Y : EditorTranslateAxisUVE::Z;
            Math::Vector3UVE worldAxisA{};
            Math::Vector3UVE worldAxisB{};
            if (!GetGizmoAxisWorldVectorUVE(m_selectedEntity, axisA, worldAxisA) ||
                !GetGizmoAxisWorldVectorUVE(m_selectedEntity, axisB, worldAxisB)) {
                continue;
            }
            Math::Vector2UVE endpointA{};
            Math::Vector2UVE endpointB{};
            if (!ProjectWorldPointUVE(viewportRect, selectedWorld.worldPosition + worldAxisA * kGizmoAxisLengthUVE, endpointA) ||
                !ProjectWorldPointUVE(viewportRect, selectedWorld.worldPosition + worldAxisB * kGizmoAxisLengthUVE, endpointB)) {
                continue;
            }
            const Math::Vector2UVE screenA{endpointA.x - center.x, endpointA.y - center.y};
            const Math::Vector2UVE screenB{endpointB.x - center.x, endpointB.y - center.y};
            const float determinant = (screenA.x * screenB.y) - (screenA.y * screenB.x);
            if (!IsFiniteUVE(determinant) || std::abs(determinant) <= kVectorEpsilonUVE) {
                continue;
            }
            const Math::Vector2UVE offset{pointerPosition.x - center.x, pointerPosition.y - center.y};
            const float u = ((offset.x * screenB.y) - (offset.y * screenB.x)) / determinant;
            const float v = ((screenA.x * offset.y) - (screenA.y * offset.x)) / determinant;
            if (!IsFiniteUVE(u) || !IsFiniteUVE(v) || u < 0.20F || u > 0.60F || v < 0.20F || v > 0.60F) {
                continue;
            }
            candidate.handleKind = GizmoHandleKindUVE::Plane;
            candidate.plane = plane;
            candidate.axis = axisA;
            candidate.entity = m_selectedEntity;
            candidate.initialLocalTransform = entityManager.GetComponentUVE<Scene::TransformComponentUVE>(m_selectedEntity);
            candidate.initialPointer = pointerPosition;
            candidate.screenPlaneAxisA = screenA;
            candidate.screenPlaneAxisB = screenB;
            candidate.worldAxisA = worldAxisA;
            candidate.worldAxisB = worldAxisB;
            candidate.initialDirty = m_sceneDirty;
            candidate.pixelsPerWorldUnit = 1.0F;
            break;
        }
    }

    if ((candidate.axis == EditorTranslateAxisUVE::None &&
         candidate.handleKind != GizmoHandleKindUVE::UniformScaleOffset) ||
        candidate.pixelsPerWorldUnit <= kVectorEpsilonUVE ||
        !IsFiniteUVE(candidate.pixelsPerWorldUnit)) {
        return false;
    }

    m_gizmoDrag = candidate;
    return true;
}

bool EditorUVE::MapTrackballPointerUVE(const Math::Vector2UVE center, const float radius,
                                         const Math::Vector2UVE pointerPosition,
                                         Math::Vector3UVE& outVector) const noexcept {
    if (!IsFiniteUVE(center.x) || !IsFiniteUVE(center.y) || !IsFiniteUVE(radius) ||
        radius <= kVectorEpsilonUVE || !IsFiniteUVE(pointerPosition.x) || !IsFiniteUVE(pointerPosition.y)) {
        return false;
    }
    float x = (pointerPosition.x - center.x) / radius;
    float y = (center.y - pointerPosition.y) / radius;
    const float radiusSquared = (x * x) + (y * y);
    if (!IsFiniteUVE(radiusSquared)) {
        return false;
    }
    if (radiusSquared > 1.0F) {
        const float inverseRadius = 1.0F / std::sqrt(radiusSquared);
        x *= inverseRadius;
        y *= inverseRadius;
        outVector = Math::Vector3UVE{x, y, 0.0F};
    } else {
        outVector = Math::Vector3UVE{x, y, std::sqrt(std::max(0.0F, 1.0F - radiusSquared))};
    }
    const float vectorLengthSquared = Math::LengthSquaredUVE(outVector);
    if (!IsFiniteUVE(vectorLengthSquared) || vectorLengthSquared <= kVectorEpsilonUVE) {
        return false;
    }
    outVector = outVector * (1.0F / std::sqrt(vectorLengthSquared));
    return true;
}

bool EditorUVE::FindClosestRingParameterUVE(const EditorViewportRectUVE& viewportRect,
                                            const Scene::EntityUVE entity,
                                            const EditorTranslateAxisUVE axis,
                                            const Math::Vector2UVE pointerPosition,
                                            float& outParameterRadians,
                                            float& outDistanceSquared) const {
    if (!IsDocumentEntityUVE(entity) || !IsViewportRectValidUVE(viewportRect) ||
        !IsFiniteUVE(pointerPosition.x) || !IsFiniteUVE(pointerPosition.y)) {
        return false;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::WorldTransformComponentUVE>(entity)) {
        return false;
    }
    const Scene::WorldTransformComponentUVE& world =
        entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(entity);
    if (world.dirty || !IsFiniteVectorUVE(world.worldPosition)) {
        return false;
    }

    Math::Vector3UVE first{};
    Math::Vector3UVE second{};
    if (!GetRingBasisUVE(axis, first, second)) {
        return false;
    }
    if (m_gizmoCoordinateSpace == EditorGizmoCoordinateSpaceUVE::Local) {
        Math::QuaternionUVE rotation{};
        if (world.dirty || !Math::TryNormalizeUVE(world.worldRotation, rotation)) {
            return false;
        }
        first = Math::RotateVectorUVE(rotation, first);
        second = Math::RotateVectorUVE(rotation, second);
    }

    constexpr int kRingSegmentCountUVE = 64;
    constexpr float kFullTurnRadiansUVE = std::numbers::pi_v<float> * 2.0F;
    const float segmentRadians = kFullTurnRadiansUVE / static_cast<float>(kRingSegmentCountUVE);
    float bestDistanceSquared = std::numeric_limits<float>::max();
    float bestParameter = 0.0F;
    bool found = false;
    for (int segment = 0; segment < kRingSegmentCountUVE; ++segment) {
        const float startParameter = static_cast<float>(segment) * segmentRadians;
        Math::Vector2UVE start{};
        Math::Vector2UVE end{};
        if (!ProjectWorldPointUVE(viewportRect, MakeRingPointUVE(world.worldPosition, first, second, startParameter),
                                  start) ||
            !ProjectWorldPointUVE(viewportRect,
                                  MakeRingPointUVE(world.worldPosition, first, second,
                                                   startParameter + segmentRadians),
                                  end)) {
            continue;
        }

        const Math::Vector2UVE segmentVector{end.x - start.x, end.y - start.y};
        const float segmentLengthSquared = LengthSquared2UVE(segmentVector);
        if (segmentLengthSquared <= kVectorEpsilonUVE) {
            continue;
        }
        const Math::Vector2UVE pointerOffset{pointerPosition.x - start.x, pointerPosition.y - start.y};
        const float parameter = std::clamp(Dot2UVE(pointerOffset, segmentVector) / segmentLengthSquared, 0.0F, 1.0F);
        const Math::Vector2UVE closest{start.x + segmentVector.x * parameter,
                                       start.y + segmentVector.y * parameter};
        const Math::Vector2UVE offset{pointerPosition.x - closest.x, pointerPosition.y - closest.y};
        const float distanceSquared = LengthSquared2UVE(offset);
        if (distanceSquared < bestDistanceSquared) {
            bestDistanceSquared = distanceSquared;
            bestParameter = startParameter + parameter * segmentRadians;
            found = true;
        }
    }

    if (!found) {
        return false;
    }
    outParameterRadians = bestParameter;
    outDistanceSquared = bestDistanceSquared;
    return true;
}

bool EditorUVE::BeginRotateGizmoDragUVE(const EditorViewportRectUVE& viewportRect,
                                        const Math::Vector2UVE pointerPosition) {
    if (!IsAuthoringCommandAllowedUVE() || !HasSingleDocumentSelectionUVE() ||
        !IsViewportRectValidUVE(viewportRect)) {
        return false;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::TransformComponentUVE>(m_selectedEntity) ||
        !entityManager.HasComponentUVE<Scene::WorldTransformComponentUVE>(m_selectedEntity)) {
        return false;
    }

    constexpr std::array<EditorTranslateAxisUVE, 3> axes{
        EditorTranslateAxisUVE::X,
        EditorTranslateAxisUVE::Y,
        EditorTranslateAxisUVE::Z,
    };
    GizmoDragUVE candidate{};
    float bestDistanceSquared = std::numeric_limits<float>::max();
    for (const EditorTranslateAxisUVE axis : axes) {
        float parameterRadians = 0.0F;
        float distanceSquared = 0.0F;
        if (!FindClosestRingParameterUVE(viewportRect, m_selectedEntity, axis, pointerPosition,
                                         parameterRadians, distanceSquared) ||
            distanceSquared > (kGizmoHandleRadiusPixelsUVE * kGizmoHandleRadiusPixelsUVE) ||
            distanceSquared >= bestDistanceSquared) {
            continue;
        }
        candidate.mode = EditorGizmoModeUVE::Rotate;
        candidate.axis = axis;
        candidate.entity = m_selectedEntity;
        candidate.initialLocalTransform =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(m_selectedEntity);
        candidate.initialPointer = pointerPosition;
        if (!GetGizmoAxisWorldVectorUVE(m_selectedEntity, axis, candidate.worldAxisA)) {
            continue;
        }
        candidate.viewportRect = viewportRect;
        candidate.initialRingParameterRadians = parameterRadians;
        candidate.initialDirty = m_sceneDirty;
        bestDistanceSquared = distanceSquared;
    }

    // Ring candidates have explicit priority; the camera-oriented center trackball is fallback only.
    if (candidate.axis == EditorTranslateAxisUVE::None) {
        const Scene::WorldTransformComponentUVE& selectedWorld =
            entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(m_selectedEntity);
        Math::Vector2UVE center{};
        Math::Vector3UVE initialTrackballVector{};
        if (!selectedWorld.dirty && ProjectWorldPointUVE(viewportRect, selectedWorld.worldPosition, center) &&
            MapTrackballPointerUVE(center, kTrackballRadiusPixelsUVE, pointerPosition, initialTrackballVector) &&
            entityManager.HasComponentUVE<Scene::WorldTransformComponentUVE>(m_viewportCamera)) {
            const float centerDistanceSquared = LengthSquared2UVE(
                Math::Vector2UVE{pointerPosition.x - center.x, pointerPosition.y - center.y});
            const Scene::WorldTransformComponentUVE& cameraWorld =
                entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(m_viewportCamera);
            Math::QuaternionUVE viewRotation{};
            if (centerDistanceSquared <= (kTrackballRadiusPixelsUVE * kTrackballRadiusPixelsUVE) &&
                !cameraWorld.dirty && Math::TryNormalizeUVE(cameraWorld.worldRotation, viewRotation)) {
                candidate.mode = EditorGizmoModeUVE::Rotate;
                candidate.handleKind = GizmoHandleKindUVE::Trackball;
                candidate.axis = EditorTranslateAxisUVE::X;
                candidate.entity = m_selectedEntity;
                candidate.initialLocalTransform =
                    entityManager.GetComponentUVE<Scene::TransformComponentUVE>(m_selectedEntity);
                candidate.initialPointer = pointerPosition;
                candidate.screenCenter = center;
                candidate.initialTrackballVector = initialTrackballVector;
                candidate.viewWorldRotation = viewRotation;
                candidate.trackballRadiusPixels = kTrackballRadiusPixelsUVE;
                candidate.viewportRect = viewportRect;
                candidate.initialDirty = m_sceneDirty;
            }
        }
    }

    if (candidate.axis == EditorTranslateAxisUVE::None) {
        return false;
    }
    m_gizmoDrag = candidate;
    return true;
}

void EditorUVE::UpdateGizmoDragUVE(const Math::Vector2UVE pointerPosition) {
    if (!IsAuthoringCommandAllowedUVE() ||
        (m_gizmoDrag.axis == EditorTranslateAxisUVE::None &&
         m_gizmoDrag.handleKind != GizmoHandleKindUVE::UniformScaleOffset) ||
        !HasSingleDocumentSelectionUVE() || !IsDocumentEntityUVE(m_gizmoDrag.entity) ||
        m_gizmoDrag.entity != m_selectedEntity || !IsFiniteUVE(pointerPosition.x) ||
        !IsFiniteUVE(pointerPosition.y)) {
        CancelGizmoDragUVE();
        return;
    }

    if (m_gizmoDrag.mode == EditorGizmoModeUVE::Rotate &&
        m_gizmoDrag.handleKind == GizmoHandleKindUVE::Trackball) {
        Math::Vector3UVE currentTrackballVector{};
        if (!MapTrackballPointerUVE(m_gizmoDrag.screenCenter, m_gizmoDrag.trackballRadiusPixels,
                                    pointerPosition, currentTrackballVector)) {
            CancelGizmoDragUVE();
            return;
        }
        const float dot = std::clamp(Math::DotUVE(m_gizmoDrag.initialTrackballVector, currentTrackballVector), -1.0F, 1.0F);
        const Math::Vector3UVE cameraAxis = Math::CrossUVE(m_gizmoDrag.initialTrackballVector, currentTrackballVector);
        const float axisLengthSquared = Math::LengthSquaredUVE(cameraAxis);
        if (!IsFiniteUVE(dot) || !IsFiniteVectorUVE(cameraAxis) || axisLengthSquared <= kVectorEpsilonUVE) {
            if (dot <= kTrackballAntipodalDotThresholdUVE) {
                CancelGizmoDragUVE();
                return;
            }
            if (ApplyLocalTransformUVE(m_gizmoDrag.entity, m_gizmoDrag.initialLocalTransform)) {
                m_sceneDirty = m_gizmoDrag.initialDirty;
            }
            return;
        }
        const float inverseAxisLength = 1.0F / std::sqrt(axisLengthSquared);
        const Math::Vector3UVE normalizedCameraAxis = cameraAxis * inverseAxisLength;
        if (!IsFiniteVectorUVE(normalizedCameraAxis)) {
            CancelGizmoDragUVE();
            return;
        }
        const float radians = std::acos(dot);
        const float rotateStepRadians =
            (m_transformSnappingSettings.rotateStepDegrees * std::numbers::pi_v<float>) / 180.0F;
        const float snappedRadians = m_transformSnappingSettings.enabled
                                         ? SnapScalarUVE(radians, rotateStepRadians)
                                         : radians;
        const Math::Vector3UVE worldAxis = Math::RotateVectorUVE(m_gizmoDrag.viewWorldRotation, normalizedCameraAxis);
        Math::QuaternionUVE localRotation{};
        if (!IsFiniteUVE(snappedRadians) || !IsFiniteVectorUVE(worldAxis) ||
            !ComputeLocalRotationForWorldAxisUVE(m_gizmoDrag.entity,
                                                 m_gizmoDrag.initialLocalTransform.localRotation,
                                                 worldAxis, snappedRadians, localRotation)) {
            CancelGizmoDragUVE();
            return;
        }
        Scene::TransformComponentUVE updated = m_gizmoDrag.initialLocalTransform;
        updated.localRotation = localRotation;
        if (!ApplyLocalTransformUVE(m_gizmoDrag.entity, updated)) {
            CancelGizmoDragUVE();
            return;
        }
        m_sceneDirty = true;
        return;
    }

    if (m_gizmoDrag.mode == EditorGizmoModeUVE::Rotate) {
        float currentParameterRadians = 0.0F;
        float distanceSquared = 0.0F;
        if (!FindClosestRingParameterUVE(m_gizmoDrag.viewportRect, m_gizmoDrag.entity, m_gizmoDrag.axis,
                                         pointerPosition, currentParameterRadians, distanceSquared)) {
            CancelGizmoDragUVE();
            return;
        }
        const float deltaRadians = std::remainder(
            currentParameterRadians - m_gizmoDrag.initialRingParameterRadians, std::numbers::pi_v<float> * 2.0F);
        const float rotateStepRadians =
            (m_transformSnappingSettings.rotateStepDegrees * std::numbers::pi_v<float>) / 180.0F;
        const float snappedDeltaRadians = m_transformSnappingSettings.enabled
                                              ? SnapScalarUVE(deltaRadians, rotateStepRadians)
                                              : deltaRadians;
        Math::QuaternionUVE localRotation{};
        if (!IsFiniteUVE(snappedDeltaRadians) ||
            !ComputeLocalRotationForWorldAxisUVE(m_gizmoDrag.entity,
                                                 m_gizmoDrag.initialLocalTransform.localRotation,
                                                 m_gizmoDrag.worldAxisA, snappedDeltaRadians, localRotation)) {
            CancelGizmoDragUVE();
            return;
        }
        Scene::TransformComponentUVE updated = m_gizmoDrag.initialLocalTransform;
        updated.localRotation = localRotation;
        if (!ApplyLocalTransformUVE(m_gizmoDrag.entity, updated)) {
            CancelGizmoDragUVE();
            return;
        }
        m_sceneDirty = true;
        return;
    }

    if (m_gizmoDrag.handleKind == GizmoHandleKindUVE::UniformScaleOffset) {
        const Math::Vector2UVE initialOffset{m_gizmoDrag.initialPointer.x - m_gizmoDrag.screenCenter.x,
                                             m_gizmoDrag.initialPointer.y - m_gizmoDrag.screenCenter.y};
        const Math::Vector2UVE currentOffset{pointerPosition.x - m_gizmoDrag.screenCenter.x,
                                              pointerPosition.y - m_gizmoDrag.screenCenter.y};
        const float initialRadius = std::sqrt(LengthSquared2UVE(initialOffset));
        const float currentRadius = std::sqrt(LengthSquared2UVE(currentOffset));
        const float rawOffset = (currentRadius - initialRadius) / m_gizmoDrag.pixelsPerWorldUnit;
        const float uniformOffset = m_transformSnappingSettings.enabled
                                        ? SnapScalarUVE(rawOffset, m_transformSnappingSettings.scaleStep)
                                        : rawOffset;
        Scene::TransformComponentUVE updated = m_gizmoDrag.initialLocalTransform;
        updated.localScale.x += uniformOffset;
        updated.localScale.y += uniformOffset;
        updated.localScale.z += uniformOffset;
        if (!IsFiniteUVE(uniformOffset) || !IsFiniteUVE(updated.localScale.x) ||
            !IsFiniteUVE(updated.localScale.y) || !IsFiniteUVE(updated.localScale.z) ||
            updated.localScale.x < kMinimumLocalScaleUVE || updated.localScale.y < kMinimumLocalScaleUVE ||
            updated.localScale.z < kMinimumLocalScaleUVE || !ApplyLocalTransformUVE(m_gizmoDrag.entity, updated)) {
            CancelGizmoDragUVE();
            return;
        }
        m_sceneDirty = true;
        return;
    }

    if (m_gizmoDrag.handleKind == GizmoHandleKindUVE::Plane) {
        const Math::Vector2UVE pointerDelta{pointerPosition.x - m_gizmoDrag.initialPointer.x,
                                             pointerPosition.y - m_gizmoDrag.initialPointer.y};
        const float determinant = (m_gizmoDrag.screenPlaneAxisA.x * m_gizmoDrag.screenPlaneAxisB.y) -
                                  (m_gizmoDrag.screenPlaneAxisA.y * m_gizmoDrag.screenPlaneAxisB.x);
        if (!IsFiniteUVE(determinant) || std::abs(determinant) <= kVectorEpsilonUVE) {
            CancelGizmoDragUVE();
            return;
        }
        float distanceA = (((pointerDelta.x * m_gizmoDrag.screenPlaneAxisB.y) -
                            (pointerDelta.y * m_gizmoDrag.screenPlaneAxisB.x)) / determinant) * kGizmoAxisLengthUVE;
        float distanceB = (((m_gizmoDrag.screenPlaneAxisA.x * pointerDelta.y) -
                            (m_gizmoDrag.screenPlaneAxisA.y * pointerDelta.x)) / determinant) * kGizmoAxisLengthUVE;
        if (m_transformSnappingSettings.enabled) {
            distanceA = SnapScalarUVE(distanceA, m_transformSnappingSettings.translateStep);
            distanceB = SnapScalarUVE(distanceB, m_transformSnappingSettings.translateStep);
        }
        Math::Vector3UVE localDelta{};
        if (!IsFiniteUVE(distanceA) || !IsFiniteUVE(distanceB) ||
            !ComputeLocalDeltaForWorldDeltaUVE(m_gizmoDrag.entity,
                (m_gizmoDrag.worldAxisA * distanceA) + (m_gizmoDrag.worldAxisB * distanceB), localDelta)) {
            CancelGizmoDragUVE();
            return;
        }
        Scene::TransformComponentUVE updated = m_gizmoDrag.initialLocalTransform;
        updated.localPosition += localDelta;
        if (!ApplyLocalTransformUVE(m_gizmoDrag.entity, updated)) {
            CancelGizmoDragUVE();
            return;
        }
        m_sceneDirty = true;
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

    if (m_gizmoDrag.mode == EditorGizmoModeUVE::Scale) {
        const float snappedWorldDistance = m_transformSnappingSettings.enabled
                                               ? SnapScalarUVE(worldDistance, m_transformSnappingSettings.scaleStep)
                                               : worldDistance;
        Scene::TransformComponentUVE updated = m_gizmoDrag.initialLocalTransform;
        float* component = nullptr;
        switch (m_gizmoDrag.axis) {
            case EditorTranslateAxisUVE::X:
                component = &updated.localScale.x;
                break;
            case EditorTranslateAxisUVE::Y:
                component = &updated.localScale.y;
                break;
            case EditorTranslateAxisUVE::Z:
                component = &updated.localScale.z;
                break;
            case EditorTranslateAxisUVE::None:
                CancelGizmoDragUVE();
                return;
        }
        *component += snappedWorldDistance;
        if (!IsFiniteUVE(*component) || *component < kMinimumLocalScaleUVE ||
            !ApplyLocalTransformUVE(m_gizmoDrag.entity, updated)) {
            CancelGizmoDragUVE();
            return;
        }
        m_sceneDirty = true;
        return;
    }

    const float snappedWorldDistance = m_transformSnappingSettings.enabled
                                           ? SnapScalarUVE(worldDistance, m_transformSnappingSettings.translateStep)
                                           : worldDistance;
    Math::Vector3UVE localDelta{};
    if (!ComputeLocalDeltaForWorldDeltaUVE(
            m_gizmoDrag.entity, m_gizmoDrag.worldAxisA * snappedWorldDistance, localDelta)) {
        CancelGizmoDragUVE();
        return;
    }

    Scene::TransformComponentUVE updated = m_gizmoDrag.initialLocalTransform;
    updated.localPosition += localDelta;
    if (!ApplyLocalTransformUVE(m_gizmoDrag.entity, updated)) {
        CancelGizmoDragUVE();
        return;
    }
    m_sceneDirty = true;
}

void EditorUVE::CommitGizmoDragUVE() {
    const GizmoDragUVE completedDrag = m_gizmoDrag;
    m_gizmoDrag = GizmoDragUVE{};
    if ((completedDrag.axis == EditorTranslateAxisUVE::None &&
         completedDrag.handleKind != GizmoHandleKindUVE::UniformScaleOffset) ||
        !IsDocumentEntityUVE(completedDrag.entity)) {
        return;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::TransformComponentUVE>(completedDrag.entity)) {
        return;
    }

    const Scene::TransformComponentUVE after =
        entityManager.GetComponentUVE<Scene::TransformComponentUVE>(completedDrag.entity);
    if (AreTransformsEqualUVE(completedDrag.initialLocalTransform, after)) {
        m_sceneDirty = completedDrag.initialDirty;
        return;
    }

    RecordHistoryUVE(TransformHistoryEntryUVE{completedDrag.entity,
                                               completedDrag.initialLocalTransform,
                                               after,
                                               EditorSelectionSnapshotUVE{{completedDrag.entity}, completedDrag.entity},
                                               CaptureSelectionSnapshotUVE(),
                                               completedDrag.initialDirty,
                                               true});
}

void EditorUVE::CancelGizmoDragUVE() noexcept {
    const GizmoDragUVE cancelledDrag = m_gizmoDrag;
    m_gizmoDrag = GizmoDragUVE{};
    if ((cancelledDrag.axis == EditorTranslateAxisUVE::None &&
         cancelledDrag.handleKind != GizmoHandleKindUVE::UniformScaleOffset) ||
        !IsDocumentEntityUVE(cancelledDrag.entity)) {
        return;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::TransformComponentUVE>(cancelledDrag.entity)) {
        return;
    }

    m_services->GetSceneGraphUVE().SetLocalTransformUVE(
        entityManager, cancelledDrag.entity, cancelledDrag.initialLocalTransform);
    m_sceneDirty = cancelledDrag.initialDirty;
}

void EditorUVE::DrawSelectionBoundsUVE(const EditorViewportRectUVE& viewportRect) {
    if (!IsViewportRectValidUVE(viewportRect)) {
        return;
    }

    constexpr std::array<std::array<std::size_t, 2>, 12> kBoxEdgesUVE{
        std::array<std::size_t, 2>{0U, 1U},
        std::array<std::size_t, 2>{1U, 2U},
        std::array<std::size_t, 2>{2U, 3U},
        std::array<std::size_t, 2>{3U, 0U},
        std::array<std::size_t, 2>{4U, 5U},
        std::array<std::size_t, 2>{5U, 6U},
        std::array<std::size_t, 2>{6U, 7U},
        std::array<std::size_t, 2>{7U, 4U},
        std::array<std::size_t, 2>{0U, 4U},
        std::array<std::size_t, 2>{1U, 5U},
        std::array<std::size_t, 2>{2U, 6U},
        std::array<std::size_t, 2>{3U, 7U},
    };

    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    for (const Scene::EntityUVE entity : m_selectedEntities) {
        const std::optional<EditorSelectionBoundsUVE> bounds = TryGetEntityBoundsUVE(entity);
        if (!bounds.has_value()) {
            continue;
        }

        std::array<Math::Vector2UVE, 8> projectedCorners{};
        bool projected = true;
        for (std::size_t index = 0U; index < bounds->worldCorners.size(); ++index) {
            projected = ProjectWorldPointUVE(viewportRect, bounds->worldCorners[index], projectedCorners[index]) && projected;
        }
        Math::Vector2UVE projectedCenter{};
        projected = ProjectWorldPointUVE(viewportRect, bounds->worldCenter, projectedCenter) && projected;
        if (!projected) {
            continue;
        }

        const bool active = entity == m_selectedEntity;
        const ImU32 boundsColor = active ? IM_COL32(255, 218, 75, 250) : IM_COL32(0, 212, 255, 180);
        const ImU32 cornerColor = active ? IM_COL32(255, 244, 190, 255) : IM_COL32(185, 248, 255, 205);
        const float thickness = active ? 2.75F : 1.5F;
        for (const std::array<std::size_t, 2>& edge : kBoxEdgesUVE) {
            const Math::Vector2UVE& first = projectedCorners[edge[0]];
            const Math::Vector2UVE& second = projectedCorners[edge[1]];
            drawList->AddLine(ImVec2{first.x, first.y}, ImVec2{second.x, second.y}, boundsColor, thickness);
        }
        for (const Math::Vector2UVE& corner : projectedCorners) {
            drawList->AddCircleFilled(ImVec2{corner.x, corner.y}, active ? 3.5F : 2.5F, cornerColor, 8);
        }
        drawList->AddCircleFilled(ImVec2{projectedCenter.x, projectedCenter.y}, active ? 4.5F : 3.5F,
                                  boundsColor, 12);
    }
}

void EditorUVE::DrawTranslateGizmoUVE(const EditorViewportRectUVE& viewportRect) {
    if (!HasSingleDocumentSelectionUVE() || !IsViewportRectValidUVE(viewportRect)) {
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

    constexpr std::array<EditorTranslatePlaneUVE, 3> planes{
        EditorTranslatePlaneUVE::XY, EditorTranslatePlaneUVE::XZ, EditorTranslatePlaneUVE::YZ};
    for (const EditorTranslatePlaneUVE plane : planes) {
        const EditorTranslateAxisUVE axisA = plane == EditorTranslatePlaneUVE::YZ ? EditorTranslateAxisUVE::Y : EditorTranslateAxisUVE::X;
        const EditorTranslateAxisUVE axisB = plane == EditorTranslatePlaneUVE::XY ? EditorTranslateAxisUVE::Y : EditorTranslateAxisUVE::Z;
        Math::Vector3UVE worldAxisA{};
        Math::Vector3UVE worldAxisB{};
        if (!GetGizmoAxisWorldVectorUVE(m_selectedEntity, axisA, worldAxisA) ||
            !GetGizmoAxisWorldVectorUVE(m_selectedEntity, axisB, worldAxisB)) {
            continue;
        }
        std::array<Math::Vector2UVE, 4> corners{};
        const std::array<Math::Vector3UVE, 4> worldCorners{
            selectedWorld.worldPosition + worldAxisA * (0.20F * kGizmoAxisLengthUVE) + worldAxisB * (0.20F * kGizmoAxisLengthUVE),
            selectedWorld.worldPosition + worldAxisA * (0.60F * kGizmoAxisLengthUVE) + worldAxisB * (0.20F * kGizmoAxisLengthUVE),
            selectedWorld.worldPosition + worldAxisA * (0.60F * kGizmoAxisLengthUVE) + worldAxisB * (0.60F * kGizmoAxisLengthUVE),
            selectedWorld.worldPosition + worldAxisA * (0.20F * kGizmoAxisLengthUVE) + worldAxisB * (0.60F * kGizmoAxisLengthUVE)};
        if (!ProjectWorldPointUVE(viewportRect, worldCorners[0], corners[0]) ||
            !ProjectWorldPointUVE(viewportRect, worldCorners[1], corners[1]) ||
            !ProjectWorldPointUVE(viewportRect, worldCorners[2], corners[2]) ||
            !ProjectWorldPointUVE(viewportRect, worldCorners[3], corners[3])) {
            continue;
        }
        const ImU32 fill = plane == EditorTranslatePlaneUVE::XY ? IM_COL32(220, 170, 60, 65) :
                           plane == EditorTranslatePlaneUVE::XZ ? IM_COL32(190, 80, 170, 65) : IM_COL32(75, 170, 175, 65);
        drawList->AddQuadFilled(ImVec2{corners[0].x, corners[0].y}, ImVec2{corners[1].x, corners[1].y},
                                ImVec2{corners[2].x, corners[2].y}, ImVec2{corners[3].x, corners[3].y}, fill);
    }

    constexpr std::array<EditorTranslateAxisUVE, 3> axes{
        EditorTranslateAxisUVE::X,
        EditorTranslateAxisUVE::Y,
        EditorTranslateAxisUVE::Z,
    };
    for (const EditorTranslateAxisUVE axis : axes) {
        Math::Vector3UVE worldAxis{};
        Math::Vector2UVE endpoint{};
        if (!GetGizmoAxisWorldVectorUVE(m_selectedEntity, axis, worldAxis) ||
            !ProjectWorldPointUVE(viewportRect, selectedWorld.worldPosition + worldAxis * kGizmoAxisLengthUVE, endpoint)) {
            continue;
        }
        const bool active = m_gizmoDrag.axis == axis;
        const ImU32 color = GizmoAxisColorUVE(axis, active);
        const ImVec2 endpointPoint{endpoint.x, endpoint.y};
        drawList->AddLine(centerPoint, endpointPoint, color, active ? 4.0F : 2.5F);
        drawList->AddCircleFilled(endpointPoint, active ? 6.5F : 5.0F, color, 12);
    }
}

void EditorUVE::DrawScaleGizmoUVE(const EditorViewportRectUVE& viewportRect) {
    if (!HasSingleDocumentSelectionUVE() || !IsViewportRectValidUVE(viewportRect)) {
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

    constexpr std::array<EditorTranslateAxisUVE, 3> axes{
        EditorTranslateAxisUVE::X,
        EditorTranslateAxisUVE::Y,
        EditorTranslateAxisUVE::Z,
    };
    ImDrawList* const drawList = ImGui::GetForegroundDrawList();
    const ImVec2 centerPoint{center.x, center.y};
    const bool uniformActive = m_gizmoDrag.mode == EditorGizmoModeUVE::Scale &&
                               m_gizmoDrag.handleKind == GizmoHandleKindUVE::UniformScaleOffset;
    const float uniformHalfSize = uniformActive ? 6.5F : 4.0F;
    drawList->AddRectFilled(ImVec2{center.x - uniformHalfSize, center.y - uniformHalfSize},
                            ImVec2{center.x + uniformHalfSize, center.y + uniformHalfSize},
                            uniformActive ? IM_COL32(255, 230, 90, 245) : IM_COL32(235, 235, 235, 220));
    for (const EditorTranslateAxisUVE axis : axes) {
        Math::Vector3UVE worldAxis{};
        Math::Vector2UVE endpoint{};
        if (!GetGizmoAxisWorldVectorUVE(m_selectedEntity, axis, worldAxis) ||
            !ProjectWorldPointUVE(viewportRect, selectedWorld.worldPosition + worldAxis * kGizmoAxisLengthUVE, endpoint)) {
            continue;
        }
        const bool active = m_gizmoDrag.mode == EditorGizmoModeUVE::Scale &&
                            m_gizmoDrag.handleKind == GizmoHandleKindUVE::Axis && m_gizmoDrag.axis == axis;
        const ImU32 color = GizmoAxisColorUVE(axis, active);
        const ImVec2 endpointPoint{endpoint.x, endpoint.y};
        const float halfSize = active ? 6.5F : 5.0F;
        drawList->AddLine(centerPoint, endpointPoint, color, active ? 4.0F : 2.5F);
        drawList->AddRectFilled(ImVec2{endpoint.x - halfSize, endpoint.y - halfSize},
                                ImVec2{endpoint.x + halfSize, endpoint.y + halfSize}, color);
    }
}

void EditorUVE::DrawRotateGizmoUVE(const EditorViewportRectUVE& viewportRect) {
    if (!HasSingleDocumentSelectionUVE() || !IsViewportRectValidUVE(viewportRect)) {
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

    constexpr std::array<EditorTranslateAxisUVE, 3> axes{
        EditorTranslateAxisUVE::X,
        EditorTranslateAxisUVE::Y,
        EditorTranslateAxisUVE::Z,
    };
    constexpr int kRingSegmentCountUVE = 64;
    constexpr float kFullTurnRadiansUVE = std::numbers::pi_v<float> * 2.0F;
    const float segmentRadians = kFullTurnRadiansUVE / static_cast<float>(kRingSegmentCountUVE);
    ImDrawList* const drawList = ImGui::GetForegroundDrawList();
    const bool trackballActive = m_gizmoDrag.mode == EditorGizmoModeUVE::Rotate &&
                                 m_gizmoDrag.handleKind == GizmoHandleKindUVE::Trackball;
    drawList->AddCircleFilled(ImVec2{center.x, center.y}, kTrackballRadiusPixelsUVE,
                              trackballActive ? IM_COL32(255, 232, 110, 64) : IM_COL32(210, 220, 235, 35), 32);
    drawList->AddCircle(ImVec2{center.x, center.y}, kTrackballRadiusPixelsUVE,
                        trackballActive ? IM_COL32(255, 232, 110, 235) : IM_COL32(210, 220, 235, 145), 32,
                        trackballActive ? 2.5F : 1.25F);

    for (const EditorTranslateAxisUVE axis : axes) {
        Math::Vector3UVE first{};
        Math::Vector3UVE second{};
        if (!GetRingBasisUVE(axis, first, second)) {
            continue;
        }
        if (m_gizmoCoordinateSpace == EditorGizmoCoordinateSpaceUVE::Local) {
            Math::QuaternionUVE rotation{};
            if (!Math::TryNormalizeUVE(selectedWorld.worldRotation, rotation)) {
                continue;
            }
            first = Math::RotateVectorUVE(rotation, first);
            second = Math::RotateVectorUVE(rotation, second);
        }

        const bool active = m_gizmoDrag.mode == EditorGizmoModeUVE::Rotate &&
                            m_gizmoDrag.handleKind == GizmoHandleKindUVE::Axis && m_gizmoDrag.axis == axis;
        const ImU32 color = GizmoAxisColorUVE(axis, active);
        for (int segment = 0; segment < kRingSegmentCountUVE; ++segment) {
            const float startParameter = static_cast<float>(segment) * segmentRadians;
            Math::Vector2UVE start{};
            Math::Vector2UVE end{};
            if (!ProjectWorldPointUVE(
                    viewportRect,
                    MakeRingPointUVE(selectedWorld.worldPosition, first, second, startParameter), start) ||
                !ProjectWorldPointUVE(
                    viewportRect,
                    MakeRingPointUVE(selectedWorld.worldPosition, first, second,
                                     startParameter + segmentRadians),
                    end)) {
                continue;
            }
            drawList->AddLine(ImVec2{start.x, start.y}, ImVec2{end.x, end.y}, color, active ? 4.0F : 2.5F);
        }
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

    ImGuiIO& io = ImGui::GetIO();
    const bool lifecycleCommandAllowed = IsLifecycleCommandAllowedUVE() && IsDocumentEntityUVE(m_selectedEntity);
    const bool gizmoModeChangeAllowed = IsAuthoringCommandAllowedUVE() &&
                                        m_gizmoDrag.axis == EditorTranslateAxisUVE::None &&
                                        m_viewportNavigationMode == EditorViewportNavigationModeUVE::None;
    const bool canEnterPlayMode = m_simulationControl != nullptr &&
                                  m_playModeState == EditorPlayModeStateUVE::Edit &&
                                  m_gizmoDrag.axis == EditorTranslateAxisUVE::None &&
                                  m_viewportNavigationMode == EditorViewportNavigationModeUVE::None;
    if (!io.WantTextInput && canEnterPlayMode && ImGui::IsKeyPressed(ImGuiKey_F5, false)) {
        static_cast<void>(EnterPlayModeUVE());
    } else if (!io.WantTextInput && m_playModeState == EditorPlayModeStateUVE::Playing &&
               ImGui::IsKeyPressed(ImGuiKey_F6, false)) {
        static_cast<void>(PausePlayModeUVE());
    } else if (!io.WantTextInput && m_playModeState == EditorPlayModeStateUVE::Paused &&
               ImGui::IsKeyPressed(ImGuiKey_F6, false)) {
        static_cast<void>(ResumePlayModeUVE());
    } else if (!io.WantTextInput && m_playModeState == EditorPlayModeStateUVE::Paused &&
               ImGui::IsKeyPressed(ImGuiKey_F10, false)) {
        static_cast<void>(StepPlayModeUVE());
    } else if (!io.WantTextInput && m_playModeState != EditorPlayModeStateUVE::Edit && io.KeyShift &&
               ImGui::IsKeyPressed(ImGuiKey_F5, false)) {
        static_cast<void>(StopPlayModeUVE());
    } else if (!io.WantTextInput && gizmoModeChangeAllowed && ImGui::IsKeyPressed(ImGuiKey_W, false)) {
        SetGizmoModeUVE(EditorGizmoModeUVE::Translate);
    } else if (!io.WantTextInput && gizmoModeChangeAllowed && ImGui::IsKeyPressed(ImGuiKey_E, false)) {
        SetGizmoModeUVE(EditorGizmoModeUVE::Rotate);
    } else if (!io.WantTextInput && gizmoModeChangeAllowed && ImGui::IsKeyPressed(ImGuiKey_R, false)) {
        SetGizmoModeUVE(EditorGizmoModeUVE::Scale);
    } else if (!io.WantTextInput && io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
        if (io.KeyShift) {
            static_cast<void>(RedoUVE());
        } else {
            static_cast<void>(UndoUVE());
        }
    } else if (!io.WantTextInput && io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) {
        static_cast<void>(RedoUVE());
    } else if (!io.WantTextInput && lifecycleCommandAllowed && io.KeyCtrl &&
               ImGui::IsKeyPressed(ImGuiKey_D, false)) {
        static_cast<void>(DuplicateSelectedEntityUVE());
    } else if (!io.WantTextInput && lifecycleCommandAllowed && ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
        static_cast<void>(DeleteSelectedEntityUVE());
    }

    ImGui::TextUnformatted("UNIVEX");
    ImGui::SameLine();
    const auto drawWorkspaceTab = [this](const char* const label, const EditorWorkspaceUVE workspace) {
        const bool active = m_activeWorkspace == workspace;
        if (ImGui::Selectable(label, active, ImGuiSelectableFlags_DontClosePopups, ImVec2{0.0F, 0.0F})) {
            m_activeWorkspace = workspace;
        }
        ImGui::SameLine();
    };
    drawWorkspaceTab("Library", EditorWorkspaceUVE::Library);
    drawWorkspaceTab("Asset", EditorWorkspaceUVE::Asset);
    drawWorkspaceTab("Scripting", EditorWorkspaceUVE::Scripting);
    drawWorkspaceTab("Debug", EditorWorkspaceUVE::Debug);
    const bool pluginActive = m_activeWorkspace == EditorWorkspaceUVE::Plugin;
    if (ImGui::Selectable("Plugin", pluginActive, ImGuiSelectableFlags_DontClosePopups, ImVec2{0.0F, 0.0F})) {
        m_activeWorkspace = EditorWorkspaceUVE::Plugin;
    }

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    if (m_playModeState == EditorPlayModeStateUVE::Edit) {
        if (ImGui::SmallButton(">") && canEnterPlayMode) {
            static_cast<void>(EnterPlayModeUVE());
        }
    } else if (m_playModeState == EditorPlayModeStateUVE::Playing) {
        if (ImGui::SmallButton("||")) {
            static_cast<void>(PausePlayModeUVE());
        }
    } else if (ImGui::SmallButton(">")) {
        static_cast<void>(ResumePlayModeUVE());
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(m_playModeState == EditorPlayModeStateUVE::Edit);
    if (ImGui::SmallButton("[]")) {
        static_cast<void>(StopPlayModeUVE());
    }
    ImGui::EndDisabled();

    ImGui::EndMainMenuBar();
}

void EditorUVE::DrawBottomDockUVE() {
    const ImGuiViewport* const mainViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(
        ImVec2{mainViewport->WorkPos.x, mainViewport->WorkPos.y + mainViewport->WorkSize.y - kBottomDockTabHeightUVE},
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2{mainViewport->WorkSize.x, kBottomDockTabHeightUVE}, ImGuiCond_Always);
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                       ImGuiWindowFlags_NoTitleBar;
    ImGui::Begin("##bottom-dock", nullptr, flags);
    const auto drawDockTab = [this](const char* const label, const EditorBottomDockUVE dock) {
        const bool active = m_activeBottomDock == dock;
        if (ImGui::Selectable(label, active, ImGuiSelectableFlags_DontClosePopups, ImVec2{0.0F, 0.0F})) {
            m_activeBottomDock = dock;
        }
        ImGui::SameLine();
    };
    drawDockTab("Debugger", EditorBottomDockUVE::Debugger);
    drawDockTab("Animator", EditorBottomDockUVE::Animator);
    drawDockTab("AI Toolbar", EditorBottomDockUVE::AIToolbar);
    const bool fileSystemActive = m_activeBottomDock == EditorBottomDockUVE::FileSystem;
    if (ImGui::Selectable("FileSystem", fileSystemActive, ImGuiSelectableFlags_DontClosePopups, ImVec2{0.0F, 0.0F})) {
        m_activeBottomDock = EditorBottomDockUVE::FileSystem;
    }
    ImGui::SameLine();
    ImGui::BeginDisabled();
    static_cast<void>(ImGui::Selectable("+ Add Dock", false, ImGuiSelectableFlags_DontClosePopups, ImVec2{0.0F, 0.0F}));
    ImGui::EndDisabled();
    ImGui::End();
}

void EditorUVE::DrawBottomDockContentUVE() {
    if (m_activeBottomDock == EditorBottomDockUVE::FileSystem) {
        DrawAssetsPanelUVE();
        return;
    }

    const ImGuiViewport* const mainViewport = ImGui::GetMainViewport();
    const float contentHeight = kAssetsPanelHeightUVE - kBottomDockTabHeightUVE;
    ImGui::SetNextWindowPos(
        ImVec2{mainViewport->WorkPos.x, mainViewport->WorkPos.y + mainViewport->WorkSize.y -
                                      kAssetsPanelHeightUVE},
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2{mainViewport->WorkSize.x, contentHeight}, ImGuiCond_Always);
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    ImGui::Begin("Lower Workspace", nullptr, flags);
    switch (m_activeBottomDock) {
        case EditorBottomDockUVE::Debugger:
            ImGui::TextUnformatted("Debugger workspace");
            ImGui::TextDisabled("Runtime diagnostics will appear here while Play Mode is active.");
            break;
        case EditorBottomDockUVE::Animator:
            ImGui::TextUnformatted("Animator workspace");
            ImGui::TextDisabled("Animation authoring remains a future editor capability.");
            break;
        case EditorBottomDockUVE::AIToolbar:
            ImGui::TextUnformatted("AI Toolbar workspace");
            ImGui::TextDisabled("AI-assisted editor tools are intentionally separate from document state.");
            break;
        case EditorBottomDockUVE::FileSystem:
            break;
    }
    ImGui::End();
}

bool EditorUVE::IsHierarchyFilterActiveUVE() const noexcept {
    return !m_hierarchyFilter.empty();
}

bool EditorUVE::IsHierarchyEntityVisibleUVE(const Scene::EntityUVE entity) const {
    return !IsHierarchyFilterActiveUVE() ||
           std::find(m_cachedHierarchyVisibleEntities.begin(), m_cachedHierarchyVisibleEntities.end(), entity) !=
               m_cachedHierarchyVisibleEntities.end();
}

void EditorUVE::InvalidateHierarchyFilterCacheUVE() noexcept {
    m_hierarchyFilterCacheDirty = true;
}

void EditorUVE::CancelHierarchyRenameUVE() noexcept {
    m_hierarchyRenameEntity = Scene::kInvalidEntityUVE;
    m_hierarchyRenameBuffer.clear();
    m_hierarchyRenameFocusRequested = false;
}

void EditorUVE::RebuildHierarchyFilterCacheUVE() {
    if (!m_hierarchyFilterCacheDirty && m_cachedHierarchyFilter == m_hierarchyFilter) {
        return;
    }
    m_cachedHierarchyVisibleEntities.clear();
    m_cachedHierarchyFilter = m_hierarchyFilter;
    m_hierarchyFilterCacheDirty = false;
    if (!IsHierarchyFilterActiveUVE()) {
        return;
    }
    const auto visit = [this](const auto& self, const Scene::EntityUVE entity) -> bool {
        if (!IsDocumentEntityUVE(entity)) {
            return false;
        }
        bool visible = ContainsCaseInsensitiveUVE(GetEntityDisplayLabelUVE(entity), m_hierarchyFilter);
        Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
        for (const Scene::EntityUVE child : m_services->GetSceneGraphUVE().GetChildrenUVE(entityManager, entity)) {
            visible = self(self, child) || visible;
        }
        if (visible) {
            m_cachedHierarchyVisibleEntities.push_back(entity);
        }
        return visible;
    };
    for (const Scene::EntityUVE root : GetDocumentRootsUVE()) {
        static_cast<void>(visit(visit, root));
    }
}

void EditorUVE::DrawHierarchyPanelUVE() {
    const ImGuiViewport* const mainViewport = ImGui::GetMainViewport();
    const float menuBarHeight = ImGui::GetFrameHeight();
    const float workspaceHeight = std::max(kMinimumViewportHeightUVE,
                                                  mainViewport->WorkSize.y - menuBarHeight - kAssetsPanelHeightUVE);
    ImGui::SetNextWindowPos(ImVec2{mainViewport->WorkPos.x, mainViewport->WorkPos.y + menuBarHeight},
                            ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2{250.0F, workspaceHeight}, ImGuiCond_Always);
    ImGui::Begin("Scene");
    std::array<char, 256> filterBuffer{};
    m_hierarchyFilter.copy(filterBuffer.data(), filterBuffer.size() - 1U);
    if (ImGui::InputTextWithHint("##hierarchy-filter", "Filter entities...", filterBuffer.data(), filterBuffer.size())) {
        m_hierarchyFilter = filterBuffer.data();
        InvalidateHierarchyFilterCacheUVE();
    }
    RebuildHierarchyFilterCacheUVE();
    ImGui::BeginDisabled(!IsAuthoringCommandAllowedUVE());
    for (const Scene::EntityUVE root : GetDocumentRootsUVE()) {
        DrawHierarchyNodeUVE(root);
    }
    ImGui::Separator();
    ImGui::TextDisabled("Drop entity here to make it a root");
    AcceptHierarchyDropTargetUVE(Scene::kInvalidEntityUVE);
    ImGui::EndDisabled();
    ImGui::End();
}

void EditorUVE::DrawHierarchyNodeUVE(const Scene::EntityUVE entity) {
    if (!IsHierarchyEntityVisibleUVE(entity)) {
        return;
    }
    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    const std::vector<Scene::EntityUVE> children =
        m_services->GetSceneGraphUVE().GetChildrenUVE(entityManager, entity);
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (children.empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }
    const bool selected = IsEntitySelectedUVE(entity);
    const bool active = entity == m_selectedEntity;
    if (selected) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    if (IsHierarchyFilterActiveUVE()) {
        ImGui::SetNextItemOpen(true, ImGuiCond_Always);
    }

    const bool renaming = entity == m_hierarchyRenameEntity;
    const std::string nodeLabel = (renaming ? "" : GetEntityDisplayLabelUVE(entity)) + "##entity-" +
                                  std::to_string(entity.index) + ":" + std::to_string(entity.generation);
    if (active) {
        ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(55, 105, 160, 215));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, IM_COL32(70, 125, 180, 245));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(60, 110, 165, 220));
    }
    const bool open = ImGui::TreeNodeEx(nodeLabel.c_str(), flags);
    if (active) {
        ImGui::PopStyleColor(3);
    }
    if (ImGui::IsItemClicked() && !renaming) {
        if (ImGui::GetIO().KeyCtrl) {
            ToggleEntitySelectionUVE(entity);
        } else {
            SelectEntityUVE(entity);
        }
    }
    if (!renaming && HasSingleDocumentSelectionUVE() && entity == m_selectedEntity &&
        IsAuthoringCommandAllowedUVE() &&
        m_gizmoDrag.axis == EditorTranslateAxisUVE::None &&
        m_viewportNavigationMode == EditorViewportNavigationModeUVE::None && ImGui::IsKeyPressed(ImGuiKey_F2)) {
        m_hierarchyRenameEntity = entity;
        m_hierarchyRenameBuffer = GetEntityDisplayLabelUVE(entity);
        m_hierarchyRenameFocusRequested = true;
    }
    if (!renaming && HasSingleDocumentSelectionUVE() && entity == m_selectedEntity &&
        IsAuthoringCommandAllowedUVE() && m_gizmoDrag.axis == EditorTranslateAxisUVE::None &&
        m_viewportNavigationMode == EditorViewportNavigationModeUVE::None) {
        ImGui::SameLine();
        const std::string renameLabel = "Rename##entity-" + std::to_string(entity.index) + ":" +
                                        std::to_string(entity.generation);
        if (ImGui::SmallButton(renameLabel.c_str())) {
            m_hierarchyRenameEntity = entity;
            m_hierarchyRenameBuffer = GetEntityDisplayLabelUVE(entity);
            m_hierarchyRenameFocusRequested = true;
        }
    }
    if (renaming) {
        ImGui::SameLine();
        std::array<char, kMaximumEntityNameBytesUVE + 1U> renameBuffer{};
        m_hierarchyRenameBuffer.copy(renameBuffer.data(), renameBuffer.size() - 1U);
        if (m_hierarchyRenameFocusRequested) {
            ImGui::SetKeyboardFocusHere();
            m_hierarchyRenameFocusRequested = false;
        }
        const bool committed = ImGui::InputText("##hierarchy-rename", renameBuffer.data(), renameBuffer.size(),
                                                ImGuiInputTextFlags_EnterReturnsTrue);
        m_hierarchyRenameBuffer = renameBuffer.data();
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            CancelHierarchyRenameUVE();
        } else if (committed) {
            if (SetSelectedEntityNameUVE(m_hierarchyRenameBuffer)) {
                InvalidateHierarchyFilterCacheUVE();
            }
            CancelHierarchyRenameUVE();
        }
    }
    if (IsLifecycleCommandAllowedUVE() && IsDocumentEntityUVE(entity) && ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload(kHierarchyEntityPayloadUVE, &entity, sizeof(entity));
        ImGui::Text("Move %s", GetEntityDisplayLabelUVE(entity).c_str());
        ImGui::EndDragDropSource();
    }
    AcceptHierarchyDropTargetUVE(entity);
    if (open) {
        for (const Scene::EntityUVE child : children) {
            DrawHierarchyNodeUVE(child);
        }
        ImGui::TreePop();
    }
}

void EditorUVE::AcceptHierarchyDropTargetUVE(const Scene::EntityUVE targetParent) {
    if (!IsLifecycleCommandAllowedUVE() ||
        (targetParent != Scene::kInvalidEntityUVE && !IsDocumentEntityUVE(targetParent)) ||
        !ImGui::BeginDragDropTarget()) {
        return;
    }

    const ImGuiPayload* const payload = ImGui::AcceptDragDropPayload(kHierarchyEntityPayloadUVE);
    if (payload != nullptr && payload->DataSize == static_cast<int>(sizeof(Scene::EntityUVE))) {
        Scene::EntityUVE source = Scene::kInvalidEntityUVE;
        std::memcpy(&source, payload->Data, sizeof(source));
        static_cast<void>(ReparentDocumentEntityUVE(source, targetParent));
    }
    ImGui::EndDragDropTarget();
}

void EditorUVE::DrawInspectorPanelUVE() {
    const ImGuiViewport* const mainViewport = ImGui::GetMainViewport();
    const float menuBarHeight = ImGui::GetFrameHeight();
    const float workspaceHeight = std::max(kMinimumViewportHeightUVE,
                                           mainViewport->WorkSize.y - menuBarHeight - kAssetsPanelHeightUVE);
    ImGui::SetNextWindowPos(
        ImVec2{mainViewport->WorkPos.x + mainViewport->WorkSize.x - 330.0F, mainViewport->WorkPos.y + menuBarHeight},
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2{330.0F, workspaceHeight}, ImGuiCond_Always);
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;
    ImGui::Begin("##right-panel", nullptr, flags);

    const auto drawRightPanelTab = [this](const char* const label, const EditorRightPanelTabUVE tab) {
        const bool active = m_activeRightPanelTab == tab;
        if (ImGui::Selectable(label, active, ImGuiSelectableFlags_DontClosePopups, ImVec2{0.0F, 0.0F})) {
            m_activeRightPanelTab = tab;
        }
        ImGui::SameLine();
    };
    drawRightPanelTab("Inspector", EditorRightPanelTabUVE::Inspector);
    drawRightPanelTab("Import", EditorRightPanelTabUVE::Import);
    const bool signalsActive = m_activeRightPanelTab == EditorRightPanelTabUVE::Signals;
    if (ImGui::Selectable("Signals", signalsActive, ImGuiSelectableFlags_DontClosePopups, ImVec2{0.0F, 0.0F})) {
        m_activeRightPanelTab = EditorRightPanelTabUVE::Signals;
    }
    ImGui::Separator();

    switch (m_activeRightPanelTab) {
        case EditorRightPanelTabUVE::Inspector:
            DrawInspectorContentUVE();
            break;
        case EditorRightPanelTabUVE::Import:
            ImGui::TextUnformatted("Import");
            ImGui::TextDisabled("Asset import settings will appear for a selected imported asset.");
            break;
        case EditorRightPanelTabUVE::Signals:
            ImGui::TextUnformatted("Signals");
            ImGui::TextDisabled("Signal bindings remain unavailable until the scripting runtime is added.");
            break;
    }
    ImGui::End();
}

void EditorUVE::DrawInspectorContentUVE() {
    if (m_selectedEntities.empty()) {
        ImGui::TextUnformatted("Select an entity in Scene or Viewport.");
        return;
    }
    if (!HasSingleDocumentSelectionUVE()) {
        ImGui::Text("%zu entities selected", m_selectedEntities.size());
        if (IsDocumentEntityUVE(m_selectedEntity)) {
            ImGui::Text("Active: %s", GetEntityDisplayLabelUVE(m_selectedEntity).c_str());
        }
        ImGui::Separator();
        for (const Scene::EntityUVE entity : m_selectedEntities) {
            if (IsDocumentEntityUVE(entity)) {
                ImGui::BulletText("%s%s", GetEntityDisplayLabelUVE(entity).c_str(),
                                  entity == m_selectedEntity ? " (Active)" : "");
            }
        }
        ImGui::TextDisabled("Single-entity editing is unavailable for multi-selection.");
        return;
    }

    ImGui::BeginDisabled(!IsAuthoringCommandAllowedUVE());
    ImGui::Text("%s", GetEntityDisplayLabelUVE(m_selectedEntity).c_str());
    ImGui::TextDisabled("%s", EntityLabelUVE(m_selectedEntity).c_str());

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    std::array<char, kMaximumEntityNameBytesUVE + 1U> nameBuffer{};
    if (entityManager.HasComponentUVE<Scene::NameComponentUVE>(m_selectedEntity)) {
        const std::string& currentName =
            entityManager.GetComponentUVE<Scene::NameComponentUVE>(m_selectedEntity).name;
        currentName.copy(nameBuffer.data(), std::min(currentName.size(), nameBuffer.size() - 1U));
    }
    if (ImGui::InputText("Name", nameBuffer.data(), nameBuffer.size())) {
        static_cast<void>(SetSelectedEntityNameUVE(nameBuffer.data()));
    }

    if (!entityManager.HasComponentUVE<Scene::TransformComponentUVE>(m_selectedEntity)) {
        ImGui::TextUnformatted("No local Transform component.");
        ImGui::EndDisabled();
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
    ImGui::EndDisabled();
}

void EditorUVE::DrawAssetsPanelUVE() {
    const ImGuiViewport* const mainViewport = ImGui::GetMainViewport();
    const float contentHeight = kAssetsPanelHeightUVE - kBottomDockTabHeightUVE;
    ImGui::SetNextWindowPos(
        ImVec2{mainViewport->WorkPos.x, mainViewport->WorkPos.y + mainViewport->WorkSize.y -
                                      kAssetsPanelHeightUVE},
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2{mainViewport->WorkSize.x, contentHeight}, ImGuiCond_Always);
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    ImGui::Begin("FileSystem", nullptr, flags);
    ImGui::BeginDisabled(!IsAuthoringCommandAllowedUVE());

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
    ImGui::EndDisabled();
    ImGui::End();
}

void EditorUVE::DrawViewportPanelUVE() {
    // Renderer3DUVE still presents to the engine window's default framebuffer. This editor window
    // therefore owns only a transparent interactive input rectangle; it does not duplicate render
    // target, shader, or presentation ownership.
    const ImGuiViewport* const mainViewport = ImGui::GetMainViewport();
    const float leftInset = 250.0F;
    const float rightInset = 330.0F;
    const float menuBarHeight = ImGui::GetFrameHeight();
    const float bottomInset = kAssetsPanelHeightUVE;
    const ImVec2 desiredPosition{mainViewport->WorkPos.x + leftInset, mainViewport->WorkPos.y + menuBarHeight};
    const ImVec2 desiredSize{
        std::max(kMinimumViewportWidthUVE, mainViewport->WorkSize.x - leftInset - rightInset),
        std::max(kMinimumViewportHeightUVE, mainViewport->WorkSize.y - menuBarHeight - bottomInset),
    };
    ImGui::SetNextWindowPos(desiredPosition, ImGuiCond_Always);
    ImGui::SetNextWindowSize(desiredSize, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0F);
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                       ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar |
                                       ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::Begin("3D Viewport", nullptr, flags);

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

        ImGuiIO& io = ImGui::GetIO();
        if (m_gizmoDrag.axis != EditorTranslateAxisUVE::None) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                UpdateGizmoDragUVE(pointerPosition);
            } else {
                CommitGizmoDragUVE();
            }
        } else if (m_viewportNavigationMode != EditorViewportNavigationModeUVE::None) {
            const ImGuiMouseButton requiredButton =
                m_viewportNavigationMode == EditorViewportNavigationModeUVE::Orbit
                    ? ImGuiMouseButton_Right
                    : ImGuiMouseButton_Middle;
            if (!ImGui::IsMouseDown(requiredButton)) {
                CancelViewportNavigationUVE();
            } else if (m_viewportNavigationMode == EditorViewportNavigationModeUVE::Orbit) {
                static_cast<void>(OrbitViewportUVE(-io.MouseDelta.x * kViewportOrbitRadiansPerPixelUVE,
                                                   -io.MouseDelta.y * kViewportOrbitRadiansPerPixelUVE));
            } else {
                static_cast<void>(PanViewportUVE(Math::Vector2UVE{io.MouseDelta.x, io.MouseDelta.y}, viewportRect));
            }
        } else {
            if (viewportHovered && !io.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_F, false)) {
                static_cast<void>(FocusSelectedEntityUVE());
            }
            if (viewportHovered && !io.WantTextInput && io.MouseWheel != 0.0F) {
                static_cast<void>(ZoomViewportUVE(io.MouseWheel));
            }
            if (viewportHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                m_viewportNavigationMode = EditorViewportNavigationModeUVE::Orbit;
            } else if (viewportHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
                m_viewportNavigationMode = EditorViewportNavigationModeUVE::Pan;
            } else if (viewportClicked) {
                if (!BeginGizmoDragUVE(viewportRect, pointerPosition)) {
                    static_cast<void>(PickViewportUVE(viewportRect, pointerPosition, io.KeyCtrl));
                }
            }
        }

        DrawSelectionBoundsUVE(viewportRect);
        if (IsAuthoringCommandAllowedUVE()) {
            if (m_gizmoMode == EditorGizmoModeUVE::Translate) {
                DrawTranslateGizmoUVE(viewportRect);
            } else if (m_gizmoMode == EditorGizmoModeUVE::Rotate) {
                DrawRotateGizmoUVE(viewportRect);
            } else {
                DrawScaleGizmoUVE(viewportRect);
            }
        }
        ImDrawList* const drawList = ImGui::GetWindowDrawList();
        const char* const modeLabel = m_gizmoMode == EditorGizmoModeUVE::Translate
                                          ? "Translate (W)"
                                          : (m_gizmoMode == EditorGizmoModeUVE::Rotate ? "Rotate (E)"
                                                                                       : "Scale (R) | center: Uniform Offset");
        drawList->AddText(ImVec2{contentOrigin.x + 10.0F, contentOrigin.y + 10.0F}, IM_COL32(230, 230, 230, 220),
                          "LMB select / drag handle | RMB orbit | MMB pan | wheel zoom | F focus");
        drawList->AddText(ImVec2{contentOrigin.x + 10.0F, contentOrigin.y + 30.0F}, IM_COL32(190, 215, 235, 220),
                          modeLabel);
        if (m_playModeState != EditorPlayModeStateUVE::Edit) {
            const bool paused = m_playModeState == EditorPlayModeStateUVE::Paused;
            const char* const playLabel = paused ? "PAUSED" : "PLAYING";
            const ImU32 badgeColor = paused ? IM_COL32(194, 132, 45, 230) : IM_COL32(24, 148, 181, 230);
            const ImVec2 badgePosition{contentOrigin.x + 10.0F, contentOrigin.y + 52.0F};
            drawList->AddRectFilled(badgePosition, ImVec2{badgePosition.x + 82.0F, badgePosition.y + 22.0F},
                                    badgeColor, 4.0F);
            drawList->AddText(ImVec2{badgePosition.x + 9.0F, badgePosition.y + 3.0F}, IM_COL32(245, 245, 245, 255),
                              playLabel);
        }
    } else {
        ImGui::TextUnformatted("Viewport is too small for picking.");
    }
    ImGui::End();
}

} // namespace UVE::Editor
