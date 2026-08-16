// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/editor/editor_bridge_uve.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <type_traits>
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
        EditorBridgeCapabilityUVE::ReadViewportSurface,
        EditorBridgeCapabilityUVE::ReadVisualScriptCanvas,
        EditorBridgeCapabilityUVE::ReadVisualScriptDebugger,
        EditorBridgeCapabilityUVE::AddVisualScriptNode,
        EditorBridgeCapabilityUVE::RemoveVisualScriptNode,
        EditorBridgeCapabilityUVE::MoveVisualScriptNode,
        EditorBridgeCapabilityUVE::AddVisualScriptLink,
        EditorBridgeCapabilityUVE::RemoveVisualScriptLink,
        EditorBridgeCapabilityUVE::SetVisualScriptSelection,
        EditorBridgeCapabilityUVE::SetVisualScriptView,
        EditorBridgeCapabilityUVE::UndoVisualScript,
        EditorBridgeCapabilityUVE::RedoVisualScript,
        EditorBridgeCapabilityUVE::ReadDeveloperConsole,
        EditorBridgeCapabilityUVE::SubmitDeveloperConsoleCommand,
        EditorBridgeCapabilityUVE::ClearDeveloperConsole,
        EditorBridgeCapabilityUVE::SetDeveloperConsoleSeverityFilter,
        EditorBridgeCapabilityUVE::SetDeveloperConsoleCompletionPrefix,
        EditorBridgeCapabilityUVE::MoveDeveloperConsoleHistory,
        EditorBridgeCapabilityUVE::SelectDataTablePreview,
        EditorBridgeCapabilityUVE::ReadScriptRuntime,
        EditorBridgeCapabilityUVE::ReadScriptRuntimeTickDiagnostics,
        EditorBridgeCapabilityUVE::SerializeVisualScriptGraph,
        EditorBridgeCapabilityUVE::DeserializeVisualScriptGraph,
        EditorBridgeCapabilityUVE::AddVisualScriptNodeType,
        EditorBridgeCapabilityUVE::SetVisualScriptPinDefault,
        EditorBridgeCapabilityUVE::ReadMotionQuery,
        EditorBridgeCapabilityUVE::DispatchMotionQueryCommand,
        EditorBridgeCapabilityUVE::DispatchMotionQueryDebugCommand,
    };
    return capabilities;
}

[[nodiscard]] bool IsMutationRequestUVE(const EditorBridgeRequestKindUVE kind) noexcept {
    return kind != EditorBridgeRequestKindUVE::ReadSnapshot &&
           kind != EditorBridgeRequestKindUVE::ReadViewportSurface &&
           kind != EditorBridgeRequestKindUVE::ReadVisualScriptCanvas &&
           kind != EditorBridgeRequestKindUVE::ReadVisualScriptDebugger &&
           kind != EditorBridgeRequestKindUVE::ReadDeveloperConsole &&
           kind != EditorBridgeRequestKindUVE::ReadScriptRuntime &&
           kind != EditorBridgeRequestKindUVE::ReadScriptRuntimeTickDiagnostics &&
           kind != EditorBridgeRequestKindUVE::SerializeVisualScriptGraph &&
           kind != EditorBridgeRequestKindUVE::ReadMotionQuery;
}

[[nodiscard]] bool IsVisualScriptMutationRequestUVE(const EditorBridgeRequestKindUVE kind) noexcept {
    switch (kind) {
        case EditorBridgeRequestKindUVE::AddVisualScriptNode:
        case EditorBridgeRequestKindUVE::RemoveVisualScriptNode:
        case EditorBridgeRequestKindUVE::MoveVisualScriptNode:
        case EditorBridgeRequestKindUVE::AddVisualScriptLink:
        case EditorBridgeRequestKindUVE::RemoveVisualScriptLink:
        case EditorBridgeRequestKindUVE::SetVisualScriptSelection:
        case EditorBridgeRequestKindUVE::SetVisualScriptView:
        case EditorBridgeRequestKindUVE::UndoVisualScript:
        case EditorBridgeRequestKindUVE::RedoVisualScript:
        case EditorBridgeRequestKindUVE::DeserializeVisualScriptGraph:
        case EditorBridgeRequestKindUVE::AddVisualScriptNodeType:
        case EditorBridgeRequestKindUVE::SetVisualScriptPinDefault:
            return true;
        default:
            return false;
    }
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

[[nodiscard]] Scripting::ScriptGraphSchemaUVE CaptureGraphSchemaUVE(
    const Scripting::ScriptGraphCanvasUVE& canvas) {
    Scripting::ScriptGraphSchemaUVE schema{};
    schema.graph = canvas.GetGraphUVE();
    for (const Scripting::ScriptGraphCanvasLayoutEntryUVE& entry : canvas.GetLayoutSnapshotUVE().entries) {
        schema.layout.push_back({entry.nodeId, entry.position.x, entry.position.y});
    }
    schema.metadata.emplace("assetType", "visual-script-graph");
    return schema;
}

[[nodiscard]] std::string FormatDataTableValueUVE(const Asset::DataTableValueUVE& value) {
    return std::visit([](const auto& current) {
        using ValueType = std::decay_t<decltype(current)>;
        if constexpr (std::is_same_v<ValueType, bool>) {
            return std::string{current ? "true" : "false"};
        } else if constexpr (std::is_same_v<ValueType, std::int64_t>) {
            return std::to_string(current);
        } else if constexpr (std::is_same_v<ValueType, double>) {
            if (!std::isfinite(current)) {
                return std::string{"<non-finite>"};
            }
            char buffer[64]{};
            const auto result = std::to_chars(buffer, buffer + sizeof(buffer), current,
                                              std::chars_format::general, 15);
            return result.ec == std::errc{} ? std::string{buffer, result.ptr} : std::string{"<number>"};
        } else {
            return current;
        }
    }, value);
}

} // namespace

