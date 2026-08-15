// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

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
/// <summary>
/// C++-authoritative visual-scripting presentation facts. The managed host receives counts and status
/// only; graph ownership, command execution, and runtime state remain native.
/// </summary>
public sealed record BridgeVisualScriptingSnapshot(
    bool IsAvailable,
    ulong GraphRevision,
    int NodeCount,
    int LinkCount,
    bool CanEdit,
    string Reason);

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
    IReadOnlyList<byte> Capabilities);

public sealed record BridgeCommand(
    ulong ExpectedRevision,
    string Kind,
    BridgeEntityRef? Entity = null,
    string? EntityName = null,
    string? HierarchyFilter = null,
    string? ContentDirectory = null,
    string? ContentFilter = null,
    string? ContentFocus = null,
    string? ContentEntryPath = null);

public sealed record BridgeCommandResult(
    bool Applied,
    string Code,
    string Message,
    BridgeEditorSnapshot Snapshot,
    BridgeEntityRef? CreatedEntity);

/// <summary>
/// Strict parser for the copied snapshot schema. Unknown additive fields are ignored for protocol
/// compatibility, but missing/wrong-typed known fields and native bound violations are deterministic
/// transport failures rather than partially rendered state.
/// </summary>
public static class BridgeSnapshotParser
{
    public const int MaximumPanelEntries = 128;
    public const int MaximumPresentationTextBytes = 256;
    public const int MaximumContentPathBytes = 4096;

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
                ParseCapabilities(RequiredArray(value, "capabilities")));
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

    private static BridgeVisualScriptingSnapshot ParseVisualScripting(JsonElement value)
    {
        RequireObject(value, "visualScripting");
        int nodeCount = RequiredInt32(value, "nodeCount");
        int linkCount = RequiredInt32(value, "linkCount");
        if (nodeCount < 0 || linkCount < 0)
        {
            throw Invalid("Visual-scripting graph counts must be non-negative.");
        }
        return new BridgeVisualScriptingSnapshot(
            RequiredBoolean(value, "available"),
            RequiredUInt64(value, "graphRevision"),
            nodeCount,
            linkCount,
            RequiredBoolean(value, "canEdit"),
            RequiredBoundedString(value, "reason"));
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
