// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

using System.Text;
using System.Text.Json;

namespace UniVex.EditorHost;

/// <summary>
/// Value-only generational entity identity copied from the native bridge. It carries no engine
/// pointer, lifetime ownership, or direct ECS behavior.
/// </summary>
public sealed record BridgeEntityRef(uint Index, uint Generation);

public sealed record BridgeEntitySnapshot(BridgeEntityRef Entity, string DisplayLabel);

public sealed record BridgeHierarchyEntry(
    BridgeEntityRef Entity,
    BridgeEntityRef? Parent,
    string DisplayLabel,
    string TypeTag,
    int Depth,
    int ChildCount,
    bool IsSelected,
    bool IsActive)
{
    public string DisplayText => $"{new string(' ', checked(Depth * 2))}{DisplayLabel}" +
                                 (string.IsNullOrEmpty(TypeTag) ? string.Empty : $" [{TypeTag}]");
}

public sealed record BridgeHierarchySnapshot(
    string Filter,
    bool IsFilterActive,
    bool IsTruncated,
    IReadOnlyList<BridgeHierarchyEntry> Entries);

public enum BridgeInspectorMode : byte
{
    NoSelection = 0,
    MultiSelection = 1,
    SingleSelection = 2,
}

public sealed record BridgeInspectorSnapshot(
    BridgeInspectorMode Mode,
    bool SelectedEntitiesTruncated,
    IReadOnlyList<BridgeEntitySnapshot> SelectedEntities,
    BridgeEntitySnapshot? ActiveEntity,
    BridgeEntitySnapshot? Parent,
    IReadOnlyList<BridgeEntitySnapshot> Ancestry,
    IReadOnlyList<string> EligibleDrawerIds,
    bool CanEditSelectedName);

public sealed record BridgeContentBrowserEntry(
    string RelativePath,
    bool IsDirectory,
    string TypeLabel,
    ulong? RegisteredAssetGuid)
{
    public string DisplayText => $"{RelativePath} [{TypeLabel}]" +
                                 (RegisteredAssetGuid.HasValue ? $" [Registered {RegisteredAssetGuid.Value:X16}]" : string.Empty);
}

public sealed record BridgeContentBrowserSnapshot(
    string ContentRoot,
    string CurrentDirectory,
    string Filter,
    string TypeFocus,
    IReadOnlyList<string> Breadcrumbs,
    ulong RefreshGeneration,
    int VisibleEntryCount,
    int DirectEntryCount,
    bool ContentRootExists,
    bool IsInitialized,
    bool LastRefreshSucceeded,
    bool IsTruncated,
    IReadOnlyList<BridgeContentBrowserEntry> Entries,
    BridgeContentBrowserEntry? SelectedEntry);

public enum BridgeViewportSurfaceState : byte
{
    Unavailable = 0,
    NativeOwned = 1,
    Detached = 2,
}

/// <summary>
/// C++-authoritative viewport surface lifecycle facts. It deliberately carries no native window,
/// OpenGL context, texture, or input-forwarding handle across the managed boundary.
/// </summary>
public sealed record BridgeMotionQueryResourceHandle(ulong Guid, ulong Generation);

public sealed record BridgeMotionQueryDatabaseRow(
    BridgeMotionQueryResourceHandle Resource,
    string DisplayName,
    string DatabaseId,
    ulong Generation,
    uint SchemaVersion,
    string SchemaId,
    int CandidateCount,
    int MaximumCandidates,
    bool IsValid,
    bool IsSelected,
    bool IsDirty);

public sealed record BridgeMotionQueryAuthoringSnapshot(
    ulong Revision,
    BridgeMotionQueryResourceHandle? SelectedResource,
    IReadOnlyList<BridgeMotionQueryDatabaseRow> Databases,
    string Diagnostic);

public sealed record BridgeMotionQueryDebuggerSnapshot(
    bool IsAttached,
    ulong Generation,
    BridgeMotionQueryResourceHandle? Database,
    ulong? SelectedCandidateIndex,
    int CandidateCount,
    int CandidatesEvaluated,
    float SelectedCost,
    string SelectedCandidateId,
    string SelectedSourceClipId,
    byte QualityTier,
    byte ContinuityCode,
    bool ContinuityApplied,
    byte TransitionCode,
    bool TransitionHeldPrevious,
    byte TelemetryCode,
    ulong TelemetryIndexEntryCount,
    ulong TelemetryCandidatesConsidered,
    bool TelemetryBudgetSaturated,
    string Provenance,
    string Message);

public sealed record BridgeMotionQueryTraceEvent(
    ulong Sequence,
    ulong TimestampNanoseconds,
    ulong FrameNumber,
    string Kind,
    BridgeMotionQueryResourceHandle? Database,
    int CandidatesConsidered,
    int CandidatesEvaluated,
    float Cost,
    ulong? SelectedCandidateIndex,
    byte QualityTier,
    byte ContinuityCode,
    bool ContinuityApplied,
    byte TransitionCode,
    bool TransitionHeldPrevious,
    byte TelemetryCode,
    ulong TelemetryIndexEntryCount,
    ulong TelemetryCandidatesConsidered,
    bool TelemetryBudgetSaturated,
    string Provenance,
    string Message);

public sealed record BridgeMotionQueryTraceSnapshot(
    ulong Generation,
    bool IsTruncated,
    IReadOnlyList<BridgeMotionQueryTraceEvent> Events);

public sealed record BridgeMotionQueryReplayComparison(
    bool Available,
    byte Code,
    byte ComparisonCode,
    ulong ComparedEventCount,
    ulong MismatchIndex,
    bool FixtureTruncated,
    bool SnapshotTruncated,
    uint MismatchFieldMask,
    string Message,
    string DiagnosticSummary,
    uint CompatibilityMismatchMask,
    string CompatibilityDiagnosticSummary);

public sealed record BridgeMotionQueryReplayBaselineEntry(
    string Name,
    ulong SourceGeneration,
    ulong EventCount,
    bool IsTruncated);

public sealed record BridgeMotionQueryReplayBaselineSnapshot(
    ulong Generation,
    bool IsTruncated,
    IReadOnlyList<BridgeMotionQueryReplayBaselineEntry> Entries);

public sealed record BridgeMotionQueryReplayComparisonHistoryEntry(
    ulong Sequence,
    string BaselineName,
    ulong RegistryGeneration,
    byte ComparisonCode,
    ulong ComparedEventCount,
    ulong MismatchIndex,
    uint MismatchFieldMask,
    string DiagnosticSummary);

public sealed record BridgeMotionQueryReplayWorkflowStatus(
    ulong RegistryGeneration,
    ulong BaselineCount,
    bool ActiveBaselineSelected,
    bool ActiveFixtureAvailable,
    bool HistoryTruncated,
    bool ReadyForComparison,
    string Diagnostic);

public sealed record BridgeMotionQueryReplayBatchEntry(
    string BaselineName,
    byte RegressionCode,
    byte ComparisonCode,
    ulong ComparedEventCount,
    ulong MismatchIndex,
    uint MismatchFieldMask,
    string DiagnosticSummary,
    uint CompatibilityMismatchMask,
    string CompatibilityDiagnosticSummary);

public sealed record BridgeMotionQueryReplayBatchSnapshot(
    bool Available,
    byte Code,
    ulong RegistryGeneration,
    ulong EvaluatedBaselineCount,
    ulong MatchCount,
    ulong MismatchCount,
    bool IsTruncated,
    string Message,
    IReadOnlyList<BridgeMotionQueryReplayBatchEntry> Results);

public sealed record BridgeMotionQueryReplayBatchHistoryEntry(
    ulong Sequence,
    ulong RegistryGeneration,
    byte Code,
    ulong EvaluatedBaselineCount,
    ulong MatchCount,
    ulong MismatchCount,
    string Message);

public sealed record BridgeMotionQueryReplaySessionFacts(
    ulong TotalIndividualComparisons,
    ulong TotalBatchRuns,
    ulong TotalBaselinesEvaluated,
    ulong TotalMatchesFound,
    ulong TotalMismatchesFound);

public sealed record BridgeMotionQueryReplayDiagnosticsView(
    bool HasActiveComparison,
    bool IsMatch,
    byte ComparisonCode,
    uint MismatchFieldMask,
    uint CompatibilityMismatchMask,
    bool HistoryTruncated,
    IReadOnlyList<BridgeMotionQueryReplayBaselineEntry> Baselines,
    IReadOnlyList<BridgeMotionQueryReplayComparisonHistoryEntry> History,
    IReadOnlyList<BridgeMotionQueryReplayBatchHistoryEntry> BatchHistory,
    BridgeMotionQueryReplaySessionFacts SessionFacts)
{
    public bool HasMismatch => HasActiveComparison && !IsMatch;
}

public sealed record BridgeMotionQuerySnapshot(
    BridgeMotionQueryAuthoringSnapshot Authoring,
    BridgeMotionQueryDebuggerSnapshot Debugger,
    BridgeMotionQueryTraceSnapshot Trace,
    bool LiveDebugActive,
    ulong LiveDebugGeneration,
    BridgeMotionQueryResourceHandle? LiveDebugDatabase,
    string LiveDebugFilter,
    int LiveDebugTotalTraceEventCount,
    int LiveDebugVisibleTraceEventCount,
    bool LiveDebugTraceTruncated,
    string LiveDebugDiagnostic,
    BridgeMotionQueryReplayComparison ReplayComparison,
    BridgeMotionQueryReplayBaselineSnapshot ReplayBaselines,
    bool ReplayComparisonHistoryTruncated,
    IReadOnlyList<BridgeMotionQueryReplayComparisonHistoryEntry> ReplayComparisonHistory,
    BridgeMotionQueryReplayWorkflowStatus ReplayWorkflow,
    BridgeMotionQueryReplayBatchSnapshot ReplayBatch,
    bool ReplayBatchHistoryTruncated,
    IReadOnlyList<BridgeMotionQueryReplayBatchHistoryEntry> ReplayBatchHistory,
    BridgeMotionQueryReplaySessionFacts ReplaySessionFacts)
{
    public BridgeMotionQueryReplayDiagnosticsView ReplayDiagnostics => new(
        ReplayComparison.Available,
        ReplayComparison.Available && ReplayComparison.ComparisonCode == 0,
        ReplayComparison.ComparisonCode,
        ReplayComparison.MismatchFieldMask,
        ReplayComparison.CompatibilityMismatchMask,
        ReplayComparisonHistoryTruncated || ReplayBatchHistoryTruncated,
        ReplayBaselines.Entries,
        ReplayComparisonHistory,
        ReplayBatchHistory,
        ReplaySessionFacts);
}

public sealed record BridgeViewportSurfaceSnapshot(
    BridgeViewportSurfaceState State,
    ulong Generation,
    uint Width,
    uint Height,
    bool NativeRendererOwnsSurface,
    bool ManagedAttachAllowed,
    string Reason)
{
    public bool IsPresentableToManagedHost => ManagedAttachAllowed && State == BridgeViewportSurfaceState.NativeOwned;
}

/// <summary>
/// Entire immutable state copied from a single C++ bridge response. Presentation code must replace
/// this value atomically after a response; it must never retain raw JsonElement or native objects.
/// </summary>
public sealed record BridgeVisualScriptPoint(float X, float Y);

public sealed record BridgeVisualScriptView(BridgeVisualScriptPoint Pan, float Zoom);

public sealed record BridgeVisualScriptPin(
    string Name,
    byte Direction,
    byte Type,
    byte Role = 1,
    string? DefaultValue = null);