EditorBridgeUVE::EditorBridgeUVE(EditorUVE& editor,
                                   const Asset::DataTableRegistryUVE* dataTableRegistry,
                                   const Scripting::ScriptDebuggerUVE* scriptDebugger,
                                   const Scripting::ScriptRuntimeUVE* scriptRuntime) noexcept
    : m_editor(&editor),
      m_visualScriptCanvas(m_visualScriptRegistry),
      m_dataTableRegistry(dataTableRegistry),
      m_scriptDebugger(scriptDebugger),
      m_scriptRuntime(scriptRuntime) {}

const std::vector<EditorBridgeCapabilityUVE>& EditorBridgeUVE::GetCapabilitiesUVE() noexcept {
    return CapabilitiesUVE();
}

bool EditorBridgeUVE::SetPreviewTableUVE(const std::string_view name) {
    if (m_dataTableRegistry == nullptr || name.size() > kEditorBridgeMaximumPresentationTextBytesUVE) {
        return false;
    }
    if (!name.empty() && !m_dataTableRegistry->ContainsUVE(name)) {
        return false;
    }
    const std::optional<std::string> nextName = name.empty()
        ? std::nullopt
        : std::optional<std::string>{std::string{name}};
    if (m_dataTablePreviewName == nextName) {
        return true;
    }
    m_dataTablePreviewName = nextName;
    if (m_lastObservedState.has_value()) {
        SynchronizeRevisionUVE();
    }
    return true;
}

void EditorBridgeUVE::SetDataTableCatalogSnapshotUVE(Asset::DataTableCatalogSnapshotUVE snapshot) {
    if (m_dataTableRegistry != nullptr || m_dataTableCatalogSnapshot == snapshot) {
        return;
    }
    m_dataTableCatalogSnapshot = std::move(snapshot);
    if (m_lastObservedState.has_value()) {
        SynchronizeRevisionUVE();
    }
}

