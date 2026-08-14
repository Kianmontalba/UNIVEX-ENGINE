// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/editor/editor_bridge_uve.h"

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
    };
    return capabilities;
}

[[nodiscard]] bool IsMutationRequestUVE(const EditorBridgeRequestKindUVE kind) noexcept {
    return kind != EditorBridgeRequestKindUVE::ReadSnapshot;
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
            if (!request.entityName.has_value()) {
                return MakeResponseUVE(request, false, "bridge.request.invalid",
                                       "SetSelectedEntityName requires a name payload.");
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
        case EditorBridgeRequestKindUVE::ReadSnapshot:
            break;
    }

    SynchronizeRevisionUVE();
    EditorBridgeResponseUVE response = MakeResponseUVE(request, applied, std::move(code), std::move(message));
    response.createdEntity = createdEntity;
    return response;
}

EditorBridgeUVE::ObservedStateUVE EditorBridgeUVE::CaptureObservedStateUVE() const {
    ObservedStateUVE observed{};
    observed.editorState = m_editor->GetStateUVE();
    observed.playModeState = m_editor->GetPlayModeStateUVE();
    observed.sceneDirty = m_editor->IsSceneDirtyUVE();
    observed.canUndo = m_editor->CanUndoUVE();
    observed.canRedo = m_editor->CanRedoUVE();
    observed.activeScenePath = m_editor->GetActiveScenePathUVE();
    for (const Scene::EntityUVE entity : m_editor->GetSelectedEntitiesUVE()) {
        observed.selectedEntities.push_back(EditorBridgeEntitySnapshotUVE{ToBridgeEntityUVE(entity),
                                                                           m_editor->GetEntityDisplayLabelUVE(entity)});
    }
    const Scene::EntityUVE active = m_editor->GetSelectedEntityUVE();
    if (m_editor->IsDocumentEntityUVE(active)) {
        observed.activeEntity = ToBridgeEntityUVE(active);
    }
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
    snapshot.activeEntity = observed.activeEntity;
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

} // namespace UVE::Editor
