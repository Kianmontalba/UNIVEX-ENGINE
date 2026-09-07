// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/editor/editor_uve.h"

#include "viewport/editor_viewport_widgets_uve.h"
#include "uve/editor/editor_theme_uve.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <functional>
#include <limits>
#include <numbers>
#include <string>
#include <string_view>
#include <system_error>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "uve/asset/mesh_asset_uve.h"
#include "uve/asset/texture_asset_uve.h"
#include "uve/asset/uve_file_envelope_uve.h"
#include "uve/config/i_config_manager_uve.h"
#include "uve/physics/raycast_query_uve.h"
#include "uve/render/render_resource_descs_uve.h"
#include "uve/platform/editor_project_package_uve.h"
#include "uve/scripting/script_builtin_nodes_uve.h"
#include "uve/scripting/script_bytecode_uve.h"
#include "uve/scripting/script_compiler_ir_uve.h"
#include "uve/scene/components/area_component_uve.h"
#include "uve/scene/components/camera_component_uve.h"
#include "uve/scene/components/collider_component_uve.h"
#include "uve/scene/components/expanded_3d_node_components_uve.h"
#include "uve/scene/components/hierarchy_component_uve.h"
#include "uve/scene/components/light_component_uve.h"
#include "uve/scene/components/name_component_uve.h"
#include "uve/scene/components/primitive_mesh_component_uve.h"
#include "uve/scene/components/prefab_instance_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"

namespace UVE::Editor {

namespace {

constexpr float kVectorEpsilonUVE = 0.00001F;
constexpr float kMinimumLocalScaleUVE = 0.001F;

// Subsetted Tabler Icons glyphs (MIT licensed; see
// engine/editor/assets/fonts/THIRD_PARTY_NOTICES.md) merged into the default ImGui font. These
// back the menu bar and dock-panel titles below: ImGui::BeginMenu()/ImGui::Begin() only accept a
// plain text label, so an inline ImGui::Image() glyph is not an option there the way it is for the
// editor's existing RGBA-texture ImageButton icons (gizmo modes, Snap, node/component add popups).
#include "uve_icon_font_bytes.inc"
#include "uve_ui_font_bytes.inc"

/// One typographic scale for the whole editor. The UI face is loaded at this size and the icon
/// face is merged just under it, so a label and the glyph beside it share a baseline instead of the
/// icon riding high the way a same-size merge leaves it.
constexpr float kEditorUiFontPixelSizeUVE = 15.0F;
constexpr float kEditorUiIconPixelSizeUVE = 14.0F;
constexpr float kEditorUiIconBaselineOffsetUVE = 2.0F;

constexpr ImWchar kIconFontGlyphRangesUVE[] = {
    0xEA03, 0xEA03, // Inspector (adjustments)
    0xEA45, 0xEA45, // Assets (box)
    0xEA54, 0xEA54, // Viewport (camera)
    0xEA98, 0xEA98, // Edit
    0xEAA4, 0xEAA4, // File
    0xEAAD, 0xEAAD, // Filesystem (folder)
    0xEB2E, 0xEB2E, // Favorites (star)
    0xEBD9, 0xEBD9, // Plugin
    0xEDBA, 0xEDBA, // Window (layout-grid)
    0xF91D, 0xF91D, // Help (help-circle)
    0xFA97, 0xFA97, // GameObject (cube)
    0xFAF7, 0xFAF7, // Contents (folder-open)
    0xFAFA, 0xFAFA, // Scene (list-tree)
    0,
};

// Full menu/panel labels, icon glyph baked in: ImGui::BeginMenu()/Begin() take a single string
// literal-shaped argument, so these can't be built from a separate icon constant concatenated at
// the call site the way adjacent string literals can (kMenuIconFileUVE is a runtime const char*,
// not a literal token, so " File" adjacency wouldn't compile).
constexpr const char* kMenuLabelFileUVE = "\xEE\xAA\xA4 File";
constexpr const char* kMenuLabelEditUVE = "\xEE\xAA\x98 Edit";
constexpr const char* kMenuLabelAssetsUVE = "\xEE\xA9\x85 Assets";
constexpr const char* kMenuLabelGameObjectUVE = "\xEF\xAA\x97 GameObject";
constexpr const char* kMenuLabelPluginUVE = "\xEE\xAF\x99 Plugin";
constexpr const char* kMenuLabelWindowUVE = "\xEE\xB6\xBA Window";
constexpr const char* kMenuLabelHelpUVE = "\xEF\xA4\x9D Help";
constexpr const char* kPanelLabelSceneUVE = "\xEF\xAB\xBA Scene##scene-panel";
constexpr const char* kPanelLabelInspectorUVE = "\xEE\xA8\x83 Inspector##right-panel";
constexpr const char* kPanelLabelFilesystemUVE = "\xEE\xAA\xAD Filesystem##project-panel";
constexpr const char* kPanelLabelContentsUVE = "\xEF\xAB\xB7 Contents##folder-contents-panel";
constexpr const char* kPanelLabelViewportUVE = "\xEE\xA9\x94 Viewport##viewport";
constexpr const char* kIconStarUVE = "\xEE\xAC\xAE";

[[nodiscard]] Math::Vector3UVE PrimitiveColliderHalfExtentsUVE(const Scene::PrimitiveMeshKindUVE kind) noexcept {
    switch (kind) {
        case Scene::PrimitiveMeshKindUVE::Cube:
        case Scene::PrimitiveMeshKindUVE::UVSphere:
            return Math::Vector3UVE{0.5F, 0.5F, 0.5F};
        case Scene::PrimitiveMeshKindUVE::Plane:
            return Math::Vector3UVE{0.5F, 0.025F, 0.5F};
    }
    return Math::Vector3UVE{0.5F, 0.5F, 0.5F};
}
constexpr float kGizmoAxisLengthUVE = 1.25F;
constexpr float kGizmoHandleRadiusPixelsUVE = 12.0F;
constexpr float kTrackballRadiusPixelsUVE = 42.0F;
constexpr float kTrackballAntipodalDotThresholdUVE = -0.999F;
constexpr float kMinimumViewportWidthUVE = 64.0F;
constexpr float kMinimumViewportHeightUVE = 64.0F;
constexpr float kAssetsPanelHeightUVE = 176.0F;

/// The bottom dock splits into the Filesystem navigation tree and the Contents workspace.
/// Filesystem is navigation - a project root, folders, Favourites, a search box - so it stays
/// narrow; Contents is where assets are actually browsed as a thumbnail grid, so it takes the rest
/// of the row and is always the wider of the two. Both panels derive their geometry from
/// ComputeFilesystemPanelWidthUVE() below rather than each recomputing a ratio, which is how the
/// two used to disagree.
constexpr float kFilesystemPanelWidthRatioUVE = 0.26F;
constexpr float kFilesystemPanelMinimumWidthUVE = 220.0F;
constexpr float kFilesystemPanelMaximumWidthUVE = 400.0F;

/// Filesystem's width for a given bottom-dock width, clamped so Contents keeps the majority of the
/// row at any window size - including the narrow-window case, where the ratio alone would let the
/// navigation tree grow past the workspace it is meant to feed.
[[nodiscard]] float ComputeFilesystemPanelWidthUVE(const float dockWidth) noexcept {
    const float byRatio = dockWidth * kFilesystemPanelWidthRatioUVE;
    const float widthCeiling = std::min(kFilesystemPanelMaximumWidthUVE, dockWidth * 0.4F);
    return std::clamp(byRatio, std::min(kFilesystemPanelMinimumWidthUVE, widthCeiling), widthCeiling);
}
constexpr float kBottomDockTabHeightUVE = 24.0F;
constexpr float kEditorTitleBarHeightUVE = 24.0F;
constexpr float kEditorMenuBarHeightUVE = 24.0F;
constexpr float kEditorToolbarHeightUVE = 30.0F;
constexpr float kEditorViewportToolCanvasHeightUVE = 30.0F;
constexpr float kFilesystemLongPressThresholdSecondsUVE = 0.60F;
/// Square resolution rendered for each Content Browser mesh thumbnail (see
/// EditorUVE::GetMeshThumbnailUVE). Matches the content-type badge icons' own baked resolution
/// (uve_content_type_icon_bytes.inc) - plenty of detail at the grid card's much smaller display
/// size without being wasteful to render per mesh.
constexpr int kMeshThumbnailSizeUVE = 64;
constexpr float kScriptCanvasLongPressThresholdSecondsUVE = 0.55F;
constexpr float kScriptCanvasLongPressMaxMovementPixelsUVE = 8.0F;
constexpr float kEditorTopChromeHeightUVE =
    kEditorTitleBarHeightUVE + kEditorMenuBarHeightUVE + kEditorToolbarHeightUVE;
constexpr std::size_t kMaximumEntityNameBytesUVE = 96U;
constexpr float kMinimumViewportDistanceUVE = 0.5F;
constexpr float kMaximumViewportDistanceUVE = 500.0F;
constexpr float kMaximumViewportPitchRadiansUVE = 1.4835299F; // 85 degrees.
constexpr float kViewportOrbitRadiansPerPixelUVE = 0.008F;
constexpr float kViewportZoomExponentPerWheelUnitUVE = 0.16F;
constexpr float kViewportNavigationRadiusPixelsUVE = 32.0F;
constexpr float kViewportNavigationHitRadiusPixelsUVE = 16.0F;
constexpr float kViewportNavigationPlateRadiusPixelsUVE = 47.0F;
constexpr float kMinimum2DCanvasZoomUVE = 0.10F;
constexpr float kMaximum2DCanvasZoomUVE = 4.00F;
constexpr const char* kHierarchyEntityPayloadUVE = "UVE_SCENE_HIERARCHY_ENTITY";

[[nodiscard]] const char* ScriptValueTypeLabelUVE(const Scripting::ScriptValueTypeUVE type) noexcept {
    switch (type) {
        case Scripting::ScriptValueTypeUVE::Execution: return "Exec";
        case Scripting::ScriptValueTypeUVE::Boolean: return "Bool";
        case Scripting::ScriptValueTypeUVE::Number: return "Number";
        case Scripting::ScriptValueTypeUVE::Vector2: return "Vector2";
        case Scripting::ScriptValueTypeUVE::Vector3: return "Vector3";
        case Scripting::ScriptValueTypeUVE::Entity: return "Entity";
        case Scripting::ScriptValueTypeUVE::Asset: return "Asset";
        case Scripting::ScriptValueTypeUVE::Component: return "Component";
        case Scripting::ScriptValueTypeUVE::Rotation: return "Rotation";
        case Scripting::ScriptValueTypeUVE::Transform: return "Transform";
        case Scripting::ScriptValueTypeUVE::Array: return "Array";
        case Scripting::ScriptValueTypeUVE::Map: return "Map";
        case Scripting::ScriptValueTypeUVE::Set: return "Set";
        case Scripting::ScriptValueTypeUVE::Struct: return "Struct";
    }
    return "Unknown";
}

[[nodiscard]] ImU32 ScriptPinColorUVE(const Scripting::ScriptPinRoleUVE role,
                                      const Scripting::ScriptValueTypeUVE type) noexcept {
    if (role == Scripting::ScriptPinRoleUVE::Execution || type == Scripting::ScriptValueTypeUVE::Execution) {
        return IM_COL32(230, 230, 230, 255);
    }
    switch (type) {
        case Scripting::ScriptValueTypeUVE::Boolean: return IM_COL32(204, 112, 226, 255);
        case Scripting::ScriptValueTypeUVE::Number: return IM_COL32(112, 184, 232, 255);
        case Scripting::ScriptValueTypeUVE::Vector2:
        case Scripting::ScriptValueTypeUVE::Vector3: return IM_COL32(90, 198, 164, 255);
        case Scripting::ScriptValueTypeUVE::Entity:
        case Scripting::ScriptValueTypeUVE::Component: return IM_COL32(232, 166, 82, 255);
        case Scripting::ScriptValueTypeUVE::Asset: return IM_COL32(242, 132, 132, 255);
        default: return IM_COL32(180, 180, 180, 255);
    }
}

[[nodiscard]] ImU32 ScriptPinColorUVE(const Scripting::ScriptGraphCanvasPinSnapshotUVE& pin) noexcept {
    return ScriptPinColorUVE(pin.role, pin.type);
}

[[nodiscard]] ImVec2 ScriptCanvasToScreenUVE(const Scripting::ScriptGraphCanvasPointUVE point,
                                              const ImVec2 origin,
                                              const Scripting::ScriptGraphCanvasViewUVE view) noexcept {
    return ImVec2{origin.x + (point.x - view.pan.x) * view.zoom,
                   origin.y + (point.y - view.pan.y) * view.zoom};
}

[[nodiscard]] Scripting::ScriptGraphCanvasPointUVE ScreenToScriptCanvasUVE(
    const ImVec2 point, const ImVec2 origin, const Scripting::ScriptGraphCanvasViewUVE view) noexcept {
    return Scripting::ScriptGraphCanvasPointUVE{
        view.pan.x + (point.x - origin.x) / std::max(view.zoom, 0.0001F),
        view.pan.y + (point.y - origin.y) / std::max(view.zoom, 0.0001F)};
}

[[nodiscard]] const char* ImportJobStateLabelUVE(const Asset::AssetImportJobStateUVE state) noexcept {
    switch (state) {
        case Asset::AssetImportJobStateUVE::Queued:
            return "Queued";
        case Asset::AssetImportJobStateUVE::Running:
            return "Running";
        case Asset::AssetImportJobStateUVE::Succeeded:
            return "Succeeded";
        case Asset::AssetImportJobStateUVE::Failed:
            return "Failed";
    }
    return "Unknown";
}

[[nodiscard]] const char* ImportDiagnosticSeverityLabelUVE(
    const Asset::AssetImportDiagnosticSeverityUVE severity) noexcept {
    switch (severity) {
        case Asset::AssetImportDiagnosticSeverityUVE::Warning:
            return "Warning";
        case Asset::AssetImportDiagnosticSeverityUVE::Error:
            return "Error";
    }
    return "Unknown";
}

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

void DrawNativeIconLabelUVE(const std::uintptr_t textureId, const char* const label) {
    if (textureId != 0U) {
        ImGui::Image(static_cast<ImTextureID>(textureId), ImVec2{16.0F, 16.0F});
        ImGui::SameLine(0.0F, 5.0F);
    }
    ImGui::TextUnformatted(label);
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

[[nodiscard]] Math::QuaternionUVE ConjugateUVE(const Math::QuaternionUVE& value) noexcept {
    return Math::QuaternionUVE{-value.x, -value.y, -value.z, value.w};
}

} // namespace

EditorUVE::EditorUVE(Core::EngineServicesUVE& services, std::filesystem::path activeScenePath,
                     const std::size_t historyCapacity, Core::ISimulationControlUVE* const simulationControl,
                     Core::IEditorViewportHostUVE* const viewportHost)
    : m_services(&services),
      m_simulationControl(simulationControl),
      m_viewportHost(viewportHost),
      m_activeScenePath(std::move(activeScenePath)),
      m_historyCapacity(std::max<std::size_t>(std::size_t{1U}, historyCapacity)) {
    if (!Scripting::RegisterBuiltInScriptNodesUVE(m_visualScriptRegistry)) {
        throw std::logic_error("Failed to register built-in Visual Scripting nodes.");
    }
    m_visualScriptBranches.push_back(
        ScriptBranchUVE{"Type 1 Scene", std::make_unique<Scripting::ScriptGraphCanvasUVE>(
                             m_visualScriptRegistry, m_historyCapacity)});
    RegisterBuiltInInspectorDrawersUVE();
}

EditorUVE::~EditorUVE() {
    ShutdownUVE();
}

void EditorUVE::InitUVE() {
    if (m_state != EditorStateUVE::Uninitialized) {
        return;
    }

    Window::IWindowManagerUVE& windowManager = m_services->GetWindowManagerUVE();
    if (windowManager.IsValidUVE() && windowManager.GetNativeWindowHandleUVE() != nullptr) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        // Docking only - no ImGuiConfigFlags_ViewportsEnable: multi-OS-window docking pulls in a
        // second GLFW/OpenGL presentation path this editor's single-window WindowManagerUVE
        // integration was never built for, and nothing in this pass's scope needs it.
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        ImGui::StyleColorsDark();
        ApplyEditorVisualThemeUVE();

        // Font setup must happen before ImGui_ImplOpenGL3_Init() below: that call builds and
        // uploads the font atlas texture immediately, so any fonts merged in afterward would be
        // silently missing from what actually gets rendered.
        ImGuiIO& io = ImGui::GetIO();

        // A real proportional UI face rather than ImGui's built-in bitmap font, which is fixed at
        // one size and reads as a debug overlay rather than as engine chrome. Oversampled
        // horizontally so stems stay even at this size without the smearing a plain 1x atlas gives.
        ImFontConfig textFontConfig{};
        // Every .inc byte array here has static storage duration for the life of the process, so
        // ImGui must not take ownership and free() it through its own allocator.
        textFontConfig.FontDataOwnedByAtlas = false;
        textFontConfig.OversampleH = 3;
        textFontConfig.OversampleV = 1;
        textFontConfig.PixelSnapH = false;
        io.Fonts->AddFontFromMemoryTTF(const_cast<std::uint8_t*>(uve_ui_font_ttf_bytes.data()),
                                       static_cast<int>(uve_ui_font_ttf_bytes.size()),
                                       kEditorUiFontPixelSizeUVE, &textFontConfig);

        ImFontConfig iconFontConfig{};
        iconFontConfig.MergeMode = true;
        iconFontConfig.PixelSnapH = true;
        iconFontConfig.FontDataOwnedByAtlas = false;
        // Icon glyphs are square and sit on their own em box, so merged at the text size they ride
        // high and read as detached from the label beside them - the misalignment visible on every
        // menu and panel title. Sizing them slightly under the text and nudging them down onto the
        // text's optical centre is what actually puts icon and label on one baseline.
        iconFontConfig.GlyphMinAdvanceX = kEditorUiIconPixelSizeUVE;
        iconFontConfig.GlyphOffset = ImVec2{0.0F, kEditorUiIconBaselineOffsetUVE};
        io.Fonts->AddFontFromMemoryTTF(const_cast<std::uint8_t*>(uve_icon_font_ttf_bytes.data()),
                                       static_cast<int>(uve_icon_font_ttf_bytes.size()),
                                       kEditorUiIconPixelSizeUVE, &iconFontConfig,
                                       kIconFontGlyphRangesUVE);

        auto* const nativeWindow = static_cast<GLFWwindow*>(windowManager.GetNativeWindowHandleUVE());
        // Install the backend's chained GLFW callbacks so the interactive overlay receives cursor
        // and pointer-button events while WindowManagerUVE's existing close/resize/focus callbacks
        // remain active. Engine input remains a separate service-level abstraction; overlay clicks
        // consume ImGui pointer state and never leak into runtime action mappings.
        const bool glfwInitialized = ImGui_ImplGlfw_InitForOpenGL(nativeWindow, true);
        const bool openglInitialized = glfwInitialized && ImGui_ImplOpenGL3_Init("#version 450 core");
        if (openglInitialized) {
            static_cast<void>(m_uiAssets.InitializeUVE());
            m_meshThumbnailRenderer.InitializeUVE();
            m_uiInitialized = true;
        } else {
            if (glfwInitialized) {
                ImGui_ImplGlfw_Shutdown();
            }
            ImGui::DestroyContext();
        }
    }

    // The editor camera is a real ECS entity so ICameraSystemUVE and Renderer3DUVE can consume it
    // exactly like any other camera - there is no parallel editor-camera abstraction. It is
    // deliberately never attached to the scene graph as a document root, so it stays out of the
    // hierarchy, out of GetDocumentRootsUVE(), and out of every save.
    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    m_viewportCamera = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<Scene::TransformComponentUVE>(m_viewportCamera);
    entityManager.AddComponentUVE<Scene::WorldTransformComponentUVE>(m_viewportCamera);
    Scene::CameraComponentUVE viewportCameraComponent{};
    viewportCameraComponent.fieldOfViewDegrees =
        m_viewportCameraController.GetSettingsUVE().fieldOfViewYRadians * 180.0F /
        std::numbers::pi_v<float>;
    viewportCameraComponent.nearPlane = m_viewportCameraController.GetNearPlaneUVE();
    viewportCameraComponent.farPlane = m_viewportCameraController.GetFarPlaneUVE();
    entityManager.AddComponentUVE<Scene::CameraComponentUVE>(m_viewportCamera, viewportCameraComponent);
    SyncViewportCameraEntityUVE();