void EditorBridgeUVE::SetDataTablePreviewSnapshotUVE(Asset::DataTableSnapshotUVE snapshot) {
    if (m_dataTableRegistry != nullptr || m_dataTablePreviewSnapshot == snapshot) {
        return;
    }
    m_dataTablePreviewSnapshot = std::move(snapshot);
    if (m_lastObservedState.has_value()) {
        SynchronizeRevisionUVE();
    }
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
    if (request.kind == EditorBridgeRequestKindUVE::ReadVisualScriptCanvas) {
        return MakeResponseUVE(request, true, "bridge.visual_scripting.snapshot.read",
                               "The visual-scripting canvas snapshot was copied.");
    }
    if (request.kind == EditorBridgeRequestKindUVE::ReadVisualScriptDebugger) {
        return MakeResponseUVE(request, true, "bridge.visual_scripting.debugger.read",
                               "The read-only visual-scripting debugger snapshot was copied.");
    }
    if (request.kind == EditorBridgeRequestKindUVE::SerializeVisualScriptGraph) {
        EditorBridgeResponseUVE response = MakeResponseUVE(
            request, false, "bridge.visual_scripting.graph_schema.invalid",
            "The visual-scripting graph schema could not be serialized.");
        Scripting::ScriptGraphSchemaUVE schema = CaptureGraphSchemaUVE(m_visualScriptCanvas);
        std::vector<Scripting::ScriptPersistenceDiagnosticUVE> diagnostics;
        if (!Scripting::EncodeScriptGraphSchemaUVE(schema, diagnostics).empty()) {
            response.applied = true;
            response.code = "bridge.visual_scripting.graph_schema.serialized";
            response.message = "The native visual-scripting graph schema was copied in deterministic order.";
            response.visualScriptGraphSchema = std::move(schema);
        }
        return response;
    }
    if (request.kind == EditorBridgeRequestKindUVE::ReadDeveloperConsole) {
        return MakeResponseUVE(request, true, "bridge.developer_console.snapshot.read",
                               "The bounded developer-console snapshot was copied.");
    }
    if (request.kind == EditorBridgeRequestKindUVE::ReadScriptRuntime) {
        return MakeResponseUVE(request, true, "bridge.script_runtime.snapshot.read",
                               "The bounded ScriptRuntime snapshot was copied.");
    }
    if (request.kind == EditorBridgeRequestKindUVE::ReadScriptRuntimeTickDiagnostics) {
        EditorBridgeResponseUVE response = MakeResponseUVE(
            request, false, "bridge.script_runtime.tick.unavailable",
            "No native ScriptRuntime is attached to this bridge session.");
        EditorBridgeScriptRuntimeTickSummaryUVE summary;
        summary.reason = "No native ScriptRuntime is attached to this bridge session.";
        if (m_scriptRuntime != nullptr) {
            const Scripting::ScriptRuntimeTickBatchResultUVE tick = m_scriptRuntime->TickDetailedUVE();
            summary.available = true;
            summary.reason = "The native ScriptRuntime diagnostic tick completed.";
            summary.enabledInstanceCount = tick.summary.enabledInstanceCount;
            summary.completedCount = tick.summary.completedCount;
            summary.instructionBudgetExceededCount = tick.summary.instructionBudgetExceededCount;
            summary.invalidInstructionCount = tick.summary.invalidInstructionCount;
            summary.diagnosticCount = tick.summary.diagnosticCount;
            response.applied = true;
            response.code = "bridge.script_runtime.tick.completed";
            response.message = "The native ScriptRuntime diagnostic tick completed and its counters were copied.";
        }
        m_lastScriptRuntimeTickSummary = summary;
        if (m_scriptRuntimeTickHistory.size() >= kEditorBridgeMaximumScriptRuntimeTickHistoryUVE) {
            m_scriptRuntimeTickHistory.pop_front();
            m_scriptRuntimeTickHistoryTruncated = true;
        }
        m_scriptRuntimeTickHistory.push_back(
            EditorBridgeScriptRuntimeTickHistoryEntryUVE{m_nextScriptRuntimeTickSequence++, summary});
        ++m_revision;
        response.snapshot.revision = m_revision;
        response.snapshot.scriptRuntimeTickSummary = summary;
        response.snapshot.scriptRuntimeTickHistoryTruncated = m_scriptRuntimeTickHistoryTruncated;
        response.snapshot.scriptRuntimeTickHistory.assign(m_scriptRuntimeTickHistory.begin(),
                                                          m_scriptRuntimeTickHistory.end());
        return response;
    }
    if (request.kind == EditorBridgeRequestKindUVE::ReadMotionQuery) {
        return MakeResponseUVE(request, true, "bridge.motion_query.snapshot.read",
                               "The copied Motion Query authoring, debugger, and trace snapshot was returned.");
    }
    if (m_editor->GetStateUVE() != EditorStateUVE::Running) {
        return MakeResponseUVE(request, false, "bridge.editor.not_running",
                               "The editor is not in a running state and cannot accept bridge commands.");
    }
    if (IsMutationRequestUVE(request.kind) && request.expectedRevision != m_revision) {
        return MakeResponseUVE(request, false, "bridge.snapshot.stale",
                               "The request was based on an older bridge-visible editor snapshot.");
    }
    if (IsVisualScriptMutationRequestUVE(request.kind) &&
        m_editor->GetPlayModeStateUVE() != EditorPlayModeStateUVE::Edit) {
        return MakeResponseUVE(request, false, "bridge.visual_scripting.edit_mode_required",
                               "Visual-scripting canvas mutations are allowed only in Edit mode.");
    }

    bool applied = false;
    std::string code = "bridge.command.rejected";
    std::string message = "The editor command was rejected without applying a mutation.";
    std::optional<EditorBridgeEntityRefUVE> createdEntity;
    std::optional<Scripting::ScriptGraphSchemaUVE> responseSchema;
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
        case EditorBridgeRequestKindUVE::ReadViewportSurface:
            code = "bridge.viewport_surface.unavailable";
            message = "This headless bridge session has no attachable managed viewport surface; native C++ retains window and OpenGL ownership.";
            break;
        case EditorBridgeRequestKindUVE::ReadVisualScriptCanvas:
            code = "bridge.visual_scripting.snapshot.read";
            message = "The visual-scripting canvas snapshot was copied.";
            break;
        case EditorBridgeRequestKindUVE::ReadVisualScriptDebugger:
            code = "bridge.visual_scripting.debugger.read";
            message = "The read-only visual-scripting debugger snapshot was copied.";
            break;
        case EditorBridgeRequestKindUVE::AddVisualScriptNodeType:
            if (!request.visualScriptNodeTypeId.has_value() || request.visualScriptNodeTypeId->empty() ||
                request.visualScriptNodeTypeId->size() > 256U || !request.visualScriptPosition.has_value()) {
                return MakeResponseUVE(request, false, "bridge.visual_scripting.request.invalid",
                                       "AddVisualScriptNodeType requires a bounded type ID and finite position payload.");
            }
            {
                const auto result = m_visualScriptCanvas.AddNodeTypeUVE(
                    *request.visualScriptNodeTypeId, *request.visualScriptPosition, request.expectedRevision);
                applied = result.IsAppliedUVE();
                code = result.code == Scripting::ScriptGraphCanvasCommandCodeUVE::StaleRevision
                    ? "bridge.snapshot.stale" : applied ? "bridge.command.applied" : "bridge.command.rejected";
                message = result.message;
            }
            break;
        case EditorBridgeRequestKindUVE::SetVisualScriptPinDefault:
            if (!request.visualScriptNodeId.has_value() || !request.visualScriptPinName.has_value() ||
                !request.visualScriptDefaultValue.has_value() || request.visualScriptPinName->empty() ||
                request.visualScriptPinName->size() > 256U ||
                request.visualScriptDefaultValue->size() > Scripting::kMaximumScriptGraphCanvasDefaultValueBytesUVE) {
                return MakeResponseUVE(request, false, "bridge.visual_scripting.request.invalid",
                                       "SetVisualScriptPinDefault requires bounded node, pin, and value payloads.");
            }
            {
                const auto result = m_visualScriptCanvas.SetPinDefaultValueUVE(
                    *request.visualScriptNodeId, *request.visualScriptPinName,
                    *request.visualScriptDefaultValue, request.expectedRevision);
                applied = result.IsAppliedUVE();
                code = result.code == Scripting::ScriptGraphCanvasCommandCodeUVE::StaleRevision
                    ? "bridge.snapshot.stale" : applied ? "bridge.command.applied" :
                    result.code == Scripting::ScriptGraphCanvasCommandCodeUVE::NoHistory
                        ? "bridge.command.noop" : "bridge.command.rejected";
                message = result.message;
            }
            break;
        case EditorBridgeRequestKindUVE::AddVisualScriptNode:
            if (!request.visualScriptNode.has_value() || !request.visualScriptPosition.has_value()) {
                return MakeResponseUVE(request, false, "bridge.visual_scripting.request.invalid",
                                       "AddVisualScriptNode requires a node and finite position payload.");
            }
            {
                const auto result = m_visualScriptCanvas.AddNodeUVE(
                    *request.visualScriptNode, *request.visualScriptPosition, request.expectedRevision);
                applied = result.IsAppliedUVE();
                code = result.code == Scripting::ScriptGraphCanvasCommandCodeUVE::StaleRevision
                    ? "bridge.snapshot.stale" : applied ? "bridge.command.applied" : "bridge.command.rejected";
                message = result.message;
            }
            break;
        case EditorBridgeRequestKindUVE::RemoveVisualScriptNode:
            if (!request.visualScriptNodeId.has_value()) {
                return MakeResponseUVE(request, false, "bridge.visual_scripting.request.invalid",
                                       "RemoveVisualScriptNode requires a node ID.");
            }
            {
                const auto result = m_visualScriptCanvas.RemoveNodeUVE(
                    *request.visualScriptNodeId, request.expectedRevision);
                applied = result.IsAppliedUVE();
                code = result.code == Scripting::ScriptGraphCanvasCommandCodeUVE::StaleRevision
                    ? "bridge.snapshot.stale" : applied ? "bridge.command.applied" : "bridge.command.rejected";
                message = result.message;
            }
            break;
        case EditorBridgeRequestKindUVE::MoveVisualScriptNode:
            if (!request.visualScriptNodeId.has_value() || !request.visualScriptPosition.has_value()) {
                return MakeResponseUVE(request, false, "bridge.visual_scripting.request.invalid",
                                       "MoveVisualScriptNode requires a node ID and finite position payload.");
            }
            {
                const auto result = m_visualScriptCanvas.MoveNodeUVE(
                    *request.visualScriptNodeId, *request.visualScriptPosition, request.expectedRevision);
                applied = result.IsAppliedUVE();
                code = result.code == Scripting::ScriptGraphCanvasCommandCodeUVE::StaleRevision
                    ? "bridge.snapshot.stale" : applied ? "bridge.command.applied" : "bridge.command.rejected";
                message = result.message;
            }
            break;
        case EditorBridgeRequestKindUVE::AddVisualScriptLink:
            if (!request.visualScriptLink.has_value()) {
                return MakeResponseUVE(request, false, "bridge.visual_scripting.request.invalid",
                                       "AddVisualScriptLink requires a link payload.");
            }
            {
                const auto result = m_visualScriptCanvas.AddLinkUVE(
                    *request.visualScriptLink, request.expectedRevision);
                applied = result.IsAppliedUVE();
                code = result.code == Scripting::ScriptGraphCanvasCommandCodeUVE::StaleRevision
                    ? "bridge.snapshot.stale" : applied ? "bridge.command.applied" : "bridge.command.rejected";
                message = result.message;
            }
            break;
        case EditorBridgeRequestKindUVE::RemoveVisualScriptLink:
            if (!request.visualScriptLink.has_value()) {
                return MakeResponseUVE(request, false, "bridge.visual_scripting.request.invalid",
                                       "RemoveVisualScriptLink requires a link payload.");
            }
            {
                const auto result = m_visualScriptCanvas.RemoveLinkUVE(
                    *request.visualScriptLink, request.expectedRevision);
                applied = result.IsAppliedUVE();
                code = result.code == Scripting::ScriptGraphCanvasCommandCodeUVE::StaleRevision
                    ? "bridge.snapshot.stale" : applied ? "bridge.command.applied" : "bridge.command.rejected";
                message = result.message;
            }
            break;
        case EditorBridgeRequestKindUVE::SetVisualScriptSelection:
            if (!request.visualScriptSelection.has_value()) {
                return MakeResponseUVE(request, false, "bridge.visual_scripting.request.invalid",
                                       "SetVisualScriptSelection requires a selection payload.");
            }
            {
                const auto result = m_visualScriptCanvas.SetSelectionUVE(
                    *request.visualScriptSelection, request.expectedRevision);
                applied = result.IsAppliedUVE();
                code = result.code == Scripting::ScriptGraphCanvasCommandCodeUVE::StaleRevision
                    ? "bridge.snapshot.stale" : applied ? "bridge.command.applied" : "bridge.command.rejected";
                message = result.message;
            }
            break;
        case EditorBridgeRequestKindUVE::SetVisualScriptView:
            if (!request.visualScriptView.has_value()) {
                return MakeResponseUVE(request, false, "bridge.visual_scripting.request.invalid",
                                       "SetVisualScriptView requires a finite view payload.");
            }
            {
                const auto result = m_visualScriptCanvas.SetViewUVE(
                    *request.visualScriptView, request.expectedRevision);
                applied = result.IsAppliedUVE();
                code = result.code == Scripting::ScriptGraphCanvasCommandCodeUVE::StaleRevision
                    ? "bridge.snapshot.stale" : applied ? "bridge.command.applied" : "bridge.command.rejected";
                message = result.message;
            }
            break;
        case EditorBridgeRequestKindUVE::UndoVisualScript: {
            const auto result = m_visualScriptCanvas.UndoUVE(request.expectedRevision);
            applied = result.IsAppliedUVE();
            code = result.code == Scripting::ScriptGraphCanvasCommandCodeUVE::StaleRevision
                ? "bridge.snapshot.stale" : applied ? "bridge.command.applied" : "bridge.command.rejected";
            message = result.message;
            break;
        }
        case EditorBridgeRequestKindUVE::RedoVisualScript: {
            const auto result = m_visualScriptCanvas.RedoUVE(request.expectedRevision);
            applied = result.IsAppliedUVE();
            code = result.code == Scripting::ScriptGraphCanvasCommandCodeUVE::StaleRevision
                ? "bridge.snapshot.stale" : applied ? "bridge.command.applied" : "bridge.command.rejected";
            message = result.message;
            break;
        }
        case EditorBridgeRequestKindUVE::SubmitDeveloperConsoleCommand:
            if (!request.developerConsoleCommand.has_value() ||
                request.developerConsoleCommand->size() > DeveloperConsoleUVE::kMaximumValueBytesUVE) {
                return MakeResponseUVE(request, false, "bridge.developer_console.request.invalid",
                                       "SubmitDeveloperConsoleCommand requires a bounded command payload.");
            }
            applied = m_developerConsole.ExecuteUVE(*request.developerConsoleCommand);
            code = applied ? "bridge.command.applied" : "bridge.developer_console.command.rejected";
            message = applied ? "The registered native console command executed."
                              : "The native console rejected or could not execute the console command.";
            break;
        case EditorBridgeRequestKindUVE::ClearDeveloperConsole:
            applied = m_developerConsole.ClearUVE();
            code = applied ? "bridge.command.applied" : "bridge.developer_console.clear.noop";
            message = applied ? "The native developer-console output was cleared."
                              : "The native developer-console output was already empty.";
            break;
        case EditorBridgeRequestKindUVE::SetDeveloperConsoleSeverityFilter:
            if (!request.developerConsoleSeverityFilter.has_value()) {
                return MakeResponseUVE(request, false, "bridge.developer_console.request.invalid",
                                       "SetDeveloperConsoleSeverityFilter requires a severity-filter payload.");
            }
            if (!m_developerConsole.IsAvailableUVE()) {
                return MakeResponseUVE(request, false, "bridge.developer_console.unavailable",
                                       "Developer-console discovery is disabled by the native shipping build policy.");
            }
            applied = m_developerConsole.SetSeverityFilterUVE(*request.developerConsoleSeverityFilter);
            code = applied ? "bridge.command.applied" : "bridge.developer_console.filter.noop";
            message = applied ? "The native developer-console severity filter was updated."
                              : "The native developer-console severity filter was already set.";
            break;
        case EditorBridgeRequestKindUVE::SetDeveloperConsoleCompletionPrefix:
            if (!request.developerConsoleCompletionPrefix.has_value() ||
                request.developerConsoleCompletionPrefix->size() > DeveloperConsoleUVE::kMaximumValueBytesUVE) {
                return MakeResponseUVE(request, false, "bridge.developer_console.request.invalid",
                                       "SetDeveloperConsoleCompletionPrefix requires a bounded prefix payload.");
            }
            if (!m_developerConsole.IsAvailableUVE()) {
                return MakeResponseUVE(request, false, "bridge.developer_console.unavailable",
                                       "Developer-console discovery is disabled by the native shipping build policy.");
            }
            applied = m_developerConsole.SetCompletionPrefixUVE(*request.developerConsoleCompletionPrefix);
            code = applied ? "bridge.command.applied" : "bridge.developer_console.completion.noop";
            message = applied ? "The native developer-console completion prefix was updated."
                              : "The native developer-console completion prefix was already set.";
            break;
        case EditorBridgeRequestKindUVE::SelectDataTablePreview:
            if (!request.dataTableName.has_value() ||
                request.dataTableName->size() > kEditorBridgeMaximumPresentationTextBytesUVE) {
                return MakeResponseUVE(request, false, "bridge.data_table.request.invalid",
                                       "SelectDataTablePreview requires a bounded table name payload.");
            }
            if (m_dataTableRegistry == nullptr) {
                return MakeResponseUVE(request, false, "bridge.data_table.registry.unavailable",
                                       "Data-table preview selection requires a native registry-backed bridge session.");
            }
            applied = SetPreviewTableUVE(*request.dataTableName);
            code = applied ? "bridge.command.applied" : "bridge.data_table.preview.invalid";
            message = applied
                ? "The selected native data-table preview was updated."
                : "The requested native data-table name is not available in the session registry.";
            break;
        case EditorBridgeRequestKindUVE::MoveDeveloperConsoleHistory:
            if (!request.developerConsoleHistoryDelta.has_value() || *request.developerConsoleHistoryDelta == 0) {
                return MakeResponseUVE(request, false, "bridge.developer_console.request.invalid",
                                       "MoveDeveloperConsoleHistory requires a non-zero history delta.");
            }
            if (!m_developerConsole.IsAvailableUVE()) {
                return MakeResponseUVE(request, false, "bridge.developer_console.unavailable",
                                       "Developer-console history is disabled by the native shipping build policy.");
            }
            applied = m_developerConsole.MoveHistoryUVE(*request.developerConsoleHistoryDelta);
            code = applied ? "bridge.command.applied" : "bridge.developer_console.history.noop";
            message = applied ? "The native developer-console history cursor moved."
                              : "The native developer-console history cursor did not move.";
            break;
        case EditorBridgeRequestKindUVE::ReadDeveloperConsole:
            code = "bridge.developer_console.snapshot.read";
            message = "The bounded developer-console snapshot was copied.";
            break;
        case EditorBridgeRequestKindUVE::ReadScriptRuntime:
            code = "bridge.script_runtime.snapshot.read";
            message = "The bounded ScriptRuntime snapshot was copied.";
            break;
        case EditorBridgeRequestKindUVE::ReadScriptRuntimeTickDiagnostics:
            code = "bridge.script_runtime.tick.completed";
            message = "The native ScriptRuntime diagnostic tick completed and its counters were copied.";
            break;
        case EditorBridgeRequestKindUVE::DispatchMotionQueryCommand: {
            if (!request.motionQueryCommand.has_value()) {
                return MakeResponseUVE(request, false, "bridge.motion_query.command.invalid",
                                       "DispatchMotionQueryCommand requires a value-only Motion Query command payload.");
            }
            const Plugins::Editor::MotionQueryEditorResponseUVE commandResponse =
                m_motionQueryAuthoring.DispatchUVE(*request.motionQueryCommand);
            applied = commandResponse.applied;
            code = applied ? "bridge.motion_query.command.applied" : "bridge.motion_query.command.rejected";
            message = commandResponse.message;
            if (applied) {
                ++m_revision;
            }
            break;
        }
        case EditorBridgeRequestKindUVE::DispatchMotionQueryDebugCommand: {
            if (!request.motionQueryDebugCommand.has_value()) {
                return MakeResponseUVE(request, false, "bridge.motion_query.debug.command.invalid",
                                       "DispatchMotionQueryDebugCommand requires a value-only debug command payload.");
            }
            const Plugins::Editor::MotionQueryLiveDebugResponseUVE debugResponse =
                m_motionQueryLiveDebugSession.DispatchUVE(*request.motionQueryDebugCommand, m_motionQueryAuthoring);
            applied = debugResponse.applied;
            code = applied ? "bridge.motion_query.debug.command.applied"
                           : "bridge.motion_query.debug.command.rejected";
            message = debugResponse.message;
            if (applied) {
                ++m_revision;
            }
            break;
        }
        case EditorBridgeRequestKindUVE::ReadMotionQuery:
            code = "bridge.motion_query.snapshot.read";
            message = "The copied Motion Query authoring, debugger, and trace snapshot was returned.";
            break;
        case EditorBridgeRequestKindUVE::SerializeVisualScriptGraph:
            code = "bridge.visual_scripting.graph_schema.serialized";
            message = "The native visual-scripting graph schema was copied in deterministic order.";
            break;
        case EditorBridgeRequestKindUVE::DeserializeVisualScriptGraph: {
            if (!request.visualScriptGraphSchema.has_value() ||
                request.visualScriptGraphSchema->size() > Scripting::ScriptGraphPersistenceLimitsUVE{}.maximumTextBytes) {
                return MakeResponseUVE(request, false, "bridge.visual_scripting.graph_schema.invalid",
                                       "DeserializeVisualScriptGraph requires a bounded graph schema JSON payload.");
            }
            const Scripting::ScriptGraphSchemaDecodeResultUVE decoded =
                Scripting::DecodeScriptGraphSchemaUVE(*request.visualScriptGraphSchema);
            if (!decoded.IsSuccessUVE()) {
                return MakeResponseUVE(request, false, "bridge.visual_scripting.graph_schema.invalid",
                                       "The graph schema was rejected before native canvas mutation.");
            }
            const Scripting::ScriptGraphCanvasCommandResultUVE appliedResult =
                m_visualScriptCanvas.ApplyGraphSchemaUVE(*decoded.schema, request.expectedRevision);
            applied = appliedResult.IsAppliedUVE();
            code = applied ? "bridge.visual_scripting.graph_schema.deserialized"
                           : "bridge.visual_scripting.graph_schema.rejected";
            message = applied ? "The graph schema was applied through native canvas history."
                              : appliedResult.message;
            if (applied) {
                responseSchema = decoded.schema;
            }
            break;
        }
        case EditorBridgeRequestKindUVE::ReadSnapshot:
            break;
    }

    SynchronizeRevisionUVE();
    EditorBridgeResponseUVE response = MakeResponseUVE(request, applied, std::move(code), std::move(message));
    response.createdEntity = createdEntity;
    response.visualScriptGraphSchema = std::move(responseSchema);
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
    observed.viewportSurface = EditorBridgeViewportSurfaceSnapshotUVE{
        EditorBridgeViewportSurfaceStateUVE::Unavailable, 0U, 0U, 0U, true, false,
        "No managed viewport surface transport is available in this headless bridge session."};
    observed.visualScripting = CaptureVisualScriptingUVE();
    observed.developerConsole = CaptureDeveloperConsoleUVE();
    observed.scriptRuntime = CaptureScriptRuntimeUVE();
    observed.dataTableCatalog = CaptureDataTableCatalogUVE();
    observed.dataTablePreview = CaptureDataTablePreviewUVE();
    return observed;
}