public sealed record BridgeVisualScriptNode(
    uint Id,
    string TypeId,
    string DisplayName,
    BridgeVisualScriptPoint Position,
    bool IsSelected,
    IReadOnlyList<BridgeVisualScriptPin> Pins,
    string Category = "Uncategorized",
    string IconId = "node.default",
    uint DisplayOrder = 0,
    uint PresentationFlags = 0);

public sealed record BridgeVisualScriptEndpoint(uint NodeId, string PinName);

public sealed record BridgeVisualScriptLink(BridgeVisualScriptEndpoint Output, BridgeVisualScriptEndpoint Input);

public sealed record BridgeVisualScriptPaletteEntry(
    string TypeId,
    string DisplayName,
    string Category,
    string IconId,
    uint DisplayOrder,
    uint PresentationFlags,
    IReadOnlyList<BridgeVisualScriptPin> Pins);

public sealed record BridgeVisualScriptGraphNode(
    uint Id,
    string TypeId,
    BridgeVisualScriptPoint Position);

public sealed record BridgeVisualScriptGraphSchema(
    uint SchemaVersion,
    IReadOnlyList<BridgeVisualScriptGraphNode> Nodes,
    IReadOnlyList<BridgeVisualScriptLink> Links,
    IReadOnlyDictionary<string, string> Metadata)
{
    public const uint CurrentSchemaVersion = 1;
}

public sealed record BridgeVisualScriptDiagnostic(
    byte Code,
    uint NodeId,
    string PinName,
    string Message,
    BridgeVisualScriptEndpoint? RelatedEndpoint = null,
    byte Severity = 2,
    string SourceContext = "")
{
    public bool IsError => Severity >= 2;
    public string DisplayText => string.IsNullOrWhiteSpace(SourceContext)
        ? Message
        : $"{SourceContext}: {Message}";
}

public sealed record BridgeVisualScriptDebuggerSnapshot(
    bool Available,
    byte State,
    ulong InstructionIndex,
    uint SourceNodeId,
    ulong ExecutedInstructions,
    string PauseReason,
    IReadOnlyList<uint> BreakpointNodeIds,
    string Reason);

public sealed record BridgeVisualScriptCanvasSnapshot(
    ulong Revision,
    ulong GraphRevision,
    BridgeVisualScriptView View,
    bool NodesTruncated,
    bool LinksTruncated,
    bool PaletteTruncated,
    bool DiagnosticsTruncated,
    bool Dirty,
    bool CanUndo,
    bool CanRedo,
    IReadOnlyList<BridgeVisualScriptNode> Nodes,
    IReadOnlyList<BridgeVisualScriptLink> Links,
    IReadOnlyList<uint> SelectedNodeIds,
    IReadOnlyList<string> PaletteNodeTypeIds,
    IReadOnlyList<BridgeVisualScriptDiagnostic> Diagnostics)
{
    public IReadOnlyList<BridgeVisualScriptPaletteEntry> PaletteDescriptors { get; init; } =
        Array.Empty<BridgeVisualScriptPaletteEntry>();
}

/// <summary>
/// C++-authoritative visual-scripting presentation facts. The managed host receives copied DTOs only;
/// graph ownership, command execution, and runtime state remain native.
/// </summary>
public sealed record BridgeVisualScriptingSnapshot(
    bool IsAvailable,
    ulong GraphRevision,
    int NodeCount,
    int LinkCount,
    bool CanEdit,
    string Reason,
    BridgeVisualScriptCanvasSnapshot Canvas,
    BridgeVisualScriptDebuggerSnapshot Debugger);

public enum BridgeDeveloperConsoleSeverity : byte
{
    Info = 0,
    Warning = 1,
    Error = 2,
}

public sealed record BridgeDeveloperConsoleEntry(BridgeDeveloperConsoleSeverity Severity, string Text);

public sealed record BridgeDeveloperConsoleCVar(string Name, string Value, bool IsReadOnly);

public sealed record BridgeDeveloperConsoleCompletion(string Identifier, string Help);

public sealed record BridgeDeveloperConsoleSnapshot(
    ulong Generation,
    bool IsAvailable,
    bool IsDevelopmentOnly,
    byte SeverityFilter,
    int HistoryCursor,
    string HistoryEntry,
    bool OutputTruncated,
    bool HistoryTruncated,
    bool CVarsTruncated,
    bool CompletionTruncated,
    IReadOnlyList<BridgeDeveloperConsoleEntry> Output,
    IReadOnlyList<string> History,
    IReadOnlyList<BridgeDeveloperConsoleCVar> CVars,
    IReadOnlyList<BridgeDeveloperConsoleCompletion> Completions);

public sealed record BridgeScriptRuntimeInstanceEntry(
    uint EntityIndex,
    uint EntityGeneration,
    ulong Generation,
    uint ProgramVersion,
    int InstructionCount,
    int StateValueCount,
    bool Enabled)
{
    public string DisplayText =>
        $"Entity {EntityIndex}:{EntityGeneration} · generation {Generation} · program v{ProgramVersion} · " +
        $"{InstructionCount} instruction(s) · {StateValueCount} state value(s) · " +
        (Enabled ? "enabled" : "disabled");

    public bool MatchesFilter(string? filter) =>
        string.IsNullOrWhiteSpace(filter) || DisplayText.Contains(filter.Trim(), StringComparison.OrdinalIgnoreCase);
}

public sealed record BridgeScriptRuntimeSnapshot(
    bool IsAvailable,
    int InstanceCount,
    bool EntriesTruncated,
    string Reason,
    IReadOnlyList<BridgeScriptRuntimeInstanceEntry> Entries)
{
    public int VisibleEnabledInstanceCount => Entries.Count(entry => entry.Enabled);
    public int VisibleDisabledInstanceCount => Entries.Count - VisibleEnabledInstanceCount;

    public int CountMatchingEntries(string? filter) => Entries.Count(entry => entry.MatchesFilter(filter));
}

public sealed record BridgeScriptRuntimeTickSummary(
    bool IsAvailable,
    string Reason,
    int EnabledInstanceCount,
    int CompletedCount,
    int InstructionBudgetExceededCount,
    int InvalidInstructionCount,
    int DiagnosticCount);

public sealed record BridgeScriptRuntimeTickHistoryEntry(
    ulong Sequence,
    BridgeScriptRuntimeTickSummary Summary);

public sealed record BridgeDataTableCatalogEntry(
    string Name,
    ulong Generation,
    int ColumnCount,
    int RowCount,
    bool IsValid)
{
    public string DisplayText => $"{Name} — {RowCount} rows, {ColumnCount} columns" +
                                 (IsValid ? string.Empty : " [diagnostics]");
}

public sealed record BridgeDataTableCatalogSnapshot(
    ulong Generation,
    bool EntriesTruncated,
    IReadOnlyList<BridgeDataTableCatalogEntry> Entries);

public sealed record BridgeDataTablePreviewColumn(string Name, byte Type)
{
    public string DisplayText => $"{Name} [{TypeName(Type)}]";

    private static string TypeName(byte type) => type switch
    {
        0 => "Boolean",
        1 => "Integer",
        2 => "Number",
        3 => "String",
        _ => "Unknown",
    };
}

public sealed record BridgeDataTablePreviewRow(string Identifier, IReadOnlyList<string> Values)
{
    public string DisplayText => string.IsNullOrEmpty(Identifier)
        ? string.Join(" | ", Values)
        : $"{Identifier}: {string.Join(" | ", Values)}";
}

public sealed record BridgeDataTablePreviewSnapshot(
    bool IsAvailable,
    ulong Generation,
    string Name,
    int TotalColumnCount,
    int TotalRowCount,
    bool ColumnsTruncated,
    bool RowsTruncated,
    bool ValuesTruncated,
    string Reason,
    IReadOnlyList<BridgeDataTablePreviewColumn> Columns,
    IReadOnlyList<BridgeDataTablePreviewRow> Rows);

public sealed record BridgeEditorSnapshot(
    uint ProtocolVersion,
    ulong Revision,
    byte EditorState,
    byte PlayModeState,
    bool SceneDirty,
    bool CanUndo,
    bool CanRedo,
    string ActiveScenePath,
    IReadOnlyList<BridgeEntitySnapshot> SelectedEntities,
    bool SelectedEntitiesTruncated,
    BridgeEntityRef? ActiveEntity,
    BridgeHierarchySnapshot Hierarchy,
    BridgeInspectorSnapshot Inspector,
    BridgeContentBrowserSnapshot ContentBrowser,
    BridgeViewportSurfaceSnapshot ViewportSurface,
    BridgeVisualScriptingSnapshot VisualScripting,
    BridgeDeveloperConsoleSnapshot DeveloperConsole,
    BridgeScriptRuntimeSnapshot ScriptRuntime,
    BridgeScriptRuntimeTickSummary ScriptRuntimeTickSummary,
    bool ScriptRuntimeTickHistoryTruncated,
    IReadOnlyList<BridgeScriptRuntimeTickHistoryEntry> ScriptRuntimeTickHistory,
    BridgeDataTableCatalogSnapshot DataTableCatalog,
    BridgeDataTablePreviewSnapshot DataTablePreview,
    IReadOnlyList<byte> Capabilities)
{
    public BridgeMotionQuerySnapshot MotionQuery { get; init; } = BridgeSnapshotParser.EmptyMotionQuery();
}

public sealed record BridgeCommand(
    ulong ExpectedRevision,
    string Kind,
    BridgeEntityRef? Entity = null,
    string? EntityName = null,
    string? HierarchyFilter = null,
    string? ContentDirectory = null,
    string? ContentFilter = null,
    string? ContentFocus = null,
    string? ContentEntryPath = null,
    uint? VisualScriptNodeId = null,
    BridgeVisualScriptNode? VisualScriptNode = null,
    string? VisualScriptNodeTypeId = null,
    BridgeVisualScriptPoint? VisualScriptPosition = null,
    BridgeVisualScriptLink? VisualScriptLink = null,
    IReadOnlyList<uint>? VisualScriptSelection = null,
    BridgeVisualScriptView? VisualScriptView = null,
    string? VisualScriptGraphSchema = null,
    string? DataTableName = null,
    string? DeveloperConsoleCommand = null,
    byte? DeveloperConsoleSeverityFilter = null,
    string? DeveloperConsoleCompletionPrefix = null,
    int? DeveloperConsoleHistoryDelta = null,
    string? VisualScriptPinName = null,
    string? VisualScriptDefaultValue = null)
{
    public string? MotionQueryCommandKind { get; init; }
    public ulong? MotionQueryCommandExpectedRevision { get; init; }
    public BridgeMotionQueryResourceHandle? MotionQueryResource { get; init; }
    public string? MotionQueryText { get; init; }
    public ulong? MotionQueryCandidateIndex { get; init; }
    public string? MotionQueryDebugCommandKind { get; init; }
    public ulong? MotionQueryDebugExpectedGeneration { get; init; }
    public BridgeMotionQueryResourceHandle? MotionQueryDebugDatabase { get; init; }
    public string? MotionQueryDebugFilter { get; init; }
    public ulong? MotionQueryDebugEventSequence { get; init; }
    public string? MotionQueryReplayBaselineName { get; init; }
    public string? MotionQueryReplayFixturePayload { get; init; }
    public string? MotionQueryReplayBaselineEnvelopePayload { get; init; }
    public string? MotionQueryReplayBaselineNewName { get; init; }
}

