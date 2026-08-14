// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/editor/editor_bridge_uve.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <utility>

namespace UVE::Editor {
namespace {

[[nodiscard]] const std::vector<EditorBridgeCapabilityUVE>& CapabilitiesUVE() noexcept {
    static const std::vector<EditorBridgeCapabilityUVE> capabilities{
        EditorBridgeCapabilityUVE::ReadSnapshot,
        EditorBridgeCapabilityUVE::SelectEntity,
        EditorBridgeCapabilityUVE::ClearSelection,
        EditorBridgeCapabilityUVE::SetSelectedEntityName,
        EditorBridgeCapabilityUVE::CreateDocumentEntity,
        EditorBridgeCapabilityUVE::Undo,
        EditorBridgeCapabilityUVE::Redo,
        EditorBridgeCapabilityUVE::ReadHierarchy,
        EditorBridgeCapabilityUVE::ReadInspector,
        EditorBridgeCapabilityUVE::ReadContentBrowser,
        EditorBridgeCapabilityUVE::ToggleEntitySelection,
        EditorBridgeCapabilityUVE::SetHierarchyFilter,
        EditorBridgeCapabilityUVE::SetContentBrowserDirectory,
        EditorBridgeCapabilityUVE::SetContentBrowserFilter,
        EditorBridgeCapabilityUVE::SetContentBrowserFocus,
        EditorBridgeCapabilityUVE::RefreshContentBrowser,
        EditorBridgeCapabilityUVE::SelectContentBrowserEntry,
    };
    return capabilities;
}

[[nodiscard]] bool IsMutationRequestUVE(const EditorBridgeRequestKindUVE kind) noexcept {
    return kind != EditorBridgeRequestKindUVE::ReadSnapshot;
}

[[nodiscard]] bool ContainsCaseInsensitiveUVE(const std::string& value, const std::string& needle) {
    if (needle.empty()) {
        return true;
    }
    if (needle.size() > value.size()) {
        return false;
    }

    const auto toLower = [](const unsigned char character) {
        return static_cast<char>(std::tolower(character));
    };
    return std::search(value.begin(), value.end(), needle.begin(), needle.end(),
                       [&toLower](const char lhs, const char rhs) {
                           return toLower(static_cast<unsigned char>(lhs)) == toLower(static_cast<unsigned char>(rhs));
                       }) != value.end();
}

[[nodiscard]] bool IsBoundedRequestTextUVE(const std::string& value) noexcept {
    return value.size() <= kEditorBridgeMaximumPresentationTextBytesUVE;
}

[[nodiscard]] bool IsBoundedContentPathUVE(const std::string& value) noexcept {
    return value.size() <= kEditorBridgeMaximumContentPathBytesUVE;
}

} // namespace

EditorBridgeUVE::EditorBridgeUVE(EditorUVE& editor) noexcept : m_editor(&editor) {}

const std::vector<EditorBridgeCapabilityUVE>& EditorBridgeUVE::GetCapabilitiesUVE() noexcept {
    return CapabilitiesUVE();
}

EditorBridgeSnapshotUVE EditorBridgeUVE::GetSnapshotUVE() {
    SynchronizeRevisionUVE();
    return BuildSnapshotUVE();
}

EditorBridgeResponseUVE EditorBridgeUVE::DispatchUVE(const EditorBridgeRequestUVE& request) {
    SynchronizeRevisionUVE();
    if (request.protocolVersion != kEditorBridgeProtocolVersionUVE) {
        return MakeResponseUVE(request, false, "bridge.protocol.unsupported",
                               "The request protocol version is not supported by this editor backend.");
    }
    if (request.kind == EditorBridgeRequestKindUVE::ReadSnapshot) {
        return MakeResponseUVE(request, true, "bridge.snapshot.read", "Bridge-visible editor state was copied.");
    }
    if (m_editor->GetStateUVE() != EditorStateUVE::Running) {
        return MakeResponseUVE(request, false, "bridge.editor.not_running",
                               "The editor is not in a running state and cannot accept bridge commands.");
    }
    if (IsMutationRequestUVE(request.kind) && request.expectedRevision != m_revision) {
        return MakeResponseUVE(request, false, "bridge.snapshot.stale",
                               "The request was based on an older bridge-visible editor snapshot.");
    }

    bool applied = false;
    std::string code = "bridge.command.rejected";
    std::string message = "The editor command was rejected without applying a mutation.";
    std::optional<EditorBridgeEntityRefUVE> createdEntity;
    switch (request.kind) {
        case EditorBridgeRequestKindUVE::SelectEntity: {
            if (!request.entity.has_value() || !request.entity->IsValidUVE() ||
                !m_editor->IsDocumentEntityUVE(ToEntityUVE(*request.entity))) {
                return MakeResponseUVE(request, false, "bridge.entity.invalid",
                                       "The requested entity is not a live document entity.");
            }
            m_editor->SelectEntityUVE(ToEntityUVE(*request.entity));
            applied = true;
            code = "bridge.command.applied";
            message = "The document entity became the active selection.";
            break;
        }
        case EditorBridgeRequestKindUVE::ClearSelection:
            m_editor->ClearSelectionUVE();
            applied = true;
            code = "bridge.command.applied";
            message = "The editor selection was cleared.";
            break;
        case EditorBridgeRequestKindUVE::SetSelectedEntityName:
            if (!request.entityName.has_value() || !IsBoundedRequestTextUVE(*request.entityName)) {
                return MakeResponseUVE(request, false, "bridge.request.invalid",
                                       "SetSelectedEntityName requires a bounded name payload.");
            }
            applied = m_editor->SetSelectedEntityNameUVE(*request.entityName);
            if (applied) {
                code = "bridge.command.applied";
                message = "The selected document entity name was updated through the native command path.";
            }
            break;
        case EditorBridgeRequestKindUVE::CreateDocumentEntity: {
            if (!request.entityKind.has_value() || !IsSupportedEntityKindUVE(*request.entityKind)) {
                return MakeResponseUVE(request, false, "bridge.request.invalid",
                                       "CreateDocumentEntity requires one supported document entity kind.");
            }
            const Scene::EntityUVE entity = m_editor->CreateDocumentEntityUVE(*request.entityKind);
            if (entity != Scene::kInvalidEntityUVE) {
                applied = true;
                createdEntity = ToBridgeEntityUVE(entity);
                code = "bridge.command.applied";
                message = "A document entity was created through the native command path.";
            }
            break;
        }
        case EditorBridgeRequestKindUVE::Undo:
            applied = m_editor->UndoUVE();
            if (applied) {
                code = "bridge.command.applied";
                message = "The native editor undo command completed.";
            }
            break;
        case EditorBridgeRequestKindUVE::Redo:
            applied = m_editor->RedoUVE();
            if (applied) {
                code = "bridge.command.applied";
                message = "The native editor redo command completed.";
            }
            break;
        case EditorBridgeRequestKindUVE::ToggleEntitySelection: {
            if (!request.entity.has_value() || !request.entity->IsValidUVE() ||
                !m_editor->IsDocumentEntityUVE(ToEntityUVE(*request.entity))) {
                return MakeResponseUVE(request, false, "bridge.entity.invalid",
                                       "The requested entity is not a live document entity.");
            }
            m_editor->ToggleEntitySelectionUVE(ToEntityUVE(*request.entity));
            applied = true;
            code = "bridge.command.applied";
            message = "The document entity selection was toggled through the native selection path.";
            break;
        }
        case EditorBridgeRequestKindUVE::SetHierarchyFilter:
            if (!request.hierarchyFilter.has_value() || !IsBoundedRequestTextUVE(*request.hierarchyFilter)) {
                return MakeResponseUVE(request, false, "bridge.request.invalid",
                                       "SetHierarchyFilter requires a bounded filter payload.");
            }
            if (m_editor->m_hierarchyFilter != *request.hierarchyFilter) {
                m_editor->m_hierarchyFilter = *request.hierarchyFilter;
                m_editor->InvalidateHierarchyFilterCacheUVE();
                applied = true;
                code = "bridge.command.applied";
                message = "The native hierarchy filter was updated.";
            }
            break;
        case EditorBridgeRequestKindUVE::SetContentBrowserDirectory: {
            if (!request.contentDirectory.has_value() || !IsBoundedContentPathUVE(*request.contentDirectory)) {
                return MakeResponseUVE(request, false, "bridge.request.invalid",
                                       "SetContentBrowserDirectory requires a bounded relative directory payload.");
            }
            const std::filesystem::path requestedDirectory = std::filesystem::path{*request.contentDirectory}.lexically_normal();
            if (requestedDirectory.is_absolute() || requestedDirectory.has_root_name() ||
                std::find(requestedDirectory.begin(), requestedDirectory.end(), std::filesystem::path{".."}) !=
                    requestedDirectory.end()) {
                return MakeResponseUVE(request, false, "bridge.request.invalid",
                                       "The requested content browser directory must be a normalized relative path.");
            }
            const Asset::ProjectFileSnapshotUVE snapshot = m_editor->m_services->GetProjectFileIndexUVE().GetSnapshotUVE();
            if (!m_editor->IsContentBrowserDirectoryInSnapshotUVE(snapshot, requestedDirectory)) {
                return MakeResponseUVE(request, false, "bridge.content.directory.invalid",
                                       "The requested content browser directory is absent from the current native snapshot.");
            }
            if (m_editor->m_contentBrowserDirectory != requestedDirectory) {
                m_editor->m_contentBrowserDirectory = requestedDirectory;
                applied = true;
                code = "bridge.command.applied";
                message = "The native content browser directory was updated.";
            }
            break;
        }
        case EditorBridgeRequestKindUVE::SetContentBrowserFilter:
            if (!request.contentFilter.has_value() || !IsBoundedRequestTextUVE(*request.contentFilter)) {
                return MakeResponseUVE(request, false, "bridge.request.invalid",
                                       "SetContentBrowserFilter requires a bounded filter payload.");
            }
            if (m_editor->m_assetFilter != *request.contentFilter) {
                m_editor->m_assetFilter = *request.contentFilter;
                applied = true;
                code = "bridge.command.applied";
                message = "The native content browser filter was updated.";
            }
            break;
        case EditorBridgeRequestKindUVE::SetContentBrowserFocus: {
            if (!request.contentFocus.has_value() || !IsBoundedRequestTextUVE(*request.contentFocus)) {
                return MakeResponseUVE(request, false, "bridge.request.invalid",
                                       "SetContentBrowserFocus requires one known bounded focus identifier.");
            }
            const std::optional<EditorUVE::ContentBrowserTypeFocusUVE> focus =
                ParseContentBrowserFocusUVE(*request.contentFocus);
            if (!focus.has_value()) {
                return MakeResponseUVE(request, false, "bridge.request.invalid",
                                       "The requested content browser focus is not supported.");
            }
            if (m_editor->m_contentBrowserTypeFocus != *focus) {
                m_editor->m_contentBrowserTypeFocus = *focus;
                applied = true;
                code = "bridge.command.applied";
                message = "The native content browser type focus was updated.";
            }
            break;
        }
        case EditorBridgeRequestKindUVE::RefreshContentBrowser: {
            Asset::IProjectFileIndexUVE& projectFileIndex = m_editor->m_services->GetProjectFileIndexUVE();
            Asset::IProjectChangeWatcherUVE& projectChangeWatcher = m_editor->m_services->GetProjectChangeWatcherUVE();
            const Asset::ProjectChangeSnapshotUVE changesBeforeRefresh = projectChangeWatcher.GetSnapshotUVE();
            m_editor->m_projectFileLastRefreshSucceeded =
                projectFileIndex.RefreshUVE(m_editor->m_services->GetAssetDatabaseUVE());
            m_editor->m_projectFileSnapshotInitialized = true;
            if (!m_editor->m_projectFileLastRefreshSucceeded) {
                SynchronizeRevisionUVE();
                return MakeResponseUVE(request, false, "bridge.content.refresh.failed",
                                       "The native project content index retained its last successful snapshot after refresh failure.");
            }
            projectChangeWatcher.AcknowledgeThroughUVE(changesBeforeRefresh.latestSequence);
            if (changesBeforeRefresh.rescanRequired) {
                projectChangeWatcher.AcknowledgeRescanUVE();
            }
            const Asset::ProjectFileSnapshotUVE refreshedSnapshot = projectFileIndex.GetSnapshotUVE();
            m_editor->ReconcileContentBrowserDirectoryUVE(refreshedSnapshot);
            applied = true;
            code = "bridge.command.applied";
            message = "The native cached project content snapshot was refreshed explicitly.";
            break;
        }
        case EditorBridgeRequestKindUVE::SelectContentBrowserEntry: {
            if (!request.contentEntryPath.has_value() || !IsBoundedContentPathUVE(*request.contentEntryPath)) {
                return MakeResponseUVE(request, false, "bridge.request.invalid",
                                       "SelectContentBrowserEntry requires a bounded relative entry path.");
            }
            const std::filesystem::path requestedPath = std::filesystem::path{*request.contentEntryPath}.lexically_normal();
            if (requestedPath.empty() || requestedPath.is_absolute() || requestedPath.has_root_name() ||
                std::find(requestedPath.begin(), requestedPath.end(), std::filesystem::path{".."}) != requestedPath.end()) {
                return MakeResponseUVE(request, false, "bridge.request.invalid",
                                       "The requested content browser entry must be a normalized relative path.");
            }
            const Asset::ProjectFileSnapshotUVE snapshot = m_editor->m_services->GetProjectFileIndexUVE().GetSnapshotUVE();
            const auto selected = std::find_if(snapshot.entries.cbegin(), snapshot.entries.cend(),
                                               [&requestedPath](const Asset::ProjectFileEntryUVE& entry) {
                                                   return entry.relativePath == requestedPath;
                                               });
            if (selected == snapshot.entries.cend()) {
                return MakeResponseUVE(request, false, "bridge.content.entry.invalid",
                                       "The requested cached project content entry is absent from the native snapshot.");
            }
            if (!m_editor->m_selectedProjectFile.has_value() ||
                m_editor->m_selectedProjectFile->relativePath != selected->relativePath ||
                m_editor->m_selectedProjectFile->kind != selected->kind) {
                m_editor->m_selectedProjectFile = *selected;
                if (selected->registeredAssetGuid.has_value()) {
                    m_editor->m_selectedAsset = Asset::AssetRecordUVE{
                        *selected->registeredAssetGuid, snapshot.contentRoot / selected->relativePath};
                } else {
                    m_editor->m_selectedAsset.reset();
                }
                applied = true;
                code = "bridge.command.applied";
                message = "The cached project content entry became the native content-browser selection.";
            }
            break;
        }
        case EditorBridgeRequestKindUVE::ReadSnapshot:
            break;
    }

    SynchronizeRevisionUVE();
    EditorBridgeResponseUVE response = MakeResponseUVE(request, applied, std::move(code), std::move(message));
    response.createdEntity = createdEntity;
    return response;
}

EditorBridgeUVE::ObservedStateUVE EditorBridgeUVE::CaptureObservedStateUVE() {
    ObservedStateUVE observed{};
    observed.editorState = m_editor->GetStateUVE();
    observed.playModeState = m_editor->GetPlayModeStateUVE();
    observed.sceneDirty = m_editor->IsSceneDirtyUVE();
    observed.canUndo = m_editor->CanUndoUVE();
    observed.canRedo = m_editor->CanRedoUVE();
    observed.activeScenePath = m_editor->GetActiveScenePathUVE();
    for (const Scene::EntityUVE entity : m_editor->GetSelectedEntitiesUVE()) {
        if (observed.selectedEntities.size() >= kEditorBridgeMaximumPanelEntriesUVE) {
            observed.selectedEntitiesTruncated = true;
            break;
        }
        observed.selectedEntities.push_back(
            EditorBridgeEntitySnapshotUVE{ToBridgeEntityUVE(entity), BoundPresentationTextUVE(m_editor->GetEntityDisplayLabelUVE(entity))});
    }
    const Scene::EntityUVE active = m_editor->GetSelectedEntityUVE();
    if (m_editor->IsDocumentEntityUVE(active)) {
        observed.activeEntity = ToBridgeEntityUVE(active);
    }
    observed.hierarchy = CaptureHierarchyUVE();
    observed.inspector = CaptureInspectorUVE();
    observed.contentBrowser = CaptureContentBrowserUVE();
    return observed;
}

void EditorBridgeUVE::SynchronizeRevisionUVE() {
    ObservedStateUVE observed = CaptureObservedStateUVE();
    if (!m_lastObservedState.has_value()) {
        m_lastObservedState = std::move(observed);
        m_revision = 1U;
        return;
    }
    if (*m_lastObservedState != observed) {
        m_lastObservedState = std::move(observed);
        ++m_revision;
    }
}

EditorBridgeSnapshotUVE EditorBridgeUVE::BuildSnapshotUVE() const {
    const ObservedStateUVE& observed = *m_lastObservedState;
    EditorBridgeSnapshotUVE snapshot{};
    snapshot.revision = m_revision;
    snapshot.editorState = observed.editorState;
    snapshot.playModeState = observed.playModeState;
    snapshot.sceneDirty = observed.sceneDirty;
    snapshot.canUndo = observed.canUndo;
    snapshot.canRedo = observed.canRedo;
    snapshot.activeScenePath = observed.activeScenePath;
    snapshot.selectedEntities = observed.selectedEntities;
    snapshot.selectedEntitiesTruncated = observed.selectedEntitiesTruncated;
    snapshot.activeEntity = observed.activeEntity;
    snapshot.hierarchy = observed.hierarchy;
    snapshot.inspector = observed.inspector;
    snapshot.contentBrowser = observed.contentBrowser;
    snapshot.capabilities = GetCapabilitiesUVE();
    return snapshot;
}

EditorBridgeResponseUVE EditorBridgeUVE::MakeResponseUVE(const EditorBridgeRequestUVE& request, const bool applied,
                                                           std::string code, std::string message) const {
    return EditorBridgeResponseUVE{kEditorBridgeProtocolVersionUVE, request.requestId, applied,
                                   std::move(code), std::move(message), BuildSnapshotUVE(), std::nullopt};
}

Scene::EntityUVE EditorBridgeUVE::ToEntityUVE(const EditorBridgeEntityRefUVE entity) noexcept {
    return Scene::EntityUVE{entity.index, entity.generation};
}

EditorBridgeEntityRefUVE EditorBridgeUVE::ToBridgeEntityUVE(const Scene::EntityUVE entity) noexcept {
    return EditorBridgeEntityRefUVE{entity.index, entity.generation};
}

bool EditorBridgeUVE::IsSupportedEntityKindUVE(const EditorEntityKindUVE kind) const noexcept {
    switch (kind) {
        case EditorEntityKindUVE::Empty:
        case EditorEntityKindUVE::Camera:
        case EditorEntityKindUVE::DirectionalLight:
        case EditorEntityKindUVE::CollisionBox:
        case EditorEntityKindUVE::Cube:
        case EditorEntityKindUVE::UVSphere:
        case EditorEntityKindUVE::Plane:
            return true;
    }
    return false;
}

std::string EditorBridgeUVE::BoundPresentationTextUVE(std::string value) {
    if (value.size() > kEditorBridgeMaximumPresentationTextBytesUVE) {
        value.resize(kEditorBridgeMaximumPresentationTextBytesUVE);
    }
    return value;
}

std::string EditorBridgeUVE::BoundContentPathUVE(std::string value) {
    if (value.size() > kEditorBridgeMaximumContentPathBytesUVE) {
        value.resize(kEditorBridgeMaximumContentPathBytesUVE);
    }
    return value;
}

EditorBridgeHierarchySnapshotUVE EditorBridgeUVE::CaptureHierarchyUVE() {
    EditorBridgeHierarchySnapshotUVE snapshot{};
    snapshot.filter = BoundPresentationTextUVE(m_editor->m_hierarchyFilter);
    snapshot.filterActive = m_editor->IsHierarchyFilterActiveUVE();
    m_editor->RebuildHierarchyFilterCacheUVE();

    Scene::IEntityManagerUVE& entityManager = m_editor->m_services->GetEntityManagerUVE();
    const auto visit = [this, &snapshot, &entityManager](const auto& self, const Scene::EntityUVE entity,
                                                          const std::size_t depth) -> void {
        if (!m_editor->IsDocumentEntityUVE(entity) || !m_editor->IsHierarchyEntityVisibleUVE(entity)) {
            return;
        }
        if (snapshot.entries.size() >= kEditorBridgeMaximumPanelEntriesUVE) {
            snapshot.truncated = true;
            return;
        }

        Scene::EntityUVE parent = Scene::kInvalidEntityUVE;
        const bool hasParent = m_editor->TryGetDocumentParentUVE(entity, parent) && parent != Scene::kInvalidEntityUVE;
        const std::vector<Scene::EntityUVE> children =
            m_editor->m_services->GetSceneGraphUVE().GetChildrenUVE(entityManager, entity);
        snapshot.entries.push_back(EditorBridgeHierarchyEntryUVE{
            ToBridgeEntityUVE(entity),
            hasParent ? std::optional<EditorBridgeEntityRefUVE>{ToBridgeEntityUVE(parent)} : std::nullopt,
            BoundPresentationTextUVE(m_editor->GetEntityDisplayLabelUVE(entity)),
            BoundPresentationTextUVE(m_editor->GetOutlinerTypeTagUVE(entity)),
            depth,
            children.size(),
            m_editor->IsEntitySelectedUVE(entity),
            entity == m_editor->m_selectedEntity,
        });
        for (const Scene::EntityUVE child : children) {
            self(self, child, depth + 1U);
        }
    };
    for (const Scene::EntityUVE root : m_editor->GetDocumentRootsUVE()) {
        visit(visit, root, 0U);
    }
    return snapshot;
}

EditorBridgeInspectorSnapshotUVE EditorBridgeUVE::CaptureInspectorUVE() const {
    EditorBridgeInspectorSnapshotUVE snapshot{};
    for (const Scene::EntityUVE entity : m_editor->GetSelectedEntitiesUVE()) {
        if (snapshot.selectedEntities.size() >= kEditorBridgeMaximumPanelEntriesUVE) {
            snapshot.selectedEntitiesTruncated = true;
            break;
        }
        snapshot.selectedEntities.push_back(
            EditorBridgeEntitySnapshotUVE{ToBridgeEntityUVE(entity), BoundPresentationTextUVE(m_editor->GetEntityDisplayLabelUVE(entity))});
    }
    if (snapshot.selectedEntities.empty()) {
        return snapshot;
    }

    const Scene::EntityUVE active = m_editor->GetSelectedEntityUVE();
    if (m_editor->IsDocumentEntityUVE(active)) {
        snapshot.activeEntity = EditorBridgeEntitySnapshotUVE{
            ToBridgeEntityUVE(active), BoundPresentationTextUVE(m_editor->GetEntityDisplayLabelUVE(active))};
    }
    if (!m_editor->HasSingleDocumentSelectionUVE() || !snapshot.activeEntity.has_value()) {
        snapshot.mode = EditorBridgeInspectorModeUVE::MultiSelection;
        return snapshot;
    }

    snapshot.mode = EditorBridgeInspectorModeUVE::SingleSelection;
    Scene::EntityUVE parent = Scene::kInvalidEntityUVE;
    if (m_editor->TryGetDocumentParentUVE(active, parent) && parent != Scene::kInvalidEntityUVE) {
        snapshot.parent = EditorBridgeEntitySnapshotUVE{
            ToBridgeEntityUVE(parent), BoundPresentationTextUVE(m_editor->GetHierarchyCandidateLabelUVE(parent))};
    }
    for (const Scene::EntityUVE ancestor : m_editor->GetDocumentAncestryUVE(active)) {
        if (snapshot.ancestry.size() >= kEditorBridgeMaximumPanelEntriesUVE) {
            break;
        }
        snapshot.ancestry.push_back(EditorBridgeEntitySnapshotUVE{
            ToBridgeEntityUVE(ancestor), BoundPresentationTextUVE(m_editor->GetHierarchyCandidateLabelUVE(ancestor))});
    }
    for (std::string identifier : m_editor->m_inspectorDrawerRegistry.GetEligibleDrawerIdsUVE(active)) {
        if (snapshot.eligibleDrawerIds.size() >= kEditorBridgeMaximumPanelEntriesUVE) {
            break;
        }
        snapshot.eligibleDrawerIds.push_back(BoundPresentationTextUVE(std::move(identifier)));
    }
    snapshot.canEditSelectedName = m_editor->IsAuthoringCommandAllowedUVE();
    return snapshot;
}

EditorBridgeContentBrowserSnapshotUVE EditorBridgeUVE::CaptureContentBrowserUVE() {
    EditorBridgeContentBrowserSnapshotUVE snapshot{};
    Asset::IProjectFileIndexUVE& projectFileIndex = m_editor->m_services->GetProjectFileIndexUVE();
    const Asset::ProjectFileSnapshotUVE nativeSnapshot = projectFileIndex.GetSnapshotUVE();
    m_editor->ReconcileContentBrowserDirectoryUVE(nativeSnapshot);

    snapshot.contentRoot = BoundPresentationTextUVE(nativeSnapshot.contentRoot.generic_string());
    snapshot.currentDirectory = BoundContentPathUVE(m_editor->m_contentBrowserDirectory.generic_string());
    snapshot.filter = BoundPresentationTextUVE(m_editor->m_assetFilter);
    snapshot.typeFocus = BoundPresentationTextUVE(EditorUVE::GetContentBrowserFocusLabelUVE(m_editor->m_contentBrowserTypeFocus));
    for (const std::filesystem::path& segment : m_editor->m_contentBrowserDirectory) {
        if (snapshot.breadcrumbs.size() >= kEditorBridgeMaximumPanelEntriesUVE) {
            break;
        }
        snapshot.breadcrumbs.push_back(BoundPresentationTextUVE(segment.generic_string()));
    }
    snapshot.refreshGeneration = nativeSnapshot.refreshGeneration;
    snapshot.contentRootExists = nativeSnapshot.contentRootExists;
    snapshot.initialized = m_editor->m_projectFileSnapshotInitialized;
    snapshot.lastRefreshSucceeded = m_editor->m_projectFileLastRefreshSucceeded;

    for (const Asset::ProjectFileEntryUVE& entry : nativeSnapshot.entries) {
        if (entry.relativePath.parent_path() != m_editor->m_contentBrowserDirectory) {
            continue;
        }
        ++snapshot.directEntryCount;
        const std::string entryPath = entry.relativePath.generic_string();
        if (!ContainsCaseInsensitiveUVE(entryPath, m_editor->m_assetFilter) ||
            !m_editor->DoesContentBrowserEntryMatchFocusUVE(entry)) {
            continue;
        }
        ++snapshot.visibleEntryCount;
        if (snapshot.entries.size() >= kEditorBridgeMaximumPanelEntriesUVE) {
            snapshot.truncated = true;
            continue;
        }
        snapshot.entries.push_back(ToContentEntryUVE(entry));
    }

    if (m_editor->m_selectedProjectFile.has_value()) {
        snapshot.selectedEntry = ToContentEntryUVE(*m_editor->m_selectedProjectFile);
    }
    return snapshot;
}

EditorBridgeContentBrowserEntryUVE EditorBridgeUVE::ToContentEntryUVE(const Asset::ProjectFileEntryUVE& entry) {
    return EditorBridgeContentBrowserEntryUVE{
        BoundContentPathUVE(entry.relativePath.generic_string()),
        entry.kind == Asset::ProjectFileEntryKindUVE::Directory,
        BoundPresentationTextUVE(EditorUVE::GetContentBrowserItemTypeLabelUVE(EditorUVE::ClassifyContentBrowserEntryUVE(entry))),
        entry.registeredAssetGuid.has_value() ? std::optional<std::uint64_t>{entry.registeredAssetGuid->value} : std::nullopt,
    };
}

std::optional<EditorUVE::ContentBrowserTypeFocusUVE> EditorBridgeUVE::ParseContentBrowserFocusUVE(
    const std::string& focus) noexcept {
    using Focus = EditorUVE::ContentBrowserTypeFocusUVE;
    if (focus == "all") {
        return Focus::All;
    }
    if (focus == "folders") {
        return Focus::Folders;
    }
    if (focus == "scene") {
        return Focus::Scene;
    }
    if (focus == "prefab") {
        return Focus::Prefab;
    }
    if (focus == "bundle") {
        return Focus::Bundle;
    }
    if (focus == "mesh") {
        return Focus::Mesh;
    }
    if (focus == "texture") {
        return Focus::Texture;
    }
    if (focus == "shader") {
        return Focus::Shader;
    }
    if (focus == "material") {
        return Focus::Material;
    }
    if (focus == "save") {
        return Focus::Save;
    }
    if (focus == "registered") {
        return Focus::Registered;
    }
    if (focus == "otherFiles") {
        return Focus::OtherFiles;
    }
    return std::nullopt;
}

} // namespace UVE::Editor