EditorBridgeScriptRuntimeSnapshotUVE EditorBridgeUVE::CaptureScriptRuntimeUVE() const {
    EditorBridgeScriptRuntimeSnapshotUVE snapshot{};
    if (m_scriptRuntime == nullptr) {
        snapshot.reason = "No native ScriptRuntime is attached to this bridge session.";
        return snapshot;
    }

    snapshot.available = true;
    snapshot.reason = "The native ScriptRuntime snapshot is available as copied read-only state.";
    const std::vector<Scripting::ScriptRuntimeInstanceSnapshotUVE> source = m_scriptRuntime->GetSnapshotUVE();
    snapshot.instanceCount = source.size();
    snapshot.entries.reserve(std::min(source.size(), kEditorBridgeMaximumPanelEntriesUVE));
    for (const Scripting::ScriptRuntimeInstanceSnapshotUVE& instance : source) {
        if (snapshot.entries.size() >= kEditorBridgeMaximumPanelEntriesUVE) {
            snapshot.entriesTruncated = true;
            break;
        }
        snapshot.entries.push_back(EditorBridgeScriptRuntimeInstanceEntryUVE{
            instance.entity.index,
            instance.entity.generation,
            instance.generation,
            instance.programVersion,
            instance.instructionCount,
            instance.stateValueCount,
            instance.enabled});
    }
    return snapshot;
}