public sealed record BridgeCommandResult(
    bool Applied,
    string Code,
    string Message,
    BridgeEditorSnapshot Snapshot,
    BridgeEntityRef? CreatedEntity,
    BridgeVisualScriptGraphSchema? VisualScriptGraphSchema = null,
    string? MotionQueryReplayBaselineEnvelopePayload = null);

/// <summary>
/// Strict parser for the copied snapshot schema. Unknown additive fields are ignored for protocol
/// compatibility, but missing/wrong-typed known fields and native bound violations are deterministic
/// transport failures rather than partially rendered state.
/// </summary>
public static class BridgeSnapshotParser
{
    public const int MaximumPanelEntries = 128;
    public const int MaximumScriptRuntimeTickHistory = 8;
    public const int MaximumPresentationTextBytes = 256;
    // Capability IDs are append-only protocol values; 76 is ReadScriptRuntime and 77 is the
    // explicit copied ScriptRuntime tick-diagnostics request.
    public const byte ReadScriptRuntimeCapability = 76;
    public const byte ReadScriptRuntimeTickDiagnosticsCapability = 77;
    public const int MaximumContentPathBytes = 4096;
    public const int MaximumGraphNodes = 4096;
    public const int MaximumGraphLinks = 8192;
    public const int MaximumGraphMetadataEntries = 128;
    public const int MaximumGraphStringBytes = 4096;

    public static BridgeEditorSnapshot Parse(JsonElement value)
    {
        try
        {
            RequireObject(value, "snapshot");
            uint protocolVersion = RequiredUInt32(value, "protocolVersion");
            if (protocolVersion != BridgeProtocolClient.ProtocolVersion)
            {
                throw Invalid("The copied snapshot protocol version is not supported by this host.");
            }

            return new BridgeEditorSnapshot(
                protocolVersion,
                RequiredUInt64(value, "revision"),
                RequiredByte(value, "editorState"),
                RequiredByte(value, "playModeState"),
                RequiredBoolean(value, "sceneDirty"),
                RequiredBoolean(value, "canUndo"),
                RequiredBoolean(value, "canRedo"),
                RequiredBoundedString(value, "activeScenePath"),
                ParseEntitySnapshots(RequiredArray(value, "selectedEntities"), "selectedEntities"),
                RequiredBoolean(value, "selectedEntitiesTruncated"),
                ParseNullableEntityRef(value.GetProperty("activeEntity"), "activeEntity"),
                ParseHierarchy(RequiredObjectMember(value, "hierarchy")),
                ParseInspector(RequiredObjectMember(value, "inspector")),
                ParseContentBrowser(RequiredObjectMember(value, "contentBrowser")),
                ParseViewportSurface(RequiredObjectMember(value, "viewportSurface")),
                ParseVisualScripting(RequiredObjectMember(value, "visualScripting")),
                value.TryGetProperty("developerConsole", out JsonElement consoleValue) && consoleValue.ValueKind != JsonValueKind.Null
                    ? ParseDeveloperConsole(consoleValue)
                    : EmptyDeveloperConsole(),
                value.TryGetProperty("scriptRuntime", out JsonElement scriptRuntimeValue) && scriptRuntimeValue.ValueKind != JsonValueKind.Null
                    ? ParseScriptRuntime(scriptRuntimeValue)
                    : EmptyScriptRuntime(),
                value.TryGetProperty("scriptRuntimeTickSummary", out JsonElement tickSummaryValue) && tickSummaryValue.ValueKind != JsonValueKind.Null
                    ? ParseTickSummary(tickSummaryValue)
                    : EmptyTickSummary(),
                OptionalBoolean(value, "scriptRuntimeTickHistoryTruncated", false),
                value.TryGetProperty("scriptRuntimeTickHistory", out JsonElement tickHistoryValue) && tickHistoryValue.ValueKind != JsonValueKind.Null
                    ? ParseTickHistory(tickHistoryValue)
                    : Array.Empty<BridgeScriptRuntimeTickHistoryEntry>(),
                value.TryGetProperty("dataTableCatalog", out JsonElement catalogValue) && catalogValue.ValueKind != JsonValueKind.Null
                    ? ParseDataTableCatalog(catalogValue)
                    : EmptyDataTableCatalog(),
                value.TryGetProperty("dataTablePreview", out JsonElement previewValue) && previewValue.ValueKind != JsonValueKind.Null
                    ? ParseDataTablePreview(previewValue)
                    : EmptyDataTablePreview(),
                ParseCapabilities(RequiredArray(value, "capabilities")))
            {
                MotionQuery = value.TryGetProperty("motionQuery", out JsonElement motionQueryValue) &&
                              motionQueryValue.ValueKind != JsonValueKind.Null
                    ? ParseMotionQuery(motionQueryValue)
                    : EmptyMotionQuery(),
            };
        }
        catch (BridgeProtocolException)
        {
            throw;
        }
        catch (Exception exception) when (exception is JsonException or InvalidOperationException or OverflowException or KeyNotFoundException or FormatException)
        {
            throw Invalid($"The backend returned an invalid copied snapshot: {exception.Message}");
        }
    }

    public static BridgeVisualScriptGraphSchema ParseVisualScriptGraphSchema(JsonElement value)
    {
        try
        {
            RequireObject(value, "visualScripting.graphSchema");
            uint schemaVersion = RequiredUInt32(value, "schemaVersion");
            if (schemaVersion != BridgeVisualScriptGraphSchema.CurrentSchemaVersion)
            {
                throw Invalid("The visual-scripting graph schema version is not supported by this host.");
            }
            JsonElement nodes = RequiredArray(value, "nodes");
            JsonElement links = RequiredArray(value, "links");
            JsonElement layout = RequiredArray(value, "layout");
            if (nodes.GetArrayLength() > MaximumGraphNodes || links.GetArrayLength() > MaximumGraphLinks ||
                layout.GetArrayLength() > MaximumGraphNodes)
            {
                throw Invalid("Visual-scripting graph schema nodes, links, or layout exceed the supported bound.");
            }
            Dictionary<uint, BridgeVisualScriptPoint> parsedLayout = new(layout.GetArrayLength());
            foreach (JsonElement entry in layout.EnumerateArray())
            {
                RequireObject(entry, "visual-scripting graph schema layout entry");
                uint nodeId = RequiredUInt32(entry, "nodeId");
                float x = entry.GetProperty("x").GetSingle();
                float y = entry.GetProperty("y").GetSingle();
                if (nodeId == 0U || !float.IsFinite(x) || !float.IsFinite(y) ||
                    !parsedLayout.TryAdd(nodeId, new BridgeVisualScriptPoint(x, y)))
                {
                    throw Invalid("Visual-scripting graph schema layout must contain unique finite node positions.");
                }
            }
            List<BridgeVisualScriptGraphNode> parsedNodes = new(nodes.GetArrayLength());
            HashSet<uint> nodeIds = new();
            foreach (JsonElement node in nodes.EnumerateArray())
            {
                RequireObject(node, "visual-scripting graph schema node");
                uint nodeId = RequiredUInt32(node, "id");
                if (!nodeIds.Add(nodeId) || !parsedLayout.TryGetValue(nodeId, out BridgeVisualScriptPoint? position) ||
                    position is null)
                {
                    throw Invalid("Visual-scripting graph schema nodes must have unique IDs and layout positions.");
                }
                parsedNodes.Add(new BridgeVisualScriptGraphNode(
                    nodeId, RequiredBoundedString(node, "typeId"), position));
            }
            if (parsedLayout.Count != parsedNodes.Count)
            {
                throw Invalid("Visual-scripting graph schema layout must contain exactly one position per node.");
            }
            List<BridgeVisualScriptLink> parsedLinks = new(links.GetArrayLength());
            foreach (JsonElement link in links.EnumerateArray())
            {
                RequireObject(link, "visual-scripting graph schema link");
                JsonElement output = RequiredObjectMember(link, "output");
                JsonElement input = RequiredObjectMember(link, "input");
                parsedLinks.Add(new BridgeVisualScriptLink(
                    new BridgeVisualScriptEndpoint(RequiredUInt32(output, "nodeId"),
                                                   RequiredBoundedString(output, "pinName")),
                    new BridgeVisualScriptEndpoint(RequiredUInt32(input, "nodeId"),
                                                   RequiredBoundedString(input, "pinName"))));
            }
            Dictionary<string, string> metadata = new(StringComparer.Ordinal);
            if (value.TryGetProperty("metadata", out JsonElement metadataValue) &&
                metadataValue.ValueKind != JsonValueKind.Null)
            {
                RequireObject(metadataValue, "visual-scripting graph schema metadata");
                if (metadataValue.EnumerateObject().Count() > MaximumGraphMetadataEntries)
                {
                    throw Invalid("Visual-scripting graph schema metadata exceeds the supported bound.");
                }
                foreach (JsonProperty property in metadataValue.EnumerateObject())
                {
                    if (Encoding.UTF8.GetByteCount(property.Name) > MaximumGraphStringBytes ||
                        property.Value.ValueKind != JsonValueKind.String ||
                        Encoding.UTF8.GetByteCount(property.Value.GetString() ?? string.Empty) > MaximumGraphStringBytes)
                    {
                        throw Invalid("Visual-scripting graph schema metadata must contain bounded strings.");
                    }
                    metadata[property.Name] = property.Value.GetString() ?? string.Empty;
                }
            }
            return new BridgeVisualScriptGraphSchema(schemaVersion, parsedNodes, parsedLinks, metadata);
        }
        catch (BridgeProtocolException)
        {
            throw;
        }
        catch (Exception exception) when (exception is JsonException or InvalidOperationException or OverflowException or KeyNotFoundException or FormatException)
        {
            throw Invalid($"The backend returned an invalid visual-scripting graph schema: {exception.Message}");
        }
    }

    public static BridgeMotionQuerySnapshot EmptyMotionQuery() =>
        new(new BridgeMotionQueryAuthoringSnapshot(0UL, null, Array.Empty<BridgeMotionQueryDatabaseRow>(),
                "No native Motion Query authoring session is attached to this bridge frame."),
            new BridgeMotionQueryDebuggerSnapshot(false, 0UL, null, null, 0, 0, 0F, string.Empty, string.Empty,
                0, 0, false, 0, false, 0, 0UL, 0UL, false, string.Empty,
                "No native Motion Query debugger snapshot is attached to this bridge frame."),
            new BridgeMotionQueryTraceSnapshot(0UL, false, Array.Empty<BridgeMotionQueryTraceEvent>()),
            false, 0UL, null, string.Empty, 0, 0, false,
            "No native Motion Query live debug session is attached to this bridge frame.",
            new BridgeMotionQueryReplayComparison(false, 0, 0, 0UL, 0UL, false, false, 0U,
                                                   string.Empty, string.Empty, 0U, string.Empty),
            new BridgeMotionQueryReplayBaselineSnapshot(0UL, false,
                                                        Array.Empty<BridgeMotionQueryReplayBaselineEntry>()),
            false,
            Array.Empty<BridgeMotionQueryReplayComparisonHistoryEntry>(),
            new BridgeMotionQueryReplayWorkflowStatus(0UL, 0UL, false, false, false, false,
                                                      "no native replay workflow status is attached"),
            new BridgeMotionQueryReplayBatchSnapshot(false, 0, 0UL, 0UL, 0UL, 0UL, false,
                                                     "no native replay batch results are available",
                                                     Array.Empty<BridgeMotionQueryReplayBatchEntry>()),
            false,
            Array.Empty<BridgeMotionQueryReplayBatchHistoryEntry>(),
            new BridgeMotionQueryReplaySessionFacts(0UL, 0UL, 0UL, 0UL, 0UL));