    m_state = EditorStateUVE::Running;
    LoadSessionSettingsUVE();
    static_cast<void>(LoadVisualScriptWorkspaceUVE());
}

void EditorUVE::TickUVE() {
    if (m_state != EditorStateUVE::Running) {
        return;
    }

    const Asset::ProjectChangeSnapshotUVE changeSnapshot = m_services->GetProjectChangeWatcherUVE().GetSnapshotUVE();
    const bool firstProjectIndexRefresh = !m_projectFileSnapshotInitialized;
    const bool newProjectChangeBaseline = changeSnapshot.latestSequence > m_projectFileLastObservedChangeSequence;
    const bool pendingRescanRetry = changeSnapshot.rescanRequired && !m_projectFileRefreshAttemptedForRescan;
    if (firstProjectIndexRefresh || newProjectChangeBaseline || pendingRescanRetry) {
        m_projectFileLastObservedChangeSequence = changeSnapshot.latestSequence;
        RefreshProjectFileIndexUVE();
    }

    PruneSelectionUVE();
    if (m_hierarchyRenameEntity != Scene::kInvalidEntityUVE &&
        (!IsAuthoringCommandAllowedUVE() || !HasSingleDocumentSelectionUVE() ||
         !IsDocumentEntityUVE(m_hierarchyRenameEntity) || m_hierarchyRenameEntity != m_selectedEntity)) {
        CancelHierarchyRenameUVE();
    }

    // Advance any in-flight nav-gizmo or standard-view snap, then push the result onto the camera
    // entity so the engine's own render for this frame sees the updated view.
    static_cast<void>(
        m_viewportCameraController.UpdateUVE(m_services->GetTimerUVE().GetDeltaTimeFloatUVE()));
    SyncViewportCameraEntityUVE();
    PublishGroundGridStateUVE();
}

bool EditorUVE::EnterPlayModeUVE() {
    if (m_state != EditorStateUVE::Running || m_playModeState != EditorPlayModeStateUVE::Edit ||
        m_simulationControl == nullptr || !IsAuthoringCommandAllowedUVE()) {
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
    DrawPluginWindowUVE();

    if (m_activeWorkspace == EditorWorkspaceUVE::Scripting) {
        DrawScriptingWorkspaceUVE();
    } else {
        DrawHierarchyPanelUVE();
        DrawViewportPanelUVE();
        DrawInspectorPanelUVE();
        DrawBottomDockContentUVE();
    }
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

bool EditorUVE::SaveSelectedPrefabUVE(const std::filesystem::path& path) {
    if (!IsLifecycleCommandAllowedUVE() || path.empty() || !IsDocumentEntityUVE(m_selectedEntity)) {
        return false;
    }
    const Asset::AssetGuidUVE guid = m_services->GetPrefabSystemUVE().SavePrefabUVE(
        m_services->GetEntityManagerUVE(), m_services->GetAssetDatabaseUVE(), m_selectedEntity, path);
    return guid != Asset::kInvalidAssetGuidUVE;
}

bool EditorUVE::RefreshSelectedPrefabUVE() {
    if (!IsLifecycleCommandAllowedUVE() || !IsDocumentEntityUVE(m_selectedEntity)) {
        return false;
    }
    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::PrefabInstanceComponentUVE>(m_selectedEntity)) {
        return false;
    }
    const Scene::PrefabRefreshResultUVE result = m_services->GetPrefabSystemUVE().RefreshInstanceUVE(
        entityManager, m_services->GetSceneGraphUVE(), m_services->GetAssetDatabaseUVE(), m_selectedEntity);
    if (!result.IsSuccessUVE()) {
        return false;
    }
    if (result.code == Scene::PrefabRefreshCodeUVE::Refreshed) {
        SelectEntityUVE(result.rootEntity);
        m_sceneDirty = true;
        InvalidateHierarchyFilterCacheUVE();
    }
    return true;
}

bool EditorUVE::DiscardSelectedPrefabOverridesAndRefreshUVE() {
    if (!IsLifecycleCommandAllowedUVE() || !IsDocumentEntityUVE(m_selectedEntity)) {
        return false;
    }
    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::PrefabInstanceComponentUVE>(m_selectedEntity)) {
        return false;
    }
    Scene::PrefabInstanceComponentUVE cleared =
        entityManager.GetComponentUVE<Scene::PrefabInstanceComponentUVE>(m_selectedEntity);
    if (cleared.overrides.empty()) {
        return RefreshSelectedPrefabUVE();
    }
    cleared.overrides.clear();
    entityManager.AddComponentUVE<Scene::PrefabInstanceComponentUVE>(m_selectedEntity, cleared);
    const Scene::PrefabRefreshResultUVE result = m_services->GetPrefabSystemUVE().RefreshInstanceUVE(
        entityManager, m_services->GetSceneGraphUVE(), m_services->GetAssetDatabaseUVE(), m_selectedEntity, true);
    if (!result.IsSuccessUVE()) {
        return false;
    }
    if (result.code == Scene::PrefabRefreshCodeUVE::Refreshed) {
        SelectEntityUVE(result.rootEntity);
        InvalidateHierarchyFilterCacheUVE();
    }
    m_sceneDirty = true;
    return true;
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
}

void EditorUVE::ClearSelectionUVE() noexcept {
    m_selectedEntities.clear();
    m_selectedEntity = Scene::kInvalidEntityUVE;
    CancelHierarchyRenameUVE();
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

bool EditorUVE::SetSelectedPrimitiveMeshUVE(const Scene::PrimitiveMeshComponentUVE& primitive) {
    if (!IsAuthoringCommandAllowedUVE() || !HasSingleDocumentSelectionUVE() ||
        !Scene::IsPrimitiveMeshComponentValidUVE(primitive)) {
        return false;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::PrimitiveMeshComponentUVE>(m_selectedEntity)) {
        return false;
    }

    const Scene::PrimitiveMeshComponentUVE before =
        entityManager.GetComponentUVE<Scene::PrimitiveMeshComponentUVE>(m_selectedEntity);
    if (before.kind == primitive.kind && before.baseColor == primitive.baseColor) {
        return false;
    }

    const EditorSelectionSnapshotUVE selectionBefore = CaptureSelectionSnapshotUVE();
    const bool dirtyBefore = m_sceneDirty;
    if (!ApplyPrimitiveMeshStateUVE(m_selectedEntity, primitive)) {
        return false;
    }

    m_sceneDirty = true;
    RecordHistoryUVE(PrimitiveAppearanceHistoryEntryUVE{
        m_selectedEntity, before, primitive, selectionBefore, CaptureSelectionSnapshotUVE(), dirtyBefore, true});
    return true;
}

bool EditorUVE::IsSceneComponentValueValidUVE(
    const EditorSceneComponentKindUVE kind, const EditorSceneComponentValueUVE& value) const noexcept {
    return std::visit(
        [kind](const auto& typedValue) noexcept {
            using ValueType = std::decay_t<decltype(typedValue)>;
            if constexpr (std::is_same_v<ValueType, Scene::CameraComponentUVE>) {
                return kind == EditorSceneComponentKindUVE::Camera && Scene::IsCameraComponentValidUVE(typedValue);
            } else if constexpr (std::is_same_v<ValueType, Scene::MeshComponentUVE>) {
                return kind == EditorSceneComponentKindUVE::Mesh && Scene::IsMeshComponentValidUVE(typedValue);
            } else if constexpr (std::is_same_v<ValueType, Scene::LightComponentUVE>) {
                return kind == EditorSceneComponentKindUVE::Light && Scene::IsLightComponentValidUVE(typedValue);
            } else if constexpr (std::is_same_v<ValueType, Scene::ColliderComponentUVE>) {
                return kind == EditorSceneComponentKindUVE::Collider && Scene::IsColliderComponentValidUVE(typedValue);
            } else if constexpr (std::is_same_v<ValueType, Scene::RigidBodyComponentUVE>) {
                return kind == EditorSceneComponentKindUVE::RigidBody && Scene::IsRigidBodyComponentValidUVE(typedValue);
            } else if constexpr (std::is_same_v<ValueType, Scene::AudioSourceComponentUVE>) {
                return kind == EditorSceneComponentKindUVE::AudioSource && Scene::IsAudioSourceComponentValidUVE(typedValue);
            } else if constexpr (std::is_same_v<ValueType, Scene::ParticleEmitterComponentUVE>) {
                return kind == EditorSceneComponentKindUVE::ParticleEmitter &&
                       Scene::IsParticleEmitterComponentValidUVE(typedValue);
            } else if constexpr (std::is_same_v<ValueType, Scene::ScriptComponentUVE>) {
                return kind == EditorSceneComponentKindUVE::Script && Scene::IsScriptComponentValidUVE(typedValue);
            } else if constexpr (std::is_same_v<ValueType, Scene::AnimationPlayerComponentUVE>) {
                return kind == EditorSceneComponentKindUVE::AnimationPlayer &&
                       Scene::IsAnimationPlayerComponentValidUVE(typedValue);
            } else if constexpr (std::is_same_v<ValueType, Scene::WorldEnvironment3DNodeComponentUVE>) {
                return kind == EditorSceneComponentKindUVE::WorldEnvironment &&
                       Scene::IsWorldEnvironment3DNodeComponentValidUVE(typedValue);
            } else {
                return false;
            }
        },
        value);
}

bool EditorUVE::AreSceneComponentValuesEqualUVE(const EditorSceneComponentValueUVE& lhs,
                                                 const EditorSceneComponentValueUVE& rhs) const noexcept {
    return std::visit(
        [](const auto& left, const auto& right) noexcept {
            using LeftType = std::decay_t<decltype(left)>;
            using RightType = std::decay_t<decltype(right)>;
            if constexpr (!std::is_same_v<LeftType, RightType>) {
                return false;
            } else if constexpr (std::is_same_v<LeftType, Scene::CameraComponentUVE>) {
                return left.fieldOfViewDegrees == right.fieldOfViewDegrees && left.nearPlane == right.nearPlane &&
                       left.farPlane == right.farPlane;
            } else if constexpr (std::is_same_v<LeftType, Scene::MeshComponentUVE>) {
                return left.meshGuid == right.meshGuid && left.materialGuid == right.materialGuid;
            } else if constexpr (std::is_same_v<LeftType, Scene::LightComponentUVE>) {
                return left.color == right.color && left.intensity == right.intensity && left.type == right.type &&
                       left.range == right.range && left.spotAngleDegrees == right.spotAngleDegrees;
            } else if constexpr (std::is_same_v<LeftType, Scene::ColliderComponentUVE>) {
                return left.halfExtents == right.halfExtents && left.collisionLayer == right.collisionLayer &&
                       left.collisionMask == right.collisionMask && left.friction == right.friction &&
                       left.restitution == right.restitution && left.density == right.density &&
                       left.shapeType == right.shapeType && left.radius == right.radius && left.height == right.height;
            } else if constexpr (std::is_same_v<LeftType, Scene::RigidBodyComponentUVE>) {
                return left.mass == right.mass && left.isKinematic == right.isKinematic &&
                       left.velocity == right.velocity && left.angularVelocity == right.angularVelocity &&
                       left.torque == right.torque && left.inverseInertia == right.inverseInertia &&
                       left.drag == right.drag && left.gravityScale == right.gravityScale;
            } else if constexpr (std::is_same_v<LeftType, Scene::AudioSourceComponentUVE>) {
                return left.audioAssetPath == right.audioAssetPath && left.mixerGroup == right.mixerGroup &&
                       left.volume == right.volume && left.looping == right.looping && left.pitch == right.pitch &&
                       left.spatial == right.spatial && left.minDistance == right.minDistance &&
                       left.maxDistance == right.maxDistance && left.attenuationCurve == right.attenuationCurve &&
                       left.playOnAwake == right.playOnAwake;
            } else if constexpr (std::is_same_v<LeftType, Scene::ParticleEmitterComponentUVE>) {
                return left.maxParticles == right.maxParticles;
            } else if constexpr (std::is_same_v<LeftType, Scene::ScriptComponentUVE>) {
                return left.scriptAssetPath == right.scriptAssetPath;
            } else if constexpr (std::is_same_v<LeftType, Scene::AnimationPlayerComponentUVE>) {
                return left.clipAssetPath == right.clipAssetPath && left.playbackSpeed == right.playbackSpeed &&
                       left.looping == right.looping && left.playOnAwake == right.playOnAwake &&
                       left.enabled == right.enabled;
            } else if constexpr (std::is_same_v<LeftType, Scene::WorldEnvironment3DNodeComponentUVE>) {
                return left.skyAssetPath == right.skyAssetPath && left.ambientColor == right.ambientColor &&
                       left.fogColor == right.fogColor && left.ambientEnergy == right.ambientEnergy &&
                       left.exposure == right.exposure && left.fogDensity == right.fogDensity &&
                       left.fogEnabled == right.fogEnabled &&
                       left.postProcessingEnabled == right.postProcessingEnabled;
            } else {
                return false;
            }
        },
        lhs,
        rhs);
}

bool EditorUVE::ApplySceneComponentStateUVE(
    const Scene::EntityUVE entity, const EditorSceneComponentKindUVE kind,
    const std::optional<EditorSceneComponentValueUVE>& value) {
    if (!IsDocumentEntityUVE(entity)) {
        return false;
    }

    const auto apply = [&]<typename T>() {
        Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
        if (!value.has_value()) {
            if (!entityManager.HasComponentUVE<T>(entity)) {
                return false;
            }
            entityManager.RemoveComponentUVE<T>(entity);
            return true;
        }
        const T* const typedValue = std::get_if<T>(&*value);
        if (typedValue == nullptr) {
            return false;
        }
        if (entityManager.HasComponentUVE<T>(entity)) {
            entityManager.GetComponentUVE<T>(entity) = *typedValue;
        } else {
            entityManager.AddComponentUVE<T>(entity, *typedValue);
        }
        return true;
    };

    switch (kind) {
        case EditorSceneComponentKindUVE::Camera:
            return apply.template operator()<Scene::CameraComponentUVE>();
        case EditorSceneComponentKindUVE::Mesh:
            return apply.template operator()<Scene::MeshComponentUVE>();
        case EditorSceneComponentKindUVE::Light:
            return apply.template operator()<Scene::LightComponentUVE>();
        case EditorSceneComponentKindUVE::Collider:
            return apply.template operator()<Scene::ColliderComponentUVE>();
        case EditorSceneComponentKindUVE::RigidBody:
            return apply.template operator()<Scene::RigidBodyComponentUVE>();
        case EditorSceneComponentKindUVE::AudioSource:
            return apply.template operator()<Scene::AudioSourceComponentUVE>();
        case EditorSceneComponentKindUVE::ParticleEmitter:
            return apply.template operator()<Scene::ParticleEmitterComponentUVE>();
        case EditorSceneComponentKindUVE::Script:
            return apply.template operator()<Scene::ScriptComponentUVE>();
        case EditorSceneComponentKindUVE::AnimationPlayer:
            return apply.template operator()<Scene::AnimationPlayerComponentUVE>();
        case EditorSceneComponentKindUVE::WorldEnvironment:
            return apply.template operator()<Scene::WorldEnvironment3DNodeComponentUVE>();
    }
    return false;
}

bool EditorUVE::SetSelectedSceneComponentUVE(const EditorSceneComponentKindUVE kind,
                                              const EditorSceneComponentValueUVE& value) {
    if (!IsAuthoringCommandAllowedUVE() || !HasSingleDocumentSelectionUVE() ||
        !IsSceneComponentValueValidUVE(kind, value)) {
        return false;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    std::optional<EditorSceneComponentValueUVE> before;
    switch (kind) {
        case EditorSceneComponentKindUVE::Camera:
            if (entityManager.HasComponentUVE<Scene::CameraComponentUVE>(m_selectedEntity)) {
                before = entityManager.GetComponentUVE<Scene::CameraComponentUVE>(m_selectedEntity);
            }
            break;
        case EditorSceneComponentKindUVE::Mesh:
            if (entityManager.HasComponentUVE<Scene::MeshComponentUVE>(m_selectedEntity)) {
                before = entityManager.GetComponentUVE<Scene::MeshComponentUVE>(m_selectedEntity);
            }
            break;
        case EditorSceneComponentKindUVE::Light:
            if (entityManager.HasComponentUVE<Scene::LightComponentUVE>(m_selectedEntity)) {
                before = entityManager.GetComponentUVE<Scene::LightComponentUVE>(m_selectedEntity);
            }
            break;
        case EditorSceneComponentKindUVE::Collider:
            if (entityManager.HasComponentUVE<Scene::ColliderComponentUVE>(m_selectedEntity)) {
                before = entityManager.GetComponentUVE<Scene::ColliderComponentUVE>(m_selectedEntity);
            }
            break;
        case EditorSceneComponentKindUVE::RigidBody:
            if (entityManager.HasComponentUVE<Scene::RigidBodyComponentUVE>(m_selectedEntity)) {
                before = entityManager.GetComponentUVE<Scene::RigidBodyComponentUVE>(m_selectedEntity);
            }
            break;
        case EditorSceneComponentKindUVE::AudioSource:
            if (entityManager.HasComponentUVE<Scene::AudioSourceComponentUVE>(m_selectedEntity)) {
                before = entityManager.GetComponentUVE<Scene::AudioSourceComponentUVE>(m_selectedEntity);
            }
            break;
        case EditorSceneComponentKindUVE::ParticleEmitter:
            if (entityManager.HasComponentUVE<Scene::ParticleEmitterComponentUVE>(m_selectedEntity)) {
                before = entityManager.GetComponentUVE<Scene::ParticleEmitterComponentUVE>(m_selectedEntity);
            }
            break;
        case EditorSceneComponentKindUVE::Script:
            if (entityManager.HasComponentUVE<Scene::ScriptComponentUVE>(m_selectedEntity)) {
                before = entityManager.GetComponentUVE<Scene::ScriptComponentUVE>(m_selectedEntity);
            }
            break;
        case EditorSceneComponentKindUVE::AnimationPlayer:
            if (entityManager.HasComponentUVE<Scene::AnimationPlayerComponentUVE>(m_selectedEntity)) {
                before = entityManager.GetComponentUVE<Scene::AnimationPlayerComponentUVE>(m_selectedEntity);
            }
            break;
        case EditorSceneComponentKindUVE::WorldEnvironment:
            if (entityManager.HasComponentUVE<Scene::WorldEnvironment3DNodeComponentUVE>(m_selectedEntity)) {
                before = entityManager.GetComponentUVE<Scene::WorldEnvironment3DNodeComponentUVE>(m_selectedEntity);
            }
            break;
    }
    if (before.has_value() && AreSceneComponentValuesEqualUVE(*before, value)) {
        return false;
    }

    const EditorSelectionSnapshotUVE selectionBefore = CaptureSelectionSnapshotUVE();
    const bool dirtyBefore = m_sceneDirty;
    if (!ApplySceneComponentStateUVE(m_selectedEntity, kind, value)) {
        return false;
    }
    m_sceneDirty = true;
    RecordHistoryUVE(SceneComponentHistoryEntryUVE{
        m_selectedEntity, kind, before, value, selectionBefore, CaptureSelectionSnapshotUVE(), dirtyBefore, true});
    return true;
}

bool EditorUVE::RemoveSelectedSceneComponentUVE(const EditorSceneComponentKindUVE kind) {
    if (!IsAuthoringCommandAllowedUVE() || !HasSingleDocumentSelectionUVE()) {
        return false;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    std::optional<EditorSceneComponentValueUVE> before;
    switch (kind) {
        case EditorSceneComponentKindUVE::Camera:
            if (entityManager.HasComponentUVE<Scene::CameraComponentUVE>(m_selectedEntity)) before = entityManager.GetComponentUVE<Scene::CameraComponentUVE>(m_selectedEntity);
            break;
        case EditorSceneComponentKindUVE::Mesh:
            if (entityManager.HasComponentUVE<Scene::MeshComponentUVE>(m_selectedEntity)) before = entityManager.GetComponentUVE<Scene::MeshComponentUVE>(m_selectedEntity);
            break;
        case EditorSceneComponentKindUVE::Light:
            if (entityManager.HasComponentUVE<Scene::LightComponentUVE>(m_selectedEntity)) before = entityManager.GetComponentUVE<Scene::LightComponentUVE>(m_selectedEntity);
            break;
        case EditorSceneComponentKindUVE::Collider:
            if (entityManager.HasComponentUVE<Scene::ColliderComponentUVE>(m_selectedEntity)) before = entityManager.GetComponentUVE<Scene::ColliderComponentUVE>(m_selectedEntity);
            break;
        case EditorSceneComponentKindUVE::RigidBody:
            if (entityManager.HasComponentUVE<Scene::RigidBodyComponentUVE>(m_selectedEntity)) before = entityManager.GetComponentUVE<Scene::RigidBodyComponentUVE>(m_selectedEntity);
            break;
        case EditorSceneComponentKindUVE::AudioSource:
            if (entityManager.HasComponentUVE<Scene::AudioSourceComponentUVE>(m_selectedEntity)) before = entityManager.GetComponentUVE<Scene::AudioSourceComponentUVE>(m_selectedEntity);
            break;
        case EditorSceneComponentKindUVE::ParticleEmitter:
            if (entityManager.HasComponentUVE<Scene::ParticleEmitterComponentUVE>(m_selectedEntity)) before = entityManager.GetComponentUVE<Scene::ParticleEmitterComponentUVE>(m_selectedEntity);
            break;
        case EditorSceneComponentKindUVE::Script:
            if (entityManager.HasComponentUVE<Scene::ScriptComponentUVE>(m_selectedEntity)) before = entityManager.GetComponentUVE<Scene::ScriptComponentUVE>(m_selectedEntity);
            break;
        case EditorSceneComponentKindUVE::AnimationPlayer:
            if (entityManager.HasComponentUVE<Scene::AnimationPlayerComponentUVE>(m_selectedEntity)) before = entityManager.GetComponentUVE<Scene::AnimationPlayerComponentUVE>(m_selectedEntity);
            break;
        case EditorSceneComponentKindUVE::WorldEnvironment:
            if (entityManager.HasComponentUVE<Scene::WorldEnvironment3DNodeComponentUVE>(m_selectedEntity)) before = entityManager.GetComponentUVE<Scene::WorldEnvironment3DNodeComponentUVE>(m_selectedEntity);
            break;
    }
    if (!before.has_value()) {
        return false;
    }

    const EditorSelectionSnapshotUVE selectionBefore = CaptureSelectionSnapshotUVE();
    const bool dirtyBefore = m_sceneDirty;
    if (!ApplySceneComponentStateUVE(m_selectedEntity, kind, std::nullopt)) {
        return false;
    }
    m_sceneDirty = true;
    RecordHistoryUVE(SceneComponentHistoryEntryUVE{
        m_selectedEntity, kind, before, std::nullopt, selectionBefore, CaptureSelectionSnapshotUVE(), dirtyBefore, true});
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

bool EditorUVE::TranslateSelectedAlongAxisUVE(const EditorTransformAxisUVE axis, const float worldDistance) {
    if (!IsAuthoringCommandAllowedUVE() || !HasSingleDocumentSelectionUVE() ||
        !IsFiniteUVE(worldDistance) || axis == EditorTransformAxisUVE::None) {
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

bool EditorUVE::RotateSelectedAroundWorldAxisUVE(const EditorTransformAxisUVE axis, const float radians) {
    if (!IsAuthoringCommandAllowedUVE() || !HasSingleDocumentSelectionUVE() ||
        !IsFiniteUVE(radians) || axis == EditorTransformAxisUVE::None) {
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

bool EditorUVE::ScaleSelectedAlongAxisUVE(const EditorTransformAxisUVE axis,
                                           const float localScaleDelta) {
    if (!IsAuthoringCommandAllowedUVE() || !HasSingleDocumentSelectionUVE() ||
        !IsFiniteUVE(localScaleDelta) || axis == EditorTransformAxisUVE::None) {
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
        case EditorTransformAxisUVE::X:
            component = &updated.localScale.x;
            break;
        case EditorTransformAxisUVE::Y:
            component = &updated.localScale.y;
            break;
        case EditorTransformAxisUVE::Z:
            component = &updated.localScale.z;
            break;
        case EditorTransformAxisUVE::None:
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
        !IsFiniteUVE(localScaleOffset)) {
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

bool EditorUVE::IsReparentModeChangeAllowedUVE() const noexcept {
    return IsAuthoringCommandAllowedUVE();
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
    if (!IsAuthoringCommandAllowedUVE() || !AreTransformSnappingSettingsValidUVE(settings)) {
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

Scene::EntityUVE EditorUVE::CreateDocumentSceneNodeUVE(
    const Scene::Nodes::SceneNodeKindUVE kind) {
    if (!IsAuthoringCommandAllowedUVE() || m_selectedEntities.size() > 1U) {
        return Scene::kInvalidEntityUVE;
    }
    const Scene::Nodes::SceneNodeDescriptorUVE* descriptor =
        Scene::Nodes::FindSceneNodeDescriptorUVE(kind);
    if (descriptor == nullptr || !descriptor->libraryCreatable) {
        return Scene::kInvalidEntityUVE;
    }

    const EditorSelectionSnapshotUVE selectionBefore = CaptureSelectionSnapshotUVE();
    const bool dirtyBefore = m_sceneDirty;
    Scene::EntityUVE entity = Scene::kInvalidEntityUVE;
    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    const auto createNodeWithComponent = [this, &entityManager](auto component) {
        Scene::EntityUVE created = CreateDocumentEntityInternalUVE(EditorEntityKindUVE::Empty, std::nullopt);
        if (created != Scene::kInvalidEntityUVE) {
            using Component = std::decay_t<decltype(component)>;
            entityManager.AddComponentUVE<Component>(created, std::move(component));
        }
        return created;
    };

    switch (kind) {
        case Scene::Nodes::SceneNodeKindUVE::Empty:
            entity = CreateDocumentEntityInternalUVE(EditorEntityKindUVE::Empty, std::nullopt);
            break;
        case Scene::Nodes::SceneNodeKindUVE::Camera3D:
            entity = CreateDocumentEntityInternalUVE(EditorEntityKindUVE::Camera, std::nullopt);
            break;
        case Scene::Nodes::SceneNodeKindUVE::MeshInstance3D:
            entity = CreateDocumentEntityInternalUVE(EditorEntityKindUVE::Empty, std::nullopt);
            if (entity != Scene::kInvalidEntityUVE) {
                entityManager.AddComponentUVE<Scene::MeshComponentUVE>(entity, Scene::MeshComponentUVE{});
            }
            break;
        case Scene::Nodes::SceneNodeKindUVE::BoxMesh3D:
            entity = CreateDocumentEntityInternalUVE(EditorEntityKindUVE::Cube, std::nullopt);
            break;
        case Scene::Nodes::SceneNodeKindUVE::SphereMesh3D:
            entity = CreateDocumentEntityInternalUVE(EditorEntityKindUVE::UVSphere, std::nullopt);
            break;
        case Scene::Nodes::SceneNodeKindUVE::PlaneMesh3D:
            entity = CreateDocumentEntityInternalUVE(EditorEntityKindUVE::Plane, std::nullopt);
            break;
        case Scene::Nodes::SceneNodeKindUVE::Light3D:
            entity = CreateDocumentEntityInternalUVE(EditorEntityKindUVE::DirectionalLight, std::nullopt);
            break;
        case Scene::Nodes::SceneNodeKindUVE::Collider3D:
            entity = CreateDocumentEntityInternalUVE(EditorEntityKindUVE::CollisionBox, std::nullopt);
            break;
        case Scene::Nodes::SceneNodeKindUVE::CharacterBody3D:
            entity = CreateDocumentEntityInternalUVE(EditorEntityKindUVE::Empty, std::nullopt);
            if (entity != Scene::kInvalidEntityUVE) {
                entityManager.AddComponentUVE<Scene::ColliderComponentUVE>(entity,
                                                                            Scene::ColliderComponentUVE{});
                Scene::RigidBodyComponentUVE body{};
                body.isKinematic = true;
                entityManager.AddComponentUVE<Scene::RigidBodyComponentUVE>(entity, body);
            }
            break;
        case Scene::Nodes::SceneNodeKindUVE::RigidBody3D:
            entity = CreateDocumentEntityInternalUVE(EditorEntityKindUVE::Empty, std::nullopt);
            if (entity != Scene::kInvalidEntityUVE) {
                entityManager.AddComponentUVE<Scene::RigidBodyComponentUVE>(
                    entity, Scene::RigidBodyComponentUVE{});
            }
            break;
        case Scene::Nodes::SceneNodeKindUVE::AnimationPlayer:
            entity = CreateDocumentEntityInternalUVE(EditorEntityKindUVE::Empty, std::nullopt);
            if (entity != Scene::kInvalidEntityUVE) {
                entityManager.AddComponentUVE<Scene::AnimationPlayerComponentUVE>(
                    entity, Scene::AnimationPlayerComponentUVE{});
            }
            break;
        case Scene::Nodes::SceneNodeKindUVE::AudioSource3D:
            entity = CreateDocumentEntityInternalUVE(EditorEntityKindUVE::Empty, std::nullopt);
            if (entity != Scene::kInvalidEntityUVE) {
                entityManager.AddComponentUVE<Scene::AudioSourceComponentUVE>(
                    entity, Scene::AudioSourceComponentUVE{});
            }
            break;
        case Scene::Nodes::SceneNodeKindUVE::ParticleEmitter3D:
            entity = CreateDocumentEntityInternalUVE(EditorEntityKindUVE::Empty, std::nullopt);
            if (entity != Scene::kInvalidEntityUVE) {
                entityManager.AddComponentUVE<Scene::ParticleEmitterComponentUVE>(
                    entity, Scene::ParticleEmitterComponentUVE{});
            }
            break;
        case Scene::Nodes::SceneNodeKindUVE::Script:
            entity = CreateDocumentEntityInternalUVE(EditorEntityKindUVE::Empty, std::nullopt);
            if (entity != Scene::kInvalidEntityUVE) {
                entityManager.AddComponentUVE<Scene::ScriptComponentUVE>(entity, Scene::ScriptComponentUVE{});
            }
            break;
        case Scene::Nodes::SceneNodeKindUVE::Area3D:
            entity = createNodeWithComponent(Scene::AreaComponentUVE{});
            break;
        case Scene::Nodes::SceneNodeKindUVE::RayCast3D:
            entity = createNodeWithComponent(Scene::RayCast3DNodeComponentUVE{});
            break;
        case Scene::Nodes::SceneNodeKindUVE::StaticBody3D:
            entity = createNodeWithComponent(Scene::ColliderComponentUVE{});
            break;
        case Scene::Nodes::SceneNodeKindUVE::AnimatableBody3D:
            entity = CreateDocumentEntityInternalUVE(EditorEntityKindUVE::Empty, std::nullopt);
            if (entity != Scene::kInvalidEntityUVE) {
                entityManager.AddComponentUVE<Scene::ColliderComponentUVE>(entity, Scene::ColliderComponentUVE{});
                Scene::RigidBodyComponentUVE body{};
                body.isKinematic = true;
                entityManager.AddComponentUVE<Scene::RigidBodyComponentUVE>(entity, body);
                entityManager.AddComponentUVE<Scene::AnimatableBody3DNodeComponentUVE>(
                    entity, Scene::AnimatableBody3DNodeComponentUVE{});
            }
            break;
        case Scene::Nodes::SceneNodeKindUVE::NavigationRegion3D:
            entity = createNodeWithComponent(Scene::NavigationRegion3DNodeComponentUVE{});
            break;
        case Scene::Nodes::SceneNodeKindUVE::NavigationAgent3D:
            entity = createNodeWithComponent(Scene::NavigationAgent3DNodeComponentUVE{});
            break;
        case Scene::Nodes::SceneNodeKindUVE::Skeleton3D:
            entity = createNodeWithComponent(Scene::Skeleton3DNodeComponentUVE{});
            break;
        case Scene::Nodes::SceneNodeKindUVE::BoneAttachment3D:
            entity = createNodeWithComponent(Scene::BoneAttachment3DNodeComponentUVE{});
            break;
        case Scene::Nodes::SceneNodeKindUVE::SpringArm3D:
            entity = createNodeWithComponent(Scene::SpringArm3DNodeComponentUVE{});
            break;
        case Scene::Nodes::SceneNodeKindUVE::Marker3D:
            entity = createNodeWithComponent(Scene::Marker3DNodeComponentUVE{});
            break;
        case Scene::Nodes::SceneNodeKindUVE::Hitbox3D:
            entity = createNodeWithComponent(Scene::Hitbox3DNodeComponentUVE{});
            break;
        case Scene::Nodes::SceneNodeKindUVE::Hurtbox3D:
            entity = createNodeWithComponent(Scene::Hurtbox3DNodeComponentUVE{});
            break;
        case Scene::Nodes::SceneNodeKindUVE::Projectile3D:
            entity = createNodeWithComponent(Scene::Projectile3DNodeComponentUVE{});
            break;
        case Scene::Nodes::SceneNodeKindUVE::InteractionArea3D:
            entity = createNodeWithComponent(Scene::InteractionArea3DNodeComponentUVE{});
            break;
        case Scene::Nodes::SceneNodeKindUVE::WorldEnvironment3D:
            entity = createNodeWithComponent(Scene::WorldEnvironment3DNodeComponentUVE{});
            break;
        case Scene::Nodes::SceneNodeKindUVE::ReflectionProbe3D:
            entity = createNodeWithComponent(Scene::ReflectionProbe3DNodeComponentUVE{});
            break;
        case Scene::Nodes::SceneNodeKindUVE::Decal3D:
            entity = createNodeWithComponent(Scene::Decal3DNodeComponentUVE{});
            break;
        case Scene::Nodes::SceneNodeKindUVE::LODGroup3D:
            entity = createNodeWithComponent(Scene::LodGroup3DNodeComponentUVE{});
            break;
        case Scene::Nodes::SceneNodeKindUVE::Occluder3D:
            entity = createNodeWithComponent(Scene::Occluder3DNodeComponentUVE{});
            break;
        case Scene::Nodes::SceneNodeKindUVE::VisibilityRegion3D:
            entity = createNodeWithComponent(Scene::VisibilityRegion3DNodeComponentUVE{});
            break;
        case Scene::Nodes::SceneNodeKindUVE::SpawnPoint3D:
            entity = createNodeWithComponent(Scene::SpawnPoint3DNodeComponentUVE{});
            break;
        case Scene::Nodes::SceneNodeKindUVE::LevelStreamer3D:
            entity = createNodeWithComponent(Scene::LevelStreamer3DNodeComponentUVE{});
            break;
        case Scene::Nodes::SceneNodeKindUVE::WorldPartition3D:
            entity = createNodeWithComponent(Scene::WorldPartition3DNodeComponentUVE{});
            break;
        case Scene::Nodes::SceneNodeKindUVE::AnimationTree:
            return Scene::kInvalidEntityUVE;
    }

    if (entity == Scene::kInvalidEntityUVE) {
        return Scene::kInvalidEntityUVE;
    }

    const std::optional<Scene::SceneSnapshotUVE> snapshot = CaptureSubtreeUVE(entity);
    if (!snapshot.has_value()) {
        DestroyDocumentSubtreeUVE(entity);
        RestoreSelectionUVE(selectionBefore);
        m_sceneDirty = dirtyBefore;
        return Scene::kInvalidEntityUVE;
    }

    SelectEntityUVE(entity);
    m_sceneDirty = true;
    RecordHistoryUVE(SceneNodeCreationHistoryEntryUVE{
        *snapshot, kind, entity, selectionBefore, CaptureSelectionSnapshotUVE(), dirtyBefore, true});
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

bool EditorUVE::ApplyPrimitiveMeshStateUVE(const Scene::EntityUVE entity,
                                            const Scene::PrimitiveMeshComponentUVE& primitive) {
    if (!IsDocumentEntityUVE(entity) || !Scene::IsPrimitiveMeshComponentValidUVE(primitive)) {
        return false;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::PrimitiveMeshComponentUVE>(entity)) {
        return false;
    }
    entityManager.GetComponentUVE<Scene::PrimitiveMeshComponentUVE>(entity) = primitive;
    if (entityManager.HasComponentUVE<Scene::ColliderComponentUVE>(entity)) {
        entityManager.GetComponentUVE<Scene::ColliderComponentUVE>(entity).halfExtents =
            PrimitiveColliderHalfExtentsUVE(primitive.kind);
    }
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

std::string EditorUVE::GetOutlinerTypeTagUVE(const Scene::EntityUVE entity) const {
    if (!IsDocumentEntityUVE(entity)) {
        return {};
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (entityManager.HasComponentUVE<Scene::PrimitiveMeshComponentUVE>(entity)) {
        switch (entityManager.GetComponentUVE<Scene::PrimitiveMeshComponentUVE>(entity).kind) {
            case Scene::PrimitiveMeshKindUVE::Plane:
                return "Plane";
            case Scene::PrimitiveMeshKindUVE::UVSphere:
                return "UV Sphere";
            case Scene::PrimitiveMeshKindUVE::Cube:
                return "Cube";
        }
    }
    if (entityManager.HasComponentUVE<Scene::CameraComponentUVE>(entity)) {
        return "Camera";
    }
    if (entityManager.HasComponentUVE<Scene::LightComponentUVE>(entity) &&
        entityManager.GetComponentUVE<Scene::LightComponentUVE>(entity).type == Scene::LightTypeUVE::Directional) {
        return "Directional Light";
    }
    if (entityManager.HasComponentUVE<Scene::ColliderComponentUVE>(entity)) {
        return "Collision Box";
    }
    return {};
}

std::vector<Scene::EntityUVE> EditorUVE::GetDocumentAncestryUVE(const Scene::EntityUVE entity) const {
    if (!IsDocumentEntityUVE(entity)) {
        return {};
    }

    std::vector<Scene::EntityUVE> ancestry;
    Scene::EntityUVE current = entity;
    while (current != Scene::kInvalidEntityUVE) {
        if (!IsDocumentEntityUVE(current) ||
            std::find(ancestry.begin(), ancestry.end(), current) != ancestry.end()) {
            return {};
        }
        ancestry.push_back(current);

        Scene::EntityUVE parent = Scene::kInvalidEntityUVE;
        if (!TryGetDocumentParentUVE(current, parent)) {
            return {};
        }
        current = parent;
    }

    std::reverse(ancestry.begin(), ancestry.end());
    return ancestry;
}

std::vector<Scene::EntityUVE> EditorUVE::GetEligibleReparentParentsUVE(const Scene::EntityUVE entity) {
    if (!IsDocumentSubtreeUVE(entity)) {
        return {};
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    std::vector<Scene::EntityUVE> excludedSubtree;
    std::vector<Scene::EntityUVE> pending{entity};
    while (!pending.empty()) {
        const Scene::EntityUVE current = pending.back();
        pending.pop_back();
        if (!IsDocumentEntityUVE(current) ||
            std::find(excludedSubtree.begin(), excludedSubtree.end(), current) != excludedSubtree.end()) {
            return {};
        }
        excludedSubtree.push_back(current);
        const std::vector<Scene::EntityUVE> children =
            m_services->GetSceneGraphUVE().GetChildrenUVE(entityManager, current);
        pending.insert(pending.end(), children.begin(), children.end());
    }

    std::vector<Scene::EntityUVE> candidates;
    std::vector<Scene::EntityUVE> visited;
    const auto visit = [this, &entityManager, &excludedSubtree, &candidates, &visited](
                           const auto& self, const Scene::EntityUVE current) -> void {
        if (!IsDocumentEntityUVE(current) || std::find(visited.begin(), visited.end(), current) != visited.end()) {
            return;
        }
        visited.push_back(current);
        if (std::find(excludedSubtree.begin(), excludedSubtree.end(), current) == excludedSubtree.end()) {
            candidates.push_back(current);
        }
        for (const Scene::EntityUVE child : m_services->GetSceneGraphUVE().GetChildrenUVE(entityManager, current)) {
            self(self, child);
        }
    };

    for (const Scene::EntityUVE root : GetDocumentRootsUVE()) {
        visit(visit, root);
    }
    return candidates;
}

std::string EditorUVE::GetHierarchyCandidateLabelUVE(const Scene::EntityUVE entity) const {
    return GetEntityDisplayLabelUVE(entity);
}

bool EditorUVE::IsLifecycleCommandAllowedUVE() const noexcept {
    return IsAuthoringCommandAllowedUVE() && HasSingleDocumentSelectionUVE();
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
        case EditorEntityKindUVE::Cube:
        case EditorEntityKindUVE::UVSphere:
        case EditorEntityKindUVE::Plane:
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
        case EditorEntityKindUVE::Cube:
            entityManager.AddComponentUVE<Scene::PrimitiveMeshComponentUVE>(
                entity, Scene::PrimitiveMeshComponentUVE{Scene::PrimitiveMeshKindUVE::Cube,
                                                          Math::Vector3UVE{0.73F, 0.48F, 0.21F}});
            entityManager.AddComponentUVE<Scene::ColliderComponentUVE>(
                entity, Scene::ColliderComponentUVE{Math::Vector3UVE{0.5F, 0.5F, 0.5F}});
            break;
        case EditorEntityKindUVE::UVSphere:
            entityManager.AddComponentUVE<Scene::PrimitiveMeshComponentUVE>(
                entity, Scene::PrimitiveMeshComponentUVE{Scene::PrimitiveMeshKindUVE::UVSphere,
                                                          Math::Vector3UVE{0.22F, 0.55F, 0.88F}});
            entityManager.AddComponentUVE<Scene::ColliderComponentUVE>(
                entity, Scene::ColliderComponentUVE{Math::Vector3UVE{0.5F, 0.5F, 0.5F}});
            break;
        case EditorEntityKindUVE::Plane:
            entityManager.AddComponentUVE<Scene::PrimitiveMeshComponentUVE>(
                entity, Scene::PrimitiveMeshComponentUVE{Scene::PrimitiveMeshKindUVE::Plane,
                                                          Math::Vector3UVE{0.32F, 0.38F, 0.30F}});
            entityManager.AddComponentUVE<Scene::ColliderComponentUVE>(
                entity, Scene::ColliderComponentUVE{Math::Vector3UVE{0.5F, 0.025F, 0.5F}});
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
            } else if constexpr (std::is_same_v<EntryType, PrimitiveAppearanceHistoryEntryUVE>) {
                if (!ApplyPrimitiveMeshStateUVE(typedEntry.entity, typedEntry.before)) {
                    return false;
                }
                RestoreSelectionUVE(typedEntry.selectionBefore);
                m_sceneDirty = typedEntry.dirtyBefore;
                return true;
            } else if constexpr (std::is_same_v<EntryType, SceneComponentHistoryEntryUVE>) {
                if (!ApplySceneComponentStateUVE(typedEntry.entity, typedEntry.kind, typedEntry.before)) {
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
            } else if constexpr (std::is_same_v<EntryType, SceneNodeCreationHistoryEntryUVE>) {
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
            } else if constexpr (std::is_same_v<EntryType, PrimitiveAppearanceHistoryEntryUVE>) {
                if (!ApplyPrimitiveMeshStateUVE(typedEntry.entity, typedEntry.after)) {
                    return false;
                }
                RestoreSelectionUVE(typedEntry.selectionAfter);
                m_sceneDirty = typedEntry.dirtyAfter;
                return true;
            } else if constexpr (std::is_same_v<EntryType, SceneComponentHistoryEntryUVE>) {
                if (!ApplySceneComponentStateUVE(typedEntry.entity, typedEntry.kind, typedEntry.after)) {
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
            } else if constexpr (std::is_same_v<EntryType, SceneNodeCreationHistoryEntryUVE>) {
                if (typedEntry.activeEntity != Scene::kInvalidEntityUVE &&
                    m_services->GetEntityManagerUVE().IsAliveUVE(typedEntry.activeEntity)) {
                    return false;
                }
                const Scene::EntityUVE restored =
                    RestoreSubtreeUnderParentUVE(typedEntry.snapshot, Scene::kInvalidEntityUVE);
                if (restored == Scene::kInvalidEntityUVE) {
                    return false;
                }
                typedEntry.activeEntity = restored;
                typedEntry.selectionAfter = EditorSelectionSnapshotUVE{{restored}, restored};
                RestoreSelectionUVE(typedEntry.selectionAfter);
                m_sceneDirty = typedEntry.dirtyAfter;
                InvalidateHierarchyFilterCacheUVE();
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
    // The editor camera is never attached as a scene-graph root, so it should not appear here at
    // all; the erase is a cheap guarantee rather than an expected code path, and it is what stops
    // an editor-owned entity from reaching the hierarchy panel or a scene save.
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

EditorViewportCameraUVE& EditorUVE::GetViewportCameraControllerUVE() noexcept {
    return m_viewportCameraController;
}

const EditorViewportCameraUVE& EditorUVE::GetViewportCameraControllerUVE() const noexcept {
    return m_viewportCameraController;
}

const EditorViewportSettingsUVE& EditorUVE::GetViewportSettingsUVE() const noexcept {
    return m_viewportSettings;
}

void EditorUVE::SetViewportProjectionUVE(const EditorViewportProjectionModeUVE projection) noexcept {
    m_viewportSettings.projection = projection;
    // The orbit camera is the single source of truth for the projection: SyncViewportCameraEntityUVE()
    // carries it onto the ECS camera entity, so the engine's own render switches with it rather
    // than the label changing on its own.
    m_viewportCameraController.SetOrthographicUVE(
        projection == EditorViewportProjectionModeUVE::Orthographic);
    SyncViewportCameraEntityUVE();
}

void EditorUVE::SetViewportDisplayModeUVE(const EditorViewportDisplayModeUVE display) noexcept {
    m_viewportSettings.display = display;
}

void EditorUVE::SetStandardViewUVE(const EditorStandardViewUVE view) noexcept {
    m_viewportSettings.standardView = view;
    if (view == EditorStandardViewUVE::User) {
        return;
    }
    m_viewportCameraController.SnapToDirectionUVE(GetStandardViewDirectionUVE(view));
    if (m_viewportSettings.autoOrthographic) {
        SetViewportProjectionUVE(EditorViewportProjectionModeUVE::Orthographic);
    }
}

EditorGizmoModeUVE EditorUVE::GetGizmoModeUVE() const noexcept {
    return m_gizmoMode;
}

void EditorUVE::SetGizmoModeUVE(const EditorGizmoModeUVE mode) noexcept {
    m_gizmoMode = mode;
}

void EditorUVE::FocusViewportOnSelectionUVE() noexcept {
    const std::optional<EditorSelectionBoundsUVE> bounds = TryGetSelectedBoundsUVE();
    if (!bounds.has_value()) {
        m_viewportCameraController.FocusUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, 0.0F);
        return;
    }

    // Frame the selection's world bounds: the centre becomes the pivot and the corner distance
    // sets how far back the camera has to sit.
    Math::Vector3UVE centre{0.0F, 0.0F, 0.0F};
    for (const Math::Vector3UVE& corner : bounds->worldCorners) {
        centre += corner;
    }
    centre *= 1.0F / static_cast<float>(bounds->worldCorners.size());

    float radius = 0.0F;
    for (const Math::Vector3UVE& corner : bounds->worldCorners) {
        radius = std::max(radius, Math::LengthUVE(corner - centre));
    }
    m_viewportCameraController.FocusUVE(centre, radius);
}

void EditorUVE::SyncViewportCameraEntityUVE() {
    if (m_viewportCamera == Scene::kInvalidEntityUVE) {
        return;
    }
    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.IsAliveUVE(m_viewportCamera)) {
        return;
    }

    const Math::Vector3UVE eye = m_viewportCameraController.GetEyeUVE();
    const Math::QuaternionUVE rotation = m_viewportCameraController.GetRotationUVE();

    // Both the authored and derived transforms are written: the camera is not parented to
    // anything, so its world transform is its local transform, and writing both keeps every
    // consumer - ICameraSystemUVE, the audio listener sync, frustum culling - reading the same
    // pose without waiting for a scene-graph update pass this entity never takes part in.
    if (entityManager.HasComponentUVE<Scene::TransformComponentUVE>(m_viewportCamera)) {
        Scene::TransformComponentUVE& transform =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(m_viewportCamera);
        transform.localPosition = eye;
        transform.localRotation = rotation;
        transform.localScale = Math::Vector3UVE{1.0F, 1.0F, 1.0F};
    }
    if (entityManager.HasComponentUVE<Scene::WorldTransformComponentUVE>(m_viewportCamera)) {
        Scene::WorldTransformComponentUVE& worldTransform =
            entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(m_viewportCamera);
        worldTransform.worldPosition = eye;
        worldTransform.worldRotation = rotation;
        worldTransform.worldScale = Math::Vector3UVE{1.0F, 1.0F, 1.0F};
        worldTransform.dirty = false;
    }
    if (entityManager.HasComponentUVE<Scene::CameraComponentUVE>(m_viewportCamera)) {
        Scene::CameraComponentUVE& camera =
            entityManager.GetComponentUVE<Scene::CameraComponentUVE>(m_viewportCamera);
        // Clip planes scale with orbit distance, which is what keeps depth precision usable from
        // centimetres out to kilometres.
        camera.nearPlane = m_viewportCameraController.GetNearPlaneUVE();
        camera.farPlane = m_viewportCameraController.GetFarPlaneUVE();
    }
}

bool EditorUVE::TryGetGizmoPivotUVE(Math::Vector3UVE& outPivot) const {
    if (!IsDocumentEntityUVE(m_selectedEntity)) {
        return false;
    }
    const Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::WorldTransformComponentUVE>(m_selectedEntity)) {
        return false;
    }
    outPivot =
        entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(m_selectedEntity).worldPosition;
    return true;
}

bool EditorUVE::IsGizmoDraggingUVE() const noexcept {
    return m_gizmoDrag.active;
}

bool EditorUVE::BeginGizmoDragUVE(const EditorViewportProjectionUVE& projection, const float unitScale,
                                  const Math::Vector2UVE pointer) {
    CancelGizmoDragUVE();
    if (!IsAuthoringCommandAllowedUVE() || !HasSingleDocumentSelectionUVE() ||
        m_gizmoMode == EditorGizmoModeUVE::Select) {
        return false;
    }

    Math::Vector3UVE pivot{};
    if (!TryGetGizmoPivotUVE(pivot)) {
        return false;
    }

    const EditorGizmoHandleHitUVE hit =
        PickGizmoHandleUVE(m_gizmoMode, m_gizmoStyle, projection, pivot, unitScale, pointer);
    if (!hit.IsHitUVE()) {
        return false;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::TransformComponentUVE>(m_selectedEntity)) {
        return false;
    }

    m_gizmoDrag.active = true;
    m_gizmoDrag.kind = hit.kind;
    m_gizmoDrag.axis = hit.axis;
    m_gizmoDrag.entity = m_selectedEntity;
    m_gizmoDrag.startTransform =
        entityManager.GetComponentUVE<Scene::TransformComponentUVE>(m_selectedEntity);
    // The pivot is frozen at the start of the gesture. Re-reading it every frame would let the
    // handle chase the object it is moving, which turns a steady drag into a runaway.
    m_gizmoDrag.pivotWorld = pivot;
    m_gizmoDrag.startPointer = pointer;
    m_gizmoDrag.changed = false;
    return true;
}

void EditorUVE::UpdateGizmoDragUVE(const EditorViewportProjectionUVE& projection, const float unitScale,
                                   const Math::Vector2UVE pointer) {
    if (!m_gizmoDrag.active) {
        return;
    }
    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.IsAliveUVE(m_gizmoDrag.entity) ||
        !entityManager.HasComponentUVE<Scene::TransformComponentUVE>(m_gizmoDrag.entity)) {
        CancelGizmoDragUVE();
        return;
    }

    const Math::Vector3UVE axisDirection = GetAxisVectorUVE(m_gizmoDrag.axis);
    // Always rebuilt from the gesture's starting transform, never from the current one: an
    // accumulated delta drifts, and returning the pointer to where it started would not return the
    // object to where it started.
    Scene::TransformComponentUVE updated = m_gizmoDrag.startTransform;

    switch (m_gizmoDrag.kind) {
        case EditorGizmoHandleKindUVE::TranslateAxis: {
            float worldDistance = 0.0F;
            if (!ComputeAxisDragDistanceUVE(projection, m_gizmoDrag.pivotWorld, axisDirection, unitScale,
                                            m_gizmoDrag.startPointer, pointer, worldDistance)) {
                return;
            }
            if (m_transformSnappingSettings.enabled) {
                worldDistance = SnapScalarUVE(worldDistance, m_transformSnappingSettings.translateStep);
            }
            Math::Vector3UVE localDelta{};
            if (!ComputeLocalDeltaForWorldDeltaUVE(m_gizmoDrag.entity, axisDirection * worldDistance,
                                                    localDelta)) {
                return;
            }
            updated.localPosition = m_gizmoDrag.startTransform.localPosition + localDelta;
            break;
        }
        case EditorGizmoHandleKindUVE::RotateAxis: {
            float radians = 0.0F;
            if (!ComputeAxisDragAngleUVE(projection, m_gizmoDrag.pivotWorld, axisDirection,
                                         m_viewportCameraController.GetEyeUVE(), m_gizmoDrag.startPointer,
                                         pointer, radians)) {
                return;
            }
            if (m_transformSnappingSettings.enabled) {
                const float stepRadians =
                    m_transformSnappingSettings.rotateStepDegrees * std::numbers::pi_v<float> / 180.0F;
                radians = SnapScalarUVE(radians, stepRadians);
            }
            Math::QuaternionUVE localRotation{};
            if (!ComputeLocalRotationForWorldAxisUVE(m_gizmoDrag.entity,
                                                      m_gizmoDrag.startTransform.localRotation,
                                                      axisDirection, radians, localRotation)) {
                return;
            }
            updated.localRotation = localRotation;
            break;
        }
        case EditorGizmoHandleKindUVE::ScaleAxis: {
            float worldDistance = 0.0F;
            if (!ComputeAxisDragDistanceUVE(projection, m_gizmoDrag.pivotWorld, axisDirection, unitScale,
                                            m_gizmoDrag.startPointer, pointer, worldDistance)) {
                return;
            }
            if (m_transformSnappingSettings.enabled) {
                worldDistance = SnapScalarUVE(worldDistance, m_transformSnappingSettings.scaleStep);
            }
            // Scale is additive along the axis and floored, matching ScaleSelectedAlongAxisUVE's own
            // contract: a drag can shrink an object but never through zero into a mirrored one.
            Math::Vector3UVE scale = m_gizmoDrag.startTransform.localScale;
            switch (m_gizmoDrag.axis) {
                case EditorTransformAxisUVE::X:
                    scale.x = std::max(kMinimumLocalScaleUVE, scale.x + worldDistance);
                    break;
                case EditorTransformAxisUVE::Y:
                    scale.y = std::max(kMinimumLocalScaleUVE, scale.y + worldDistance);
                    break;
                case EditorTransformAxisUVE::Z:
                    scale.z = std::max(kMinimumLocalScaleUVE, scale.z + worldDistance);
                    break;
                case EditorTransformAxisUVE::None:
                    return;
            }
            updated.localScale = scale;
            break;
        }
        case EditorGizmoHandleKindUVE::None:
            return;
    }

    if (!IsTransformFiniteUVE(updated)) {
        return;
    }
    // Written straight through, with no history entry: the gesture is recorded as a single entry by
    // CommitGizmoDragUVE(), rather than flooding undo with one step per frame of the drag.
    if (ApplyLocalTransformUVE(m_gizmoDrag.entity, updated)) {
        m_gizmoDrag.changed = !AreTransformsEqualUVE(m_gizmoDrag.startTransform, updated);
    }
}

void EditorUVE::CommitGizmoDragUVE() {
    if (!m_gizmoDrag.active) {
        return;
    }
    const GizmoDragUVE drag = m_gizmoDrag;
    m_gizmoDrag = GizmoDragUVE{};

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!drag.changed || !entityManager.IsAliveUVE(drag.entity) ||
        !entityManager.HasComponentUVE<Scene::TransformComponentUVE>(drag.entity)) {
        return;
    }

    // Rewind to the pre-drag transform and re-apply the final one through the ordinary command
    // path, so the gesture lands in history as one undoable step with the correct before/after
    // rather than as the hundreds of silent writes the drag actually performed.
    const Scene::TransformComponentUVE finalTransform =
        entityManager.GetComponentUVE<Scene::TransformComponentUVE>(drag.entity);
    if (!ApplyLocalTransformUVE(drag.entity, drag.startTransform)) {
        return;
    }
    static_cast<void>(SetSelectedLocalTransformUVE(finalTransform));
}

void EditorUVE::CancelGizmoDragUVE() {
    if (!m_gizmoDrag.active) {
        return;
    }
    const GizmoDragUVE drag = m_gizmoDrag;
    m_gizmoDrag = GizmoDragUVE{};

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (drag.changed && entityManager.IsAliveUVE(drag.entity) &&
        entityManager.HasComponentUVE<Scene::TransformComponentUVE>(drag.entity)) {
        static_cast<void>(ApplyLocalTransformUVE(drag.entity, drag.startTransform));
    }
}

Render::EditorGroundGridStateUVE EditorUVE::ComputeGroundGridStateUVE() const {
    Render::EditorGroundGridStateUVE grid{};
    // The grid is world space: its origin is the world origin and its spacing is a world quantity,
    // so selecting an entity never moves it - there is deliberately no selection input here at all.
    // Only the fade distances follow the camera, and they are derived here rather than in the
    // renderer, because camera policy is the editor's business.
    grid.enabled = m_viewportSettings.showGrid && m_state == EditorStateUVE::Running;
    const float orbitDistance = m_viewportCameraController.GetDistanceUVE();
    grid.fadeStartDistance = orbitDistance * 12.0F;
    grid.fadeEndDistance = orbitDistance * 45.0F;
    // Axis colours are shared with the gizmo's, so world X reads the same red in the grid and on a
    // transform handle.
    grid.axisColorX = m_gizmoStyle.axisColorX;
    grid.axisColorZ = m_gizmoStyle.axisColorZ;
    return grid;
}

void EditorUVE::PublishGroundGridStateUVE() {
    m_services->GetRenderer3DUVE().SetEditorGroundGridStateUVE(ComputeGroundGridStateUVE());
}

void EditorUVE::PublishViewportRegionUVE(const std::optional<Render::ViewportRectUVE>& region) {
    if (m_viewportHost == nullptr) {
        return;
    }
    const bool unchanged =
        m_publishedViewportRegion.has_value() == region.has_value() &&
        (!region.has_value() || (m_publishedViewportRegion->x == region->x &&
                                 m_publishedViewportRegion->y == region->y &&
                                 m_publishedViewportRegion->width == region->width &&
                                 m_publishedViewportRegion->height == region->height));
    if (unchanged) {
        return;
    }
    m_publishedViewportRegion = region;
    m_viewportHost->SetEditorViewportRegionUVE(region);
}

Editor2DCanvasStateUVE EditorUVE::Get2DCanvasStateUVE() const noexcept {
    return m_2dCanvasState;
}
bool EditorUVE::Set2DCanvasZoomUVE(const float zoom) noexcept {
    if (!std::isfinite(zoom) || zoom < kMinimum2DCanvasZoomUVE || zoom > kMaximum2DCanvasZoomUVE) {
        return false;
    }
    m_2dCanvasState.zoom = zoom;
    return true;
}
void EditorUVE::Reset2DCanvasViewUVE() noexcept {
    m_2dCanvasState = Editor2DCanvasStateUVE{};
    m_2dCanvasPanning = false;
}
bool EditorUVE::IsSceneDirtyUVE() const noexcept {

    return m_sceneDirty;
}

EditorToolSessionPhaseUVE EditorUVE::GetToolSessionPhaseUVE() const noexcept {
    return m_toolSession.GetPhaseUVE();
}

EditorToolSessionOutcomeUVE EditorUVE::GetLastToolSessionOutcomeUVE() const noexcept {
    return m_toolSession.GetLastOutcomeUVE();
}

const std::optional<Asset::AssetRecordUVE>& EditorUVE::GetSelectedAssetUVE() const noexcept {
    return m_selectedAsset;
}

const std::optional<Asset::ProjectFileEntryUVE>& EditorUVE::GetSelectedProjectFileUVE() const noexcept {
    return m_selectedProjectFile;
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

Scripting::ScriptGraphCanvasUVE& EditorUVE::GetVisualScriptCanvasUVE() noexcept {
    return ActiveVisualScriptCanvasUVE();
}

Scripting::ScriptGraphCanvasUVE& EditorUVE::ActiveVisualScriptCanvasUVE() noexcept {
    return *m_visualScriptBranches[m_activeVisualScriptBranch].canvas;
}

const Scripting::ScriptGraphCanvasUVE& EditorUVE::ActiveVisualScriptCanvasUVE() const noexcept {
    return *m_visualScriptBranches[m_activeVisualScriptBranch].canvas;
}

std::vector<std::string> EditorUVE::GetVisualScriptBranchNamesUVE() const {
    std::vector<std::string> names;
    names.reserve(m_visualScriptBranches.size());
    for (const ScriptBranchUVE& branch : m_visualScriptBranches) {
        names.push_back(branch.name);
    }
    return names;
}

const std::string& EditorUVE::GetActiveVisualScriptBranchNameUVE() const noexcept {
    return m_visualScriptBranches[m_activeVisualScriptBranch].name;
}

bool EditorUVE::CreateVisualScriptBranchUVE(std::string name) {
    const auto invalidName = [&name] {
        return name.empty() || name.size() > 96U ||
               std::any_of(name.begin(), name.end(), [](const char value) {
                   return std::iscntrl(static_cast<unsigned char>(value)) != 0 || value == '/' || value == 0x5c;
               });
    };
    if (m_state != EditorStateUVE::Running || invalidName() ||
        std::any_of(m_visualScriptBranches.begin(), m_visualScriptBranches.end(),
                    [&name](const ScriptBranchUVE& branch) { return branch.name == name; })) {
        return false;
    }
    m_visualScriptBranches.push_back(
        ScriptBranchUVE{std::move(name), std::make_unique<Scripting::ScriptGraphCanvasUVE>(
                            m_visualScriptRegistry, m_historyCapacity)});
    m_activeVisualScriptBranch = m_visualScriptBranches.size() - 1U;
    return true;
}

bool EditorUVE::SelectVisualScriptBranchUVE(std::string name) {
    if (m_state != EditorStateUVE::Running) {
        return false;
    }
    const auto iterator = std::find_if(m_visualScriptBranches.begin(), m_visualScriptBranches.end(),
                                       [&name](const ScriptBranchUVE& branch) { return branch.name == name; });
    if (iterator == m_visualScriptBranches.end()) {
        return false;
    }
    m_activeVisualScriptBranch = static_cast<std::size_t>(std::distance(m_visualScriptBranches.begin(), iterator));
    return true;
}

bool EditorUVE::RenameActiveVisualScriptBranchUVE(std::string name) {
    const auto invalidName = [&name] {
        return name.empty() || name.size() > 96U ||
               std::any_of(name.begin(), name.end(), [](const char value) {
                   return std::iscntrl(static_cast<unsigned char>(value)) != 0 || value == '/' || value == 0x5c;
               });
    };
    if (m_state != EditorStateUVE::Running || invalidName() ||
        std::any_of(m_visualScriptBranches.begin(), m_visualScriptBranches.end(),
                    [this, &name](const ScriptBranchUVE& branch) {
                        return &branch != &m_visualScriptBranches[m_activeVisualScriptBranch] && branch.name == name;
                    })) {
        return false;
    }
    m_visualScriptBranches[m_activeVisualScriptBranch].name = std::move(name);
    return true;
}

std::filesystem::path ScriptWorkspacePathUVE(const std::filesystem::path& scenePath) {
    std::filesystem::path path = scenePath;
    path.replace_extension(".scripting");
    return path.empty() ? std::filesystem::path{"main.scripting"} : path;
}

bool EditorUVE::SaveVisualScriptWorkspaceUVE() {
    if (m_state != EditorStateUVE::Running || m_visualScriptBranches.empty()) {
        return false;
    }
    Scripting::ScriptGraphWorkspaceSchemaUVE workspace{};
    workspace.branches.reserve(m_visualScriptBranches.size());
    for (const ScriptBranchUVE& branch : m_visualScriptBranches) {
        const Scripting::ScriptGraphCanvasLayoutSnapshotUVE layout = branch.canvas->GetLayoutSnapshotUVE();
        Scripting::ScriptGraphSchemaUVE schema{};
        schema.graph = branch.canvas->GetGraphUVE();
        schema.layout.reserve(layout.entries.size());
        for (const auto& entry : layout.entries) {
            schema.layout.push_back(Scripting::ScriptGraphLayoutEntryUVE{
                entry.nodeId, entry.position.x, entry.position.y});
        }
        workspace.branches.push_back(Scripting::ScriptGraphWorkspaceBranchUVE{
            branch.name, std::move(schema), {layout.view.pan.x, layout.view.pan.y, layout.view.zoom}});
    }
    std::vector<Scripting::ScriptPersistenceDiagnosticUVE> diagnostics;
    const std::string encoded = Scripting::EncodeScriptGraphWorkspaceUVE(workspace, diagnostics);
    if (encoded.empty() || !diagnostics.empty()) {
        return false;
    }
    const std::filesystem::path path = ScriptWorkspacePathUVE(m_activeScenePath);
    const std::string virtualPath = path.generic_string();
    Asset::IFileSystemUVE& fileSystem = m_services->GetFileSystemUVE();
    const std::filesystem::path resolvedPath = fileSystem.ResolveRealPathUVE(virtualPath);
    if (!resolvedPath.empty()) {
        const std::filesystem::path temporaryPath = resolvedPath.string() + ".tmp";
        std::error_code error;
        if (!resolvedPath.parent_path().empty()) {
            std::filesystem::create_directories(resolvedPath.parent_path(), error);
        }
        std::ofstream output(temporaryPath, std::ios::binary | std::ios::trunc);
        if (!output.is_open()) {
            return false;
        }
        output.write(encoded.data(), static_cast<std::streamsize>(encoded.size()));
        output.flush();
        const bool outputGood = output.good();
        output.close();
        if (!outputGood) {
            std::filesystem::remove(temporaryPath, error);
            return false;
        }
        std::filesystem::rename(temporaryPath, resolvedPath, error);
        if (error) {
            std::filesystem::remove(resolvedPath, error);
            error.clear();
            std::filesystem::rename(temporaryPath, resolvedPath, error);
        }
        if (error) {
            std::filesystem::remove(temporaryPath, error);
            return false;
        }
        return true;
    }
    std::vector<std::byte> bytes(encoded.size());
    if (!encoded.empty()) {
        std::memcpy(bytes.data(), encoded.data(), encoded.size());
    }
    if (fileSystem.WriteFileUVE(virtualPath, bytes)) {
        return true;
    }
    // Some editor test/legacy configurations have no mounted project directory. Preserve the
    // established raw-path behavior as a compatibility fallback after the native VFS attempt.
    const std::filesystem::path temporaryPath = path.string() + ".tmp";
    std::error_code error;
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path(), error);
    }
    std::ofstream output(temporaryPath, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }
    output.write(encoded.data(), static_cast<std::streamsize>(encoded.size()));
    output.flush();
    const bool outputGood = output.good();
    output.close();
    if (!outputGood) {
        std::filesystem::remove(temporaryPath, error);
        return false;
    }
    std::filesystem::rename(temporaryPath, path, error);
    if (error) {
        std::filesystem::remove(path, error);
        error.clear();
        std::filesystem::rename(temporaryPath, path, error);
    }
    if (error) {
        std::filesystem::remove(temporaryPath, error);
        return false;
    }
    return true;
}

bool EditorUVE::LoadVisualScriptWorkspaceUVE() {
    if (m_state != EditorStateUVE::Running) {
        return false;
    }
    const std::filesystem::path path = ScriptWorkspacePathUVE(m_activeScenePath);
    const std::string virtualPath = path.generic_string();
    std::string text;
    if (const std::optional<std::vector<std::byte>> bytes = m_services->GetFileSystemUVE().ReadFileUVE(virtualPath);
        bytes.has_value()) {
        text.assign(reinterpret_cast<const char*>(bytes->data()), bytes->size());
    } else {
        if (!std::filesystem::exists(path)) {
            return false;
        }
        std::ifstream input(path, std::ios::binary);
        if (!input.is_open()) {
            return false;
        }
        std::ostringstream buffer;
        buffer << input.rdbuf();
        text = buffer.str();
    }
    const Scripting::ScriptGraphWorkspaceDecodeResultUVE decoded =
        Scripting::DecodeScriptGraphWorkspaceUVE(text);
    if (!decoded.IsSuccessUVE()) {
        return false;
    }
    std::vector<ScriptBranchUVE> loaded;
    loaded.reserve(decoded.workspace->branches.size());
    for (const auto& branch : decoded.workspace->branches) {
        auto canvas = std::make_unique<Scripting::ScriptGraphCanvasUVE>(m_visualScriptRegistry, m_historyCapacity);
        Scripting::ScriptGraphCanvasLayoutSnapshotUVE layout{};
        layout.view = {branch.view.panX, branch.view.panY, branch.view.zoom};
        layout.entries.reserve(branch.schema.layout.size());
        for (const auto& entry : branch.schema.layout) {
            layout.entries.push_back(Scripting::ScriptGraphCanvasLayoutEntryUVE{
                entry.nodeId, {entry.x, entry.y}});
        }
        if (!canvas->RestorePersistenceUVE(branch.schema, std::move(layout)).IsAppliedUVE()) {
            return false;
        }
        loaded.push_back(ScriptBranchUVE{branch.name, std::move(canvas)});
    }
    if (loaded.empty()) {
        return false;
    }
    m_visualScriptBranches = std::move(loaded);
    m_activeVisualScriptBranch = 0U;
    return true;
}

const Scripting::ScriptNodeRegistryUVE& EditorUVE::GetVisualScriptRegistryUVE() const noexcept {
    return m_visualScriptRegistry;
}

void EditorUVE::ShutdownUVE() {
    if (m_state == EditorStateUVE::Shutdown || m_state == EditorStateUVE::Uninitialized) {
        return;
    }

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
        ClearTextureThumbnailCacheUVE();
        ClearMeshThumbnailCacheUVE();
        m_meshThumbnailRenderer.ShutdownUVE();
        m_uiAssets.ShutdownUVE();
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        m_uiInitialized = false;
    }

    ClearSelectionUVE();
    ClearHistoryUVE();

    // Hand the whole window back to Core before the panel that owned the sub-rect goes away.
    PublishViewportRegionUVE(std::nullopt);
    if (m_viewportCamera != Scene::kInvalidEntityUVE) {
        Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
        if (entityManager.IsAliveUVE(m_viewportCamera)) {
            entityManager.DestroyEntityUVE(m_viewportCamera);
        }
        m_viewportCamera = Scene::kInvalidEntityUVE;
    }

    m_state = EditorStateUVE::Shutdown;
}

bool EditorUVE::IsDocumentEntityUVE(const Scene::EntityUVE entity) const noexcept {
    // The editor camera is a live ECS entity but is not document data: excluding it here is what
    // keeps it unselectable, unreparentable, undeletable and out of every command path, exactly
    // as it is kept out of GetDocumentRootsUVE() and out of every save.
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
        case EditorEntityKindUVE::Cube:
            return "Cube";
        case EditorEntityKindUVE::UVSphere:
            return "UV Sphere";
        case EditorEntityKindUVE::Plane:
            return "Plane";
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

bool EditorUVE::IsFiniteVectorUVE(const Math::Vector3UVE& vector) const noexcept {
    return IsFiniteUVE(vector.x) && IsFiniteUVE(vector.y) && IsFiniteUVE(vector.z);
}

Math::Vector3UVE EditorUVE::GetAxisVectorUVE(const EditorTransformAxisUVE axis) const noexcept {
    switch (axis) {
        case EditorTransformAxisUVE::X:
            return Math::Vector3UVE{1.0F, 0.0F, 0.0F};
        case EditorTransformAxisUVE::Y:
            return Math::Vector3UVE{0.0F, 1.0F, 0.0F};
        case EditorTransformAxisUVE::Z:
            return Math::Vector3UVE{0.0F, 0.0F, 1.0F};
        case EditorTransformAxisUVE::None:
            return Math::Vector3UVE{};
    }
    return Math::Vector3UVE{};
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

void EditorUVE::ApplyLayoutPresetUVE(const EditorLayoutPresetUVE preset) noexcept {
    switch (preset) {
        case EditorLayoutPresetUVE::Default:
            m_scenePanelVisible = true;
            m_inspectorPanelVisible = true;
            m_bottomDockVisible = true;
            m_activeRightPanelTab = EditorRightPanelTabUVE::Inspector;
            m_activeBottomDock = EditorBottomDockUVE::FileSystem;
            break;
        case EditorLayoutPresetUVE::FocusViewport:
            m_scenePanelVisible = false;
            m_inspectorPanelVisible = false;
            m_bottomDockVisible = false;
            break;
        case EditorLayoutPresetUVE::ContentReview:
            m_scenePanelVisible = true;
            m_inspectorPanelVisible = true;
            m_bottomDockVisible = true;
            m_activeRightPanelTab = EditorRightPanelTabUVE::Import;
            m_activeBottomDock = EditorBottomDockUVE::FileSystem;
            break;
    }
}

void EditorUVE::LoadSessionSettingsUVE() {
    Config::IConfigManagerUVE& config = m_services->GetConfigManagerUVE();
    constexpr std::int64_t kSessionVersion = 1;
    const std::int64_t version = config.GetIntUVE("editor.sessionSettingsVersion", 0);
    if (version > kSessionVersion) {
        return;
    }
    const auto getEnum = [&config](const std::string_view key, const std::int64_t fallback, const std::int64_t maximum) {
        const std::int64_t value = config.GetIntUVE(key, fallback);
        return value >= 0 && value <= maximum ? value : fallback;
    };
    m_scenePanelVisible = config.GetBoolUVE("editor.panels.sceneVisible", true);
    m_inspectorPanelVisible = config.GetBoolUVE("editor.panels.inspectorVisible", true);
    m_bottomDockVisible = config.GetBoolUVE("editor.panels.bottomDockVisible", true);
    m_activeWorkspace = static_cast<EditorWorkspaceUVE>(getEnum("editor.workspace.active", 0, 4));
    m_activeRightPanelTab = static_cast<EditorRightPanelTabUVE>(getEnum("editor.rightPanel.activeTab", 0, 2));
    m_activeBottomDock = static_cast<EditorBottomDockUVE>(getEnum("editor.bottomDock.active", 3, 3));
    EditorTransformSnappingSettingsUVE snapping{};
    const auto getPositiveSnapValue = [&config](const std::string_view key, const float fallback) {
        const float candidate = static_cast<float>(config.GetDoubleUVE(key, fallback));
        return IsFiniteUVE(candidate) && candidate > 0.0F ? candidate : fallback;
    };
    snapping.enabled = config.GetBoolUVE("editor.viewport.snap.enabled", false);
    snapping.translateStep = getPositiveSnapValue("editor.viewport.snap.translateStep", snapping.translateStep);
    snapping.rotateStepDegrees =
        getPositiveSnapValue("editor.viewport.snap.rotateStepDegrees", snapping.rotateStepDegrees);
    snapping.scaleStep = getPositiveSnapValue("editor.viewport.snap.scaleStep", snapping.scaleStep);
    m_transformSnappingSettings = snapping;
    constexpr std::int64_t kMaxPersistedFavoritesUVE = 128;
    const std::int64_t favoritesCount =
        std::clamp(config.GetIntUVE("editor.favorites.count", 0), std::int64_t{0}, kMaxPersistedFavoritesUVE);
    m_favoriteProjectPaths.clear();
    m_favoriteProjectPaths.reserve(static_cast<std::size_t>(favoritesCount));
    for (std::int64_t index = 0; index < favoritesCount; ++index) {
        const std::string stored = config.GetStringUVE("editor.favorites." + std::to_string(index), "");
        if (!stored.empty()) {
            m_favoriteProjectPaths.emplace_back(stored);
        }
    }
}

bool EditorUVE::SaveSessionSettingsUVE() {
    if (m_state != EditorStateUVE::Running ||
        !AreTransformSnappingSettingsValidUVE(m_transformSnappingSettings)) {
        return false;
    }
    Config::IConfigManagerUVE& config = m_services->GetConfigManagerUVE();
    config.SetIntUVE("editor.sessionSettingsVersion", 1);
    config.SetIntUVE("editor.workspace.active", static_cast<std::int64_t>(m_activeWorkspace));
    config.SetIntUVE("editor.rightPanel.activeTab", static_cast<std::int64_t>(m_activeRightPanelTab));
    config.SetIntUVE("editor.bottomDock.active", static_cast<std::int64_t>(m_activeBottomDock));
    config.SetBoolUVE("editor.panels.sceneVisible", m_scenePanelVisible);
    config.SetBoolUVE("editor.panels.inspectorVisible", m_inspectorPanelVisible);
    config.SetBoolUVE("editor.panels.bottomDockVisible", m_bottomDockVisible);
    config.SetBoolUVE("editor.viewport.snap.enabled", m_transformSnappingSettings.enabled);
    config.SetDoubleUVE("editor.viewport.snap.translateStep", m_transformSnappingSettings.translateStep);
    config.SetDoubleUVE("editor.viewport.snap.rotateStepDegrees", m_transformSnappingSettings.rotateStepDegrees);
    config.SetDoubleUVE("editor.viewport.snap.scaleStep", m_transformSnappingSettings.scaleStep);
    constexpr std::size_t kMaxPersistedFavoritesUVE = 128U;
    const std::size_t favoritesToPersist = std::min(m_favoriteProjectPaths.size(), kMaxPersistedFavoritesUVE);
    config.SetIntUVE("editor.favorites.count", static_cast<std::int64_t>(favoritesToPersist));
    for (std::size_t index = 0U; index < favoritesToPersist; ++index) {
        config.SetStringUVE("editor.favorites." + std::to_string(index),
                             m_favoriteProjectPaths[index].generic_string());
    }
    return config.SaveUVE();
}

void EditorUVE::DrawMenuBarUVE() {
    const ImGuiViewport* const mainViewport = ImGui::GetMainViewport();
    ImGuiIO& io = ImGui::GetIO();
    const bool lifecycleCommandAllowed = IsLifecycleCommandAllowedUVE() && IsDocumentEntityUVE(m_selectedEntity);
    const bool canEnterPlayMode = m_simulationControl != nullptr &&
                                  m_playModeState == EditorPlayModeStateUVE::Edit;
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
    } else if (!io.WantTextInput && io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
        static_cast<void>(io.KeyShift ? RedoUVE() : UndoUVE());
    } else if (!io.WantTextInput && io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) {
        static_cast<void>(RedoUVE());
    } else if (!io.WantTextInput && lifecycleCommandAllowed && io.KeyCtrl &&
               ImGui::IsKeyPressed(ImGuiKey_D, false)) {
        static_cast<void>(DuplicateSelectedEntityUVE());
    } else if (!io.WantTextInput && lifecycleCommandAllowed && ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
        static_cast<void>(DeleteSelectedEntityUVE());
    }

    constexpr ImGuiWindowFlags chromeFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                              ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar |
                                              ImGuiWindowFlags_NoScrollWithMouse;
    const auto beginChrome = [mainViewport, chromeFlags](const char* const id, const float y, const float height) {
        ImGui::SetNextWindowPos(ImVec2{mainViewport->WorkPos.x, mainViewport->WorkPos.y + y}, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2{mainViewport->WorkSize.x, height}, ImGuiCond_Always);
        return ImGui::Begin(id, nullptr, chromeFlags);
    };

    if (beginChrome("##uve-titlebar", 0.0F, kEditorTitleBarHeightUVE)) {
        ImDrawList* const titleDrawList = ImGui::GetWindowDrawList();
        const ImVec2 titleMin = ImGui::GetWindowPos();
        const ImVec2 titleMax{titleMin.x + ImGui::GetWindowWidth(), titleMin.y + kEditorTitleBarHeightUVE};
        titleDrawList->AddRectFilled(titleMin, titleMax, IM_COL32(17, 21, 26, 255));
        titleDrawList->AddLine(ImVec2{titleMin.x, titleMax.y - 1.0F}, ImVec2{titleMax.x, titleMax.y - 1.0F},
                               IM_COL32(48, 55, 64, 235), 1.0F);
        if (m_uiAssets.IsReadyUVE()) {
            ImGui::Image(static_cast<ImTextureID>(m_uiAssets.GetLogoTextureIdUVE()), ImVec2{20.0F, 20.0F});
            ImGui::SameLine(0.0F, 6.0F);
        } else {
            ImGui::TextUnformatted("UVE");
            ImGui::SameLine();
        }
        const char* workspaceLabel = "Library";
        switch (m_activeWorkspace) {
            case EditorWorkspaceUVE::Library: workspaceLabel = "Library"; break;
            case EditorWorkspaceUVE::Asset: workspaceLabel = "Asset"; break;
            case EditorWorkspaceUVE::Scripting: workspaceLabel = "Scripting"; break;
            case EditorWorkspaceUVE::Debug: workspaceLabel = "Debug"; break;
            case EditorWorkspaceUVE::Plugin: workspaceLabel = "Plugin"; break;
        }
        ImGui::TextDisabled("| %s", workspaceLabel);
        ImGui::SameLine();
        ImGui::TextDisabled("| %s | %zu selected | %s", m_sceneDirty ? "unsaved" : "saved",
                            m_selectedEntities.size(),
                            m_playModeState == EditorPlayModeStateUVE::Edit
                                ? "edit"
                                : (m_playModeState == EditorPlayModeStateUVE::Paused ? "paused" : "playing"));
        ImGui::SameLine(ImGui::GetWindowWidth() - 220.0F);
        ImGui::TextDisabled("UVE Editor 0.1");
        ImGui::End();
    }

    // The menu row uses an explicit menu-bar window context so its horizontal menu items and EndMenuBar
    // lifecycle are valid even though the row is not ImGui's global MainMenuBar.
    ImGui::SetNextWindowPos(ImVec2{mainViewport->WorkPos.x,
                                   mainViewport->WorkPos.y + kEditorTitleBarHeightUVE}, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2{mainViewport->WorkSize.x, kEditorMenuBarHeightUVE}, ImGuiCond_Always);
    if (ImGui::Begin("##uve-menu-row", nullptr, chromeFlags | ImGuiWindowFlags_MenuBar)) {
        ImDrawList* const menuDrawList = ImGui::GetWindowDrawList();
        const ImVec2 menuMin = ImGui::GetWindowPos();
        const ImVec2 menuMax{menuMin.x + ImGui::GetWindowWidth(), menuMin.y + kEditorMenuBarHeightUVE};
        menuDrawList->AddRectFilled(menuMin, menuMax, IM_COL32(27, 32, 38, 255));
        menuDrawList->AddLine(ImVec2{menuMin.x, menuMax.y - 1.0F}, ImVec2{menuMax.x, menuMax.y - 1.0F},
                              IM_COL32(48, 55, 64, 235), 1.0F);
        ImGui::BeginMenuBar();
        if (ImGui::BeginMenu(kMenuLabelFileUVE)) {
            const bool canSave = IsAuthoringCommandAllowedUVE() && !m_activeScenePath.empty();
            ImGui::BeginDisabled(!canSave);
            if (ImGui::MenuItem("Save Scene")) {
                static_cast<void>(SaveSceneUVE());
            }
            ImGui::EndDisabled();
            if (ImGui::MenuItem("Load Scene")) {
                static_cast<void>(LoadSceneUVE());
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save Editor Preferences")) {
                static_cast<void>(SaveSessionSettingsUVE());
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(kMenuLabelEditUVE)) {
            ImGui::BeginDisabled(!CanUndoUVE());
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
                static_cast<void>(UndoUVE());
            }
            ImGui::EndDisabled();
            ImGui::BeginDisabled(!CanRedoUVE());
            if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
                static_cast<void>(RedoUVE());
            }
            ImGui::EndDisabled();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(kMenuLabelAssetsUVE)) {
            if (ImGui::MenuItem("Open Project Browser")) {
                m_activeBottomDock = EditorBottomDockUVE::FileSystem;
                m_bottomDockVisible = true;
            }
            ImGui::MenuItem("Import Queue", nullptr, false, false);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(kMenuLabelGameObjectUVE)) {
            ImGui::BeginDisabled(!IsAuthoringCommandAllowedUVE());
            if (ImGui::MenuItem("Create Empty")) {
                static_cast<void>(CreateDocumentEntityUVE(EditorEntityKindUVE::Empty));
            }
            if (ImGui::MenuItem("Create Cube")) {
                static_cast<void>(CreateDocumentEntityUVE(EditorEntityKindUVE::Cube));
            }
            ImGui::EndDisabled();
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem(kMenuLabelPluginUVE)) {
            m_pluginWindowVisible = true;
        }
        if (ImGui::BeginMenu(kMenuLabelWindowUVE)) {
            ImGui::MenuItem("Scene", nullptr, &m_scenePanelVisible);
            ImGui::MenuItem("Inspector", nullptr, &m_inspectorPanelVisible);
            ImGui::MenuItem("Filesystem + Debug Dock", nullptr, &m_bottomDockVisible);
            ImGui::MenuItem("Plugin Tools", nullptr, &m_pluginWindowVisible);
            ImGui::Separator();
            if (ImGui::MenuItem("Default Layout")) {
                ApplyLayoutPresetUVE(EditorLayoutPresetUVE::Default);
            }
            if (ImGui::MenuItem("Focus Viewport")) {
                ApplyLayoutPresetUVE(EditorLayoutPresetUVE::FocusViewport);
            }
            if (ImGui::MenuItem("Content Review")) {
                ApplyLayoutPresetUVE(EditorLayoutPresetUVE::ContentReview);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Scene Workspace", nullptr, m_activeWorkspace == EditorWorkspaceUVE::Library)) {
                m_activeWorkspace = EditorWorkspaceUVE::Library;
            }
            if (ImGui::MenuItem("Scripting Workspace", nullptr, m_activeWorkspace == EditorWorkspaceUVE::Scripting)) {
                m_activeWorkspace = EditorWorkspaceUVE::Scripting;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(kMenuLabelHelpUVE)) {
            ImGui::MenuItem("UVE Editor Reference", nullptr, false, false);
            ImGui::MenuItem("About UNIVEX Engine", nullptr, false, false);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
        const float playbackGroupWidth = 2.0F * 58.0F + 4.0F;
        ImGui::SetCursorScreenPos(ImVec2{menuMin.x + (ImGui::GetWindowWidth() - playbackGroupWidth) * 0.5F,
                                       menuMin.y + 1.0F});
        const auto drawMenuPlaybackButton = [](const char* const id, const char* const label, const int iconKind,
                                                const bool enabled) {
            ImGui::BeginDisabled(!enabled);
            ImGui::PushID(id);
            const bool pressed = ImGui::Button("##menu-playback", ImVec2{58.0F, 22.0F});
            const ImVec2 minimum = ImGui::GetItemRectMin();
            const ImVec2 maximum = ImGui::GetItemRectMax();
            const float centerY = (minimum.y + maximum.y) * 0.5F;
            ImDrawList* const iconDrawList = ImGui::GetWindowDrawList();
            const ImU32 iconColor = ImGui::GetColorU32(ImGuiCol_Text);
            if (iconKind == 0) {
                iconDrawList->AddTriangleFilled(ImVec2{minimum.x + 7.0F, centerY - 6.0F},
                                                ImVec2{minimum.x + 7.0F, centerY + 6.0F},
                                                ImVec2{minimum.x + 17.0F, centerY}, iconColor);
            } else if (iconKind == 1) {
                iconDrawList->AddRectFilled(ImVec2{minimum.x + 7.0F, centerY - 6.0F},
                                            ImVec2{minimum.x + 11.0F, centerY + 6.0F}, iconColor);
                iconDrawList->AddRectFilled(ImVec2{minimum.x + 13.0F, centerY - 6.0F},
                                            ImVec2{minimum.x + 17.0F, centerY + 6.0F}, iconColor);
            } else {
                iconDrawList->AddRectFilled(ImVec2{minimum.x + 7.0F, centerY - 5.0F},
                                            ImVec2{minimum.x + 17.0F, centerY + 5.0F}, iconColor);
            }
            iconDrawList->AddText(ImVec2{minimum.x + 23.0F, minimum.y + 4.0F}, iconColor, label);
            ImGui::PopID();
            ImGui::EndDisabled();
            return pressed;
        };
        const bool menuCanEnterPlayMode = m_simulationControl != nullptr &&
                                           m_playModeState == EditorPlayModeStateUVE::Edit;
        const char* const playLabel = m_playModeState == EditorPlayModeStateUVE::Playing ? "Pause" :
                                      (m_playModeState == EditorPlayModeStateUVE::Paused ? "Resume" : "Play");
        const int playIconKind = m_playModeState == EditorPlayModeStateUVE::Playing ? 1 : 0;
        const bool playEnabled = m_playModeState == EditorPlayModeStateUVE::Edit ? menuCanEnterPlayMode : true;
        if (drawMenuPlaybackButton("##menu-playback-play", playLabel, playIconKind, playEnabled)) {
            if (m_playModeState == EditorPlayModeStateUVE::Edit) {
                static_cast<void>(EnterPlayModeUVE());
            } else if (m_playModeState == EditorPlayModeStateUVE::Playing) {
                static_cast<void>(PausePlayModeUVE());
            } else {
                static_cast<void>(ResumePlayModeUVE());
            }
        }
        ImGui::SameLine(0.0F, 4.0F);
        if (drawMenuPlaybackButton("##menu-playback-stop", "Stop", 2,
                                  m_playModeState != EditorPlayModeStateUVE::Edit)) {
            static_cast<void>(StopPlayModeUVE());
        }
        ImGui::End();
    }

    if (beginChrome("##uve-tool-row", kEditorTitleBarHeightUVE + kEditorMenuBarHeightUVE,
                    kEditorToolbarHeightUVE)) {
        ImDrawList* const toolbarDrawList = ImGui::GetWindowDrawList();
        const ImVec2 toolbarMin = ImGui::GetWindowPos();
        const ImVec2 toolbarMax{toolbarMin.x + ImGui::GetWindowWidth(), toolbarMin.y + kEditorToolbarHeightUVE};
        toolbarDrawList->AddRectFilled(toolbarMin, toolbarMax, IM_COL32(32, 37, 43, 255));
        toolbarDrawList->AddLine(ImVec2{toolbarMin.x, toolbarMin.y}, ImVec2{toolbarMax.x, toolbarMin.y},
                                 IM_COL32(48, 55, 64, 235), 1.0F);
        const auto drawWorkspace = [this](const char* const label, const EditorWorkspaceUVE workspace) {
            const bool active = m_activeWorkspace == workspace;
            if (active) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.20F, 0.21F, 0.23F, 1.0F});
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.32F, 0.35F, 0.39F, 1.0F});
            }
            if (ImGui::SmallButton(label)) {
                m_activeWorkspace = workspace;
            }
            if (active) {
                ImGui::PopStyleColor(2);
            }
            ImGui::SameLine();
        };
        drawWorkspace("Scene", EditorWorkspaceUVE::Library);
        drawWorkspace("Scripting", EditorWorkspaceUVE::Scripting);
        ImGui::End();
    }
}

void EditorUVE::DrawPluginWindowUVE() {
    if (!m_pluginWindowVisible) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2{340.0F, 0.0F}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2{ImGui::GetMainViewport()->WorkPos.x + 260.0F,
                                   ImGui::GetMainViewport()->WorkPos.y + 104.0F},
                            ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Plugin Tools", &m_pluginWindowVisible, ImGuiWindowFlags_AlwaysAutoResize)) {
        DrawNativeIconLabelUVE(m_uiAssets.GetGeneralIconTextureIdUVE("plugin"), "Editor tools");
        ImGui::Separator();
        ImGui::TextDisabled("No editor plugins are currently installed.");
    }
    ImGui::End();
}

void EditorUVE::DrawBottomDockUVE() {
    // Filesystem and Debug are rendered as one bottom canvas by DrawBottomDockContentUVE().
    // Keep this entry point for session/layout compatibility; the old selector strip is intentionally gone.
}

void EditorUVE::DrawBottomDockContentUVE() {
    if (!m_bottomDockVisible) {
        return;
    }
    if (m_activeBottomDock == EditorBottomDockUVE::FileSystem) {
        DrawAssetsPanelUVE();
        DrawFolderContentsPanelUVE();
        DrawFilesystemContextPopupUVE();
        return;
    }

    const ImGuiViewport* const mainViewport = ImGui::GetMainViewport();
    const float contentHeight = kAssetsPanelHeightUVE;
    // FirstUseEver, not Always - see DrawHierarchyPanelUVE()'s comment on the same change.
    ImGui::SetNextWindowPos(
        ImVec2{mainViewport->WorkPos.x, mainViewport->WorkPos.y + mainViewport->WorkSize.y -
                                      kAssetsPanelHeightUVE},
        ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2{mainViewport->WorkSize.x, contentHeight}, ImGuiCond_FirstUseEver);
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
    ImGui::Begin("Debug##lower-workspace", nullptr, flags);
    switch (m_activeBottomDock) {
        case EditorBottomDockUVE::Debugger: {
            ImGui::TextUnformatted("Debug Output");
            const Render::Renderer3DFrameDiagnosticsUVE diagnostics =
                m_services->GetRenderer3DUVE().GetLastFrameDiagnosticsUVE();
            ImGui::Text("Frame: primitives %zu | meshes %zu | particles %zu",
                        diagnostics.primitiveItemsExtracted, diagnostics.meshItemsExtracted,
                        diagnostics.particleItemsExtracted);
            ImGui::Text("Submission: mesh draws %zu | primitive draws %zu | GL draws %zu",
                        diagnostics.meshDrawCallsRecorded, diagnostics.primitiveDrawCallsRecorded,
                        diagnostics.glDrawCallsIssued);
            ImGui::Text("Scene state: %s | Undo: %s | Redo: %s",
                        m_sceneDirty ? "unsaved" : "saved", CanUndoUVE() ? "available" : "empty",
                        CanRedoUVE() ? "available" : "empty");
            break;
        }
        case EditorBottomDockUVE::Animator:
            ImGui::TextDisabled("Playback controls are centered in the File / Edit menu canvas.");
            break;
        case EditorBottomDockUVE::AIToolbar:
            ImGui::TextUnformatted("AI Tools");
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
        const std::string displayLabel = GetEntityDisplayLabelUVE(entity);
        const std::string typeTag = GetOutlinerTypeTagUVE(entity);
        const bool hasTypeQuery = m_hierarchyFilter.rfind("type:", 0U) == 0U;
        const std::string_view typeQuery = hasTypeQuery ? std::string_view{m_hierarchyFilter}.substr(5U) : std::string_view{};
        const bool typeMatches = !hasTypeQuery || ContainsCaseInsensitiveUVE(typeTag, typeQuery);
        Scene::EntityUVE parent = Scene::kInvalidEntityUVE;
        const bool isRoot = !TryGetDocumentParentUVE(entity, parent) || parent == Scene::kInvalidEntityUVE;
        const bool nameMatches = hasTypeQuery || ContainsCaseInsensitiveUVE(displayLabel, m_hierarchyFilter) ||
                                 (m_hierarchyFilter == "root" && isRoot);
        bool visible = typeMatches && nameMatches;
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

void EditorUVE::DrawViewportToolbarUVE() {
    namespace Widgets = ViewportWidgets;

    const ImGuiStyle& style = ImGui::GetStyle();
    const float iconButton = ImGui::GetFrameHeight();
    // Captured before anything is drawn: once the left group has been laid out the cursor has
    // already advanced to the next line, so this is the only point at which the row's own top is
    // still readable.
    const float rowTop = ImGui::GetCursorScreenPos().y;

    // ---- left group: the viewport's own menus -----------------------------------------------
    if (Widgets::ToolIconButtonUVE("viewport-menu", Widgets::ToolIconUVE::Menu, false, iconButton,
                                   "Viewport Options")) {
        ImGui::OpenPopup("viewport-options-popup");
    }
    if (ImGui::BeginPopup("viewport-options-popup")) {
        if (ImGui::MenuItem("Focus Selection", "F")) {
            FocusViewportOnSelectionUVE();
        }
        if (ImGui::MenuItem("Reset View", "Home")) {
            m_viewportCameraController = EditorViewportCameraUVE{};
            m_viewportSettings.standardView = EditorStandardViewUVE::User;
            SetViewportProjectionUVE(EditorViewportProjectionModeUVE::Perspective);
        }
        ImGui::Separator();
        for (const EditorStandardViewUVE view :
             {EditorStandardViewUVE::Top, EditorStandardViewUVE::Bottom, EditorStandardViewUVE::Front,
              EditorStandardViewUVE::Rear, EditorStandardViewUVE::Left, EditorStandardViewUVE::Right}) {
            if (ImGui::MenuItem(GetStandardViewNameUVE(view), nullptr,
                                m_viewportSettings.standardView == view)) {
                SetStandardViewUVE(view);
            }
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine(0.0F, style.ItemSpacing.x);
    // The projection control shows the CURRENT mode as its label and opens the alternative on
    // click - one control that reports state, not two permanently visible buttons.
    if (ImGui::Button(GetProjectionModeNameUVE(m_viewportSettings.projection))) {
        ImGui::OpenPopup("viewport-projection-popup");
    }
    if (ImGui::BeginPopup("viewport-projection-popup")) {
        for (const EditorViewportProjectionModeUVE projection :
             {EditorViewportProjectionModeUVE::Perspective,
              EditorViewportProjectionModeUVE::Orthographic}) {
            if (ImGui::MenuItem(GetProjectionModeNameUVE(projection), nullptr,
                                m_viewportSettings.projection == projection)) {
                SetViewportProjectionUVE(projection);
                // An explicit projection choice is a user decision about the free view, so it must
                // not be silently undone by the auto-orthographic axis-view rule.
                m_viewportSettings.standardView = EditorStandardViewUVE::User;
            }
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine(0.0F, style.ItemSpacing.x);
    if (ImGui::Button(GetDisplayModeNameUVE(m_viewportSettings.display))) {
        ImGui::OpenPopup("viewport-display-popup");
    }
    if (ImGui::BeginPopup("viewport-display-popup")) {
        for (const EditorViewportDisplayModeUVE display :
             {EditorViewportDisplayModeUVE::Normal, EditorViewportDisplayModeUVE::Unshaded,
              EditorViewportDisplayModeUVE::Wireframe}) {
            if (ImGui::MenuItem(GetDisplayModeNameUVE(display), nullptr,
                                m_viewportSettings.display == display)) {
                SetViewportDisplayModeUVE(display);
            }
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine(0.0F, style.ItemSpacing.x);
    if (ImGui::Button("Show")) {
        ImGui::OpenPopup("viewport-show-popup");
    }
    if (ImGui::BeginPopup("viewport-show-popup")) {
        ImGui::MenuItem("Grid", "G", &m_viewportSettings.showGrid);
        ImGui::MenuItem("Orientation Gizmo", "N", &m_viewportSettings.showNavGizmo);
        ImGui::MenuItem("Transform Gizmo", "H", &m_viewportSettings.showTransformGizmo);
        ImGui::Separator();
        ImGui::MenuItem("Auto-Orthographic Axis Views", nullptr, &m_viewportSettings.autoOrthographic);
        ImGui::EndPopup();
    }

    // ---- right group: the transform tools and viewport toggles --------------------------------
    // Laid out from the right edge so the row reads as two groups with the viewport between them,
    // and so it stays correct at any panel width instead of relying on a hardcoded offset.
    struct ToolEntryUVE final {
        EditorGizmoModeUVE mode;
        Widgets::ToolIconUVE icon;
        const char* id;
        const char* tooltip;
    };
    const std::array<ToolEntryUVE, 5> tools = {{
        {EditorGizmoModeUVE::Select, Widgets::ToolIconUVE::Select, "tool-select", "Select (Q)"},
        {EditorGizmoModeUVE::Move, Widgets::ToolIconUVE::Move, "tool-move", "Move (W)"},
        {EditorGizmoModeUVE::Rotate, Widgets::ToolIconUVE::Rotate, "tool-rotate", "Rotate (E)"},
        {EditorGizmoModeUVE::Scale, Widgets::ToolIconUVE::Scale, "tool-scale", "Scale (R)"},
        {EditorGizmoModeUVE::Universal, Widgets::ToolIconUVE::Universal, "tool-universal",
         "Universal (T)"},
    }};

    constexpr int kTrailingToggleCountUVE = 3;
    const auto buttonCount = static_cast<float>(tools.size() + kTrailingToggleCountUVE);
    const float rightGroupWidth =
        (buttonCount * iconButton) + ((buttonCount - 1.0F) * style.ItemSpacing.x);

    // Positioned from the panel's right edge in absolute screen coordinates rather than by a
    // computed SameLine() spacing: the spacing form silently pushes the group outside the window's
    // clip rect when the arithmetic overshoots by even a pixel, which drops the whole tool row.
    const float panelRight = ImGui::GetWindowPos().x + ImGui::GetWindowSize().x;
    const float leftGroupRight = ImGui::GetItemRectMax().x + style.ItemSpacing.x;
    const float rightGroupLeft =
        std::max(leftGroupRight, panelRight - style.FramePadding.x - rightGroupWidth);
    ImGui::SetCursorScreenPos(ImVec2{rightGroupLeft, rowTop});

    for (const ToolEntryUVE& tool : tools) {
        if (Widgets::ToolIconButtonUVE(tool.id, tool.icon, m_gizmoMode == tool.mode, iconButton,
                                       tool.tooltip)) {
            SetGizmoModeUVE(tool.mode);
        }
        ImGui::SameLine(0.0F, style.ItemSpacing.x);
    }

    if (Widgets::ToolIconButtonUVE("viewport-grid", Widgets::ToolIconUVE::Grid,
                                   m_viewportSettings.showGrid, iconButton, "Show Grid (G)")) {
        m_viewportSettings.showGrid = !m_viewportSettings.showGrid;
    }
    ImGui::SameLine(0.0F, style.ItemSpacing.x);
    if (Widgets::ToolIconButtonUVE("viewport-snap", Widgets::ToolIconUVE::Snap,
                                   m_transformSnappingSettings.enabled, iconButton, "Transform Snapping")) {
        EditorTransformSnappingSettingsUVE snapping = m_transformSnappingSettings;
        snapping.enabled = !snapping.enabled;
        static_cast<void>(SetTransformSnappingSettingsUVE(snapping));
    }
    ImGui::SameLine(0.0F, style.ItemSpacing.x);
    if (Widgets::ToolIconButtonUVE("viewport-nav", Widgets::ToolIconUVE::Camera,
                                   m_viewportSettings.showNavGizmo, iconButton,
                                   "Orientation Gizmo (N)")) {
        m_viewportSettings.showNavGizmo = !m_viewportSettings.showNavGizmo;
    }
}

void EditorUVE::HandleViewportNavigationInputUVE(const Math::Vector2UVE& viewportSize) {
    const ImGuiIO& io = ImGui::GetIO();
    const ImVec2 dragDelta = io.MouseDelta;

    // Left drag orbits, middle/right drag pans, wheel dollies - the standard editor gesture set.
    // Deltas are handed to the camera unmodified: the turntable convention lives in
    // EditorViewportCameraUVE and is asserted by its own tests, so negating anything here would
    // reintroduce exactly the inversion this rebuild set out to fix.
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0F)) {
        m_viewportCameraController.OrbitUVE(dragDelta.x, dragDelta.y);
        m_viewportSettings.standardView = EditorStandardViewUVE::User;
        if (m_viewportSettings.autoOrthographic &&
            m_viewportSettings.projection == EditorViewportProjectionModeUVE::Orthographic) {
            // Orbiting away from an axis view returns to perspective, because a free orbit in
            // orthographic is almost never what the user was after when they pressed Front.
            SetViewportProjectionUVE(EditorViewportProjectionModeUVE::Perspective);
        }
    } else if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0F) ||
               ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0F)) {
        m_viewportCameraController.PanUVE(dragDelta.x, dragDelta.y,
                                          static_cast<int>(viewportSize.y));
    }

    if (std::fabs(io.MouseWheel) > 0.0F) {
        m_viewportCameraController.DollyUVE(io.MouseWheel);
    }
}

void EditorUVE::DrawNavGizmoOverlayUVE(const Math::Vector2UVE& viewportOrigin,
                                       const Math::Vector2UVE& viewportSize) {
    const float widgetSize = m_gizmoStyle.navPixelSize;
    const float margin = m_gizmoStyle.navMarginPx;
    if (viewportSize.x < (widgetSize + (margin * 2.0F)) ||
        viewportSize.y < (widgetSize + (margin * 2.0F))) {
        return;
    }

    const ImVec2 widgetOrigin{viewportOrigin.x + viewportSize.x - widgetSize - margin,
                              viewportOrigin.y + margin};

    const Math::Matrix4x4UVE viewRotation = m_viewportCameraController.GetViewMatrixUVE();
    const Math::Vector3UVE viewDirection = Math::NormalizeUVE(
        m_viewportCameraController.GetTargetUVE() - m_viewportCameraController.GetEyeUVE());
    const EditorNavGizmoMeshesUVE meshes = BuildNavGizmoMeshesUVE(m_gizmoStyle, viewDirection);

    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    const float halfExtent = GetNavViewHalfExtentUVE(m_gizmoStyle);
    ViewportWidgets::DrawNavGizmoMeshUVE(*drawList, meshes.underlay, viewRotation, widgetOrigin,
                                         widgetSize, halfExtent);
    ViewportWidgets::DrawNavGizmoMeshUVE(*drawList, meshes.overlay, viewRotation, widgetOrigin,
                                         widgetSize, halfExtent);

    // The widget answers two gestures. Which one happened is decided at release by whether the
    // pointer travelled past a few pixels, so a click never jerks the view and a drag never snaps
    // at the end of itself.
    ImGui::SetCursorScreenPos(widgetOrigin);
    ImGui::InvisibleButton("##viewport-nav-gizmo", ImVec2{widgetSize, widgetSize});
    const bool hovered = ImGui::IsItemHovered();
    const ImGuiIO& io = ImGui::GetIO();

    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        m_navGizmoPressed = true;
        m_navGizmoDragged = false;
        m_navGizmoPressPosition = Math::Vector2UVE{io.MousePos.x, io.MousePos.y};
    }
    if (m_navGizmoPressed && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        const float travelX = io.MousePos.x - m_navGizmoPressPosition.x;
        const float travelY = io.MousePos.y - m_navGizmoPressPosition.y;
        constexpr float kDragThresholdPixelsUVE = 4.0F;
        if (((travelX * travelX) + (travelY * travelY)) >
            (kDragThresholdPixelsUVE * kDragThresholdPixelsUVE)) {
            m_navGizmoDragged = true;
        }
        if (m_navGizmoDragged) {
            m_viewportCameraController.OrbitUVE(io.MouseDelta.x, io.MouseDelta.y);
            m_viewportSettings.standardView = EditorStandardViewUVE::User;
        }
    }
    if (m_navGizmoPressed && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        if (!m_navGizmoDragged) {
            const EditorNavPickResultUVE pick =
                PickNavGizmoUVE(m_gizmoStyle, viewRotation, io.MousePos.x - widgetOrigin.x,
                                io.MousePos.y - widgetOrigin.y, widgetSize);
            if (pick.hit) {
                m_viewportCameraController.SnapToDirectionUVE(pick.direction);
                if (m_viewportSettings.autoOrthographic) {
                    SetViewportProjectionUVE(EditorViewportProjectionModeUVE::Orthographic);
                }
            }
        }
        m_navGizmoPressed = false;
        m_navGizmoDragged = false;
    }
}

void EditorUVE::DrawViewportPanelUVE() {
    const ImGuiViewport* const mainViewport = ImGui::GetMainViewport();
    const float menuBarHeight = kEditorTopChromeHeightUVE;
    const float workspaceHeight =
        std::max(kMinimumViewportHeightUVE,
                 mainViewport->WorkSize.y - menuBarHeight -
                     (m_bottomDockVisible ? kAssetsPanelHeightUVE : 0.0F));
    const float scenePanelWidth = std::clamp(mainViewport->WorkSize.x * 0.19F, 208.0F, 292.0F);
    const float inspectorWidth = std::clamp(mainViewport->WorkSize.x * 0.21F, 232.0F, 340.0F);

    ImGui::SetNextWindowPos(
        ImVec2{mainViewport->WorkPos.x + scenePanelWidth, mainViewport->WorkPos.y + menuBarHeight},
        ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(
        ImVec2{std::max(kMinimumViewportHeightUVE,
                        mainViewport->WorkSize.x - scenePanelWidth - inspectorWidth),
               workspaceHeight},
        ImGuiCond_FirstUseEver);

    // The engine has already drawn the 3D frame into this panel's sub-rect by the time the overlay
    // runs, so the window background must not paint over it. Only the toolbar strip is opaque.
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4{0.0F, 0.0F, 0.0F, 0.0F});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    const bool open = ImGui::Begin(kPanelLabelViewportUVE, nullptr, ImGuiWindowFlags_NoScrollbar |
                                                                        ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    if (!open) {
        ImGui::End();
        // A closed or fully collapsed viewport hands the whole window back to Core rather than
        // leaving the renderer aimed at a stale rectangle.
        PublishViewportRegionUVE(std::nullopt);
        return;
    }

    // ---- header toolbar ------------------------------------------------------------------------
    const ImVec2 panelOrigin = ImGui::GetCursorScreenPos();
    const float panelWidth = ImGui::GetContentRegionAvail().x;
    const ImGuiStyle& style = ImGui::GetStyle();
    const float toolbarHeight = ImGui::GetFrameHeight() + (style.FramePadding.y * 2.0F);

    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(panelOrigin, ImVec2{panelOrigin.x + panelWidth, panelOrigin.y + toolbarHeight},
                            ImGui::GetColorU32(ImGuiCol_MenuBarBg));
    drawList->AddLine(ImVec2{panelOrigin.x, panelOrigin.y + toolbarHeight},
                      ImVec2{panelOrigin.x + panelWidth, panelOrigin.y + toolbarHeight},
                      ImGui::GetColorU32(ImGuiCol_Separator), 1.0F);

    ImGui::SetCursorScreenPos(ImVec2{panelOrigin.x + style.FramePadding.x,
                                     panelOrigin.y + style.FramePadding.y});
    ImGui::BeginGroup();
    DrawViewportToolbarUVE();
    ImGui::EndGroup();

    // ---- scene region --------------------------------------------------------------------------
    const ImVec2 sceneOrigin{panelOrigin.x, panelOrigin.y + toolbarHeight};
    const ImVec2 sceneSize{panelWidth,
                           std::max(1.0F, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y -
                                              sceneOrigin.y)};

    ImGui::SetCursorScreenPos(sceneOrigin);
    ImGui::InvisibleButton("##viewport-scene", sceneSize,
                           ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight |
                               ImGuiButtonFlags_MouseButtonMiddle);
    const bool sceneHovered = ImGui::IsItemHovered();
    const bool sceneActive = ImGui::IsItemActive();

    // Publish the region in framebuffer pixels with a GL bottom-left origin, which is the
    // convention Render::ViewportRectUVE and EngineCoreUVE::SetEditorViewportRegionUVE document.
    const ImGuiIO& io = ImGui::GetIO();
    const float scaleX = (io.DisplayFramebufferScale.x > 0.0F) ? io.DisplayFramebufferScale.x : 1.0F;
    const float scaleY = (io.DisplayFramebufferScale.y > 0.0F) ? io.DisplayFramebufferScale.y : 1.0F;
    const float framebufferHeight = io.DisplaySize.y * scaleY;
    const float regionTop = (sceneOrigin.y - mainViewport->Pos.y) * scaleY;
    const float regionHeight = sceneSize.y * scaleY;
    const float regionLeft = (sceneOrigin.x - mainViewport->Pos.x) * scaleX;
    const float regionWidth = sceneSize.x * scaleX;
    if (regionWidth >= 1.0F && regionHeight >= 1.0F) {
        Render::ViewportRectUVE region{};
        region.x = static_cast<std::uint32_t>(std::max(0.0F, regionLeft));
        region.y = static_cast<std::uint32_t>(
            std::max(0.0F, framebufferHeight - regionTop - regionHeight));
        region.width = static_cast<std::uint32_t>(regionWidth);
        region.height = static_cast<std::uint32_t>(regionHeight);
        PublishViewportRegionUVE(region);
    }

    // ---- overlays ------------------------------------------------------------------------------
    const float aspectRatio = (sceneSize.y > 0.0F) ? (sceneSize.x / sceneSize.y) : 1.0F;
    if (m_viewportSettings.showTransformGizmo && m_gizmoMode != EditorGizmoModeUVE::Select &&
        IsDocumentEntityUVE(m_selectedEntity)) {
        ViewportWidgets::ViewportProjectionUVE projection{};
        projection.viewProjection = m_viewportCameraController.GetViewProjectionUVE(aspectRatio);
        projection.view = m_viewportCameraController.GetViewMatrixUVE();
        projection.origin = sceneOrigin;
        projection.size = sceneSize;

        // The gizmo sits on the selected entity's own transform pivot, never on the viewport
        // centre, the grid origin, or a bounding-box corner.
        Math::Vector3UVE pivot{};
        if (TryGetGizmoPivotUVE(pivot)) {
            // One gizmo unit in world space, chosen so the widget keeps a constant pixel radius
            // whether the camera is centimetres or kilometres away.
            const float distanceToPivot =
                std::max(0.001F, Math::LengthUVE(pivot - m_viewportCameraController.GetEyeUVE()));
            const float worldPerPixel =
                (2.0F * distanceToPivot *
                 std::tan(m_viewportCameraController.GetSettingsUVE().fieldOfViewYRadians * 0.5F)) /
                std::max(1.0F, sceneSize.y);
            const float unitScale = (m_gizmoStyle.gizmoPixelRadius * worldPerPixel) * 0.5F;

            const Math::Vector3UVE viewDirection = Math::NormalizeUVE(
                pivot - m_viewportCameraController.GetEyeUVE());
            const float unitsPerPixel = (unitScale > 0.0F) ? (worldPerPixel / unitScale) : 0.0F;
            const EditorGizmoMeshUVE mesh =
                BuildGizmoMeshUVE(m_gizmoMode, m_gizmoStyle, viewDirection, unitsPerPixel);
            ViewportWidgets::DrawGizmoMeshUVE(*drawList, mesh, projection, pivot, unitScale);

            // ---- handle dragging -------------------------------------------------------------
            EditorViewportProjectionUVE pickProjection{};
            pickProjection.viewProjection = projection.viewProjection;
            pickProjection.origin = Math::Vector2UVE{sceneOrigin.x, sceneOrigin.y};
            pickProjection.size = Math::Vector2UVE{sceneSize.x, sceneSize.y};
            const ImGuiIO& gizmoIo = ImGui::GetIO();
            const Math::Vector2UVE pointer{gizmoIo.MousePos.x, gizmoIo.MousePos.y};

            if (!m_gizmoDrag.active && sceneHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                static_cast<void>(BeginGizmoDragUVE(pickProjection, unitScale, pointer));
            }
            if (m_gizmoDrag.active) {
                if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    UpdateGizmoDragUVE(pickProjection, unitScale, pointer);
                } else {
                    CommitGizmoDragUVE();
                }
            }
        }
    }

    // Camera navigation is offered only when a handle is not being dragged, so a drag that strays
    // off its handle keeps transforming the object instead of suddenly orbiting the view.
    if ((sceneHovered || sceneActive) && !m_gizmoDrag.active) {
        HandleViewportNavigationInputUVE(Math::Vector2UVE{sceneSize.x, sceneSize.y});
    }

    if (m_viewportSettings.showNavGizmo) {
        DrawNavGizmoOverlayUVE(Math::Vector2UVE{sceneOrigin.x, sceneOrigin.y},
                               Math::Vector2UVE{sceneSize.x, sceneSize.y});
    }

    ImGui::End();
}

void EditorUVE::DrawHierarchyPanelUVE() {
    if (!m_scenePanelVisible) {
        return;
    }
    const ImGuiViewport* const mainViewport = ImGui::GetMainViewport();
    const float menuBarHeight = kEditorTopChromeHeightUVE;
    const float workspaceHeight = std::max(kMinimumViewportHeightUVE,
                                                  mainViewport->WorkSize.y - menuBarHeight - (m_bottomDockVisible ? kAssetsPanelHeightUVE : 0.0F));
    // ImGuiCond_FirstUseEver, not Always: this is the panel's default floating position/size
    // (matching the pre-docking layout exactly) for a fresh session with no saved imgui.ini
    // layout - once docking is enabled, forcing it every frame would fight the user's own
    // drag/resize/dock placement and ImGui's own persisted layout on subsequent launches.
    ImGui::SetNextWindowPos(ImVec2{mainViewport->WorkPos.x, mainViewport->WorkPos.y + menuBarHeight},
                            ImGuiCond_FirstUseEver);
    const float scenePanelWidth = std::clamp(mainViewport->WorkSize.x * 0.19F, 208.0F, 292.0F);
    ImGui::SetNextWindowSize(ImVec2{scenePanelWidth, workspaceHeight}, ImGuiCond_FirstUseEver);
    ImGui::Begin(kPanelLabelSceneUVE);
    std::array<char, 256> filterBuffer{};
    m_hierarchyFilter.copy(filterBuffer.data(), filterBuffer.size() - 1U);
    const float addNodeButtonWidth = ImGui::GetFrameHeight();
    const bool canCreateNode = IsAuthoringCommandAllowedUVE();
    ImGui::PushID("scene-add-node");
    ImGui::BeginDisabled(!canCreateNode);
    if (ImGui::Button("+", ImVec2{addNodeButtonWidth, addNodeButtonWidth})) {
        ImGui::OpenPopup("scene-add-node-popup");
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::SetTooltip("Add Node");
    }
    ImGui::SameLine(0.0F, ImGui::GetStyle().ItemSpacing.x);
    const bool hasSelectedScriptTarget = HasSingleDocumentSelectionUVE() && IsDocumentEntityUVE(m_selectedEntity);
    const float scriptButtonWidth = hasSelectedScriptTarget ? ImGui::GetFrameHeight() : 0.0F;
    ImGui::SetNextItemWidth(std::max(1.0F, ImGui::GetContentRegionAvail().x - scriptButtonWidth -
                                               (hasSelectedScriptTarget ? ImGui::GetStyle().ItemSpacing.x : 0.0F)));
    if (ImGui::InputTextWithHint("##hierarchy-filter", "Search Nodes", filterBuffer.data(), filterBuffer.size())) {
        m_hierarchyFilter = filterBuffer.data();
        InvalidateHierarchyFilterCacheUVE();
    }
    bool scriptButtonClicked = false;
    if (hasSelectedScriptTarget) {
        ImGui::SameLine(0.0F, ImGui::GetStyle().ItemSpacing.x);
        ImGui::BeginDisabled(!IsAuthoringCommandAllowedUVE());
        scriptButtonClicked = ImGui::SmallButton("Script");
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
            ImGui::SetTooltip("Open or create script branch for selected Node3D");
        }
    }
    if (scriptButtonClicked && hasSelectedScriptTarget) {
        const std::string branchName = GetEntityDisplayLabelUVE(m_selectedEntity);
        if (!SelectVisualScriptBranchUVE(branchName)) {
            static_cast<void>(CreateVisualScriptBranchUVE(branchName));
        }
        m_activeWorkspace = EditorWorkspaceUVE::Scripting;
    }
    if (ImGui::BeginPopup("scene-add-node-popup")) {
        ImGui::TextDisabled("Add Node");
        ImGui::Separator();
        std::string_view lastCategory;
        for (const Scene::Nodes::SceneNodeDescriptorUVE& descriptor :
             Scene::Nodes::GetSceneNodeDescriptorsUVE()) {
            if (descriptor.category != lastCategory) {
                if (!lastCategory.empty()) {
                    ImGui::Separator();
                }
                ImGui::TextUnformatted(descriptor.category.data());
                lastCategory = descriptor.category;
            }
            ImGui::BeginDisabled(!descriptor.libraryCreatable);
            if (ImGui::MenuItem(descriptor.displayName.data())) {
                static_cast<void>(CreateDocumentSceneNodeUVE(descriptor.kind));
            }
            ImGui::EndDisabled();
        }
        ImGui::EndPopup();
    }
    ImGui::PopID();
    RebuildHierarchyFilterCacheUVE();
    const float hierarchyItemsHeight = std::max(36.0F, ImGui::GetContentRegionAvail().y);
    if (ImGui::BeginChild("##scene-hierarchy-items", ImVec2{0.0F, hierarchyItemsHeight}, true,
                           ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
        ImGui::BeginDisabled(!IsAuthoringCommandAllowedUVE());
        for (const Scene::EntityUVE root : GetDocumentRootsUVE()) {
            DrawHierarchyNodeUVE(root);
        }
        if (!GetDocumentRootsUVE().empty()) {
            ImGui::Separator();
            ImGui::TextDisabled("Drop entity here to make it a root");
            AcceptHierarchyDropTargetUVE(Scene::kInvalidEntityUVE);
        }
        ImGui::EndDisabled();
        ImGui::EndChild();
    }
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
    const std::string visibleLabel = renaming ? "" : "    " + GetEntityDisplayLabelUVE(entity);
    const std::string nodeLabel = visibleLabel + "##entity-" + std::to_string(entity.index) + ":" +
                                  std::to_string(entity.generation);
    if (active) {
        ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(66, 84, 101, 235));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, IM_COL32(101, 130, 154, 245));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(88, 112, 133, 240));
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
        IsAuthoringCommandAllowedUVE() && ImGui::IsKeyPressed(ImGuiKey_F2)) {
        m_hierarchyRenameEntity = entity;
        m_hierarchyRenameBuffer = GetEntityDisplayLabelUVE(entity);
        m_hierarchyRenameFocusRequested = true;
    }
    if (!renaming && HasSingleDocumentSelectionUVE() && entity == m_selectedEntity &&
        IsAuthoringCommandAllowedUVE()) {
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
    if (!m_inspectorPanelVisible) {
        return;
    }
    const ImGuiViewport* const mainViewport = ImGui::GetMainViewport();
    const float menuBarHeight = kEditorTopChromeHeightUVE;
    const float workspaceHeight = std::max(kMinimumViewportHeightUVE,
                                           mainViewport->WorkSize.y - menuBarHeight - (m_bottomDockVisible ? kAssetsPanelHeightUVE : 0.0F));
    const float inspectorPanelWidth = std::clamp(mainViewport->WorkSize.x * 0.22F, 264.0F, 356.0F);
    // FirstUseEver, not Always - see DrawHierarchyPanelUVE()'s comment on the same change.
    ImGui::SetNextWindowPos(
        ImVec2{mainViewport->WorkPos.x + mainViewport->WorkSize.x - inspectorPanelWidth,
               mainViewport->WorkPos.y + menuBarHeight}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2{inspectorPanelWidth, workspaceHeight}, ImGuiCond_FirstUseEver);
    // NoTitleBar dropped (was the only flag actually blocking dragging - dockable/draggable
    // windows need a title bar as their default drag handle) and given a real title: an internal
    // Inspector/Import/Signals tab strip already exists below via Selectable(), so the window
    // title identifies the *panel* to dock/drag by, while that internal strip still switches the
    // panel's *content* - two different, non-conflicting notions of "tab".
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
    ImGui::Begin(kPanelLabelInspectorUVE, nullptr, flags);

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
            DrawImportQueueMonitorUVE();
            break;
        case EditorRightPanelTabUVE::Signals:
            ImGui::TextUnformatted("Signals");
            ImGui::TextDisabled("Signal bindings remain unavailable until the scripting runtime is added.");
            break;
    }
    ImGui::End();
}

void EditorUVE::DrawImportQueueMonitorUVE() {
    ImGui::TextUnformatted("Import Queue");
    ImGui::TextDisabled("Read-only monitor. Enqueue and retry are programmatic-only in v1.");

    const std::vector<Asset::AssetImportJobUVE> jobs = m_services->GetAssetImportQueueUVE().GetJobsUVE();
    if (jobs.empty()) {
        ImGui::TextDisabled("No import jobs have been queued.");
        return;
    }

    ImGui::Text("%zu job(s)", jobs.size());
    ImGui::BeginChild("##import-queue-monitor", ImVec2{0.0F, 0.0F}, true);
    for (const Asset::AssetImportJobUVE& job : jobs) {
        const std::string header = "Job #" + std::to_string(job.id.value) + " — " +
                                   ImportJobStateLabelUVE(job.state) + "##import-job-" +
                                   std::to_string(job.id.value);
        if (ImGui::TreeNodeEx(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            const std::string sourcePath = job.request.sourcePath.generic_string();
            const std::string destinationPath = job.request.destinationPath.generic_string();
            ImGui::TextWrapped("Source: %s", sourcePath.c_str());
            ImGui::TextWrapped("Destination: %s", destinationPath.c_str());
            ImGui::Text("Attempt: %u", job.attemptCount);
            ImGui::Text("Result: %s", job.cacheHit ? "Cache hit" : "Importer path");
            if (job.resultGuid.has_value()) {
                ImGui::Text("GUID: %016llX", static_cast<unsigned long long>(job.resultGuid->value));
            }
            for (const Asset::AssetImportDiagnosticUVE& diagnostic : job.diagnostics) {
                ImGui::Separator();
                ImGui::Text("%s (attempt %u)", ImportDiagnosticSeverityLabelUVE(diagnostic.severity),
                            diagnostic.attempt);
                ImGui::TextWrapped("%s", diagnostic.message.c_str());
            }
            ImGui::TreePop();
        }
    }
    ImGui::EndChild();
}

void EditorUVE::DrawInspectorContentUVE() {
    if (m_selectedEntities.empty()) {
        ImGui::BeginChild("##inspector-empty-state", ImVec2{0.0F, 64.0F}, true);
        ImGui::TextColored(ImVec4{0.80F, 0.82F, 0.85F, 1.0F}, "NO ENTITY SELECTED");
        ImGui::TextDisabled("Select an entity in Scene or Viewport to inspect it.");
        ImGui::EndChild();
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
    std::array<char, 128> inspectorFilterBuffer{};
    m_inspectorFilter.copy(inspectorFilterBuffer.data(), inspectorFilterBuffer.size() - 1U);
    ImGui::SetNextItemWidth(-1.0F);
    if (ImGui::InputTextWithHint("##inspector-filter", "Search properties...",
                                inspectorFilterBuffer.data(), inspectorFilterBuffer.size())) {
        m_inspectorFilter = inspectorFilterBuffer.data();
    }
    if (m_inspectorFilter.empty()) {
        m_inspectorDrawerRegistry.DrawEligibleUVE(m_selectedEntity);
    } else {
        m_inspectorDrawerRegistry.DrawEligibleMatchingUVE(m_selectedEntity, m_inspectorFilter);
    }
    DrawSceneComponentAddPanelUVE();
    if (!m_services->GetEntityManagerUVE().HasComponentUVE<Scene::TransformComponentUVE>(m_selectedEntity)) {
        ImGui::TextUnformatted("No local Transform component.");
    }
    ImGui::EndDisabled();
}

void EditorUVE::RegisterBuiltInInspectorDrawersUVE() {
    static_cast<void>(m_inspectorDrawerRegistry.RegisterDrawerUVE(InspectorDrawerEntryUVE{
        "name",
        [this](const Scene::EntityUVE entity) { return IsDocumentEntityUVE(entity); },
        [this](const Scene::EntityUVE entity) { DrawNameInspectorDrawerUVE(entity); },
    }));
    static_cast<void>(m_inspectorDrawerRegistry.RegisterDrawerUVE(InspectorDrawerEntryUVE{
        "hierarchy",
        [this](const Scene::EntityUVE entity) { return IsDocumentEntityUVE(entity); },
        [this](const Scene::EntityUVE entity) { DrawHierarchyInspectorDrawerUVE(entity); },
    }));
    static_cast<void>(m_inspectorDrawerRegistry.RegisterDrawerUVE(InspectorDrawerEntryUVE{
        "transform",
        [this](const Scene::EntityUVE entity) {
            return IsDocumentEntityUVE(entity) &&
                   m_services->GetEntityManagerUVE().HasComponentUVE<Scene::TransformComponentUVE>(entity);
        },
        [this](const Scene::EntityUVE entity) { DrawTransformInspectorDrawerUVE(entity); },
    }));
    static_cast<void>(m_inspectorDrawerRegistry.RegisterDrawerUVE(InspectorDrawerEntryUVE{
        "primitive-mesh",
        [this](const Scene::EntityUVE entity) {
            return IsDocumentEntityUVE(entity) &&
                   m_services->GetEntityManagerUVE().HasComponentUVE<Scene::TransformComponentUVE>(entity) &&
                   m_services->GetEntityManagerUVE().HasComponentUVE<Scene::PrimitiveMeshComponentUVE>(entity);
        },
        [this](const Scene::EntityUVE entity) { DrawPrimitiveMeshInspectorDrawerUVE(entity); },
    }));
    const auto registerComponentDrawer = [this](const char* const id, const EditorSceneComponentKindUVE kind) {
        static_cast<void>(m_inspectorDrawerRegistry.RegisterDrawerUVE(InspectorDrawerEntryUVE{
            id,
            [this, kind](const Scene::EntityUVE entity) {
                if (!IsDocumentEntityUVE(entity)) {
                    return false;
                }
                const Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
                switch (kind) {
                    case EditorSceneComponentKindUVE::Camera:
                        return entityManager.HasComponentUVE<Scene::CameraComponentUVE>(entity);
                    case EditorSceneComponentKindUVE::Mesh:
                        return entityManager.HasComponentUVE<Scene::MeshComponentUVE>(entity);
                    case EditorSceneComponentKindUVE::Light:
                        return entityManager.HasComponentUVE<Scene::LightComponentUVE>(entity);
                    case EditorSceneComponentKindUVE::Collider:
                        return entityManager.HasComponentUVE<Scene::ColliderComponentUVE>(entity);
                    case EditorSceneComponentKindUVE::RigidBody:
                        return entityManager.HasComponentUVE<Scene::RigidBodyComponentUVE>(entity);
                    case EditorSceneComponentKindUVE::AudioSource:
                        return entityManager.HasComponentUVE<Scene::AudioSourceComponentUVE>(entity);
                    case EditorSceneComponentKindUVE::ParticleEmitter:
                        return entityManager.HasComponentUVE<Scene::ParticleEmitterComponentUVE>(entity);
                    case EditorSceneComponentKindUVE::Script:
                        return entityManager.HasComponentUVE<Scene::ScriptComponentUVE>(entity);
                    case EditorSceneComponentKindUVE::AnimationPlayer:
                        return entityManager.HasComponentUVE<Scene::AnimationPlayerComponentUVE>(entity);
                    case EditorSceneComponentKindUVE::WorldEnvironment:
                        return entityManager.HasComponentUVE<Scene::WorldEnvironment3DNodeComponentUVE>(entity);
                }
                return false;
            },
            [this, kind](const Scene::EntityUVE entity) { DrawSceneComponentInspectorDrawerUVE(entity, kind); },
        }));
    };
    registerComponentDrawer("camera", EditorSceneComponentKindUVE::Camera);
    registerComponentDrawer("mesh", EditorSceneComponentKindUVE::Mesh);
    registerComponentDrawer("light", EditorSceneComponentKindUVE::Light);
    registerComponentDrawer("collider", EditorSceneComponentKindUVE::Collider);
    registerComponentDrawer("rigid-body", EditorSceneComponentKindUVE::RigidBody);
    registerComponentDrawer("audio-source", EditorSceneComponentKindUVE::AudioSource);
    registerComponentDrawer("particle-emitter", EditorSceneComponentKindUVE::ParticleEmitter);
    registerComponentDrawer("script", EditorSceneComponentKindUVE::Script);
    registerComponentDrawer("animation-player", EditorSceneComponentKindUVE::AnimationPlayer);
    static_cast<void>(m_inspectorDrawerRegistry.RegisterDrawerUVE(InspectorDrawerEntryUVE{
        "world-environment",
        [this](const Scene::EntityUVE entity) {
            return IsDocumentEntityUVE(entity) &&
                   m_services->GetEntityManagerUVE().HasComponentUVE<Scene::WorldEnvironment3DNodeComponentUVE>(entity);
        },
        [this](const Scene::EntityUVE entity) { DrawWorldEnvironmentInspectorDrawerUVE(entity); },
    }));
    static_cast<void>(m_inspectorDrawerRegistry.RegisterDrawerUVE(InspectorDrawerEntryUVE{
        "prefab-instance",
        [this](const Scene::EntityUVE entity) {
            return IsDocumentEntityUVE(entity) &&
                   m_services->GetEntityManagerUVE().HasComponentUVE<Scene::PrefabInstanceComponentUVE>(entity);
        },
        [this](const Scene::EntityUVE entity) { DrawPrefabInspectorDrawerUVE(entity); },
    }));
}

void EditorUVE::DrawNameInspectorDrawerUVE(const Scene::EntityUVE entity) {
    if (!IsDocumentEntityUVE(entity)) {
        return;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    DrawNativeIconLabelUVE(0U, "Name");
    std::array<char, kMaximumEntityNameBytesUVE + 1U> nameBuffer{};
    if (entityManager.HasComponentUVE<Scene::NameComponentUVE>(entity)) {
        const std::string& currentName = entityManager.GetComponentUVE<Scene::NameComponentUVE>(entity).name;
        currentName.copy(nameBuffer.data(), std::min(currentName.size(), nameBuffer.size() - 1U));
    }
    if (ImGui::InputText("Name", nameBuffer.data(), nameBuffer.size())) {
        static_cast<void>(SetSelectedEntityNameUVE(nameBuffer.data()));
    }
}

void EditorUVE::DrawHierarchyInspectorDrawerUVE(const Scene::EntityUVE entity) {
    if (!IsDocumentEntityUVE(entity) || entity != m_selectedEntity) {
        return;
    }

    ImGui::Separator();
    DrawNativeIconLabelUVE(0U, "Hierarchy");
    Scene::EntityUVE currentParent = Scene::kInvalidEntityUVE;
    if (!TryGetDocumentParentUVE(entity, currentParent)) {
        ImGui::TextDisabled("Parent unavailable due to invalid hierarchy state.");
        return;
    }

    if (currentParent == Scene::kInvalidEntityUVE) {
        ImGui::TextDisabled("Parent: Root");
    } else {
        ImGui::Text("Parent: %s", GetHierarchyCandidateLabelUVE(currentParent).c_str());
    }

    const std::vector<Scene::EntityUVE> ancestry = GetDocumentAncestryUVE(entity);
    if (!ancestry.empty()) {
        ImGui::TextDisabled("Ancestry (read-only)");
        for (const Scene::EntityUVE ancestor : ancestry) {
            ImGui::BulletText("%s", GetHierarchyCandidateLabelUVE(ancestor).c_str());
        }
    }

    const bool canReparent = IsLifecycleCommandAllowedUVE();
    ImGui::BeginDisabled(!canReparent);
    int reparentModeIndex =
        m_reparentTransformMode == EditorReparentTransformModeUVE::KeepWorld ? 1 : 0;
    constexpr const char* kReparentModes[] = {"Keep Local", "Keep World"};
    if (ImGui::Combo("Reparent Transform", &reparentModeIndex, kReparentModes,
                     static_cast<int>(std::size(kReparentModes)))) {
        const EditorReparentTransformModeUVE requestedMode = reparentModeIndex == 1
                                                                  ? EditorReparentTransformModeUVE::KeepWorld
                                                                  : EditorReparentTransformModeUVE::KeepLocal;
        static_cast<void>(SetReparentTransformModeUVE(requestedMode));
    }

    const std::string parentPreview = currentParent == Scene::kInvalidEntityUVE
                                          ? "Root"
                                          : GetHierarchyCandidateLabelUVE(currentParent);
    if (ImGui::BeginCombo("New Parent", parentPreview.c_str())) {
        for (const Scene::EntityUVE candidate : GetEligibleReparentParentsUVE(entity)) {
            const bool isCurrentParent = candidate == currentParent;
            ImGui::BeginDisabled(isCurrentParent);
            const std::string candidateLabel = GetHierarchyCandidateLabelUVE(candidate) + "##reparent-" +
                                               std::to_string(candidate.index) + ":" +
                                               std::to_string(candidate.generation);
            if (ImGui::Selectable(candidateLabel.c_str(), false) && !isCurrentParent) {
                static_cast<void>(ReparentSelectedEntityUVE(candidate));
            }
            ImGui::EndDisabled();
        }
        ImGui::EndCombo();
    }

    ImGui::BeginDisabled(currentParent == Scene::kInvalidEntityUVE);
    if (ImGui::Button("Make Root")) {
        static_cast<void>(ReparentSelectedEntityUVE(Scene::kInvalidEntityUVE));
    }
    ImGui::EndDisabled();
    ImGui::EndDisabled();
}

void EditorUVE::DrawTransformInspectorDrawerUVE(const Scene::EntityUVE entity) {
    if (!IsDocumentEntityUVE(entity)) {
        return;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::TransformComponentUVE>(entity)) {
        return;
    }

    Scene::TransformComponentUVE edited = entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity);
    ImGui::Separator();
    DrawNativeIconLabelUVE(0U, "Transform");
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
}

void EditorUVE::DrawPrimitiveMeshInspectorDrawerUVE(const Scene::EntityUVE entity) {
    if (!IsDocumentEntityUVE(entity)) {
        return;
    }

    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::PrimitiveMeshComponentUVE>(entity)) {
        return;
    }

    ImGui::Separator();
    DrawNativeIconLabelUVE(0U, "Primitive");
    const Scene::PrimitiveMeshComponentUVE current =
        entityManager.GetComponentUVE<Scene::PrimitiveMeshComponentUVE>(entity);
    int kindIndex = static_cast<int>(current.kind);
    constexpr const char* kPrimitiveKinds[] = {"Cube", "UV Sphere", "Plane"};
    const bool kindChanged = ImGui::Combo("Primitive Kind", &kindIndex, kPrimitiveKinds,
                                          static_cast<int>(std::size(kPrimitiveKinds)));
    float baseColor[3]{current.baseColor.x, current.baseColor.y, current.baseColor.z};
    const bool colorChanged =
        ImGui::ColorEdit3("Base Color", baseColor, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB);
    if (kindChanged || colorChanged) {
        Scene::PrimitiveMeshComponentUVE updated = current;
        updated.kind = static_cast<Scene::PrimitiveMeshKindUVE>(kindIndex);
        updated.baseColor = Math::Vector3UVE{baseColor[0], baseColor[1], baseColor[2]};
        static_cast<void>(SetSelectedPrimitiveMeshUVE(updated));
    }
}

void EditorUVE::DrawWorldEnvironmentInspectorDrawerUVE(const Scene::EntityUVE entity) {
    if (!IsDocumentEntityUVE(entity) || entity != m_selectedEntity) {
        return;
    }
    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::WorldEnvironment3DNodeComponentUVE>(entity)) {
        return;
    }

    const Scene::WorldEnvironment3DNodeComponentUVE current =
        entityManager.GetComponentUVE<Scene::WorldEnvironment3DNodeComponentUVE>(entity);
    Scene::WorldEnvironment3DNodeComponentUVE edited = current;
    bool changed = false;

    ImGui::Separator();
    DrawNativeIconLabelUVE(m_uiAssets.GetGeneralIconTextureIdUVE("environment"), "World Environment");
    ImGui::TextDisabled("Scene environment settings are authored on this node and persisted in the scene.");

    if (ImGui::CollapsingHeader("Environment", ImGuiTreeNodeFlags_DefaultOpen)) {
        std::array<char, 257> skyAssetPathBuffer{};
        current.skyAssetPath.copy(skyAssetPathBuffer.data(), skyAssetPathBuffer.size() - 1U);
        if (ImGui::InputTextWithHint("Environment Resource", "Optional sky asset path...",
                                    skyAssetPathBuffer.data(), skyAssetPathBuffer.size())) {
            edited.skyAssetPath = skyAssetPathBuffer.data();
            changed = true;
        }
        ImGui::TextDisabled("The environment resource path is authored here; runtime sky sampling is not active yet.");
    }

    if (ImGui::CollapsingHeader("Background", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextDisabled(edited.skyAssetPath.empty() ? "Clear color background" : "Sky resource background");
        ImGui::TextDisabled("Background rendering remains scene-driven; no hidden editor light is created.");
    }

    if (ImGui::CollapsingHeader("Sky", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Asset: %s", edited.skyAssetPath.empty() ? "None" : edited.skyAssetPath.c_str());
        ImGui::TextDisabled("Sky asset assignment is persisted; runtime sky sampling is not active yet.");
    }

    const std::uintptr_t sunIconTextureId = m_uiAssets.GetGeneralIconTextureIdUVE("sun");
    if (sunIconTextureId != 0U) {
        ImGui::Image(static_cast<ImTextureID>(sunIconTextureId), ImVec2{16.0F, 16.0F});
        ImGui::SameLine(0.0F, 5.0F);
    }
    if (ImGui::CollapsingHeader("Ambient Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        float ambientColor[3]{edited.ambientColor.x, edited.ambientColor.y, edited.ambientColor.z};
        const bool colorChanged =
            ImGui::ColorEdit3("Ambient Color", ambientColor, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB);
        float ambientEnergy = edited.ambientEnergy;
        const bool energyChanged = ImGui::DragFloat("Ambient Energy", &ambientEnergy, 0.05F, 0.0F, 32.0F, "%.3f");
        if (colorChanged || energyChanged) {
            edited.ambientColor = Math::Vector3UVE{ambientColor[0], ambientColor[1], ambientColor[2]};
            edited.ambientEnergy = ambientEnergy;
            changed = true;
        }
    }

    if (ImGui::CollapsingHeader("Reflected Light")) {
        ImGui::TextDisabled("Reflection probes are authored separately and are not synthesized here.");
    }

    if (ImGui::CollapsingHeader("Tonemap", ImGuiTreeNodeFlags_DefaultOpen)) {
        float exposure = edited.exposure;
        const bool exposureChanged = ImGui::DragFloat("Exposure", &exposure, 0.05F, 0.001F, 32.0F, "%.3f");
        ImGui::BeginDisabled();
        ImGui::Checkbox("Post Processing (reserved)", &edited.postProcessingEnabled);
        ImGui::EndDisabled();
        if (exposureChanged) {
            edited.exposure = exposure;
            changed = true;
        }
    }

    if (ImGui::CollapsingHeader("Fog")) {
        const bool fogEnabledChanged = ImGui::Checkbox("Enabled", &edited.fogEnabled);
        float fogColor[3]{edited.fogColor.x, edited.fogColor.y, edited.fogColor.z};
        const bool fogColorChanged =
            ImGui::ColorEdit3("Fog Color", fogColor, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB);
        float fogDensity = edited.fogDensity;
        const bool fogDensityChanged = ImGui::DragFloat("Density", &fogDensity, 0.001F, 0.0F, 10.0F, "%.4f");
        if (fogEnabledChanged || fogColorChanged || fogDensityChanged) {
            edited.fogColor = Math::Vector3UVE{fogColor[0], fogColor[1], fogColor[2]};
            edited.fogDensity = fogDensity;
            changed = true;
        }
    }

    if (changed && !SetSelectedSceneComponentUVE(EditorSceneComponentKindUVE::WorldEnvironment, edited)) {
        ImGui::TextDisabled("Input was rejected by the authored-value validator.");
    }
}

void EditorUVE::DrawSceneComponentInspectorDrawerUVE(const Scene::EntityUVE entity,
                                                        const EditorSceneComponentKindUVE kind) {
    if (!IsDocumentEntityUVE(entity) || entity != m_selectedEntity) {
        return;
    }

    const char* title = "Component";
    switch (kind) {
        case EditorSceneComponentKindUVE::Camera: title = "Camera"; break;
        case EditorSceneComponentKindUVE::Mesh: title = "Mesh"; break;
        case EditorSceneComponentKindUVE::Light: title = "Light"; break;
        case EditorSceneComponentKindUVE::Collider: title = "Collider"; break;
        case EditorSceneComponentKindUVE::RigidBody: title = "Rigid Body"; break;
        case EditorSceneComponentKindUVE::AudioSource: title = "Audio Source"; break;
        case EditorSceneComponentKindUVE::ParticleEmitter: title = "Particle Emitter"; break;
        case EditorSceneComponentKindUVE::Script: title = "Script"; break;
        case EditorSceneComponentKindUVE::AnimationPlayer: title = "Animation Player"; break;
        case EditorSceneComponentKindUVE::WorldEnvironment: title = "World Environment"; break;
    }
    ImGui::Separator();
    DrawNativeIconLabelUVE(0U, title);
    ImGui::TextDisabled("Authored component state is validated and persisted by EditorUVE.");
    if (ImGui::Button((std::string("Remove ") + title).c_str())) {
        static_cast<void>(RemoveSelectedSceneComponentUVE(kind));
    }
}

void EditorUVE::DrawPrefabInspectorDrawerUVE(const Scene::EntityUVE entity) {
    if (!IsDocumentEntityUVE(entity) || entity != m_selectedEntity) {
        return;
    }
    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!entityManager.HasComponentUVE<Scene::PrefabInstanceComponentUVE>(entity)) {
        return;
    }
    const Scene::PrefabInstanceComponentUVE& instance =
        entityManager.GetComponentUVE<Scene::PrefabInstanceComponentUVE>(entity);
    const std::filesystem::path sourcePath =
        m_services->GetAssetDatabaseUVE().ResolveUVE(instance.sourcePrefabGuid);
    const std::optional<std::uint64_t> observedRevision =
        Scene::ComputePrefabSourceRevisionUVE(sourcePath);
    ImGui::Separator();
    DrawNativeIconLabelUVE(0U, "Prefab Instance");
    ImGui::Text("Source GUID: %llu", static_cast<unsigned long long>(instance.sourcePrefabGuid.value));
    ImGui::Text("Instance revision: %llu", static_cast<unsigned long long>(instance.instanceRevision));
    ImGui::Text("Source revision: %s", observedRevision.has_value() ? "available" : "unavailable");
    ImGui::Text("Local overrides: %zu", instance.overrides.size());
    if (!instance.overrides.empty()) {
        ImGui::TextColored(ImVec4{1.0F, 0.72F, 0.25F, 1.0F}, "Merge required before refresh.");
        if (ImGui::Button("Discard Overrides & Refresh")) {
            static_cast<void>(DiscardSelectedPrefabOverridesAndRefreshUVE());
        }
    } else if (observedRevision.has_value() && *observedRevision != instance.instanceRevision) {
        if (ImGui::Button("Refresh Prefab")) {
            static_cast<void>(RefreshSelectedPrefabUVE());
        }
    } else {
        ImGui::TextDisabled("Prefab instance is current.");
    }
    if (!sourcePath.empty()) {
        ImGui::TextDisabled("%s", sourcePath.generic_string().c_str());
    }
}

void EditorUVE::DrawSceneComponentAddPanelUVE() {
    if (!IsDocumentEntityUVE(m_selectedEntity)) {
        return;
    }
    Scene::IEntityManagerUVE& entityManager = m_services->GetEntityManagerUVE();
    if (!ImGui::CollapsingHeader("Add Component", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    if (ImGui::BeginTable("##component-grid", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoBordersInBody)) {
        ImGui::TableSetupColumn("Component", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 70.0F);
        const auto addIfMissing = [this, &entityManager](const char* label, const EditorSceneComponentKindUVE kind,
                                                           const EditorSceneComponentValueUVE& value, const bool present) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::BeginDisabled(present);
            if (ImGui::SmallButton(label) && !present) {
                static_cast<void>(SetSelectedSceneComponentUVE(kind, value));
            }
            ImGui::EndDisabled();
            ImGui::TableSetColumnIndex(1);
            ImGui::TextDisabled(present ? "Attached" : "Available");
        };

        addIfMissing("Camera", EditorSceneComponentKindUVE::Camera, Scene::CameraComponentUVE{},
                     entityManager.HasComponentUVE<Scene::CameraComponentUVE>(m_selectedEntity));
        addIfMissing("Mesh", EditorSceneComponentKindUVE::Mesh, Scene::MeshComponentUVE{},
                     entityManager.HasComponentUVE<Scene::MeshComponentUVE>(m_selectedEntity));
        addIfMissing("Light", EditorSceneComponentKindUVE::Light, Scene::LightComponentUVE{},
                     entityManager.HasComponentUVE<Scene::LightComponentUVE>(m_selectedEntity));
        addIfMissing("Collider", EditorSceneComponentKindUVE::Collider, Scene::ColliderComponentUVE{},
                     entityManager.HasComponentUVE<Scene::ColliderComponentUVE>(m_selectedEntity));
        addIfMissing("Rigid Body", EditorSceneComponentKindUVE::RigidBody, Scene::RigidBodyComponentUVE{},
                     entityManager.HasComponentUVE<Scene::RigidBodyComponentUVE>(m_selectedEntity));
        addIfMissing("Audio Source", EditorSceneComponentKindUVE::AudioSource, Scene::AudioSourceComponentUVE{},
                     entityManager.HasComponentUVE<Scene::AudioSourceComponentUVE>(m_selectedEntity));
        addIfMissing("Particle Emitter", EditorSceneComponentKindUVE::ParticleEmitter,
                     Scene::ParticleEmitterComponentUVE{},
                     entityManager.HasComponentUVE<Scene::ParticleEmitterComponentUVE>(m_selectedEntity));
        addIfMissing("Script", EditorSceneComponentKindUVE::Script, Scene::ScriptComponentUVE{},
                     entityManager.HasComponentUVE<Scene::ScriptComponentUVE>(m_selectedEntity));
        addIfMissing("Animation Player", EditorSceneComponentKindUVE::AnimationPlayer,
                     Scene::AnimationPlayerComponentUVE{},
                     entityManager.HasComponentUVE<Scene::AnimationPlayerComponentUVE>(m_selectedEntity));
        addIfMissing("World Environment", EditorSceneComponentKindUVE::WorldEnvironment,
                     Scene::WorldEnvironment3DNodeComponentUVE{},
                     entityManager.HasComponentUVE<Scene::WorldEnvironment3DNodeComponentUVE>(m_selectedEntity));
        ImGui::EndTable();
    }
}

EditorUVE::ContentBrowserItemTypeUVE EditorUVE::ClassifyContentBrowserEntryUVE(
    const Asset::ProjectFileEntryUVE& entry) {
    if (entry.kind == Asset::ProjectFileEntryKindUVE::Directory) {
        return ContentBrowserItemTypeUVE::Folder;
    }

    std::string extension = entry.relativePath.extension().generic_string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](const unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    if (extension == ".uvescene") {
        return ContentBrowserItemTypeUVE::Scene;
    }
    if (extension == ".uveprefab") {
        return ContentBrowserItemTypeUVE::Prefab;
    }
    if (extension == ".uvebundle") {
        return ContentBrowserItemTypeUVE::Bundle;
    }
    if (extension == ".uvemodel") {
        return ContentBrowserItemTypeUVE::Mesh;
    }
    if (extension == ".uvetex") {
        return ContentBrowserItemTypeUVE::Texture;
    }
    if (extension == ".uveshader") {
        return ContentBrowserItemTypeUVE::Shader;
    }
    if (extension == ".uvemat") {
        return ContentBrowserItemTypeUVE::Material;
    }
    if (extension == ".uvesave") {
        return ContentBrowserItemTypeUVE::Save;
    }
    if (extension == ".uvemotionquery") {
        return ContentBrowserItemTypeUVE::MotionQuery;
    }
    return ContentBrowserItemTypeUVE::File;
}

const char* EditorUVE::GetContentBrowserItemTypeLabelUVE(const ContentBrowserItemTypeUVE type) noexcept {
    switch (type) {
        case ContentBrowserItemTypeUVE::Folder:
            return "Folder";
        case ContentBrowserItemTypeUVE::Scene:
            return "Scene";
        case ContentBrowserItemTypeUVE::Prefab:
            return "Prefab";
        case ContentBrowserItemTypeUVE::Bundle:
            return "Bundle";
        case ContentBrowserItemTypeUVE::Mesh:
            return "Mesh";
        case ContentBrowserItemTypeUVE::Texture:
            return "Texture";
        case ContentBrowserItemTypeUVE::Shader:
            return "Shader";
        case ContentBrowserItemTypeUVE::Material:
            return "Material";
        case ContentBrowserItemTypeUVE::Save:
            return "Save";
        case ContentBrowserItemTypeUVE::MotionQuery:
            return "Motion Query";
        case ContentBrowserItemTypeUVE::File:
            return "File";
    }
    return "File";
}

const char* EditorUVE::GetContentBrowserFocusLabelUVE(const ContentBrowserTypeFocusUVE focus) noexcept {
    switch (focus) {
        case ContentBrowserTypeFocusUVE::All:
            return "All";
        case ContentBrowserTypeFocusUVE::Folders:
            return "Folders";
        case ContentBrowserTypeFocusUVE::Scene:
            return "Scene";
        case ContentBrowserTypeFocusUVE::Prefab:
            return "Prefab";
        case ContentBrowserTypeFocusUVE::Bundle:
            return "Bundle";
        case ContentBrowserTypeFocusUVE::Mesh:
            return "Mesh";
        case ContentBrowserTypeFocusUVE::Texture:
            return "Texture";
        case ContentBrowserTypeFocusUVE::Shader:
            return "Shader";
        case ContentBrowserTypeFocusUVE::Material:
            return "Material";
        case ContentBrowserTypeFocusUVE::Save:
            return "Save";
        case ContentBrowserTypeFocusUVE::MotionQuery:
            return "Motion Query";
        case ContentBrowserTypeFocusUVE::Registered:
            return "Registered";
        case ContentBrowserTypeFocusUVE::OtherFiles:
            return "Other Files";
    }
    return "All";
}

bool EditorUVE::DoesContentBrowserEntryMatchFocusUVE(const Asset::ProjectFileEntryUVE& entry) const {
    const ContentBrowserItemTypeUVE type = ClassifyContentBrowserEntryUVE(entry);
    switch (m_contentBrowserTypeFocus) {
        case ContentBrowserTypeFocusUVE::All:
            return true;
        case ContentBrowserTypeFocusUVE::Folders:
            return type == ContentBrowserItemTypeUVE::Folder;
        case ContentBrowserTypeFocusUVE::Scene:
            return type == ContentBrowserItemTypeUVE::Scene;
        case ContentBrowserTypeFocusUVE::Prefab:
            return type == ContentBrowserItemTypeUVE::Prefab;
        case ContentBrowserTypeFocusUVE::Bundle:
            return type == ContentBrowserItemTypeUVE::Bundle;
        case ContentBrowserTypeFocusUVE::Mesh:
            return type == ContentBrowserItemTypeUVE::Mesh;
        case ContentBrowserTypeFocusUVE::Texture:
            return type == ContentBrowserItemTypeUVE::Texture;
        case ContentBrowserTypeFocusUVE::Shader:
            return type == ContentBrowserItemTypeUVE::Shader;
        case ContentBrowserTypeFocusUVE::Material:
            return type == ContentBrowserItemTypeUVE::Material;
        case ContentBrowserTypeFocusUVE::Save:
            return type == ContentBrowserItemTypeUVE::Save;
        case ContentBrowserTypeFocusUVE::MotionQuery:
            return type == ContentBrowserItemTypeUVE::MotionQuery;
        case ContentBrowserTypeFocusUVE::Registered:
            return entry.kind == Asset::ProjectFileEntryKindUVE::File && entry.registeredAssetGuid.has_value();
        case ContentBrowserTypeFocusUVE::OtherFiles:
            return type == ContentBrowserItemTypeUVE::File;
    }
    return false;
}

bool EditorUVE::IsContentBrowserDirectoryInSnapshotUVE(const Asset::ProjectFileSnapshotUVE& snapshot,
                                                        const std::filesystem::path& directory) const {
    if (directory.empty()) {
        return true;
    }
    return std::any_of(snapshot.entries.begin(), snapshot.entries.end(), [&directory](const Asset::ProjectFileEntryUVE& entry) {
        return entry.kind == Asset::ProjectFileEntryKindUVE::Directory && entry.relativePath == directory;
    });
}

void EditorUVE::ReconcileContentBrowserDirectoryUVE(const Asset::ProjectFileSnapshotUVE& snapshot) noexcept {
    if (!IsContentBrowserDirectoryInSnapshotUVE(snapshot, m_contentBrowserDirectory)) {
        m_contentBrowserDirectory.clear();
    }
}

bool EditorUVE::IsProjectPathFavoritedUVE(const std::filesystem::path& relativePath) const {
    return std::find(m_favoriteProjectPaths.begin(), m_favoriteProjectPaths.end(), relativePath) !=
           m_favoriteProjectPaths.end();
}

void EditorUVE::ToggleProjectPathFavoriteUVE(const std::filesystem::path& relativePath) {
    const auto it = std::find(m_favoriteProjectPaths.begin(), m_favoriteProjectPaths.end(), relativePath);
    if (it != m_favoriteProjectPaths.end()) {
        m_favoriteProjectPaths.erase(it);
    } else {
        m_favoriteProjectPaths.push_back(relativePath);
    }
}

std::uintptr_t EditorUVE::GetTextureThumbnailUVE(const std::filesystem::path& relativePath) {
    const std::string cacheKey = relativePath.generic_string();
    const auto cachedIt = m_textureThumbnailCache.find(cacheKey);
    if (cachedIt != m_textureThumbnailCache.end()) {
        return cachedIt->second;
    }
    const Asset::ProjectFileSnapshotUVE snapshot = m_services->GetProjectFileIndexUVE().GetSnapshotUVE();
    const std::filesystem::path absolutePath = snapshot.contentRoot / relativePath;
    Asset::TextureAssetUVE texture;
    std::uintptr_t textureId = 0U;
    if (Asset::LoadTextureAssetUVE(absolutePath, texture) && texture.width > 0U && texture.height > 0U &&
        texture.format == Asset::TextureFormatUVE::RGBA8Unorm) {
        textureId = EditorUiAssetsUVE::UploadDynamicTextureUVE(reinterpret_cast<const std::uint8_t*>(texture.pixels.data()),
                                                                static_cast<int>(texture.width),
                                                                static_cast<int>(texture.height));
    }
    m_textureThumbnailCache.emplace(cacheKey, textureId);
    return textureId;
}

void EditorUVE::ClearTextureThumbnailCacheUVE() noexcept {
    for (auto& [path, textureId] : m_textureThumbnailCache) {
        EditorUiAssetsUVE::DeleteDynamicTextureUVE(textureId);
    }
    m_textureThumbnailCache.clear();
}

std::uintptr_t EditorUVE::GetMeshThumbnailUVE(const std::filesystem::path& relativePath) {
    const std::string cacheKey = relativePath.generic_string();
    const auto cachedIt = m_meshThumbnailCache.find(cacheKey);
    if (cachedIt != m_meshThumbnailCache.end()) {
        return cachedIt->second;
    }
    const Asset::ProjectFileSnapshotUVE snapshot = m_services->GetProjectFileIndexUVE().GetSnapshotUVE();
    const std::filesystem::path absolutePath = snapshot.contentRoot / relativePath;
    Asset::MeshAssetUVE mesh;
    std::uintptr_t textureId = 0U;
    if (Asset::LoadMeshAssetUVE(absolutePath, mesh)) {
        textureId = m_meshThumbnailRenderer.RenderThumbnailUVE(mesh, kMeshThumbnailSizeUVE, kMeshThumbnailSizeUVE);
    }
    m_meshThumbnailCache.emplace(cacheKey, textureId);
    return textureId;
}

void EditorUVE::ClearMeshThumbnailCacheUVE() noexcept {
    for (auto& [path, textureId] : m_meshThumbnailCache) {
        EditorUiAssetsUVE::DeleteDynamicTextureUVE(textureId);
    }
    m_meshThumbnailCache.clear();
}

void EditorUVE::DrawFolderContentsPanelUVE() {
    const ImGuiViewport* const mainViewport = ImGui::GetMainViewport();
    const float contentHeight = kAssetsPanelHeightUVE;
    const float projectWidth = ComputeFilesystemPanelWidthUVE(mainViewport->WorkSize.x);
    const float contentsWidth = std::max(280.0F, mainViewport->WorkSize.x - projectWidth);
    // FirstUseEver, not Always - see DrawHierarchyPanelUVE()'s comment on the same change.
    ImGui::SetNextWindowPos(
        ImVec2{mainViewport->WorkPos.x + projectWidth,
               mainViewport->WorkPos.y + mainViewport->WorkSize.y - kAssetsPanelHeightUVE},
        ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2{contentsWidth, contentHeight}, ImGuiCond_FirstUseEver);
    // NoTitleBar dropped (see DrawInspectorPanelUVE()'s comment) and given a real title.
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
    ImGui::Begin(kPanelLabelContentsUVE, nullptr, flags);

    const Asset::ProjectFileSnapshotUVE snapshot = m_services->GetProjectFileIndexUVE().GetSnapshotUVE();
    std::filesystem::path selectedDirectory = m_contentBrowserDirectory;
    if (m_selectedProjectFile.has_value() &&
        m_selectedProjectFile->kind == Asset::ProjectFileEntryKindUVE::Directory) {
        selectedDirectory = m_selectedProjectFile->relativePath;
    }
    const std::string directoryLabel = selectedDirectory.empty() ? "main" : selectedDirectory.generic_string();
    ImGui::TextDisabled("CONTENTS");
    ImGui::SameLine();
    ImGui::TextUnformatted(directoryLabel.c_str());
    ImGui::Separator();

    const auto openContext = [this](const Asset::ProjectFileEntryUVE& entry) {
        m_filesystemContextEntry = entry;
        m_filesystemContextFilter.clear();
        m_filesystemContextVisible = true;
    };
    const auto trackLongPress = [this, &openContext](const Asset::ProjectFileEntryUVE& entry,
                                                       const bool hovered) {
        if (!hovered || !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                m_filesystemLongPressPath.clear();
                m_filesystemLongPressSeconds = 0.0F;
            }
            return;
        }
        if (m_filesystemLongPressPath != entry.relativePath) {
            m_filesystemLongPressPath = entry.relativePath;
            m_filesystemLongPressSeconds = 0.0F;
        }
        m_filesystemLongPressSeconds += std::max(0.0F, ImGui::GetIO().DeltaTime);
        if (m_filesystemLongPressSeconds >= kFilesystemLongPressThresholdSecondsUVE) {
            openContext(entry);
            m_filesystemLongPressSeconds = 0.0F;
            m_filesystemLongPressPath.clear();
        }
    };

    constexpr float kCardWidthUVE = 76.0F;
    constexpr float kCardHeightUVE = 82.0F;
    constexpr float kCardIconSizeUVE = 44.0F;
    constexpr float kCardPaddingUVE = 4.0F;
    const auto truncateLabelUVE = [](const std::string& label, const float maxWidth) {
        if (ImGui::CalcTextSize(label.c_str()).x <= maxWidth) {
            return label;
        }
        std::string truncated = label;
        while (!truncated.empty() &&
               ImGui::CalcTextSize((truncated + "...").c_str()).x > maxWidth) {
            truncated.pop_back();
        }
        return truncated.empty() ? truncated : truncated + "...";
    };

    const float contentItemsHeight = std::max(36.0F, ImGui::GetContentRegionAvail().y);
    if (ImGui::BeginChild("##folder-contents-items", ImVec2{0.0F, contentItemsHeight}, true,
                           ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
        const float availableWidth = std::max(kCardWidthUVE, ImGui::GetContentRegionAvail().x);
        const int columns = std::max(1, static_cast<int>(availableWidth / kCardWidthUVE));
        const ImVec2 gridOrigin = ImGui::GetCursorPos();
        ImDrawList* const gridDrawList = ImGui::GetWindowDrawList();
        std::size_t visibleCount = 0U;
        for (const Asset::ProjectFileEntryUVE& entry : snapshot.entries) {
            if (entry.relativePath.parent_path() != selectedDirectory) {
                continue;
            }
            const std::string entryPath = entry.relativePath.generic_string();
            if (!ContainsCaseInsensitiveUVE(entryPath, m_assetFilter)) {
                continue;
            }
            const int column = static_cast<int>(visibleCount) % columns;
            const int row = static_cast<int>(visibleCount) / columns;
            ++visibleCount;
            const ContentBrowserItemTypeUVE type = ClassifyContentBrowserEntryUVE(entry);
            const std::string displayLabel = entry.relativePath.filename().generic_string();
            const std::string rowId = "folder-content-entry-" + entry.relativePath.generic_string();
            ImGui::PushID(rowId.c_str());
            ImGui::SetCursorPos(ImVec2{gridOrigin.x + static_cast<float>(column) * kCardWidthUVE,
                                       gridOrigin.y + static_cast<float>(row) * kCardHeightUVE});
            const ImVec2 cardMin = ImGui::GetCursorScreenPos();
            const bool selected = m_selectedProjectFile.has_value() &&
                                  m_selectedProjectFile->relativePath == entry.relativePath;
            const bool clicked = ImGui::Selectable("##card", selected, ImGuiSelectableFlags_AllowDoubleClick,
                                                   ImVec2{kCardWidthUVE - kCardPaddingUVE, kCardHeightUVE - kCardPaddingUVE});
            const bool rowHovered = ImGui::IsItemHovered();
            if (rowHovered) {
                ImGui::SetTooltip("%s\nType: %s", displayLabel.c_str(), GetContentBrowserItemTypeLabelUVE(type));
            }
            const std::uintptr_t contentThumbnail =
                type == ContentBrowserItemTypeUVE::Texture ? GetTextureThumbnailUVE(entry.relativePath)
                : type == ContentBrowserItemTypeUVE::Mesh  ? GetMeshThumbnailUVE(entry.relativePath)
                                                            : 0U;
            const std::uintptr_t iconTexture =
                contentThumbnail != 0U ? contentThumbnail
                : type == ContentBrowserItemTypeUVE::Folder
                    ? 0U
                    : m_uiAssets.GetContentTypeIconTextureIdUVE(GetContentBrowserItemTypeLabelUVE(type));
            if (iconTexture != 0U) {
                const float iconX = cardMin.x + (kCardWidthUVE - kCardIconSizeUVE) * 0.5F;
                gridDrawList->AddImage(static_cast<ImTextureID>(iconTexture), ImVec2{iconX, cardMin.y + 4.0F},
                                       ImVec2{iconX + kCardIconSizeUVE, cardMin.y + 4.0F + kCardIconSizeUVE});
            }
            const std::string truncatedLabel = truncateLabelUVE(displayLabel, kCardWidthUVE - kCardPaddingUVE);
            const float labelWidth = ImGui::CalcTextSize(truncatedLabel.c_str()).x;
            const float labelX = cardMin.x + std::max(0.0F, (kCardWidthUVE - labelWidth) * 0.5F);
            gridDrawList->AddText(ImVec2{labelX, cardMin.y + kCardIconSizeUVE + 8.0F},
                                 ImGui::GetColorU32(ImGuiCol_Text), truncatedLabel.c_str());
            const bool contextClicked = rowHovered &&
                                         (ImGui::IsMouseClicked(ImGuiMouseButton_Right) ||
                                          ImGui::IsMouseReleased(ImGuiMouseButton_Right));
            if (clicked) {
                m_selectedProjectFile = entry;
                if (entry.registeredAssetGuid.has_value()) {
                    m_selectedAsset = Asset::AssetRecordUVE{*entry.registeredAssetGuid, snapshot.contentRoot / entry.relativePath};
                } else {
                    m_selectedAsset.reset();
                }
                if (entry.kind == Asset::ProjectFileEntryKindUVE::Directory &&
                    ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    m_contentBrowserDirectory = entry.relativePath;
                    m_contentBrowserShowingFavorites = false;
                }
            }
            ImGui::PopID();
            if (contextClicked) {
                m_selectedProjectFile = entry;
                if (entry.registeredAssetGuid.has_value()) {
                    m_selectedAsset = Asset::AssetRecordUVE{*entry.registeredAssetGuid,
                                                             snapshot.contentRoot / entry.relativePath};
                } else {
                    m_selectedAsset.reset();
                }
                openContext(entry);
            } else {
                trackLongPress(entry, rowHovered);
            }
        }
        if (visibleCount == 0U) {
            ImGui::SetCursorPos(gridOrigin);
            ImGui::TextDisabled(selectedDirectory.empty() ? "main is empty." : "This folder is empty.");
        } else {
            const int totalRows = (static_cast<int>(visibleCount) + columns - 1) / columns;
            ImGui::SetCursorPos(
                ImVec2{gridOrigin.x, gridOrigin.y + static_cast<float>(totalRows) * kCardHeightUVE});
            ImGui::Dummy(ImVec2{0.0F, 0.0F});
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

void EditorUVE::DrawFilesystemContextPopupUVE() {
    if (!m_filesystemContextVisible) {
        return;
    }
    const ImGuiViewport* const contextViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2{contextViewport->WorkPos.x + 360.0F, contextViewport->WorkPos.y + 180.0F},
                            ImGuiCond_Appearing);
    ImGui::SetNextWindowSize(ImVec2{320.0F, 0.0F}, ImGuiCond_Appearing);
    if (!ImGui::Begin("Filesystem Components##context", &m_filesystemContextVisible,
                      ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::End();
        return;
    }

    if (!m_filesystemContextEntry.has_value()) {
        ImGui::TextDisabled("No Filesystem entry selected.");
        ImGui::End();
        return;
    }

    const Asset::ProjectFileEntryUVE& contextEntry = *m_filesystemContextEntry;
    ImGui::TextDisabled("%s", contextEntry.relativePath.generic_string().c_str());
    ImGui::Separator();
    std::array<char, 257> filterBuffer{};
    std::strncpy(filterBuffer.data(), m_filesystemContextFilter.c_str(), filterBuffer.size() - 1U);
    ImGui::SetNextItemWidth(260.0F);
    if (ImGui::InputTextWithHint("##filesystem-context-search", "Search components", filterBuffer.data(),
                                 filterBuffer.size())) {
        m_filesystemContextFilter = filterBuffer.data();
    }

    const auto matches = [this](const std::string_view label) {
        return ContainsCaseInsensitiveUVE(label, m_filesystemContextFilter);
    };
    const bool contextEntryFavorited = IsProjectPathFavoritedUVE(contextEntry.relativePath);
    const char* const favoriteActionText = contextEntryFavorited ? "Remove from Favorites" : "Add to Favorites";
    if (matches(favoriteActionText)) {
        const std::string favoriteMenuLabel = std::string(kIconStarUVE) + " " + favoriteActionText;
        if (ImGui::MenuItem(favoriteMenuLabel.c_str())) {
            ToggleProjectPathFavoriteUVE(contextEntry.relativePath);
            m_filesystemContextVisible = false;
        }
    }
    if (contextEntry.kind == Asset::ProjectFileEntryKindUVE::Directory && matches("Open folder")) {
        if (ImGui::MenuItem("Open folder")) {
            m_contentBrowserDirectory = contextEntry.relativePath;
            m_contentBrowserShowingFavorites = false;
            m_selectedProjectFile = contextEntry;
            m_selectedAsset.reset();
            m_filesystemContextVisible = false;
        }
    }

    ImGui::Separator();
    ImGui::TextDisabled("Scene components");
    const auto componentAction = [this, &matches](const char* const label, const EditorSceneComponentKindUVE kind,
                                                    const EditorSceneComponentValueUVE& value) {
        if (!matches(label)) {
            return;
        }
        const bool enabled = IsDocumentEntityUVE(m_selectedEntity) && IsAuthoringCommandAllowedUVE();
        ImGui::BeginDisabled(!enabled);
        if (ImGui::MenuItem(label)) {
            if (SetSelectedSceneComponentUVE(kind, value)) {
                m_activeRightPanelTab = EditorRightPanelTabUVE::Inspector;
                m_inspectorPanelVisible = true;
            }
            m_filesystemContextVisible = false;
        }
        ImGui::EndDisabled();
    };

    componentAction("Camera", EditorSceneComponentKindUVE::Camera, Scene::CameraComponentUVE{});
    componentAction("Mesh", EditorSceneComponentKindUVE::Mesh, Scene::MeshComponentUVE{});
    componentAction("Light", EditorSceneComponentKindUVE::Light, Scene::LightComponentUVE{});
    componentAction("Collider", EditorSceneComponentKindUVE::Collider, Scene::ColliderComponentUVE{});
    componentAction("Rigid Body", EditorSceneComponentKindUVE::RigidBody, Scene::RigidBodyComponentUVE{});
    componentAction("Audio Source", EditorSceneComponentKindUVE::AudioSource, Scene::AudioSourceComponentUVE{});
    componentAction("Particle Emitter", EditorSceneComponentKindUVE::ParticleEmitter,
                    Scene::ParticleEmitterComponentUVE{});
    componentAction("Script", EditorSceneComponentKindUVE::Script, Scene::ScriptComponentUVE{});
    componentAction("Animation Player", EditorSceneComponentKindUVE::AnimationPlayer,
                    Scene::AnimationPlayerComponentUVE{});
    componentAction("World Environment", EditorSceneComponentKindUVE::WorldEnvironment,
                    Scene::WorldEnvironment3DNodeComponentUVE{});
    if (!IsDocumentEntityUVE(m_selectedEntity)) {
        ImGui::TextDisabled("Select a Scene node to attach a component.");
    }
    ImGui::End();
}

void EditorUVE::RefreshProjectFileIndexUVE() {
    Asset::IProjectFileIndexUVE& projectFileIndex = m_services->GetProjectFileIndexUVE();
    Asset::IProjectChangeWatcherUVE& projectChangeWatcher = m_services->GetProjectChangeWatcherUVE();
    const Asset::ProjectChangeSnapshotUVE changesBeforeRefresh = projectChangeWatcher.GetSnapshotUVE();
    m_projectFileLastRefreshSucceeded = projectFileIndex.RefreshUVE(m_services->GetAssetDatabaseUVE());
    m_projectFileSnapshotInitialized = true;
    if (m_projectFileLastRefreshSucceeded) {
        projectChangeWatcher.AcknowledgeThroughUVE(changesBeforeRefresh.latestSequence);
        m_projectFileRefreshAttemptedForRescan = false;
        if (changesBeforeRefresh.rescanRequired) {
            // A successful full index refresh is the explicit boundary that safely clears watcher overflow.
            projectChangeWatcher.AcknowledgeRescanUVE();
        }
        // On-disk content may have changed since these were cached; re-decode lazily on next display.
        ClearTextureThumbnailCacheUVE();
        ClearMeshThumbnailCacheUVE();
    } else {
        m_projectFileRefreshAttemptedForRescan = changesBeforeRefresh.rescanRequired;
    }
}

void EditorUVE::DrawAssetsPanelUVE() {
    const ImGuiViewport* const mainViewport = ImGui::GetMainViewport();
    const float contentHeight = kAssetsPanelHeightUVE;
    const float projectWidth = ComputeFilesystemPanelWidthUVE(mainViewport->WorkSize.x);
    // FirstUseEver, not Always - see DrawHierarchyPanelUVE()'s comment on the same change.
    ImGui::SetNextWindowPos(
        ImVec2{mainViewport->WorkPos.x,
               mainViewport->WorkPos.y + mainViewport->WorkSize.y - kAssetsPanelHeightUVE},
        ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2{projectWidth, contentHeight}, ImGuiCond_FirstUseEver);
    // NoTitleBar dropped (see DrawInspectorPanelUVE()'s comment) and given a real title. The
    // panel's own "FILESYSTEM" text label a few lines below is unrelated in-content chrome, not
    // this window's identifying title, so both can coexist without looking redundant.
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
    ImGui::Begin(kPanelLabelFilesystemUVE, nullptr, flags);

    Asset::IProjectFileIndexUVE& projectFileIndex = m_services->GetProjectFileIndexUVE();
    const Asset::ProjectChangeSnapshotUVE changeSnapshot = m_services->GetProjectChangeWatcherUVE().GetSnapshotUVE();
    const Asset::ProjectFileSnapshotUVE snapshot = projectFileIndex.GetSnapshotUVE();
    ImGui::TextDisabled("FILESYSTEM");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 18.0F);
    if (ImGui::SmallButton("...##filesystem-menu")) {
        ImGui::OpenPopup("filesystem-overflow-menu");
    }
    if (ImGui::BeginPopup("filesystem-overflow-menu")) {
        ImGui::TextDisabled("Filesystem dock");
        ImGui::Separator();
        if (ImGui::MenuItem("Debug")) {
            m_activeBottomDock = EditorBottomDockUVE::Debugger;
        }
        if (ImGui::MenuItem("Hide dock")) {
            m_bottomDockVisible = false;
        }
        ImGui::EndPopup();
    }
    if (!m_projectFileLastRefreshSucceeded) {
        ImGui::SameLine();
        if (ImGui::SmallButton("Retry")) {
            m_projectFileSnapshotInitialized = false;
            m_projectFileRefreshAttemptedForRescan = false;
            RefreshProjectFileIndexUVE();
        }
        ImGui::SameLine();
        ImGui::TextColored(ImVec4{0.95F, 0.55F, 0.35F, 1.0F}, "scan failed");
    }
    if (changeSnapshot.rescanRequired) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4{0.95F, 0.72F, 0.30F, 1.0F}, "rescan required");
    }
    ImGui::Separator();
    ReconcileContentBrowserDirectoryUVE(snapshot);

    if (m_selectedProjectFile.has_value()) {
        const auto selectedIt = std::find_if(
            snapshot.entries.begin(), snapshot.entries.end(), [this](const Asset::ProjectFileEntryUVE& entry) {
                return entry.relativePath == m_selectedProjectFile->relativePath && entry.kind == m_selectedProjectFile->kind;
            });
        if (selectedIt == snapshot.entries.end()) {
            m_selectedProjectFile.reset();
            m_selectedAsset.reset();
        } else {
            m_selectedProjectFile = *selectedIt;
            if (selectedIt->registeredAssetGuid.has_value()) {
                m_selectedAsset = Asset::AssetRecordUVE{*selectedIt->registeredAssetGuid,
                                                         snapshot.contentRoot / selectedIt->relativePath};
            } else {
                m_selectedAsset.reset();
            }
        }
    }

    ImGui::Separator();
    const bool showingMainRoot = !m_contentBrowserShowingFavorites && m_contentBrowserDirectory.empty();
    if (showingMainRoot) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.20F, 0.21F, 0.23F, 1.0F});
    }
    if (ImGui::SmallButton("main##content-root")) {
        m_contentBrowserShowingFavorites = false;
        m_contentBrowserDirectory.clear();
        m_selectedProjectFile.reset();
        m_selectedAsset.reset();
    }
    if (showingMainRoot) {
        ImGui::PopStyleColor();
    }
    ImGui::SameLine();
    if (m_contentBrowserShowingFavorites) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.20F, 0.21F, 0.23F, 1.0F});
    }
    const std::string favoritesButtonLabel = std::string(kIconStarUVE) + " Favorites##favorites-root";
    if (ImGui::SmallButton(favoritesButtonLabel.c_str())) {
        m_contentBrowserShowingFavorites = true;
        m_selectedProjectFile.reset();
        m_selectedAsset.reset();
    }
    if (m_contentBrowserShowingFavorites) {
        ImGui::PopStyleColor();
    }

    std::array<char, 256> filterBuffer{};
    const std::size_t copiedCharacters = std::min(m_assetFilter.size(), filterBuffer.size() - 1U);
    m_assetFilter.copy(filterBuffer.data(), copiedCharacters);
    ImGui::SetNextItemWidth(std::max(90.0F, ImGui::GetContentRegionAvail().x * 0.42F));
    if (ImGui::InputTextWithHint("##content-filter", "Search", filterBuffer.data(), filterBuffer.size())) {
        m_assetFilter = filterBuffer.data();
    }
    const bool hasActiveFilters = !m_assetFilter.empty();

    const auto selectEntry = [this, &snapshot](const Asset::ProjectFileEntryUVE& entry) {
        m_selectedProjectFile = entry;
        if (entry.registeredAssetGuid.has_value()) {
            m_selectedAsset = Asset::AssetRecordUVE{*entry.registeredAssetGuid, snapshot.contentRoot / entry.relativePath};
        } else {
            m_selectedAsset.reset();
        }
    };
    const auto openContext = [this](const Asset::ProjectFileEntryUVE& entry) {
        m_filesystemContextEntry = entry;
        m_filesystemContextFilter.clear();
        m_filesystemContextVisible = true;
    };
    const auto trackLongPress = [this, &openContext](const Asset::ProjectFileEntryUVE& entry,
                                                       const bool hovered) {
        if (!hovered || !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                m_filesystemLongPressPath.clear();
                m_filesystemLongPressSeconds = 0.0F;
            }
            return;
        }
        if (m_filesystemLongPressPath != entry.relativePath) {
            m_filesystemLongPressPath = entry.relativePath;
            m_filesystemLongPressSeconds = 0.0F;
        }
        m_filesystemLongPressSeconds += std::max(0.0F, ImGui::GetIO().DeltaTime);
        if (m_filesystemLongPressSeconds >= kFilesystemLongPressThresholdSecondsUVE) {
            openContext(entry);
            m_filesystemLongPressSeconds = 0.0F;
            m_filesystemLongPressPath.clear();
        }
    };

    std::vector<const Asset::ProjectFileEntryUVE*> visibleEntries;
    for (const Asset::ProjectFileEntryUVE& entry : snapshot.entries) {
        if (m_contentBrowserShowingFavorites) {
            if (!IsProjectPathFavoritedUVE(entry.relativePath)) {
                continue;
            }
        } else if (entry.relativePath.parent_path() != m_contentBrowserDirectory) {
            continue;
        }
        const std::string entryPath = entry.relativePath.generic_string();
        if (ContainsCaseInsensitiveUVE(entryPath, m_assetFilter)) {
            visibleEntries.push_back(&entry);
        }
    }

    const float itemsHeight = std::max(36.0F, ImGui::GetContentRegionAvail().y);
    ImGui::BeginChild("##content-browser-items", ImVec2{0.0F, itemsHeight}, true,
                      ImGuiWindowFlags_AlwaysVerticalScrollbar);
    if (!m_projectFileLastRefreshSucceeded && snapshot.refreshGeneration == 0U) {
        ImGui::TextUnformatted("Project content root could not be scanned. Correct the root; the next automatic scan will retry.");
    } else if (!snapshot.contentRootExists) {
        ImGui::TextUnformatted("Project content root does not exist yet. Add content; the next automatic scan will index it.");
    } else if (snapshot.entries.empty()) {
        ImGui::TextUnformatted("Project content root is empty.");
    } else if (visibleEntries.empty()) {
        if (m_contentBrowserShowingFavorites) {
            ImGui::TextUnformatted("No favorites yet. Right-click a file or folder and choose \"Add to Favorites\".");
        } else if (hasActiveFilters) {
            ImGui::TextUnformatted("No entries in this folder match the active filters.");
        } else {
            ImGui::TextUnformatted("This folder has no direct entries.");
        }
    } else {
        for (const Asset::ProjectFileEntryUVE* const entry : visibleEntries) {
            const bool selected = m_selectedProjectFile.has_value() &&
                                  m_selectedProjectFile->relativePath == entry->relativePath &&
                                  m_selectedProjectFile->kind == entry->kind;
            const ContentBrowserItemTypeUVE type = ClassifyContentBrowserEntryUVE(*entry);
            const std::string displayLabel = entry->relativePath.filename().generic_string();

            const std::string rowId = "content-browser-entry-" + entry->relativePath.generic_string();
            ImGui::PushID(rowId.c_str());
            const ImVec2 rowMin = ImGui::GetCursorScreenPos();
            const float rowHeight = ImGui::GetTextLineHeight() + 4.0F;
            const bool clicked = ImGui::Selectable("##entry", selected, ImGuiSelectableFlags_AllowDoubleClick,
                                                   ImVec2{0.0F, rowHeight});
            const bool rowHovered = ImGui::IsItemHovered();
            if (rowHovered) {
                const char* const registeredSuffix = entry->registeredAssetGuid.has_value() ? " (Registered)" : "";
                ImGui::SetTooltip("Type: %s%s", GetContentBrowserItemTypeLabelUVE(type), registeredSuffix);
            }
            const bool contextClicked = rowHovered &&
                                         (ImGui::IsMouseClicked(ImGuiMouseButton_Right) ||
                                          ImGui::IsMouseReleased(ImGuiMouseButton_Right));
            ImDrawList* const rowDrawList = ImGui::GetWindowDrawList();
            const std::uintptr_t rowIconTexture =
                m_uiAssets.IsReadyUVE() ? (type == ContentBrowserItemTypeUVE::Folder
                                               ? 0U
                                               : m_uiAssets.GetContentTypeIconTextureIdUVE(
                                                     GetContentBrowserItemTypeLabelUVE(type)))
                                        : 0U;
            const float textOffset = rowIconTexture != 0U ? 27.0F : 4.0F;
            if (rowIconTexture != 0U) {
                const float iconY = rowMin.y + std::max(0.0F, (rowHeight - 16.0F) * 0.5F);
                rowDrawList->AddImage(static_cast<ImTextureID>(rowIconTexture), ImVec2{rowMin.x + 4.0F, iconY},
                                      ImVec2{rowMin.x + 22.0F, iconY + 16.0F});
            }
            const float textY = rowMin.y + std::max(0.0F, (rowHeight - ImGui::GetTextLineHeight()) * 0.5F);
            rowDrawList->AddText(ImVec2{rowMin.x + textOffset, textY}, ImGui::GetColorU32(ImGuiCol_Text),
                                 displayLabel.c_str());
            ImGui::PopID();
            if (clicked) {
                selectEntry(*entry);
                if (entry->kind == Asset::ProjectFileEntryKindUVE::Directory &&
                    ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    m_contentBrowserDirectory = entry->relativePath;
                    m_contentBrowserShowingFavorites = false;
                }
            }
            if (contextClicked) {
                selectEntry(*entry);
                openContext(*entry);
            } else {
                trackLongPress(*entry, rowHovered);
            }
        }
    }
    ImGui::EndChild();

    ImGui::End();
}

void EditorUVE::CompileVisualScriptUVE() {
    const Scripting::ScriptGraphCanvasSnapshotUVE snapshot = ActiveVisualScriptCanvasUVE().GetSnapshotUVE();
    m_scriptCompileAttempted = true;
    m_scriptCompileSucceeded = false;
    m_scriptLastCompiledGraphRevision = snapshot.graphRevision;
    m_scriptCompileInstructionCount = 0U;
    m_scriptCompileMessage.clear();

    const Scripting::ScriptIrCompileResultUVE compiled =
        Scripting::CompileScriptGraphToIrUVE(ActiveVisualScriptCanvasUVE().GetGraphUVE(), m_visualScriptRegistry);
    if (!compiled.IsSuccessUVE()) {
        if (compiled.diagnostics.empty()) {
            m_scriptCompileMessage = "Graph compilation was rejected without a diagnostic.";
        } else {
            const auto& diagnostic = compiled.diagnostics.front();
            m_scriptCompileMessage = "Node " + std::to_string(diagnostic.nodeId) + ": " + diagnostic.message;
            if (!diagnostic.pinName.empty()) {
                m_scriptCompileMessage += " (" + diagnostic.pinName + ")";
            }
        }
        return;
    }

    std::vector<Scripting::ScriptBytecodeDiagnosticUVE> loweringDiagnostics;
    const std::optional<Scripting::ScriptBytecodeProgramUVE> bytecode =
        Scripting::LowerIrToBytecodeUVE(*compiled.program, loweringDiagnostics);
    if (!bytecode.has_value() || !loweringDiagnostics.empty()) {
        if (loweringDiagnostics.empty()) {
            m_scriptCompileMessage = "Bytecode lowering was rejected without a diagnostic.";
        } else {
            m_scriptCompileMessage = "Bytecode lowering: " + loweringDiagnostics.front().message;
        }
        return;
    }

    m_scriptCompileSucceeded = true;
    m_scriptCompileInstructionCount = bytecode->instructions.size();
    m_scriptCompileMessage = "Compiled " + std::to_string(m_scriptCompileInstructionCount) + " instructions.";
}

void EditorUVE::DrawScriptingWorkspaceUVE() {
    const ImGuiViewport* const mainViewport = ImGui::GetMainViewport();
    const ImVec2 position{mainViewport->WorkPos.x,
                          mainViewport->WorkPos.y + kEditorTopChromeHeightUVE};
    const ImVec2 size{mainViewport->WorkSize.x,
                      std::max(120.0F, mainViewport->WorkSize.y - kEditorTopChromeHeightUVE)};
    ImGui::SetNextWindowPos(position, ImGuiCond_Always);
    ImGui::SetNextWindowSize(size, ImGuiCond_Always);
    constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
                                               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;
    if (!ImGui::Begin("Scripting Workspace##uve", nullptr, windowFlags)) {
        ImGui::End();
        return;
    }

    const Scripting::ScriptGraphCanvasSnapshotUVE snapshot = ActiveVisualScriptCanvasUVE().GetSnapshotUVE();
    const auto selectedNode = [&snapshot]() -> const Scripting::ScriptGraphCanvasNodeSnapshotUVE* {
        if (snapshot.selectedNodeIds.size() != 1U) {
            return nullptr;
        }
        const auto iterator = std::find_if(
            snapshot.nodes.cbegin(), snapshot.nodes.cend(),
            [&snapshot](const Scripting::ScriptGraphCanvasNodeSnapshotUVE& node) {
                return node.id == snapshot.selectedNodeIds.front();
            });
        return iterator == snapshot.nodes.cend() ? nullptr : &*iterator;
    };
    const auto findNode = [&snapshot](const std::uint32_t nodeId)
        -> const Scripting::ScriptGraphCanvasNodeSnapshotUVE* {
        const auto iterator = std::find_if(
            snapshot.nodes.cbegin(), snapshot.nodes.cend(),
            [nodeId](const Scripting::ScriptGraphCanvasNodeSnapshotUVE& node) { return node.id == nodeId; });
        return iterator == snapshot.nodes.cend() ? nullptr : &*iterator;
    };
    const auto findPin = [](const Scripting::ScriptGraphCanvasNodeSnapshotUVE& node,
                            const std::string& name) -> const Scripting::ScriptGraphCanvasPinSnapshotUVE* {
        const auto iterator = std::find_if(
            node.pins.cbegin(), node.pins.cend(),
            [&name](const Scripting::ScriptGraphCanvasPinSnapshotUVE& pin) { return pin.name == name; });
        return iterator == node.pins.cend() ? nullptr : &*iterator;
    };

    if (ImGui::BeginChild("##scripting-toolbar", ImVec2{0.0F, 28.0F}, false)) {
        ImGui::TextColored(ImVec4{0.70F, 0.72F, 0.76F, 1.0F}, "GRAPH");
        ImGui::SameLine();
        ImGui::TextDisabled("native canvas | branch %s | revision %llu",
                            GetActiveVisualScriptBranchNameUVE().c_str(),
                            static_cast<unsigned long long>(snapshot.revision));
        ImGui::SameLine();
        if (ImGui::BeginCombo("##script-branch-combo", GetActiveVisualScriptBranchNameUVE().c_str(),
                              ImGuiComboFlags_HeightSmall)) {
            for (const std::string& branchName : GetVisualScriptBranchNamesUVE()) {
                const bool selected = branchName == GetActiveVisualScriptBranchNameUVE();
                if (ImGui::Selectable(branchName.c_str(), selected)) {
                    static_cast<void>(SelectVisualScriptBranchUVE(branchName));
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("+ Branch")) {
            m_scriptBranchDialogBuffer = "Type 2 Scene";
            m_scriptBranchDialogRenaming = false;
            ImGui::OpenPopup("script-branch-name-popup");
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Rename")) {
            m_scriptBranchDialogBuffer = GetActiveVisualScriptBranchNameUVE();
            m_scriptBranchDialogRenaming = true;
            ImGui::OpenPopup("script-branch-name-popup");
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Save .scripting")) {
            static_cast<void>(SaveVisualScriptWorkspaceUVE());
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Load .scripting")) {
            static_cast<void>(LoadVisualScriptWorkspaceUVE());
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Undo")) {
            static_cast<void>(ActiveVisualScriptCanvasUVE().UndoUVE());
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Compiler")) {
            CompileVisualScriptUVE();
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Redo")) {
            static_cast<void>(ActiveVisualScriptCanvasUVE().RedoUVE());
        }
        ImGui::SameLine();
        ImGui::TextDisabled("LMB select/drag/link | RMB/MMB pan | long-press search | wheel zoom");
        if (ImGui::BeginPopup("script-branch-name-popup")) {
            ImGui::TextUnformatted(m_scriptBranchDialogRenaming ? "Rename script branch" : "Create script branch");
            ImGui::Separator();
            std::array<char, 97> nameBuffer{};
            std::strncpy(nameBuffer.data(), m_scriptBranchDialogBuffer.c_str(), nameBuffer.size() - 1U);
            const bool submitted = ImGui::InputText("Name", nameBuffer.data(), nameBuffer.size(),
                                                    ImGuiInputTextFlags_EnterReturnsTrue);
            m_scriptBranchDialogBuffer = nameBuffer.data();
            if (submitted || ImGui::Button(m_scriptBranchDialogRenaming ? "Rename" : "Create")) {
                const bool applied = m_scriptBranchDialogRenaming
                    ? RenameActiveVisualScriptBranchUVE(m_scriptBranchDialogBuffer)
                    : CreateVisualScriptBranchUVE(m_scriptBranchDialogBuffer);
                if (applied) {
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
    ImGui::EndChild();

    const ImVec2 workspaceSize = ImGui::GetContentRegionAvail();
    if (ImGui::BeginChild("##scripting-layout", workspaceSize, false)) {
        if (ImGui::BeginChild("##script-node3d-hierarchy", ImVec2{220.0F, 0.0F}, true)) {
            ImGui::TextDisabled("NODE3D");
            ImGui::Separator();
            RebuildHierarchyFilterCacheUVE();
            ImGui::BeginDisabled(!IsAuthoringCommandAllowedUVE());
            for (const Scene::EntityUVE root : GetDocumentRootsUVE()) {
                DrawHierarchyNodeUVE(root);
            }
            ImGui::EndDisabled();
        }
        ImGui::EndChild();
        ImGui::SameLine();

        const float detailsWidth = 276.0F;
        const float canvasWidth = std::max(180.0F, ImGui::GetContentRegionAvail().x - detailsWidth - ImGui::GetStyle().ItemSpacing.x);
        if (ImGui::BeginChild("##script-canvas-frame", ImVec2{canvasWidth, 0.0F}, true)) {
            const ImVec2 canvasOrigin = ImGui::GetCursorScreenPos();
            const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
            const Scripting::ScriptGraphCanvasViewUVE view = snapshot.view;
            ImGui::InvisibleButton("##script-canvas-input", canvasSize,
                                   ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight |
                                       ImGuiButtonFlags_MouseButtonMiddle);
            const bool canvasHovered = ImGui::IsItemHovered();
            const ImVec2 mouse = ImGui::GetMousePos();
            const ImVec2 mouseLocal{mouse.x - canvasOrigin.x, mouse.y - canvasOrigin.y};
            ImDrawList* const drawList = ImGui::GetWindowDrawList();
            drawList->AddRectFilled(canvasOrigin,
                                    ImVec2{canvasOrigin.x + canvasSize.x, canvasOrigin.y + canvasSize.y},
                                    IM_COL32(20, 22, 25, 255));
            drawList->AddText(ImVec2{canvasOrigin.x + 16.0F, canvasOrigin.y + 12.0F},
                              IM_COL32(166, 172, 180, 235), "GRAPH CANVAS");
            constexpr float gridSpacing = 24.0F;
            const float gridOffsetX = std::fmod(-view.pan.x * view.zoom, gridSpacing);
            const float gridOffsetY = std::fmod(-view.pan.y * view.zoom, gridSpacing);
            for (float x = canvasOrigin.x + gridOffsetX; x < canvasOrigin.x + canvasSize.x; x += gridSpacing) {
                drawList->AddLine(ImVec2{x, canvasOrigin.y}, ImVec2{x, canvasOrigin.y + canvasSize.y},
                                  IM_COL32(42, 45, 50, 220));
            }
            for (float y = canvasOrigin.y + gridOffsetY; y < canvasOrigin.y + canvasSize.y; y += gridSpacing) {
                drawList->AddLine(ImVec2{canvasOrigin.x, y}, ImVec2{canvasOrigin.x + canvasSize.x, y},
                                  IM_COL32(42, 45, 50, 220));
            }

            const auto nodePosition = [this](const Scripting::ScriptGraphCanvasNodeSnapshotUVE& node) {
                return m_scriptCanvasDragging && node.id == m_scriptCanvasDragNodeId
                    ? m_scriptCanvasDragPreviewPosition : node.position;
            };
            constexpr float nodeWidth = 228.0F;
            constexpr float headerHeight = 26.0F;
            constexpr float pinRowHeight = 19.0F;
            const auto nodeHeight = [](const Scripting::ScriptGraphCanvasNodeSnapshotUVE& node) {
                return 38.0F + pinRowHeight * static_cast<float>(std::max<std::size_t>(1U, node.pins.size()));
            };
            const auto pinScreenPosition = [&](const Scripting::ScriptGraphCanvasNodeSnapshotUVE& node,
                                               const Scripting::ScriptGraphCanvasPinSnapshotUVE& pin) {
                const auto iterator = std::find_if(node.pins.cbegin(), node.pins.cend(),
                                                   [&pin](const auto& candidate) { return candidate.name == pin.name; });
                const std::size_t pinIndex = iterator == node.pins.cend()
                    ? 0U : static_cast<std::size_t>(std::distance(node.pins.cbegin(), iterator));
                const ImVec2 nodeMin = ScriptCanvasToScreenUVE(nodePosition(node), canvasOrigin, view);
                const float y = nodeMin.y + headerHeight + 13.0F + pinRowHeight * static_cast<float>(pinIndex);
                return pin.direction == Scripting::ScriptPinDirectionUVE::Input
                    ? ImVec2{nodeMin.x + 8.0F, y} : ImVec2{nodeMin.x + nodeWidth - 8.0F, y};
            };

            for (const Scripting::ScriptGraphCanvasLinkSnapshotUVE& link : snapshot.links) {
                const auto* const outputNode = findNode(link.link.output.nodeId);
                const auto* const inputNode = findNode(link.link.input.nodeId);
                if (outputNode == nullptr || inputNode == nullptr) {
                    continue;
                }
                const auto* const outputPin = findPin(*outputNode, link.link.output.pinName);
                const auto* const inputPin = findPin(*inputNode, link.link.input.pinName);
                if (outputPin == nullptr || inputPin == nullptr) {
                    continue;
                }
                const ImVec2 start = pinScreenPosition(*outputNode, *outputPin);
                const ImVec2 end = pinScreenPosition(*inputNode, *inputPin);
                const float tangent = std::max(36.0F, std::abs(end.x - start.x) * 0.45F);
                drawList->AddBezierCubic(start, ImVec2{start.x + tangent, start.y},
                                         ImVec2{end.x - tangent, end.y}, end,
                                         IM_COL32(148, 174, 196, 235), 2.0F);
            }

            for (const Scripting::ScriptGraphCanvasNodeSnapshotUVE& node : snapshot.nodes) {
                const ImVec2 nodeMin = ScriptCanvasToScreenUVE(nodePosition(node), canvasOrigin, view);
                const float nodeHeightPixels = nodeHeight(node);
                const ImVec2 nodeMax{nodeMin.x + nodeWidth, nodeMin.y + nodeHeightPixels};
                const bool selected = std::find(snapshot.selectedNodeIds.cbegin(), snapshot.selectedNodeIds.cend(), node.id) !=
                                      snapshot.selectedNodeIds.cend();
                const ImU32 bodyColor = selected ? IM_COL32(58, 65, 72, 255) : IM_COL32(46, 50, 56, 255);
                drawList->AddRectFilled(nodeMin, nodeMax, bodyColor, 4.0F);
                drawList->AddRectFilled(nodeMin, ImVec2{nodeMax.x, nodeMin.y + headerHeight},
                                        selected ? IM_COL32(96, 112, 128, 255) : IM_COL32(70, 82, 94, 255), 4.0F,
                                        ImDrawFlags_RoundCornersTop);
                drawList->AddRect(nodeMin, nodeMax, selected ? IM_COL32(205, 180, 108, 255) : IM_COL32(105, 112, 120, 255),
                                  4.0F, 0, selected ? 2.0F : 1.0F);
                const std::string title = node.displayName.empty() ? node.typeId : node.displayName;
                drawList->AddText(ImVec2{nodeMin.x + 10.0F, nodeMin.y + 6.0F}, IM_COL32(226, 241, 252, 255), title.c_str());
                for (std::size_t pinIndex = 0U; pinIndex < node.pins.size(); ++pinIndex) {
                    const auto& pin = node.pins[pinIndex];
                    const ImVec2 pinPosition = pinScreenPosition(node, pin);
                    drawList->AddCircleFilled(pinPosition, 5.0F * std::clamp(view.zoom, 0.75F, 1.25F),
                                              ScriptPinColorUVE(pin));
                    const float textX = pin.direction == Scripting::ScriptPinDirectionUVE::Input
                        ? nodeMin.x + 17.0F : nodeMin.x + 14.0F;
                    const ImVec2 textPosition{pin.direction == Scripting::ScriptPinDirectionUVE::Input
                                                  ? textX : nodeMin.x + nodeWidth - 14.0F - ImGui::CalcTextSize(pin.name.c_str()).x,
                                              pinPosition.y - 7.0F};
                    drawList->AddText(textPosition, IM_COL32(214, 220, 227, 255), pin.name.c_str());
                }
            }

            const auto findNodeAt = [&](const ImVec2 point) -> const Scripting::ScriptGraphCanvasNodeSnapshotUVE* {
                for (auto iterator = snapshot.nodes.crbegin(); iterator != snapshot.nodes.crend(); ++iterator) {
                    const ImVec2 nodeMin = ScriptCanvasToScreenUVE(nodePosition(*iterator), canvasOrigin, view);
                    const ImVec2 nodeMax{nodeMin.x + nodeWidth, nodeMin.y + nodeHeight(*iterator)};
                    if (point.x >= nodeMin.x && point.x <= nodeMax.x && point.y >= nodeMin.y && point.y <= nodeMax.y) {
                        return &*iterator;
                    }
                }
                return nullptr;
            };
            const auto findPinAt = [&](const ImVec2 point, const Scripting::ScriptGraphCanvasNodeSnapshotUVE& node)
                -> const Scripting::ScriptGraphCanvasPinSnapshotUVE* {
                for (const auto& pin : node.pins) {
                    const ImVec2 pinPosition = pinScreenPosition(node, pin);
                    const float dx = point.x - pinPosition.x;
                    const float dy = point.y - pinPosition.y;
                    if ((dx * dx) + (dy * dy) <= 64.0F) {
                        return &pin;
                    }
                }
                return nullptr;
            };

            if (canvasHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && findNodeAt(mouse) == nullptr) {
                m_scriptCanvasLongPressPending = true;
                m_scriptCanvasLongPressSeconds = 0.0F;
                m_scriptCanvasLongPressStartPointer =
                    Scripting::ScriptGraphCanvasPointUVE{mouseLocal.x, mouseLocal.y};
            }
            bool openedLongPressPopup = false;
            if (m_scriptCanvasLongPressPending) {
                const float dx = mouseLocal.x - m_scriptCanvasLongPressStartPointer.x;
                const float dy = mouseLocal.y - m_scriptCanvasLongPressStartPointer.y;
                const bool movedTooFar = (dx * dx) + (dy * dy) >
                                         kScriptCanvasLongPressMaxMovementPixelsUVE *
                                             kScriptCanvasLongPressMaxMovementPixelsUVE;
                if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) || movedTooFar ||
                    ImGui::IsMouseDown(ImGuiMouseButton_Right) || ImGui::IsMouseDown(ImGuiMouseButton_Middle) ||
                    m_scriptCanvasPanning || m_scriptCanvasLinkSourceNodeId != 0U) {
                    m_scriptCanvasLongPressPending = false;
                    m_scriptCanvasLongPressSeconds = 0.0F;
                } else {
                    m_scriptCanvasLongPressSeconds += ImGui::GetIO().DeltaTime;
                    if (m_scriptCanvasLongPressSeconds >= kScriptCanvasLongPressThresholdSecondsUVE) {
                        const ImVec2 pressScreen{canvasOrigin.x + m_scriptCanvasLongPressStartPointer.x,
                                                canvasOrigin.y + m_scriptCanvasLongPressStartPointer.y};
                        m_scriptCanvasContextMenuPosition = ScreenToScriptCanvasUVE(pressScreen, canvasOrigin, view);
                        m_scriptCanvasContextFilter.clear();
                        ImGui::OpenPopup("script-node-search-popup");
                        m_scriptCanvasLongPressPending = false;
                        m_scriptCanvasLongPressSeconds = 0.0F;
                        openedLongPressPopup = true;
                    }
                }
            }

            if (m_scriptCanvasDragging) {
                if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                    const Scripting::ScriptGraphCanvasPointUVE currentGraphPosition =
                        ScreenToScriptCanvasUVE(mouse, canvasOrigin, view);
                    m_scriptCanvasDragPreviewPosition = Scripting::ScriptGraphCanvasPointUVE{
                        m_scriptCanvasDragStartPosition.x + currentGraphPosition.x - m_scriptCanvasDragStartPointer.x,
                        m_scriptCanvasDragStartPosition.y + currentGraphPosition.y - m_scriptCanvasDragStartPointer.y};
                } else {
                    static_cast<void>(ActiveVisualScriptCanvasUVE().MoveNodeUVE(
                        m_scriptCanvasDragNodeId, m_scriptCanvasDragPreviewPosition, m_scriptCanvasDragRevision));
                    m_scriptCanvasDragging = false;
                    m_scriptCanvasDragNodeId = 0U;
                }
            } else if (m_scriptCanvasPanning) {
                if (ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsMouseDown(ImGuiMouseButton_Right) ||
                    ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
                    Scripting::ScriptGraphCanvasViewUVE nextView = m_scriptCanvasPanViewStart;
                    nextView.pan.x = m_scriptCanvasPanViewStart.pan.x -
                                     (mouseLocal.x - m_scriptCanvasPanStart.x) / std::max(view.zoom, 0.1F);
                    nextView.pan.y = m_scriptCanvasPanViewStart.pan.y -
                                     (mouseLocal.y - m_scriptCanvasPanStart.y) / std::max(view.zoom, 0.1F);
                    static_cast<void>(ActiveVisualScriptCanvasUVE().SetViewUVE(nextView));
                } else {
                    m_scriptCanvasPanning = false;
                }
            } else if (canvasHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                const auto* const node = findNodeAt(mouse);
                if (node != nullptr) {
                    const auto* const pin = findPinAt(mouse, *node);
                    static_cast<void>(ActiveVisualScriptCanvasUVE().SetSelectionUVE({node->id}));
                    if (pin != nullptr) {
                        if (pin->direction == Scripting::ScriptPinDirectionUVE::Output) {
                            m_scriptCanvasLinkSourceNodeId = node->id;
                            m_scriptCanvasLinkSourcePin = pin->name;
                        } else if (m_scriptCanvasLinkSourceNodeId != 0U) {
                            const auto result = ActiveVisualScriptCanvasUVE().AddLinkUVE(
                                Scripting::ScriptLinkUVE{{m_scriptCanvasLinkSourceNodeId, m_scriptCanvasLinkSourcePin},
                                                         {node->id, pin->name}});
                            if (result.IsAppliedUVE()) {
                                m_scriptCanvasLinkSourceNodeId = 0U;
                                m_scriptCanvasLinkSourcePin.clear();
                            }
                        }
                    } else {
                        const Scripting::ScriptGraphCanvasPointUVE graphPosition =
                            ScreenToScriptCanvasUVE(mouse, canvasOrigin, view);
                        m_scriptCanvasDragging = true;
                        m_scriptCanvasDragNodeId = node->id;
                        m_scriptCanvasDragStartPosition = node->position;
                        m_scriptCanvasDragStartPointer = graphPosition;
                        m_scriptCanvasDragPreviewPosition = node->position;
                        m_scriptCanvasDragRevision = snapshot.revision;
                    }
                } else {
                    static_cast<void>(ActiveVisualScriptCanvasUVE().SetSelectionUVE({}));
                    m_scriptCanvasLinkSourceNodeId = 0U;
                    m_scriptCanvasLinkSourcePin.clear();
                }
            } else if (!openedLongPressPopup && canvasHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                if (findNodeAt(mouse) == nullptr) {
                    m_scriptCanvasContextMenuPosition = ScreenToScriptCanvasUVE(mouse, canvasOrigin, view);
                    m_scriptCanvasContextFilter.clear();
                    ImGui::OpenPopup("script-node-search-popup");
                } else {
                    m_scriptCanvasPanning = true;
                    m_scriptCanvasPanStart = Scripting::ScriptGraphCanvasPointUVE{mouseLocal.x, mouseLocal.y};
                    m_scriptCanvasPanViewStart = view;
                }
            } else if (canvasHovered &&
                       (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle))) {
                m_scriptCanvasPanning = true;
                m_scriptCanvasPanStart = Scripting::ScriptGraphCanvasPointUVE{mouseLocal.x, mouseLocal.y};
                m_scriptCanvasPanViewStart = view;
            }
            if (canvasHovered && ImGui::GetIO().MouseWheel != 0.0F) {
                const Scripting::ScriptGraphCanvasPointUVE graphUnderPointer =
                    ScreenToScriptCanvasUVE(mouse, canvasOrigin, view);
                Scripting::ScriptGraphCanvasViewUVE nextView = view;
                nextView.zoom = std::clamp(view.zoom * std::pow(1.12F, ImGui::GetIO().MouseWheel),
                                           Scripting::kMinimumScriptGraphCanvasZoomUVE,
                                           Scripting::kMaximumScriptGraphCanvasZoomUVE);
                nextView.pan.x = graphUnderPointer.x - mouseLocal.x / nextView.zoom;
                nextView.pan.y = graphUnderPointer.y - mouseLocal.y / nextView.zoom;
                static_cast<void>(ActiveVisualScriptCanvasUVE().SetViewUVE(nextView));
            }
            if (m_scriptCanvasLinkSourceNodeId != 0U && !m_scriptCanvasLinkSourcePin.empty()) {
                const auto* const sourceNode = findNode(m_scriptCanvasLinkSourceNodeId);
                if (sourceNode != nullptr) {
                    const auto* const sourcePin = findPin(*sourceNode, m_scriptCanvasLinkSourcePin);
                    if (sourcePin != nullptr) {
                        const ImVec2 start = pinScreenPosition(*sourceNode, *sourcePin);
                        drawList->AddLine(start, mouse, IM_COL32(240, 208, 116, 230), 2.0F);
                    }
                }
            }
            if (snapshot.nodes.empty()) {
                drawList->AddText(ImVec2{canvasOrigin.x + 20.0F, canvasOrigin.y + 20.0F},
                                  IM_COL32(184, 184, 188, 255),
                                  "Right-click or long-press to search and add a registered node.");
            }
            if (ImGui::BeginPopup("script-node-search-popup")) {
                std::array<char, 257> contextFilterBuffer{};
                std::strncpy(contextFilterBuffer.data(), m_scriptCanvasContextFilter.c_str(),
                             contextFilterBuffer.size() - 1U);
                ImGui::SetNextItemWidth(280.0F);
                if (ImGui::InputTextWithHint("##script-context-search", "Search registered nodes", contextFilterBuffer.data(),
                                             contextFilterBuffer.size())) {
                    m_scriptCanvasContextFilter = contextFilterBuffer.data();
                }
                ImGui::BeginChild("##script-context-results", ImVec2{280.0F, 220.0F}, false);
                std::size_t visibleContextNodes = 0U;
                for (const Scripting::ScriptGraphCanvasPaletteEntryUVE& entry : snapshot.paletteDescriptors) {
                    if (!ContainsCaseInsensitiveUVE(entry.displayName, m_scriptCanvasContextFilter) &&
                        !ContainsCaseInsensitiveUVE(entry.category, m_scriptCanvasContextFilter) &&
                        !ContainsCaseInsensitiveUVE(entry.typeId, m_scriptCanvasContextFilter)) {
                        continue;
                    }
                    ++visibleContextNodes;
                    const std::string label = (entry.displayName.empty() ? entry.typeId : entry.displayName) +
                                              "##context-node-" + entry.typeId;
                    if (ImGui::Selectable(label.c_str())) {
                        static_cast<void>(ActiveVisualScriptCanvasUVE().AddNodeTypeUVE(
                            entry.typeId, m_scriptCanvasContextMenuPosition, snapshot.revision));
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine(ImGui::GetWindowWidth() - 76.0F);
                    ImGui::TextDisabled("%s", entry.category.c_str());
                }
                if (visibleContextNodes == 0U) {
                    ImGui::TextDisabled(m_scriptCanvasContextFilter.empty()
                        ? "No registered nodes are available."
                        : "No results. Try a node name or category.");
                }
                ImGui::EndChild();
                ImGui::EndPopup();
            }
        }
        ImGui::EndChild();
        ImGui::SameLine();

        if (ImGui::BeginChild("##script-details", ImVec2{0.0F, 0.0F}, true)) {
            ImGui::TextColored(ImVec4{0.70F, 0.72F, 0.76F, 1.0F}, "DETAILS");
            ImGui::SameLine();
            ImGui::TextDisabled("node properties");
            ImGui::Separator();
            const auto* const node = selectedNode();
            if (node == nullptr) {
                ImGui::TextDisabled("Select one node to inspect its pins.");
            } else {
                ImGui::TextWrapped("%s", node->displayName.empty() ? node->typeId.c_str() : node->displayName.c_str());
                ImGui::TextDisabled("Type: %s | Node ID: %u", node->typeId.c_str(), node->id);
                ImGui::Separator();
                for (const auto& pin : node->pins) {
                    const ImU32 color = ScriptPinColorUVE(pin);
                    const ImVec4 colorFloat{
                        static_cast<float>((color >> IM_COL32_R_SHIFT) & 0xffU) / 255.0F,
                        static_cast<float>((color >> IM_COL32_G_SHIFT) & 0xffU) / 255.0F,
                        static_cast<float>((color >> IM_COL32_B_SHIFT) & 0xffU) / 255.0F, 1.0F};
                    ImGui::TextColored(colorFloat, "%s %s | %s", pin.direction == Scripting::ScriptPinDirectionUVE::Input ? "IN" : "OUT",
                                       pin.name.c_str(), ScriptValueTypeLabelUVE(pin.type));
                    if (pin.direction == Scripting::ScriptPinDirectionUVE::Input && pin.role == Scripting::ScriptPinRoleUVE::Data &&
                        (pin.type == Scripting::ScriptValueTypeUVE::Number || pin.type == Scripting::ScriptValueTypeUVE::Boolean)) {
                        if (m_scriptCanvasDefaultEditNodeId != node->id || m_scriptCanvasDefaultEditPin != pin.name) {
                            m_scriptCanvasDefaultEditNodeId = node->id;
                            m_scriptCanvasDefaultEditPin = pin.name;
                            m_scriptCanvasDefaultEditBuffer = pin.defaultValue.value_or("");
                        }
                        std::array<char, 257> defaultBuffer{};
                        std::strncpy(defaultBuffer.data(), m_scriptCanvasDefaultEditBuffer.c_str(), defaultBuffer.size() - 1U);
                        const std::string inputId = "Default##" + std::to_string(node->id) + "-" + pin.name;
                        if (ImGui::InputText(inputId.c_str(), defaultBuffer.data(), defaultBuffer.size(),
                                             ImGuiInputTextFlags_EnterReturnsTrue)) {
                            m_scriptCanvasDefaultEditBuffer = defaultBuffer.data();
                            static_cast<void>(ActiveVisualScriptCanvasUVE().SetPinDefaultValueUVE(
                                node->id, pin.name, m_scriptCanvasDefaultEditBuffer));
                        } else {
                            m_scriptCanvasDefaultEditBuffer = defaultBuffer.data();
                        }
                    }
                }
            }
            ImGui::Separator();
            ImGui::TextUnformatted("Validation");
            if (snapshot.diagnostics.empty()) {
                ImGui::TextColored(ImVec4{0.45F, 0.86F, 0.63F, 1.0F}, "No graph diagnostics.");
            } else {
                for (const auto& diagnostic : snapshot.diagnostics) {
                    ImGui::TextWrapped("Node %u: %s", diagnostic.nodeId, diagnostic.message.c_str());
                }
            }
            ImGui::Separator();
            ImGui::TextUnformatted("Compiler");
            if (!m_scriptCompileAttempted) {
                ImGui::TextDisabled("Not compiled yet.");
            } else if (m_scriptLastCompiledGraphRevision != snapshot.graphRevision) {
                ImGui::TextColored(ImVec4{0.93F, 0.72F, 0.35F, 1.0F},
                                   "Graph changed since the last compile.");
            } else {
                const ImVec4 statusColor = m_scriptCompileSucceeded
                    ? ImVec4{0.45F, 0.86F, 0.63F, 1.0F}
                    : ImVec4{0.96F, 0.43F, 0.43F, 1.0F};
                ImGui::TextColored(statusColor, "%s", m_scriptCompileMessage.c_str());
            }
            if (!snapshot.selectedNodeIds.empty() && ImGui::SmallButton("Delete selected node")) {
                for (const std::uint32_t nodeId : snapshot.selectedNodeIds) {
                    static_cast<void>(ActiveVisualScriptCanvasUVE().RemoveNodeUVE(nodeId));
                }
            }
            ImGui::TextDisabled("Graph edits use native validation, revision checks, and canvas history.");
        }
        ImGui::EndChild();
    }
    ImGui::EndChild();
    ImGui::End();
}


} // namespace UVE::Editor