EditorBridgeMotionQuerySnapshotUVE EditorBridgeUVE::CaptureMotionQueryUVE() const {
    const Plugins::Editor::MotionQueryEditorSnapshotUVE authoring = m_motionQueryAuthoring.GetSnapshotUVE();
    const Plugins::Editor::MotionQueryLiveDebugSnapshotUVE liveDebug =
        m_motionQueryLiveDebugSession.GetSnapshotUVE();
    const Plugins::Editor::MotionQueryDebuggerSnapshotUVE debugger = liveDebug.debugger;
    const Plugins::Editor::MotionQueryTraceSnapshotUVE trace{
        liveDebug.generation, liveDebug.traceTruncated, liveDebug.traceEvents};
    EditorBridgeMotionQuerySnapshotUVE snapshot{};
    snapshot.authoring.revision = authoring.revision;
    snapshot.authoring.selectedResource = authoring.selectedResource;
    snapshot.authoring.diagnostic = authoring.diagnostic;
    for (const auto& row : authoring.databases) {
        if (snapshot.authoring.databases.size() >= kEditorBridgeMaximumPanelEntriesUVE) {
            break;
        }
        snapshot.authoring.databases.push_back(row);
    }
    snapshot.debugger.attached = debugger.attached;
    snapshot.debugger.generation = debugger.generation;
    snapshot.debugger.database = debugger.database;
    snapshot.debugger.selectedCandidateIndex = debugger.selectedCandidateIndex;
    snapshot.debugger.candidateCount = debugger.candidateCount;
    snapshot.debugger.candidatesEvaluated = debugger.candidatesEvaluated;
    snapshot.debugger.selectedCost = debugger.selectedCost;
    snapshot.debugger.selectedCandidateId = debugger.selectedCandidateId;
    snapshot.debugger.selectedSourceClipId = debugger.selectedSourceClipId;
    snapshot.debugger.message = debugger.message;
    snapshot.trace.generation = trace.generation;
    snapshot.trace.truncated = trace.truncated || trace.events.size() > kEditorBridgeMaximumPanelEntriesUVE;
    const std::size_t firstEvent = trace.events.size() > kEditorBridgeMaximumPanelEntriesUVE
        ? trace.events.size() - kEditorBridgeMaximumPanelEntriesUVE : 0U;
    snapshot.trace.events.assign(trace.events.begin() + static_cast<std::ptrdiff_t>(firstEvent), trace.events.end());
    snapshot.liveDebugActive = liveDebug.active;
    snapshot.liveDebugGeneration = liveDebug.generation;
    snapshot.liveDebugDatabase = liveDebug.database;
    snapshot.liveDebugFilter = liveDebug.filter;
    snapshot.liveDebugTotalTraceEventCount = liveDebug.totalTraceEventCount;
    snapshot.liveDebugVisibleTraceEventCount = liveDebug.visibleTraceEventCount;
    snapshot.liveDebugTraceTruncated = liveDebug.traceTruncated;
    snapshot.liveDebugDiagnostic = liveDebug.diagnostic;
    return snapshot;
}