    private static BridgeMotionQueryResourceHandle ParseMotionQueryResource(JsonElement value, string context)
    {
        RequireObject(value, context);
        ulong guid = RequiredUInt64(value, "guid");
        ulong generation = RequiredUInt64(value, "generation");
        if (guid == 0UL || generation == 0UL)
        {
            throw Invalid($"{context} must contain a non-zero resource handle.");
        }
        return new BridgeMotionQueryResourceHandle(guid, generation);
    }

    private static BridgeMotionQueryResourceHandle? ParseNullableMotionQueryResource(JsonElement value, string context) =>
        value.ValueKind == JsonValueKind.Null ? null : ParseMotionQueryResource(value, context);

    private static BridgeMotionQuerySnapshot ParseMotionQuery(JsonElement value)
    {
        RequireObject(value, "motionQuery");
        JsonElement authoring = RequiredObjectMember(value, "authoring");
        JsonElement databases = RequiredArray(authoring, "databases");
        EnsureBoundedArray(databases, "motionQuery.authoring.databases");
        List<BridgeMotionQueryDatabaseRow> rows = new(databases.GetArrayLength());
        foreach (JsonElement row in databases.EnumerateArray())
        {
            RequireObject(row, "motion-query authoring database row");
            int candidateCount = RequiredInt32(row, "candidateCount");
            int maximumCandidates = RequiredInt32(row, "maximumCandidates");
            if (candidateCount < 0 || maximumCandidates <= 0 || candidateCount > maximumCandidates)
            {
                throw Invalid("Motion Query authoring candidate counts are invalid.");
            }
            rows.Add(new BridgeMotionQueryDatabaseRow(
                ParseMotionQueryResource(RequiredObjectMember(row, "resource"), "motion-query database resource"),
                RequiredBoundedString(row, "displayName"), RequiredBoundedString(row, "databaseId"),
                RequiredUInt64(row, "generation"), RequiredUInt32(row, "schemaVersion"),
                RequiredBoundedString(row, "schemaId"), candidateCount, maximumCandidates,
                RequiredBoolean(row, "valid"), RequiredBoolean(row, "selected"), RequiredBoolean(row, "dirty")));
        }
        BridgeMotionQueryAuthoringSnapshot parsedAuthoring = new(
            RequiredUInt64(authoring, "revision"),
            ParseNullableMotionQueryResource(authoring.GetProperty("selectedResource"),
                                             "motion-query authoring selected resource"),
            rows, RequiredBoundedString(authoring, "diagnostic"));

        JsonElement debugger = RequiredObjectMember(value, "debugger");
        int candidateCountDebugger = RequiredInt32(debugger, "candidateCount");
        int candidatesEvaluated = RequiredInt32(debugger, "candidatesEvaluated");
        float selectedCost = debugger.GetProperty("selectedCost").GetSingle();
        if (candidateCountDebugger < 0 || candidatesEvaluated < 0 || candidatesEvaluated > candidateCountDebugger ||
            !float.IsFinite(selectedCost) || selectedCost < 0F)
        {
            throw Invalid("Motion Query debugger counters or selected cost are invalid.");
        }
        JsonElement selectedIndexValue = debugger.GetProperty("selectedCandidateIndex");
        ulong? selectedIndex = selectedIndexValue.ValueKind == JsonValueKind.Null
            ? null : selectedIndexValue.GetUInt64();
        byte debuggerQualityTier = OptionalByte(debugger, "qualityTier", 0);
        byte debuggerContinuityCode = OptionalByte(debugger, "continuityCode", 0);
        byte debuggerTransitionCode = OptionalByte(debugger, "transitionCode", 0);
        if (debuggerQualityTier > 2U || debuggerContinuityCode > 5U || debuggerTransitionCode > 5U)
        {
            throw Invalid("Motion Query debugger decision provenance contains an unsupported code.");
        }
        BridgeMotionQueryDebuggerSnapshot parsedDebugger = new(
            RequiredBoolean(debugger, "attached"), RequiredUInt64(debugger, "generation"),
            ParseNullableMotionQueryResource(debugger.GetProperty("database"), "motion-query debugger database"),
            selectedIndex, candidateCountDebugger, candidatesEvaluated, selectedCost,
            RequiredBoundedString(debugger, "selectedCandidateId"),
            RequiredBoundedString(debugger, "selectedSourceClipId"),
            OptionalByte(debugger, "qualityTier", 0), OptionalByte(debugger, "continuityCode", 0),
            OptionalBoolean(debugger, "continuityApplied", false), OptionalByte(debugger, "transitionCode", 0),
            OptionalBoolean(debugger, "transitionHeldPrevious", false),
            OptionalByte(debugger, "telemetryCode", 0),
            OptionalUInt64(debugger, "telemetryIndexEntryCount", 0UL),
            OptionalUInt64(debugger, "telemetryCandidatesConsidered", 0UL),
            OptionalBoolean(debugger, "telemetryBudgetSaturated", false),
            OptionalBoundedString(debugger, "provenance", string.Empty),
            RequiredBoundedString(debugger, "message"));

        JsonElement trace = RequiredObjectMember(value, "trace");
        JsonElement events = RequiredArray(trace, "events");
        EnsureBoundedArray(events, "motionQuery.trace.events");
        List<BridgeMotionQueryTraceEvent> parsedEvents = new(events.GetArrayLength());
        ulong previousSequence = 0UL;
        ulong previousTimestamp = 0UL;
        ulong previousFrame = 0UL;
        foreach (JsonElement eventValue in events.EnumerateArray())
        {
            RequireObject(eventValue, "motion-query trace event");
            ulong sequence = RequiredUInt64(eventValue, "sequence");
            ulong timestamp = RequiredUInt64(eventValue, "timestampNanoseconds");
            ulong frame = RequiredUInt64(eventValue, "frameNumber");
            int considered = RequiredInt32(eventValue, "candidatesConsidered");
            int evaluated = RequiredInt32(eventValue, "candidatesEvaluated");
            float cost = eventValue.GetProperty("cost").GetSingle();
            if (sequence == 0UL || sequence <= previousSequence || timestamp < previousTimestamp || frame < previousFrame ||
                considered < 0 || evaluated < 0 || evaluated > considered || !float.IsFinite(cost) || cost < 0F)
            {
                throw Invalid("Motion Query trace events must be monotonic and bounded.");
            }
            previousSequence = sequence;
            previousTimestamp = timestamp;
            previousFrame = frame;
            ulong? selectedCandidateIndex = OptionalNullableUInt64(eventValue, "selectedCandidateIndex");
            byte qualityTier = OptionalByte(eventValue, "qualityTier", 0);
            byte continuityCode = OptionalByte(eventValue, "continuityCode", 0);
            byte transitionCode = OptionalByte(eventValue, "transitionCode", 0);
            byte telemetryCode = OptionalByte(eventValue, "telemetryCode", 0);
            ulong telemetryIndexEntryCount = OptionalUInt64(eventValue, "telemetryIndexEntryCount", 0UL);
            ulong telemetryCandidatesConsidered = OptionalUInt64(eventValue, "telemetryCandidatesConsidered", 0UL);
            if (qualityTier > 2U || continuityCode > 5U || transitionCode > 5U || telemetryCode > 1U)
            {
                throw Invalid("Motion Query trace decision provenance contains an unsupported code.");
            }
            parsedEvents.Add(new BridgeMotionQueryTraceEvent(
                sequence, timestamp, frame, RequiredBoundedString(eventValue, "kind"),
                ParseNullableMotionQueryResource(eventValue.GetProperty("database"), "motion-query trace database"),
                considered, evaluated, cost, selectedCandidateIndex, qualityTier, continuityCode,
                OptionalBoolean(eventValue, "continuityApplied", false), transitionCode,
                OptionalBoolean(eventValue, "transitionHeldPrevious", false), telemetryCode,
                telemetryIndexEntryCount, telemetryCandidatesConsidered,
                OptionalBoolean(eventValue, "telemetryBudgetSaturated", false),
                OptionalBoundedString(eventValue, "provenance", string.Empty),
                RequiredBoundedString(eventValue, "message")));
        }
        BridgeMotionQueryReplayComparison replayComparison = new(
            false, 0, 0, 0UL, 0UL, false, false, 0U, string.Empty, string.Empty, 0U, string.Empty);
        if (value.TryGetProperty("replayComparison", out JsonElement replayValue))
        {
            RequireObject(replayValue, "motion-query replay comparison");
            bool replayAvailable = RequiredBoolean(replayValue, "available");
            byte replayCode = OptionalByte(replayValue, "code", 0);
            byte replayComparisonCode = OptionalByte(replayValue, "comparisonCode", 0);
            if (replayCode > 3U || replayComparisonCode > 6U)
            {
                throw Invalid("Motion Query replay comparison contains an unsupported result code.");
            }
            replayComparison = new BridgeMotionQueryReplayComparison(
                replayAvailable, replayCode, replayComparisonCode,
                OptionalUInt64(replayValue, "comparedEventCount", 0UL),
                OptionalUInt64(replayValue, "mismatchIndex", 0UL),
                OptionalBoolean(replayValue, "fixtureTruncated", false),
                OptionalBoolean(replayValue, "snapshotTruncated", false),
                OptionalUInt32(replayValue, "mismatchFieldMask", 0U),
                OptionalBoundedString(replayValue, "message", string.Empty),
                OptionalBoundedString(replayValue, "diagnosticSummary", string.Empty),
                OptionalUInt32(replayValue, "compatibilityMismatchMask", 0U),
                OptionalBoundedString(replayValue, "compatibilityDiagnosticSummary", string.Empty));
        }
        BridgeMotionQueryReplayBaselineSnapshot replayBaselines =
            new(0UL, false, Array.Empty<BridgeMotionQueryReplayBaselineEntry>());
        BridgeMotionQueryReplayWorkflowStatus replayWorkflow =
            new(0UL, 0UL, false, false, false, false,
                "no native replay workflow status is attached");
        if (value.TryGetProperty("replayBaselines", out JsonElement replayBaselinesValue))
        {
            RequireObject(replayBaselinesValue, "motion-query replay baselines");
            JsonElement entriesValue = replayBaselinesValue.GetProperty("entries");
            if (entriesValue.ValueKind != JsonValueKind.Array || entriesValue.GetArrayLength() > MaximumPanelEntries)
            {
                throw Invalid("Motion Query replay baseline entries are invalid or exceed the bounded panel limit.");
            }
            List<BridgeMotionQueryReplayBaselineEntry> entries = new();
            string previousName = string.Empty;
            foreach (JsonElement entryValue in entriesValue.EnumerateArray())
            {
                RequireObject(entryValue, "motion-query replay baseline entry");
                string name = RequiredBoundedString(entryValue, "name");
                if (name.Length == 0 || (previousName.Length > 0 && string.CompareOrdinal(previousName, name) >= 0))
                {
                    throw Invalid("Motion Query replay baseline entries must be non-empty and strictly sorted.");
                }
                entries.Add(new BridgeMotionQueryReplayBaselineEntry(
                    name,
                    RequiredUInt64(entryValue, "sourceGeneration"),
                    RequiredUInt64(entryValue, "eventCount"),
                    RequiredBoolean(entryValue, "truncated")));
                previousName = name;
            }
            replayBaselines = new(
                RequiredUInt64(replayBaselinesValue, "generation"),
                RequiredBoolean(replayBaselinesValue, "truncated"), entries);
        }
        if (value.TryGetProperty("replayWorkflow", out JsonElement replayWorkflowValue))
        {
            RequireObject(replayWorkflowValue, "motion-query replay workflow status");
            replayWorkflow = new BridgeMotionQueryReplayWorkflowStatus(
                RequiredUInt64(replayWorkflowValue, "registryGeneration"),
                RequiredUInt64(replayWorkflowValue, "baselineCount"),
                RequiredBoolean(replayWorkflowValue, "activeBaselineSelected"),
                RequiredBoolean(replayWorkflowValue, "activeFixtureAvailable"),
                RequiredBoolean(replayWorkflowValue, "historyTruncated"),
                RequiredBoolean(replayWorkflowValue, "readyForComparison"),
                OptionalBoundedString(replayWorkflowValue, "diagnostic", string.Empty));
        }
        bool replayHistoryTruncated = OptionalBoolean(value, "replayComparisonHistoryTruncated", false);
        List<BridgeMotionQueryReplayComparisonHistoryEntry> replayHistory = new();
        if (value.TryGetProperty("replayComparisonHistory", out JsonElement replayHistoryValue))
        {
            if (replayHistoryValue.ValueKind != JsonValueKind.Array ||
                replayHistoryValue.GetArrayLength() > MaximumPanelEntries)
            {
                throw Invalid("Motion Query replay comparison history is invalid or exceeds the bounded panel limit.");
            }
            ulong previousHistorySequence = 0UL;
            foreach (JsonElement historyValue in replayHistoryValue.EnumerateArray())
            {
                RequireObject(historyValue, "motion-query replay comparison history entry");
                ulong sequence = RequiredUInt64(historyValue, "sequence");
                if (sequence == 0UL || sequence <= previousHistorySequence)
                {
                    throw Invalid("Motion Query replay comparison history sequences must be strictly increasing.");
                }
                byte comparisonCode = OptionalByte(historyValue, "comparisonCode", 0);
                if (comparisonCode > 6U)
                {
                    throw Invalid("Motion Query replay comparison history contains an unsupported result code.");
                }
                replayHistory.Add(new BridgeMotionQueryReplayComparisonHistoryEntry(
                    sequence,
                    OptionalBoundedString(historyValue, "baselineName", string.Empty),
                    RequiredUInt64(historyValue, "registryGeneration"),
                    comparisonCode,
                    RequiredUInt64(historyValue, "comparedEventCount"),
                    RequiredUInt64(historyValue, "mismatchIndex"),
                    OptionalUInt32(historyValue, "mismatchFieldMask", 0U),
                    OptionalBoundedString(historyValue, "diagnosticSummary", string.Empty)));
                previousHistorySequence = sequence;
            }
        }

        BridgeMotionQueryReplayBatchSnapshot replayBatch = new(false, 0, 0UL, 0UL, 0UL, 0UL, false,
                                                                "no native replay batch results are available",
                                                                Array.Empty<BridgeMotionQueryReplayBatchEntry>());
        if (value.TryGetProperty("replayBatch", out JsonElement replayBatchValue))
        {
            RequireObject(replayBatchValue, "motion-query replay batch");
            JsonElement resultsValue = RequiredArray(replayBatchValue, "results");
            EnsureBoundedArray(resultsValue, "motionQuery.replayBatch.results");
            List<BridgeMotionQueryReplayBatchEntry> results = new(resultsValue.GetArrayLength());
            foreach (JsonElement resultValue in resultsValue.EnumerateArray())
            {
                RequireObject(resultValue, "motion-query replay batch entry");
                results.Add(new BridgeMotionQueryReplayBatchEntry(
                    RequiredBoundedString(resultValue, "baselineName"),
                    RequiredByte(resultValue, "regressionCode"),
                    OptionalByte(resultValue, "comparisonCode", 0),
                    OptionalUInt64(resultValue, "comparedEventCount", 0UL),
                    OptionalUInt64(resultValue, "mismatchIndex", 0UL),
                    OptionalUInt32(resultValue, "mismatchFieldMask", 0U),
                    OptionalBoundedString(resultValue, "diagnosticSummary", string.Empty),
                    OptionalUInt32(resultValue, "compatibilityMismatchMask", 0U),
                    OptionalBoundedString(resultValue, "compatibilityDiagnosticSummary", string.Empty)));
            }
            replayBatch = new BridgeMotionQueryReplayBatchSnapshot(
                RequiredBoolean(replayBatchValue, "available"),
                RequiredByte(replayBatchValue, "code"),
                RequiredUInt64(replayBatchValue, "registryGeneration"),
                RequiredUInt64(replayBatchValue, "evaluatedBaselineCount"),
                RequiredUInt64(replayBatchValue, "matchCount"),
                RequiredUInt64(replayBatchValue, "mismatchCount"),
                RequiredBoolean(replayBatchValue, "truncated"),
                RequiredBoundedString(replayBatchValue, "message"),
                results);
        }

        bool replayBatchHistoryTruncated = OptionalBoolean(value, "replayBatchHistoryTruncated", false);
        List<BridgeMotionQueryReplayBatchHistoryEntry> replayBatchHistory = new();
        if (value.TryGetProperty("replayBatchHistory", out JsonElement replayBatchHistoryValue))
        {
            EnsureBoundedArray(replayBatchHistoryValue, "motionQuery.replayBatchHistory");
            ulong previousBatchSequence = 0UL;
            foreach (JsonElement historyValue in replayBatchHistoryValue.EnumerateArray())
            {
                RequireObject(historyValue, "motion-query replay batch history entry");
                ulong sequence = RequiredUInt64(historyValue, "sequence");
                if (sequence == 0UL || sequence <= previousBatchSequence)
                {
                    throw Invalid("Motion Query replay batch history sequences must be strictly increasing.");
                }
                replayBatchHistory.Add(new BridgeMotionQueryReplayBatchHistoryEntry(
                    sequence,
                    RequiredUInt64(historyValue, "registryGeneration"),
                    RequiredByte(historyValue, "code"),
                    RequiredUInt64(historyValue, "evaluatedBaselineCount"),
                    RequiredUInt64(historyValue, "matchCount"),
                    RequiredUInt64(historyValue, "mismatchCount"),
                    RequiredBoundedString(historyValue, "message")));
                previousBatchSequence = sequence;
            }
        }

        BridgeMotionQueryReplaySessionFacts replaySessionFacts = new(0UL, 0UL, 0UL, 0UL, 0UL);
        if (value.TryGetProperty("replaySessionFacts", out JsonElement sessionFactsValue))
        {
            RequireObject(sessionFactsValue, "motion-query replay session facts");
            replaySessionFacts = new BridgeMotionQueryReplaySessionFacts(
                RequiredUInt64(sessionFactsValue, "totalIndividualComparisons"),
                RequiredUInt64(sessionFactsValue, "totalBatchRuns"),
                RequiredUInt64(sessionFactsValue, "totalBaselinesEvaluated"),
                RequiredUInt64(sessionFactsValue, "totalMatchesFound"),
                RequiredUInt64(sessionFactsValue, "totalMismatchesFound"));
        }

        int liveTotal = OptionalInt32(value, "liveDebugTotalTraceEventCount", parsedEvents.Count);
        int liveVisible = OptionalInt32(value, "liveDebugVisibleTraceEventCount", parsedEvents.Count);
        if (liveTotal < 0 || liveVisible < 0 || liveVisible > liveTotal) {
            throw Invalid("Motion Query live-debug trace counts are invalid.");
        }
        return new BridgeMotionQuerySnapshot(
            parsedAuthoring, parsedDebugger,
            new BridgeMotionQueryTraceSnapshot(RequiredUInt64(trace, "generation"),
                                                RequiredBoolean(trace, "truncated"), parsedEvents),
            OptionalBoolean(value, "liveDebugActive", false),
            value.TryGetProperty("liveDebugGeneration", out JsonElement liveGeneration) &&
                    liveGeneration.ValueKind != JsonValueKind.Null
                ? RequiredUInt64(value, "liveDebugGeneration")
                : 0UL,
            value.TryGetProperty("liveDebugDatabase", out JsonElement liveDatabase) &&
                    liveDatabase.ValueKind != JsonValueKind.Null
                ? ParseNullableMotionQueryResource(liveDatabase, "motion-query live-debug database")
                : null,
            OptionalBoundedString(value, "liveDebugFilter", string.Empty),
            liveTotal,
            liveVisible,
            OptionalBoolean(value, "liveDebugTraceTruncated", false),
            OptionalBoundedString(value, "liveDebugDiagnostic",
                                  "No native Motion Query live debug session is attached to this bridge frame."),
            replayComparison,
            replayBaselines,
            replayHistoryTruncated,
            replayHistory,
            replayWorkflow,
            replayBatch,
            replayBatchHistoryTruncated,
            replayBatchHistory,
            replaySessionFacts);
    }

    private static BridgeVisualScriptingSnapshot ParseVisualScripting(JsonElement value)
    {
        RequireObject(value, "visualScripting");
        int nodeCount = RequiredInt32(value, "nodeCount");
        int linkCount = RequiredInt32(value, "linkCount");
        if (nodeCount < 0 || linkCount < 0)
        {
            throw Invalid("Visual-scripting graph counts must be non-negative.");
        }
        ulong graphRevision = RequiredUInt64(value, "graphRevision");
        BridgeVisualScriptCanvasSnapshot canvas = value.TryGetProperty("canvas", out JsonElement canvasValue) &&
                                                     canvasValue.ValueKind != JsonValueKind.Null
            ? ParseVisualScriptCanvas(canvasValue)
            : EmptyVisualScriptCanvas(graphRevision);
        BridgeVisualScriptDebuggerSnapshot debugger = value.TryGetProperty("debugger", out JsonElement debuggerValue) &&
                                                        debuggerValue.ValueKind != JsonValueKind.Null
            ? ParseVisualScriptDebugger(debuggerValue)
            : EmptyVisualScriptDebugger();
        return new BridgeVisualScriptingSnapshot(
            RequiredBoolean(value, "available"),
            graphRevision,
            nodeCount,
            linkCount,
            RequiredBoolean(value, "canEdit"),
            RequiredBoundedString(value, "reason"),
            canvas,
            debugger);
    }

    private static BridgeVisualScriptCanvasSnapshot EmptyVisualScriptCanvas(ulong graphRevision) =>
        new(0UL, graphRevision, new BridgeVisualScriptView(new BridgeVisualScriptPoint(0F, 0F), 1F),
            false, false, false, false, false, false, false,
            Array.Empty<BridgeVisualScriptNode>(), Array.Empty<BridgeVisualScriptLink>(),
            Array.Empty<uint>(), Array.Empty<string>(), Array.Empty<BridgeVisualScriptDiagnostic>());

    private static BridgeVisualScriptDebuggerSnapshot EmptyVisualScriptDebugger() =>
        new(false, 0, 0UL, 0U, 0UL, string.Empty, Array.Empty<uint>(),
            "No visual-scripting debugger is attached to this bridge session.");