EditorBridgeDeveloperConsoleSnapshotUVE EditorBridgeUVE::CaptureDeveloperConsoleUVE() const {
    return EditorBridgeDeveloperConsoleSnapshotUVE{m_developerConsole.GetSnapshotUVE()};
}

EditorBridgeDataTableCatalogSnapshotUVE EditorBridgeUVE::CaptureDataTableCatalogUVE() const {
    const Asset::DataTableCatalogSnapshotUVE source = m_dataTableRegistry != nullptr
        ? m_dataTableRegistry->GetCatalogSnapshotUVE()
        : m_dataTableCatalogSnapshot;
    EditorBridgeDataTableCatalogSnapshotUVE snapshot{};
    snapshot.generation = source.generation;
    snapshot.entriesTruncated = source.entriesTruncated;
    snapshot.entries.reserve(source.entries.size());
    for (const Asset::DataTableCatalogEntryUVE& entry : source.entries) {
        if (snapshot.entries.size() >= kEditorBridgeMaximumPanelEntriesUVE) {
            snapshot.entriesTruncated = true;
            break;
        }
        snapshot.entries.push_back(EditorBridgeDataTableCatalogEntryUVE{
            BoundPresentationTextUVE(entry.name), entry.generation, entry.columnCount, entry.rowCount, entry.valid});
    }
    return snapshot;
}