    private static BridgeVisualScriptDebuggerSnapshot ParseVisualScriptDebugger(JsonElement value)
    {
        RequireObject(value, "visualScripting.debugger");
        JsonElement breakpoints = RequiredArray(value, "breakpointNodeIds");
        EnsureBoundedArray(breakpoints, "visualScripting.debugger.breakpointNodeIds");
        List<uint> parsedBreakpoints = new(breakpoints.GetArrayLength());
        foreach (JsonElement breakpoint in breakpoints.EnumerateArray())
        {
            parsedBreakpoints.Add(breakpoint.GetUInt32());
        }
        return new BridgeVisualScriptDebuggerSnapshot(
            RequiredBoolean(value, "available"),
            RequiredByte(value, "state"),
            RequiredUInt64(value, "instructionIndex"),
            RequiredUInt32(value, "sourceNodeId"),
            RequiredUInt64(value, "executedInstructions"),
            RequiredBoundedString(value, "pauseReason"),
            parsedBreakpoints,
            RequiredBoundedString(value, "reason"));
    }

    private static BridgeDeveloperConsoleSnapshot EmptyDeveloperConsole() =>
        new(0UL, false, true, 0, -1, string.Empty, false, false, false, false,
            Array.Empty<BridgeDeveloperConsoleEntry>(), Array.Empty<string>(), Array.Empty<BridgeDeveloperConsoleCVar>(),
            Array.Empty<BridgeDeveloperConsoleCompletion>());

    private static BridgeScriptRuntimeSnapshot EmptyScriptRuntime() =>
        new(false, 0, false, "No native ScriptRuntime is attached to this bridge session.",
            Array.Empty<BridgeScriptRuntimeInstanceEntry>());

    private static BridgeScriptRuntimeSnapshot ParseScriptRuntime(JsonElement value)
    {
        RequireObject(value, "scriptRuntime");
        JsonElement entries = RequiredArray(value, "entries");
        EnsureBoundedArray(entries, "scriptRuntime.entries");
        int instanceCount = RequiredInt32(value, "instanceCount");
        if (instanceCount < 0)
        {
            throw Invalid("ScriptRuntime instance count must be non-negative.");
        }

        List<BridgeScriptRuntimeInstanceEntry> parsed = new(entries.GetArrayLength());
        foreach (JsonElement entry in entries.EnumerateArray())
        {
            RequireObject(entry, "ScriptRuntime instance entry");
            int instructionCount = RequiredInt32(entry, "instructionCount");
            int stateValueCount = RequiredInt32(entry, "stateValueCount");
            if (instructionCount < 0 || stateValueCount < 0)
            {
                throw Invalid("ScriptRuntime instance counts must be non-negative.");
            }
            parsed.Add(new BridgeScriptRuntimeInstanceEntry(
                RequiredUInt32(entry, "entityIndex"),
                RequiredUInt32(entry, "entityGeneration"),
                RequiredUInt64(entry, "generation"),
                RequiredUInt32(entry, "programVersion"),
                instructionCount,
                stateValueCount,
                RequiredBoolean(entry, "enabled")));
        }

        bool available = RequiredBoolean(value, "available");
        string reason = OptionalBoundedString(value, "reason", available
            ? "The native ScriptRuntime snapshot is available as copied read-only state."
            : "No native ScriptRuntime is attached to this bridge session.");
        return new BridgeScriptRuntimeSnapshot(
            available, instanceCount, RequiredBoolean(value, "entriesTruncated"), reason, parsed);
    }

    private static BridgeScriptRuntimeTickSummary EmptyTickSummary() =>
        new(false, "No ScriptRuntime diagnostic tick has been requested.", 0, 0, 0, 0, 0);

    public static BridgeScriptRuntimeTickSummary ParseTickSummaryForResponse(JsonElement value) => ParseTickSummary(value);

    private static BridgeScriptRuntimeTickSummary ParseTickSummary(JsonElement value)
    {
        RequireObject(value, "scriptRuntimeTickSummary");
        int enabledInstanceCount = RequiredInt32(value, "enabledInstanceCount");
        int completedCount = RequiredInt32(value, "completedCount");
        int instructionBudgetExceededCount = RequiredInt32(value, "instructionBudgetExceededCount");
        int invalidInstructionCount = RequiredInt32(value, "invalidInstructionCount");
        int diagnosticCount = RequiredInt32(value, "diagnosticCount");
        if (enabledInstanceCount < 0 || completedCount < 0 || instructionBudgetExceededCount < 0 ||
            invalidInstructionCount < 0 || diagnosticCount < 0)
        {
            throw Invalid("ScriptRuntime diagnostic tick counts must be non-negative.");
        }
        return new BridgeScriptRuntimeTickSummary(
            RequiredBoolean(value, "available"), RequiredBoundedString(value, "reason"),
            enabledInstanceCount, completedCount, instructionBudgetExceededCount,
            invalidInstructionCount, diagnosticCount);
    }

    private static IReadOnlyList<BridgeScriptRuntimeTickHistoryEntry> ParseTickHistory(JsonElement value)
    {
        if (value.ValueKind != JsonValueKind.Array)
        {
            throw Invalid("The copied scriptRuntimeTickHistory value must be an array.");
        }
        if (value.GetArrayLength() > MaximumScriptRuntimeTickHistory)
        {
            throw Invalid("The copied ScriptRuntime tick history exceeds the supported bound.");
        }
        List<BridgeScriptRuntimeTickHistoryEntry> parsed = new(value.GetArrayLength());
        ulong previousSequence = 0UL;
        foreach (JsonElement entry in value.EnumerateArray())
        {
            RequireObject(entry, "ScriptRuntime tick history entry");
            ulong sequence = RequiredUInt64(entry, "sequence");
            if (sequence == 0UL || sequence <= previousSequence)
            {
                throw Invalid("ScriptRuntime tick history sequences must be strictly increasing and non-zero.");
            }
            previousSequence = sequence;
            parsed.Add(new BridgeScriptRuntimeTickHistoryEntry(
                sequence, ParseTickSummary(RequiredObjectMember(entry, "summary"))));
        }
        return parsed;
    }

    private static BridgeDataTableCatalogSnapshot EmptyDataTableCatalog() =>
        new(0UL, false, Array.Empty<BridgeDataTableCatalogEntry>());

    private static BridgeDataTableCatalogSnapshot ParseDataTableCatalog(JsonElement value)
    {
        RequireObject(value, "dataTableCatalog");
        JsonElement entries = RequiredArray(value, "entries");
        EnsureBoundedArray(entries, "dataTableCatalog.entries");
        List<BridgeDataTableCatalogEntry> parsed = new(entries.GetArrayLength());
        foreach (JsonElement entry in entries.EnumerateArray())
        {
            RequireObject(entry, "data-table catalog entry");
            int columnCount = RequiredInt32(entry, "columnCount");
            int rowCount = RequiredInt32(entry, "rowCount");
            if (columnCount < 0 || rowCount < 0)
            {
                throw Invalid("Data-table catalog counts must be non-negative.");
            }
            parsed.Add(new BridgeDataTableCatalogEntry(
                RequiredBoundedString(entry, "name"), RequiredUInt64(entry, "generation"),
                columnCount, rowCount, RequiredBoolean(entry, "valid")));
        }
        return new BridgeDataTableCatalogSnapshot(
            RequiredUInt64(value, "generation"), RequiredBoolean(value, "entriesTruncated"), parsed);
    }

    private static BridgeDataTablePreviewSnapshot EmptyDataTablePreview() =>
        new(false, 0UL, string.Empty, 0, 0, false, false, false,
            "No native data-table preview is available in this bridge session.",
            Array.Empty<BridgeDataTablePreviewColumn>(), Array.Empty<BridgeDataTablePreviewRow>());

    private static BridgeDataTablePreviewSnapshot ParseDataTablePreview(JsonElement value)
    {
        RequireObject(value, "dataTablePreview");
        JsonElement columns = RequiredArray(value, "columns");
        JsonElement rows = RequiredArray(value, "rows");
        EnsureBoundedArray(columns, "dataTablePreview.columns");
        EnsureBoundedArray(rows, "dataTablePreview.rows");
        int totalColumnCount = RequiredInt32(value, "totalColumnCount");
        int totalRowCount = RequiredInt32(value, "totalRowCount");
        if (totalColumnCount < 0 || totalRowCount < 0)
        {
            throw Invalid("Data-table preview counts must be non-negative.");
        }
        List<BridgeDataTablePreviewColumn> parsedColumns = new(columns.GetArrayLength());
        foreach (JsonElement column in columns.EnumerateArray())
        {
            RequireObject(column, "data-table preview column");
            byte type = RequiredByte(column, "type");
            if (type > 3)
            {
                throw Invalid("The backend returned an unsupported data-table preview column type.");
            }
            parsedColumns.Add(new BridgeDataTablePreviewColumn(RequiredBoundedString(column, "name"), type));
        }
        List<BridgeDataTablePreviewRow> parsedRows = new(rows.GetArrayLength());
        foreach (JsonElement row in rows.EnumerateArray())
        {
            RequireObject(row, "data-table preview row");
            JsonElement values = RequiredArray(row, "values");
            EnsureBoundedArray(values, "dataTablePreview.row.values");
            List<string> parsedValues = new(values.GetArrayLength());
            foreach (JsonElement cell in values.EnumerateArray())
            {
                parsedValues.Add(BoundedStringValue(cell, "data-table preview cell"));
            }
            parsedRows.Add(new BridgeDataTablePreviewRow(RequiredBoundedString(row, "identifier"), parsedValues));
        }
        return new BridgeDataTablePreviewSnapshot(
            RequiredBoolean(value, "available"), RequiredUInt64(value, "generation"),
            RequiredBoundedString(value, "name"), totalColumnCount, totalRowCount,
            RequiredBoolean(value, "columnsTruncated"), RequiredBoolean(value, "rowsTruncated"),
            RequiredBoolean(value, "valuesTruncated"), RequiredBoundedString(value, "reason"),
            parsedColumns, parsedRows);
    }

    private static BridgeDeveloperConsoleSnapshot ParseDeveloperConsole(JsonElement value)
    {
        RequireObject(value, "developerConsole");
        JsonElement output = RequiredArray(value, "output");
        JsonElement history = RequiredArray(value, "history");
        JsonElement cvars = RequiredArray(value, "cvars");
        EnsureBoundedArray(output, "developerConsole.output");
        EnsureBoundedArray(history, "developerConsole.history");
        EnsureBoundedArray(cvars, "developerConsole.cvars");
        byte severityFilter = OptionalByte(value, "severityFilter", 0);
        if (severityFilter > 3)
        {
            throw Invalid("The backend returned an unsupported developer-console severity filter.");
        }
        int historyCursor = OptionalInt32(value, "historyCursor", -1);
        if (historyCursor < -1 || historyCursor >= history.GetArrayLength())
        {
            throw Invalid("The backend returned an invalid developer-console history cursor.");
        }
        string historyEntry = OptionalBoundedString(value, "historyEntry", string.Empty);
        if (historyCursor >= 0 && historyEntry != (history[historyCursor].GetString() ?? string.Empty))
        {
            throw Invalid("The developer-console history cursor and entry disagree.");
        }
        JsonElement completions = OptionalArray(value, "completions");
        EnsureBoundedArray(completions, "developerConsole.completions");

        List<BridgeDeveloperConsoleEntry> parsedOutput = new(output.GetArrayLength());
        foreach (JsonElement entry in output.EnumerateArray())
        {
            RequireObject(entry, "developer-console output entry");
            byte severity = RequiredByte(entry, "severity");
            if (!Enum.IsDefined((BridgeDeveloperConsoleSeverity)severity))
            {
                throw Invalid("The backend returned an unsupported developer-console severity.");
            }
            parsedOutput.Add(new BridgeDeveloperConsoleEntry((BridgeDeveloperConsoleSeverity)severity,
                RequiredBoundedString(entry, "text")));
        }

        List<string> parsedHistory = new(history.GetArrayLength());
        foreach (JsonElement command in history.EnumerateArray())
        {
            parsedHistory.Add(BoundedStringValue(command, "developer-console history command"));
        }

        List<BridgeDeveloperConsoleCVar> parsedCVars = new(cvars.GetArrayLength());
        foreach (JsonElement cvar in cvars.EnumerateArray())
        {
            RequireObject(cvar, "developer-console cvar");
            parsedCVars.Add(new BridgeDeveloperConsoleCVar(
                RequiredBoundedString(cvar, "name"), RequiredBoundedString(cvar, "value"),
                RequiredBoolean(cvar, "readOnly")));
        }

        List<BridgeDeveloperConsoleCompletion> parsedCompletions = new(completions.GetArrayLength());
        foreach (JsonElement completion in completions.EnumerateArray())
        {
            RequireObject(completion, "developer-console completion");
            parsedCompletions.Add(new BridgeDeveloperConsoleCompletion(
                RequiredBoundedString(completion, "identifier"), RequiredBoundedString(completion, "help")));
        }

        return new BridgeDeveloperConsoleSnapshot(
            RequiredUInt64(value, "generation"), OptionalBoolean(value, "available", true),
            OptionalBoolean(value, "developmentOnly", true), severityFilter, historyCursor, historyEntry,
            RequiredBoolean(value, "outputTruncated"), RequiredBoolean(value, "historyTruncated"),
            RequiredBoolean(value, "cvarsTruncated"), OptionalBoolean(value, "completionTruncated", false),
            parsedOutput, parsedHistory, parsedCVars, parsedCompletions);
    }

    private static BridgeVisualScriptCanvasSnapshot ParseVisualScriptCanvas(JsonElement value)
    {
        RequireObject(value, "visualScripting.canvas");
        JsonElement nodes = RequiredArray(value, "nodes");
        JsonElement links = RequiredArray(value, "links");
        JsonElement selectedNodeIds = RequiredArray(value, "selectedNodeIds");
        JsonElement palette = RequiredArray(value, "paletteNodeTypeIds");
        JsonElement diagnostics = RequiredArray(value, "diagnostics");
        EnsureBoundedArray(nodes, "visualScripting.canvas.nodes");
        EnsureBoundedArray(links, "visualScripting.canvas.links");
        EnsureBoundedArray(selectedNodeIds, "visualScripting.canvas.selectedNodeIds");
        EnsureBoundedArray(palette, "visualScripting.canvas.paletteNodeTypeIds");
        EnsureBoundedArray(diagnostics, "visualScripting.canvas.diagnostics");

        JsonElement pan = RequiredObjectMember(value, "pan");
        float panX = pan.GetProperty("x").GetSingle();
        float panY = pan.GetProperty("y").GetSingle();
        float zoom = value.GetProperty("zoom").GetSingle();
        if (!float.IsFinite(panX) || !float.IsFinite(panY) || !float.IsFinite(zoom) || zoom < 0.1F || zoom > 8F)
        {
            throw Invalid("The visual-scripting canvas view is not finite or is outside the supported zoom range.");
        }

        List<BridgeVisualScriptNode> parsedNodes = new(nodes.GetArrayLength());
        foreach (JsonElement node in nodes.EnumerateArray())
        {
            RequireObject(node, "visual-scripting canvas node");
            JsonElement pins = RequiredArray(node, "pins");
            EnsureBoundedArray(pins, "visual-scripting canvas node pins");
            List<BridgeVisualScriptPin> parsedPins = new(pins.GetArrayLength());
            foreach (JsonElement pin in pins.EnumerateArray())
            {
                RequireObject(pin, "visual-scripting canvas pin");
                byte type = RequiredByte(pin, "type");
                byte role = OptionalByte(pin, "role", type == 0 ? (byte)0 : (byte)1);
                if (role > 1)
                {
                    throw Invalid("The visual-scripting canvas pin role is unsupported.");
                }
                string? defaultValue = pin.TryGetProperty("defaultValue", out JsonElement defaultValueValue) &&
                                       defaultValueValue.ValueKind != JsonValueKind.Null
                    ? RequiredBoundedString(pin, "defaultValue")
                    : null;
                parsedPins.Add(new BridgeVisualScriptPin(
                    RequiredBoundedString(pin, "name"), RequiredByte(pin, "direction"), type, role, defaultValue));
            }
            parsedNodes.Add(new BridgeVisualScriptNode(
                node.GetProperty("id").GetUInt32(),
                RequiredBoundedString(node, "typeId"),
                RequiredBoundedString(node, "displayName"),
                new BridgeVisualScriptPoint(node.GetProperty("x").GetSingle(), node.GetProperty("y").GetSingle()),
                RequiredBoolean(node, "selected"),
                parsedPins,
                OptionalBoundedString(node, "category", "Uncategorized"),
                OptionalBoundedString(node, "iconId", "node.default"),
                OptionalUInt32(node, "displayOrder", 0U),
                OptionalUInt32(node, "presentationFlags", 0U)));
        }

        List<BridgeVisualScriptLink> parsedLinks = new(links.GetArrayLength());
        foreach (JsonElement link in links.EnumerateArray())
        {
            RequireObject(link, "visual-scripting canvas link");
            JsonElement output = RequiredObjectMember(link, "output");
            JsonElement input = RequiredObjectMember(link, "input");
            parsedLinks.Add(new BridgeVisualScriptLink(
                new BridgeVisualScriptEndpoint(output.GetProperty("nodeId").GetUInt32(),
                                               RequiredBoundedString(output, "pinName")),
                new BridgeVisualScriptEndpoint(input.GetProperty("nodeId").GetUInt32(),
                                               RequiredBoundedString(input, "pinName"))));
        }

        List<uint> parsedSelection = new(selectedNodeIds.GetArrayLength());
        foreach (JsonElement nodeId in selectedNodeIds.EnumerateArray())
        {
            parsedSelection.Add(nodeId.GetUInt32());
        }
        List<string> parsedPalette = new(palette.GetArrayLength());
        foreach (JsonElement typeId in palette.EnumerateArray())
        {
            parsedPalette.Add(BoundedStringValue(typeId, "visual-scripting palette type ID"));
        }
        List<BridgeVisualScriptPaletteEntry> parsedPaletteDescriptors = new();
        if (value.TryGetProperty("paletteDescriptors", out JsonElement paletteDescriptors) &&
            paletteDescriptors.ValueKind != JsonValueKind.Null)
        {
            EnsureBoundedArray(paletteDescriptors, "visualScripting.canvas.paletteDescriptors");
            parsedPaletteDescriptors = new List<BridgeVisualScriptPaletteEntry>(paletteDescriptors.GetArrayLength());
            foreach (JsonElement descriptor in paletteDescriptors.EnumerateArray())
            {
                RequireObject(descriptor, "visual-scripting palette descriptor");
                JsonElement descriptorPins = RequiredArray(descriptor, "pins");
                EnsureBoundedArray(descriptorPins, "visual-scripting palette descriptor pins");
                List<BridgeVisualScriptPin> parsedDescriptorPins = new(descriptorPins.GetArrayLength());
                foreach (JsonElement pin in descriptorPins.EnumerateArray())
                {
                    RequireObject(pin, "visual-scripting palette descriptor pin");
                    byte type = RequiredByte(pin, "type");
                    byte role = OptionalByte(pin, "role", type == 0 ? (byte)0 : (byte)1);
                    if (role > 1)
                    {
                        throw Invalid("The visual-scripting palette descriptor pin role is unsupported.");
                    }
                    string? defaultValue = pin.TryGetProperty("defaultValue", out JsonElement defaultValueValue) &&
                                           defaultValueValue.ValueKind != JsonValueKind.Null
                        ? RequiredBoundedString(pin, "defaultValue")
                        : null;
                    parsedDescriptorPins.Add(new BridgeVisualScriptPin(
                        RequiredBoundedString(pin, "name"), RequiredByte(pin, "direction"), type, role, defaultValue));
                }
                parsedPaletteDescriptors.Add(new BridgeVisualScriptPaletteEntry(
                    RequiredBoundedString(descriptor, "typeId"),
                    RequiredBoundedString(descriptor, "displayName"),
                    OptionalBoundedString(descriptor, "category", "Uncategorized"),
                    OptionalBoundedString(descriptor, "iconId", "node.default"),
                    OptionalUInt32(descriptor, "displayOrder", 0U),
                    OptionalUInt32(descriptor, "presentationFlags", 0U),
                    parsedDescriptorPins));
            }
        }
        List<BridgeVisualScriptDiagnostic> parsedDiagnostics = new(diagnostics.GetArrayLength());
        foreach (JsonElement diagnostic in diagnostics.EnumerateArray())
        {
            RequireObject(diagnostic, "visual-scripting diagnostic");
            BridgeVisualScriptEndpoint? relatedEndpoint = null;
            if (diagnostic.TryGetProperty("relatedEndpoint", out JsonElement relatedValue) &&
                relatedValue.ValueKind != JsonValueKind.Null)
            {
                RequireObject(relatedValue, "visual-scripting diagnostic related endpoint");
                relatedEndpoint = new BridgeVisualScriptEndpoint(
                    relatedValue.GetProperty("nodeId").GetUInt32(),
                    RequiredBoundedString(relatedValue, "pinName"));
            }
            byte severity = OptionalByte(diagnostic, "severity", 2);
            if (severity > 2)
            {
                throw Invalid("The visual-scripting diagnostic severity is unsupported.");
            }
            parsedDiagnostics.Add(new BridgeVisualScriptDiagnostic(
                RequiredByte(diagnostic, "code"), diagnostic.GetProperty("nodeId").GetUInt32(),
                RequiredBoundedString(diagnostic, "pinName"), RequiredBoundedString(diagnostic, "message"),
                relatedEndpoint, severity, OptionalBoundedString(diagnostic, "sourceContext", string.Empty)));
        }

        return new BridgeVisualScriptCanvasSnapshot(
            RequiredUInt64(value, "revision"), RequiredUInt64(value, "graphRevision"),
            new BridgeVisualScriptView(new BridgeVisualScriptPoint(panX, panY), zoom),
            RequiredBoolean(value, "nodesTruncated"), RequiredBoolean(value, "linksTruncated"),
            RequiredBoolean(value, "paletteTruncated"), RequiredBoolean(value, "diagnosticsTruncated"),
            OptionalBoolean(value, "dirty", false), OptionalBoolean(value, "canUndo", false),
            OptionalBoolean(value, "canRedo", false),
            parsedNodes, parsedLinks, parsedSelection, parsedPalette, parsedDiagnostics)
        {
            PaletteDescriptors = parsedPaletteDescriptors,
        };
    }

    private static BridgeHierarchySnapshot ParseHierarchy(JsonElement value)
    {
        JsonElement entries = RequiredArray(value, "entries");
        EnsureBoundedArray(entries, "hierarchy.entries");
        List<BridgeHierarchyEntry> parsed = new(entries.GetArrayLength());
        foreach (JsonElement entry in entries.EnumerateArray())
        {
            RequireObject(entry, "hierarchy entry");
            int depth = RequiredInt32(entry, "depth");
            int childCount = RequiredInt32(entry, "childCount");
            if (depth < 0 || childCount < 0)
            {
                throw Invalid("Hierarchy depth and child count must be non-negative.");
            }
            parsed.Add(new BridgeHierarchyEntry(
                ParseEntityRef(RequiredObjectMember(entry, "entity"), "hierarchy.entity"),
                ParseNullableEntityRef(entry.GetProperty("parent"), "hierarchy.parent"),
                RequiredBoundedString(entry, "displayLabel"),
                RequiredBoundedString(entry, "typeTag"),
                depth,
                childCount,
                RequiredBoolean(entry, "selected"),
                RequiredBoolean(entry, "active")));
        }
        return new BridgeHierarchySnapshot(
            RequiredBoundedString(value, "filter"),
            RequiredBoolean(value, "filterActive"),
            RequiredBoolean(value, "truncated"),
            parsed);
    }