EditorBridgeDataTablePreviewSnapshotUVE EditorBridgeUVE::CaptureDataTablePreviewUVE() const {
    Asset::DataTableSnapshotUVE source = m_dataTablePreviewSnapshot;
    if (m_dataTableRegistry != nullptr) {
        source = Asset::DataTableSnapshotUVE{};
        if (m_dataTablePreviewName.has_value()) {
            static_cast<void>(m_dataTableRegistry->TryGetSnapshotUVE(*m_dataTablePreviewName, source));
        }
    }

    EditorBridgeDataTablePreviewSnapshotUVE preview{};
    preview.generation = source.generation;
    preview.name = BoundPresentationTextUVE(source.name);
    preview.available = !preview.name.empty();
    preview.totalColumnCount = source.columns.size();
    preview.totalRowCount = source.rows.size();
    preview.reason = preview.available
        ? "Native table preview is available as copied read-only facts."
        : m_dataTableRegistry != nullptr && m_dataTablePreviewName.has_value()
            ? "The selected native data-table preview is no longer available."
            : "No native data-table preview is selected in this bridge session.";

    for (const Asset::DataTableColumnUVE& column : source.columns) {
        if (preview.columns.size() >= kEditorBridgeMaximumPanelEntriesUVE) {
            preview.columnsTruncated = true;
            break;
        }
        preview.columns.push_back(EditorBridgeDataTablePreviewColumnUVE{
            BoundPresentationTextUVE(column.name), column.type});
    }
    for (const Asset::DataTableRowUVE& row : source.rows) {
        if (preview.rows.size() >= kEditorBridgeMaximumPanelEntriesUVE) {
            preview.rowsTruncated = true;
            break;
        }
        EditorBridgeDataTablePreviewRowUVE copiedRow{};
        copiedRow.identifier = BoundPresentationTextUVE(row.identifier);
        const std::size_t visibleValueCount = std::min(row.values.size(), preview.columns.size());
        for (std::size_t index = 0U; index < visibleValueCount; ++index) {
            std::string displayValue = FormatDataTableValueUVE(row.values[index]);
            if (displayValue.size() > kEditorBridgeMaximumPresentationTextBytesUVE) {
                displayValue.resize(kEditorBridgeMaximumPresentationTextBytesUVE);
                preview.valuesTruncated = true;
            }
            copiedRow.values.push_back(std::move(displayValue));
        }
        if (row.values.size() != visibleValueCount) {
            preview.valuesTruncated = true;
        }
        preview.rows.push_back(std::move(copiedRow));
    }
    return preview;
}