    private static BridgeInspectorSnapshot ParseInspector(JsonElement value)
    {
        byte rawMode = RequiredByte(value, "mode");
        if (!Enum.IsDefined((BridgeInspectorMode)rawMode))
        {
            throw Invalid("The backend returned an unsupported Inspector mode.");
        }

        JsonElement ancestry = RequiredArray(value, "ancestry");
        JsonElement drawerIds = RequiredArray(value, "eligibleDrawerIds");
        EnsureBoundedArray(ancestry, "inspector.ancestry");
        EnsureBoundedArray(drawerIds, "inspector.eligibleDrawerIds");
        List<BridgeEntitySnapshot> parsedAncestry = ParseEntitySnapshots(ancestry, "inspector.ancestry");
        List<string> parsedDrawerIds = new(drawerIds.GetArrayLength());
        foreach (JsonElement identifier in drawerIds.EnumerateArray())
        {
            parsedDrawerIds.Add(BoundedStringValue(identifier, "inspector drawer identifier"));
        }

        return new BridgeInspectorSnapshot(
            (BridgeInspectorMode)rawMode,
            RequiredBoolean(value, "selectedEntitiesTruncated"),
            ParseEntitySnapshots(RequiredArray(value, "selectedEntities"), "inspector.selectedEntities"),
            ParseNullableEntitySnapshot(value.GetProperty("activeEntity"), "inspector.activeEntity"),
            ParseNullableEntitySnapshot(value.GetProperty("parent"), "inspector.parent"),
            parsedAncestry,
            parsedDrawerIds,
            RequiredBoolean(value, "canEditSelectedName"));
    }

    private static BridgeViewportSurfaceSnapshot ParseViewportSurface(JsonElement value)
    {
        byte rawState = RequiredByte(value, "state");
        if (!Enum.IsDefined((BridgeViewportSurfaceState)rawState))
        {
            throw Invalid("The backend returned an unsupported viewport-surface state.");
        }

        bool managedAttachAllowed = RequiredBoolean(value, "managedAttachAllowed");
        if (managedAttachAllowed)
        {
            throw Invalid("Managed viewport surface attachment is not supported by this host contract.");
        }

        return new BridgeViewportSurfaceSnapshot(
            (BridgeViewportSurfaceState)rawState,
            RequiredUInt64(value, "generation"),
            RequiredUInt32(value, "width"),
            RequiredUInt32(value, "height"),
            RequiredBoolean(value, "nativeRendererOwnsSurface"),
            managedAttachAllowed,
            RequiredBoundedString(value, "reason"));
    }

    private static BridgeContentBrowserSnapshot ParseContentBrowser(JsonElement value)
    {
        JsonElement entries = RequiredArray(value, "entries");
        EnsureBoundedArray(entries, "contentBrowser.entries");
        List<BridgeContentBrowserEntry> parsed = new(entries.GetArrayLength());
        foreach (JsonElement entry in entries.EnumerateArray())
        {
            parsed.Add(ParseContentEntry(entry));
        }

        int visibleEntryCount = RequiredInt32(value, "visibleEntryCount");
        int directEntryCount = RequiredInt32(value, "directEntryCount");
        if (visibleEntryCount < 0 || directEntryCount < 0 || visibleEntryCount > directEntryCount)
        {
            throw Invalid("The content-browser entry counts are invalid.");
        }

        JsonElement breadcrumbs = RequiredArray(value, "breadcrumbs");
        EnsureBoundedArray(breadcrumbs, "contentBrowser.breadcrumbs");
        List<string> parsedBreadcrumbs = new(breadcrumbs.GetArrayLength());
        foreach (JsonElement breadcrumb in breadcrumbs.EnumerateArray())
        {
            parsedBreadcrumbs.Add(BoundedStringValue(breadcrumb, "content browser breadcrumb"));
        }

        return new BridgeContentBrowserSnapshot(
            RequiredBoundedString(value, "contentRoot"),
            RequiredBoundedContentPath(value, "currentDirectory"),
            RequiredBoundedString(value, "filter"),
            RequiredBoundedString(value, "typeFocus"),
            parsedBreadcrumbs,
            RequiredUInt64(value, "refreshGeneration"),
            visibleEntryCount,
            directEntryCount,
            RequiredBoolean(value, "contentRootExists"),
            RequiredBoolean(value, "initialized"),
            RequiredBoolean(value, "lastRefreshSucceeded"),
            RequiredBoolean(value, "truncated"),
            parsed,
            ParseNullableContentEntry(value.GetProperty("selectedEntry")));
    }

    private static List<BridgeEntitySnapshot> ParseEntitySnapshots(JsonElement array, string name)
    {
        EnsureBoundedArray(array, name);
        List<BridgeEntitySnapshot> parsed = new(array.GetArrayLength());
        foreach (JsonElement value in array.EnumerateArray())
        {
            parsed.Add(ParseEntitySnapshot(value, name));
        }
        return parsed;
    }

    private static BridgeEntitySnapshot ParseEntitySnapshot(JsonElement value, string name)
    {
        RequireObject(value, name);
        return new BridgeEntitySnapshot(
            ParseEntityRef(RequiredObjectMember(value, "entity"), $"{name}.entity"),
            RequiredBoundedString(value, "displayLabel"));
    }

    private static BridgeEntitySnapshot? ParseNullableEntitySnapshot(JsonElement value, string name)
    {
        return value.ValueKind == JsonValueKind.Null ? null : ParseEntitySnapshot(value, name);
    }

    private static BridgeEntityRef ParseEntityRef(JsonElement value, string name)
    {
        RequireObject(value, name);
        return new BridgeEntityRef(RequiredUInt32(value, "index"), RequiredUInt32(value, "generation"));
    }

    private static BridgeEntityRef? ParseNullableEntityRef(JsonElement value, string name)
    {
        return value.ValueKind == JsonValueKind.Null ? null : ParseEntityRef(value, name);
    }

    private static BridgeContentBrowserEntry ParseContentEntry(JsonElement value)
    {
        RequireObject(value, "content browser entry");
        ulong? registeredAssetGuid = value.GetProperty("registeredAssetGuid").ValueKind switch
        {
            JsonValueKind.Null => null,
            JsonValueKind.Number => value.GetProperty("registeredAssetGuid").GetUInt64(),
            _ => throw Invalid("The content browser registered asset GUID is invalid."),
        };
        return new BridgeContentBrowserEntry(
            RequiredBoundedContentPath(value, "relativePath"),
            RequiredBoolean(value, "isDirectory"),
            RequiredBoundedString(value, "typeLabel"),
            registeredAssetGuid);
    }

    private static BridgeContentBrowserEntry? ParseNullableContentEntry(JsonElement value)
    {
        return value.ValueKind == JsonValueKind.Null ? null : ParseContentEntry(value);
    }

    private static List<byte> ParseCapabilities(JsonElement array)
    {
        List<byte> capabilities = new(array.GetArrayLength());
        foreach (JsonElement capability in array.EnumerateArray())
        {
            capabilities.Add(capability.ValueKind == JsonValueKind.Number
                ? capability.GetByte()
                : throw Invalid("The copied capability list is invalid."));
        }
        return capabilities;
    }

    private static JsonElement RequiredObjectMember(JsonElement value, string name)
    {
        JsonElement result = value.GetProperty(name);
        RequireObject(result, name);
        return result;
    }

    private static JsonElement RequiredArray(JsonElement value, string name)
    {
        JsonElement result = value.GetProperty(name);
        if (result.ValueKind != JsonValueKind.Array)
        {
            throw Invalid($"The copied {name} value must be an array.");
        }
        return result;
    }

    private static JsonElement OptionalArray(JsonElement value, string name)
    {
        if (value.TryGetProperty(name, out JsonElement result))
        {
            if (result.ValueKind != JsonValueKind.Array)
            {
                throw Invalid($"The copied {name} value must be an array.");
            }
            return result;
        }
        using JsonDocument empty = JsonDocument.Parse("[]");
        return empty.RootElement.Clone();
    }

    private static byte OptionalByte(JsonElement value, string name, byte fallback) =>
        value.TryGetProperty(name, out JsonElement result) ? result.GetByte() : fallback;

    private static int OptionalInt32(JsonElement value, string name, int fallback) =>
        value.TryGetProperty(name, out JsonElement result) ? result.GetInt32() : fallback;

    private static uint OptionalUInt32(JsonElement value, string name, uint fallback) =>
        value.TryGetProperty(name, out JsonElement result) ? result.GetUInt32() : fallback;

    private static ulong OptionalUInt64(JsonElement value, string name, ulong fallback) =>
        value.TryGetProperty(name, out JsonElement result) ? result.GetUInt64() : fallback;

    private static ulong? OptionalNullableUInt64(JsonElement value, string name) =>
        !value.TryGetProperty(name, out JsonElement result) || result.ValueKind == JsonValueKind.Null
            ? null
            : result.GetUInt64();

    private static bool OptionalBoolean(JsonElement value, string name, bool fallback) =>
        value.TryGetProperty(name, out JsonElement result) ? result.GetBoolean() : fallback;

    private static string OptionalBoundedString(JsonElement value, string name, string fallback) =>
        value.TryGetProperty(name, out JsonElement result) ? BoundedStringValue(result, name) : fallback;

    private static uint RequiredUInt32(JsonElement value, string name) => value.GetProperty(name).GetUInt32();

    private static ulong RequiredUInt64(JsonElement value, string name) => value.GetProperty(name).GetUInt64();

    private static byte RequiredByte(JsonElement value, string name) => value.GetProperty(name).GetByte();

    private static int RequiredInt32(JsonElement value, string name) => value.GetProperty(name).GetInt32();

    private static bool RequiredBoolean(JsonElement value, string name) => value.GetProperty(name).GetBoolean();

    private static string RequiredBoundedString(JsonElement value, string name) =>
        BoundedStringValue(value.GetProperty(name), name, MaximumPresentationTextBytes);

    private static string RequiredBoundedContentPath(JsonElement value, string name) =>
        BoundedStringValue(value.GetProperty(name), name, MaximumContentPathBytes);

    private static string BoundedStringValue(JsonElement value, string name, int maximumLength = MaximumPresentationTextBytes)
    {
        if (value.ValueKind != JsonValueKind.String)
        {
            throw Invalid($"The copied {name} value must be a string.");
        }
        string result = value.GetString() ?? string.Empty;
        if (result.Length > maximumLength)
        {
            throw Invalid($"The copied {name} value exceeds the supported bound.");
        }
        return result;
    }

    private static void RequireObject(JsonElement value, string name)
    {
        if (value.ValueKind != JsonValueKind.Object)
        {
            throw Invalid($"The copied {name} value must be an object.");
        }
    }

    private static void EnsureBoundedArray(JsonElement array, string name)
    {
        if (array.GetArrayLength() > MaximumPanelEntries)
        {
            throw Invalid($"The copied {name} array exceeds the supported presentation bound.");
        }
    }

    private static BridgeProtocolException Invalid(string message) =>
        new("bridge.snapshot.invalid", message);
}