EditorBridgeVisualScriptingSnapshotUVE EditorBridgeUVE::CaptureVisualScriptingUVE() const {
    const Scripting::ScriptGraphCanvasSnapshotUVE canvas = m_visualScriptCanvas.GetSnapshotUVE();
    const bool running = m_editor->GetStateUVE() == EditorStateUVE::Running;
    const bool canEdit = running && m_editor->GetPlayModeStateUVE() == EditorPlayModeStateUVE::Edit;
    const std::string reason = !running
        ? "The native visual-scripting canvas is unavailable before the editor session is running."
        : canEdit
            ? "The native visual-scripting canvas is available through the bounded bridge contract."
            : "The native visual-scripting canvas is read-only outside Edit mode.";
    EditorBridgeVisualScriptDebuggerSnapshotUVE debugger{};
    if (m_scriptDebugger != nullptr) {
        const Scripting::ScriptDebuggerSnapshotUVE source = m_scriptDebugger->GetSnapshotUVE();
        debugger = EditorBridgeVisualScriptDebuggerSnapshotUVE{
            true, source.state, source.instructionIndex, source.sourceNodeId, source.executedInstructions,
            source.pauseReason, source.breakpointNodeIds,
            "The native visual-scripting debugger snapshot is available as copied read-only state."};
    } else {
        debugger.reason = "No visual-scripting debugger is attached to this bridge session.";
    }
    return EditorBridgeVisualScriptingSnapshotUVE{
        running, canvas.graphRevision, canvas.nodes.size(), canvas.links.size(), canEdit, reason, canvas, debugger};
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
    snapshot.viewportSurface = observed.viewportSurface;
    snapshot.visualScripting = observed.visualScripting;
    snapshot.developerConsole = observed.developerConsole;
    snapshot.scriptRuntime = observed.scriptRuntime;
    snapshot.scriptRuntimeTickSummary = m_lastScriptRuntimeTickSummary;
    snapshot.scriptRuntimeTickHistoryTruncated = m_scriptRuntimeTickHistoryTruncated;
    snapshot.scriptRuntimeTickHistory.assign(m_scriptRuntimeTickHistory.begin(), m_scriptRuntimeTickHistory.end());
    snapshot.dataTableCatalog = observed.dataTableCatalog;
    snapshot.dataTablePreview = observed.dataTablePreview;
    snapshot.motionQuery = CaptureMotionQueryUVE();
    snapshot.capabilities = GetCapabilitiesUVE();
    return snapshot;
}

EditorBridgeResponseUVE EditorBridgeUVE::MakeResponseUVE(const EditorBridgeRequestUVE& request, const bool applied,
                                                           std::string code, std::string message) const {
    return EditorBridgeResponseUVE{kEditorBridgeProtocolVersionUVE, request.requestId, applied,
                                   std::move(code), std::move(message), BuildSnapshotUVE(), std::nullopt,
                                   std::nullopt};
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
