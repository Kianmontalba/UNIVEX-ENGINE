// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

using System.Buffers.Binary;
using System.Text.Json;

namespace UniVex.EditorHost.Tests;

public sealed class BridgeProtocolClientTests
{
    [Fact]
    public async Task HelloAsync_WritesVersionedRequestAndReadsCompatibleCopiedSnapshot()
    {
        await using MemoryStream input = BuildFrames(new
        {
            jsonrpc = "2.0",
            id = 1,
            result = new
            {
                code = "bridge.hello.compatible",
                compatible = true,
                protocolVersion = BridgeProtocolClient.ProtocolVersion,
                snapshot = Snapshot(sceneDirty: false),
            },
        });
        await using MemoryStream output = new();
        await using BridgeProtocolClient client = new(input, output);

        BridgeHelloResult result = await client.HelloAsync(CancellationToken.None);

        Assert.True(result.Compatible);
        Assert.Equal(BridgeProtocolClient.ProtocolVersion, result.BackendProtocolVersion);
        Assert.Equal("bridge.hello.compatible", result.Code);
        Assert.False(result.Snapshot.SceneDirty);
        Assert.Empty(result.Snapshot.Hierarchy.Entries);
        Assert.Equal(BridgeInspectorMode.NoSelection, result.Snapshot.Inspector.Mode);
        Assert.Empty(result.Snapshot.ContentBrowser.Entries);

        output.Position = 0;
        using JsonDocument request = JsonDocument.Parse(await ReadFrameAsync(output));
        Assert.Equal("2.0", request.RootElement.GetProperty("jsonrpc").GetString());
        Assert.Equal("bridge.hello", request.RootElement.GetProperty("method").GetString());
        Assert.Equal(BridgeProtocolClient.ProtocolVersion,
            request.RootElement.GetProperty("params").GetProperty("protocolVersion").GetUInt32());
    }

    [Fact]
    public async Task HelloAsync_RejectsMismatchedResponseIdentifier()
    {
        await using MemoryStream input = BuildFrames(new
        {
            jsonrpc = "2.0",
            id = 99,
            result = new
            {
                code = "bridge.hello.compatible",
                compatible = true,
                protocolVersion = BridgeProtocolClient.ProtocolVersion,
                snapshot = Snapshot(sceneDirty: false),
            },
        });
        await using MemoryStream output = new();
        await using BridgeProtocolClient client = new(input, output);

        BridgeProtocolException exception = await Assert.ThrowsAsync<BridgeProtocolException>(
            () => client.HelloAsync(CancellationToken.None));

        Assert.Equal("bridge.transport.response_id_mismatch", exception.Code);
    }

    [Fact]
    public async Task GetSnapshotAsync_RejectsEndOfStreamBeforeFrameCompletes()
    {
        await using MemoryStream input = new();
        await using MemoryStream output = new();
        await using BridgeProtocolClient client = new(input, output);

        BridgeProtocolException exception = await Assert.ThrowsAsync<BridgeProtocolException>(
            () => client.GetSnapshotAsync(CancellationToken.None));

        Assert.Equal("bridge.transport.eof", exception.Code);
    }

    [Theory]
    [InlineData(0U, "bridge.transport.frame.zero_length")]
    [InlineData((uint)BridgeProtocolClient.MaximumFrameBytes + 1U, "bridge.transport.frame.oversized")]
    public async Task GetSnapshotAsync_ClassifiesInvalidResponseFrameLength(uint length, string expectedCode)
    {
        await using MemoryStream input = BuildRawBytes(FrameHeader(length));
        await using MemoryStream output = new();
        await using BridgeProtocolClient client = new(input, output);

        BridgeProtocolException exception = await Assert.ThrowsAsync<BridgeProtocolException>(
            () => client.GetSnapshotAsync(CancellationToken.None));

        Assert.Equal(expectedCode, exception.Code);
    }

    [Theory]
    [InlineData(new byte[] { 0x00, 0x00 }, "bridge.transport.frame.truncated_header")]
    [InlineData(new byte[] { 0x00, 0x00, 0x00, 0x02, (byte)'{' }, "bridge.transport.frame.truncated_body")]
    public async Task GetSnapshotAsync_ClassifiesTruncatedResponseFrame(byte[] bytes, string expectedCode)
    {
        await using MemoryStream input = BuildRawBytes(bytes);
        await using MemoryStream output = new();
        await using BridgeProtocolClient client = new(input, output);

        BridgeProtocolException exception = await Assert.ThrowsAsync<BridgeProtocolException>(
            () => client.GetSnapshotAsync(CancellationToken.None));

        Assert.Equal(expectedCode, exception.Code);
    }

    [Fact]
    public async Task GetSnapshotAsync_ClassifiesInvalidJsonAndInvalidJsonRpcEnvelope()
    {
        await using MemoryStream invalidJsonInput = BuildRawBytes(FrameHeader(1U), new byte[] { (byte)'{' });
        await using MemoryStream invalidJsonOutput = new();
        await using BridgeProtocolClient invalidJsonClient = new(invalidJsonInput, invalidJsonOutput);

        BridgeProtocolException invalidJson = await Assert.ThrowsAsync<BridgeProtocolException>(
            () => invalidJsonClient.GetSnapshotAsync(CancellationToken.None));
        Assert.Equal("bridge.transport.json.invalid", invalidJson.Code);

        await using MemoryStream invalidEnvelopeInput = BuildFrames(new { id = 1, result = new { } });
        await using MemoryStream invalidEnvelopeOutput = new();
        await using BridgeProtocolClient invalidEnvelopeClient = new(invalidEnvelopeInput, invalidEnvelopeOutput);

        BridgeProtocolException invalidEnvelope = await Assert.ThrowsAsync<BridgeProtocolException>(
            () => invalidEnvelopeClient.GetSnapshotAsync(CancellationToken.None));
        Assert.Equal("bridge.transport.response.invalid", invalidEnvelope.Code);
    }

    [Fact]
    public async Task DispatchAsync_WritesNamedPanelCommandAndReturnsTypedCopiedSnapshot()
    {
        await using MemoryStream input = BuildFrames(new
        {
            jsonrpc = "2.0",
            id = 1,
            result = new
            {
                protocolVersion = BridgeProtocolClient.ProtocolVersion,
                requestId = 1UL,
                applied = true,
                code = "bridge.command.applied",
                message = "The native hierarchy filter was updated.",
                snapshot = Snapshot(sceneDirty: false),
                createdEntity = (object?)null,
            },
        });
        await using MemoryStream output = new();
        await using BridgeProtocolClient client = new(input, output);

        BridgeCommandResult result = await client.DispatchAsync(
            new BridgeCommand(7UL, "setHierarchyFilter", HierarchyFilter: "camera"), CancellationToken.None);

        Assert.True(result.Applied);
        Assert.Equal("bridge.command.applied", result.Code);
        Assert.Equal(1UL, result.Snapshot.Revision);
        output.Position = 0;
        using JsonDocument request = JsonDocument.Parse(await ReadFrameAsync(output));
        JsonElement parameters = request.RootElement.GetProperty("params");
        Assert.Equal("bridge.dispatch", request.RootElement.GetProperty("method").GetString());
        Assert.Equal(7UL, parameters.GetProperty("expectedRevision").GetUInt64());
        Assert.Equal("setHierarchyFilter", parameters.GetProperty("kind").GetString());
        Assert.Equal("camera", parameters.GetProperty("hierarchyFilter").GetString());
    }

    [Fact]
    public void ReadScriptRuntimeCapability_UsesAppendOnlyProtocolId()
    {
        Assert.Equal((byte)36, BridgeSnapshotParser.ReadScriptRuntimeCapability);
    }

    [Fact]
    public async Task DispatchAsync_WritesReadScriptRuntimeRequest()
    {
        await using MemoryStream input = BuildFrames(new
        {
            jsonrpc = "2.0",
            id = 1,
            result = new
            {
                protocolVersion = BridgeProtocolClient.ProtocolVersion,
                requestId = 1UL,
                applied = true,
                code = "bridge.script_runtime.snapshot.read",
                message = "The bounded ScriptRuntime snapshot was copied.",
                snapshot = Snapshot(sceneDirty: false),
                createdEntity = (object?)null,
            },
        });
        await using MemoryStream output = new();
        await using BridgeProtocolClient client = new(input, output);

        BridgeCommandResult result = await client.DispatchAsync(
            new BridgeCommand(999UL, "readScriptRuntime"), CancellationToken.None);

        Assert.True(result.Applied);
        Assert.Equal("bridge.script_runtime.snapshot.read", result.Code);
        Assert.True(result.Snapshot.ScriptRuntime.IsAvailable);
        output.Position = 0;
        using JsonDocument request = JsonDocument.Parse(await ReadFrameAsync(output));
        JsonElement parameters = request.RootElement.GetProperty("params");
        Assert.Equal("readScriptRuntime", parameters.GetProperty("kind").GetString());
        Assert.Equal(999UL, parameters.GetProperty("expectedRevision").GetUInt64());
    }

    [Fact]
    public async Task DispatchAsync_WritesDataTablePreviewSelectionPayload()
    {
        await using MemoryStream input = BuildFrames(new
        {
            jsonrpc = "2.0",
            id = 1,
            result = new
            {
                protocolVersion = BridgeProtocolClient.ProtocolVersion,
                requestId = 1UL,
                applied = true,
                code = "bridge.command.applied",
                message = "The selected native data-table preview was updated.",
                snapshot = Snapshot(sceneDirty: false),
                createdEntity = (object?)null,
            },
        });
        await using MemoryStream output = new();
        await using BridgeProtocolClient client = new(input, output);

        BridgeCommandResult result = await client.DispatchAsync(
            new BridgeCommand(1UL, "selectDataTablePreview", DataTableName: "weapons"), CancellationToken.None);

        Assert.True(result.Applied);
        Assert.Equal("bridge.command.applied", result.Code);
        output.Position = 0;
        using JsonDocument request = JsonDocument.Parse(await ReadFrameAsync(output));
        JsonElement parameters = request.RootElement.GetProperty("params");
        Assert.Equal("selectDataTablePreview", parameters.GetProperty("kind").GetString());
        Assert.Equal("weapons", parameters.GetProperty("dataTableName").GetString());
    }

    [Fact]
    public void SnapshotParser_RejectsPanelRowsBeyondNativeBound()
    {
        object[] entries = Enumerable.Range(0, BridgeSnapshotParser.MaximumPanelEntries + 1)
            .Select(index => (object)new
            {
                entity = new { index, generation = 1U },
                parent = (object?)null,
                displayLabel = "Entity",
                typeTag = string.Empty,
                depth = 0,
                childCount = 0,
                selected = false,
                active = false,
            }).ToArray();
        using JsonDocument document = JsonDocument.Parse(JsonSerializer.Serialize(new
        {
            protocolVersion = BridgeProtocolClient.ProtocolVersion,
            revision = 1UL,
            editorState = 1,
            playModeState = 0,
            sceneDirty = false,
            canUndo = false,
            canRedo = false,
            activeScenePath = "editor_scene.uvescene",
            selectedEntities = Array.Empty<object>(),
            selectedEntitiesTruncated = false,
            activeEntity = (object?)null,
            hierarchy = new { filter = string.Empty, filterActive = false, truncated = true, entries },
            inspector = new
            {
                mode = 0,
                selectedEntitiesTruncated = false,
                selectedEntities = Array.Empty<object>(),
                activeEntity = (object?)null,
                parent = (object?)null,
                ancestry = Array.Empty<object>(),
                eligibleDrawerIds = Array.Empty<string>(),
                canEditSelectedName = false,
            },
            contentBrowser = new
            {
                contentRoot = "assets",
                currentDirectory = string.Empty,
                filter = string.Empty,
                typeFocus = "All",
                breadcrumbs = Array.Empty<string>(),
                refreshGeneration = 0UL,
                visibleEntryCount = 0,
                directEntryCount = 0,
                contentRootExists = true,
                initialized = false,
                lastRefreshSucceeded = true,
                truncated = false,
                entries = Array.Empty<object>(),
                selectedEntry = (object?)null,
            },
            viewportSurface = new
            {
                state = 0,
                generation = 0UL,
                width = 0U,
                height = 0U,
                nativeRendererOwnsSurface = true,
                managedAttachAllowed = false,
                reason = "No managed viewport surface transport is available in this headless bridge session.",
            },
            visualScripting = new { available = false, graphRevision = 0UL, nodeCount = 0, linkCount = 0, canEdit = false, reason = "Native visual-scripting presentation is unavailable in this headless bridge session." },
            developerConsole = new
        {
            generation = 3UL,
            available = true,
            developmentOnly = true,
            severityFilter = 0,
            historyCursor = 0,
            historyEntry = "help",
            outputTruncated = false,
            historyTruncated = false,
            cvarsTruncated = false,
            completionTruncated = false,
            output = new[] { new { severity = 0, text = "ready" } },
            history = new[] { "help" },
            cvars = new[] { new { name = "r.vsync", value = "1", readOnly = true } },
            completions = new[] { new { identifier = "help", help = "List registered native commands." } },
        },
        scriptRuntime = new
        {
            available = true,
            instanceCount = 2,
            entriesTruncated = false,
            reason = "The native ScriptRuntime snapshot is available as copied read-only state.",
            entries = new[]
            {
                new
                {
                    entityIndex = 7U,
                    entityGeneration = 3U,
                    generation = 9UL,
                    programVersion = 4U,
                    instructionCount = 12,
                    stateValueCount = 2,
                    enabled = true,
                },
                new
                {
                    entityIndex = 8U,
                    entityGeneration = 1U,
                    generation = 10UL,
                    programVersion = 5U,
                    instructionCount = 3,
                    stateValueCount = 0,
                    enabled = false,
                },
            },
        },
        dataTableCatalog = new
        {
            generation = 4UL,
            entriesTruncated = false,
            entries = new[] { new { name = "weapons", generation = 2UL, columnCount = 1, rowCount = 1, valid = true } },
        },
        dataTablePreview = new
        {
            available = true,
            generation = 2UL,
            name = "weapons",
            totalColumnCount = 1,
            totalRowCount = 1,
            columnsTruncated = false,
            rowsTruncated = false,
            valuesTruncated = false,
            reason = "Native table preview is available as copied read-only facts.",
            columns = new[] { new { name = "damage", type = 1 } },
            rows = new[] { new { identifier = "pistol", values = new[] { "25" } } },
        },
        capabilities = Array.Empty<int>(),
        }));

        BridgeProtocolException exception = Assert.Throws<BridgeProtocolException>(() =>
            BridgeSnapshotParser.Parse(document.RootElement));

        Assert.Equal("bridge.snapshot.invalid", exception.Code);
    }

    [Fact]
    public void SnapshotParser_AcceptsBoundedVisualScriptCanvasDto()
    {
        using JsonDocument document = JsonDocument.Parse(JsonSerializer.Serialize(Snapshot(sceneDirty: false, includeCanvas: true)));

        BridgeEditorSnapshot snapshot = BridgeSnapshotParser.Parse(document.RootElement);

        Assert.Equal(1UL, snapshot.VisualScripting.Canvas.Revision);
        Assert.Equal(1UL, snapshot.VisualScripting.Canvas.GraphRevision);
        Assert.Equal(1, snapshot.VisualScripting.Canvas.Nodes.Count);
        Assert.Equal(1U, snapshot.VisualScripting.Canvas.Nodes[0].Id);
        Assert.Equal("test.source", snapshot.VisualScripting.Canvas.Nodes[0].TypeId);
        Assert.Equal(1, snapshot.VisualScripting.Canvas.Nodes[0].Pins.Count);
        Assert.Equal("Out", snapshot.VisualScripting.Canvas.Nodes[0].Pins[0].Name);
        Assert.Equal(1, snapshot.VisualScripting.Canvas.Links.Count);
        Assert.Equal(new uint[] { 1U }, snapshot.VisualScripting.Canvas.SelectedNodeIds);
        Assert.Equal(1.5F, snapshot.VisualScripting.Canvas.View.Zoom);
        Assert.Single(snapshot.VisualScripting.Canvas.Diagnostics);
        Assert.True(snapshot.VisualScripting.Debugger.Available);
        Assert.Equal((byte)2, snapshot.VisualScripting.Debugger.State);
        Assert.Equal(1UL, snapshot.VisualScripting.Debugger.InstructionIndex);
        Assert.Equal(20U, snapshot.VisualScripting.Debugger.SourceNodeId);
        Assert.Equal("Breakpoint reached.", snapshot.VisualScripting.Debugger.PauseReason);
        Assert.Equal(new uint[] { 20U }, snapshot.VisualScripting.Debugger.BreakpointNodeIds);
    }

    [Fact]
    public void SnapshotParser_AcceptsBoundedDeveloperConsoleDto()
    {
        using JsonDocument document = JsonDocument.Parse(JsonSerializer.Serialize(Snapshot(sceneDirty: false)));

        BridgeEditorSnapshot snapshot = BridgeSnapshotParser.Parse(document.RootElement);

        Assert.Equal(3UL, snapshot.DeveloperConsole.Generation);
        Assert.True(snapshot.DeveloperConsole.IsAvailable);
        Assert.True(snapshot.DeveloperConsole.IsDevelopmentOnly);
        Assert.Equal((byte)0, snapshot.DeveloperConsole.SeverityFilter);
        Assert.Equal(0, snapshot.DeveloperConsole.HistoryCursor);
        Assert.Equal("help", snapshot.DeveloperConsole.HistoryEntry);
        Assert.Single(snapshot.DeveloperConsole.Completions);
        Assert.Equal("help", snapshot.DeveloperConsole.Completions[0].Identifier);
        Assert.Single(snapshot.DeveloperConsole.Output);
        Assert.Equal(BridgeDeveloperConsoleSeverity.Info, snapshot.DeveloperConsole.Output[0].Severity);
        Assert.Equal("ready", snapshot.DeveloperConsole.Output[0].Text);
        Assert.Equal(new[] { "help" }, snapshot.DeveloperConsole.History);
        Assert.True(snapshot.DeveloperConsole.CVars[0].IsReadOnly);
    }

    [Fact]
    public void ScriptRuntimeBridgeSnapshot_ParsesValidEntriesAndFallsBackToEmpty()
    {
        using JsonDocument document = JsonDocument.Parse(JsonSerializer.Serialize(Snapshot(sceneDirty: false)));

        BridgeEditorSnapshot snapshot = BridgeSnapshotParser.Parse(document.RootElement);

        Assert.True(snapshot.ScriptRuntime.IsAvailable);
        Assert.Equal(2, snapshot.ScriptRuntime.InstanceCount);
        Assert.False(snapshot.ScriptRuntime.EntriesTruncated);
        Assert.Equal("The native ScriptRuntime snapshot is available as copied read-only state.",
            snapshot.ScriptRuntime.Reason);
        Assert.Equal(2, snapshot.ScriptRuntime.Entries.Count);
        Assert.Equal(1, snapshot.ScriptRuntime.VisibleEnabledInstanceCount);
        Assert.Equal(1, snapshot.ScriptRuntime.VisibleDisabledInstanceCount);
        Assert.Equal(1, snapshot.ScriptRuntime.CountMatchingEntries("entity 7"));
        Assert.Equal(0, snapshot.ScriptRuntime.CountMatchingEntries("entity 99"));
        BridgeScriptRuntimeInstanceEntry first = snapshot.ScriptRuntime.Entries[0];
        Assert.Equal(7U, first.EntityIndex);
        Assert.Equal(3U, first.EntityGeneration);
        Assert.Equal(9UL, first.Generation);
        Assert.Equal(4U, first.ProgramVersion);
        Assert.Equal(12, first.InstructionCount);
        Assert.Equal(2, first.StateValueCount);
        Assert.True(first.Enabled);
        Assert.False(snapshot.ScriptRuntime.Entries[1].Enabled);

        Dictionary<string, JsonElement> members = document.RootElement.EnumerateObject()
            .Where(property => property.Name != "scriptRuntime")
            .ToDictionary(property => property.Name, property => property.Value.Clone());
        using JsonDocument legacy = JsonDocument.Parse(JsonSerializer.Serialize(members));

        BridgeEditorSnapshot fallback = BridgeSnapshotParser.Parse(legacy.RootElement);

        Assert.False(fallback.ScriptRuntime.IsAvailable);
        Assert.Equal(0, fallback.ScriptRuntime.InstanceCount);
        Assert.Equal("No native ScriptRuntime is attached to this bridge session.", fallback.ScriptRuntime.Reason);
        Assert.Empty(fallback.ScriptRuntime.Entries);
    }

    [Fact]
    public void ScriptRuntimeBridgeSnapshot_RejectsEntriesBeyondNativeBound()
    {
        using JsonDocument document = JsonDocument.Parse(JsonSerializer.Serialize(Snapshot(sceneDirty: false)));
        object[] entries = Enumerable.Range(0, BridgeSnapshotParser.MaximumPanelEntries + 1)
            .Select(index => (object)new
            {
                entityIndex = (uint)index,
                entityGeneration = 1U,
                generation = 1UL,
                programVersion = 1U,
                instructionCount = 1,
                stateValueCount = 0,
                enabled = true,
            }).ToArray();
        Dictionary<string, JsonElement> members = document.RootElement.EnumerateObject()
            .ToDictionary(property => property.Name, property => property.Value.Clone());
        members["scriptRuntime"] = JsonSerializer.SerializeToElement(new
        {
            available = true,
            instanceCount = entries.Length,
            entriesTruncated = true,
            entries,
        });
        using JsonDocument oversized = JsonDocument.Parse(JsonSerializer.Serialize(members));

        BridgeProtocolException exception = Assert.Throws<BridgeProtocolException>(() =>
            BridgeSnapshotParser.Parse(oversized.RootElement));

        Assert.Equal("bridge.snapshot.invalid", exception.Code);
    }

    [Fact]
    public void SnapshotParser_AcceptsReadOnlyDataTableCatalogDto()
    {
        using JsonDocument document = JsonDocument.Parse(JsonSerializer.Serialize(Snapshot(sceneDirty: false)));

        BridgeEditorSnapshot snapshot = BridgeSnapshotParser.Parse(document.RootElement);

        Assert.Equal(4UL, snapshot.DataTableCatalog.Generation);
        Assert.False(snapshot.DataTableCatalog.EntriesTruncated);
        var entry = Assert.Single(snapshot.DataTableCatalog.Entries);
        Assert.Equal("weapons", entry.Name);
        Assert.Equal(2UL, entry.Generation);
        Assert.Equal(1, entry.ColumnCount);
        Assert.Equal(1, entry.RowCount);
        Assert.True(entry.IsValid);
        Assert.Contains("weapons", entry.DisplayText);
    }

    [Fact]
    public void SnapshotParser_UsesEmptyCatalogFallbackForOlderFrames()
    {
        using JsonDocument document = JsonDocument.Parse(JsonSerializer.Serialize(Snapshot(sceneDirty: false)));
        using JsonDocument withoutCatalog = JsonDocument.Parse(document.RootElement.GetRawText());
        Dictionary<string, JsonElement> members = withoutCatalog.RootElement.EnumerateObject()
            .Where(property => property.Name != "dataTableCatalog")
            .ToDictionary(property => property.Name, property => property.Value.Clone());
        using JsonDocument legacy = JsonDocument.Parse(JsonSerializer.Serialize(members));

        BridgeEditorSnapshot snapshot = BridgeSnapshotParser.Parse(legacy.RootElement);

        Assert.Equal(0UL, snapshot.DataTableCatalog.Generation);
        Assert.Empty(snapshot.DataTableCatalog.Entries);
    }

    [Fact]
    public void SnapshotParser_AcceptsReadOnlyDataTablePreviewDto()
    {
        using JsonDocument document = JsonDocument.Parse(JsonSerializer.Serialize(Snapshot(sceneDirty: false)));

        BridgeEditorSnapshot snapshot = BridgeSnapshotParser.Parse(document.RootElement);

        Assert.True(snapshot.DataTablePreview.IsAvailable);
        Assert.Equal(2UL, snapshot.DataTablePreview.Generation);
        Assert.Equal("weapons", snapshot.DataTablePreview.Name);
        Assert.Equal(1, snapshot.DataTablePreview.TotalColumnCount);
        Assert.Equal(1, snapshot.DataTablePreview.TotalRowCount);
        Assert.Single(snapshot.DataTablePreview.Columns);
        Assert.Equal("damage [Integer]", snapshot.DataTablePreview.Columns[0].DisplayText);
        var row = Assert.Single(snapshot.DataTablePreview.Rows);
        Assert.Equal("pistol: 25", row.DisplayText);
    }

    [Fact]
    public async Task DispatchAsync_SerializesNamedDeveloperConsoleCommand()
    {
        await using MemoryStream input = BuildFrames(new
        {
            jsonrpc = "2.0",
            id = 1,
            result = new
            {
                protocolVersion = BridgeProtocolClient.ProtocolVersion,
                requestId = 1UL,
                applied = true,
                code = "bridge.command.applied",
                message = "The registered native console command executed.",
                snapshot = Snapshot(sceneDirty: false),
                createdEntity = (object?)null,
            },
        });
        await using MemoryStream output = new();
        await using BridgeProtocolClient client = new(input, output);

        await client.DispatchAsync(new BridgeCommand(4UL, "submitDeveloperConsoleCommand",
            DeveloperConsoleCommand: "help"), CancellationToken.None);

        output.Position = 0;
        using JsonDocument request = JsonDocument.Parse(await ReadFrameAsync(output));
        JsonElement parameters = request.RootElement.GetProperty("params");
        Assert.Equal("submitDeveloperConsoleCommand", parameters.GetProperty("kind").GetString());
        Assert.Equal("help", parameters.GetProperty("developerConsoleCommand").GetString());
    }

    [Fact]
    public async Task DispatchAsync_SerializesConsoleDiscoveryPayloads()
    {
        await using MemoryStream input = BuildFrames(new
        {
            jsonrpc = "2.0",
            id = 1,
            result = new
            {
                protocolVersion = BridgeProtocolClient.ProtocolVersion,
                requestId = 1UL,
                applied = true,
                code = "bridge.command.applied",
                message = "The console discovery state changed.",
                snapshot = Snapshot(sceneDirty: false),
                createdEntity = (object?)null,
            },
        });
        await using MemoryStream output = new();
        await using BridgeProtocolClient client = new(input, output);

        await client.DispatchAsync(new BridgeCommand(9UL, "setDeveloperConsoleSeverityFilter",
            DeveloperConsoleSeverityFilter: 2, DeveloperConsoleCompletionPrefix: "ca", DeveloperConsoleHistoryDelta: -1),
            CancellationToken.None);

        output.Position = 0;
        using JsonDocument request = JsonDocument.Parse(await ReadFrameAsync(output));
        JsonElement parameters = request.RootElement.GetProperty("params");
        Assert.Equal((byte)2, parameters.GetProperty("developerConsoleSeverityFilter").GetByte());
        Assert.Equal("ca", parameters.GetProperty("developerConsoleCompletionPrefix").GetString());
        Assert.Equal(-1, parameters.GetProperty("developerConsoleHistoryDelta").GetInt32());
    }

    [Fact]
    public void SnapshotParser_RejectsManagedViewportAttachPermission()
    {
        using JsonDocument document = JsonDocument.Parse(JsonSerializer.Serialize(SnapshotWithSurface(managedAttachAllowed: true)));

        BridgeProtocolException exception = Assert.Throws<BridgeProtocolException>(() =>
            BridgeSnapshotParser.Parse(document.RootElement));

        Assert.Equal("bridge.snapshot.invalid", exception.Code);
    }

    private static MemoryStream BuildFrames(object message)
    {
        byte[] body = JsonSerializer.SerializeToUtf8Bytes(message);
        byte[] header = new byte[sizeof(uint)];
        BinaryPrimitives.WriteUInt32BigEndian(header, checked((uint)body.Length));
        MemoryStream stream = new();
        stream.Write(header);
        stream.Write(body);
        stream.Position = 0;
        return stream;
    }

    private static MemoryStream BuildRawBytes(params byte[][] chunks)
    {
        MemoryStream stream = new();
        foreach (byte[] chunk in chunks)
        {
            stream.Write(chunk);
        }
        stream.Position = 0;
        return stream;
    }

    private static byte[] FrameHeader(uint length)
    {
        byte[] header = new byte[sizeof(uint)];
        BinaryPrimitives.WriteUInt32BigEndian(header, length);
        return header;
    }

    private static async Task<byte[]> ReadFrameAsync(Stream input)
    {
        byte[] header = new byte[sizeof(uint)];
        await input.ReadExactlyAsync(header);
        uint length = BinaryPrimitives.ReadUInt32BigEndian(header);
        byte[] body = new byte[length];
        await input.ReadExactlyAsync(body);
        return body;
    }

    private static object SnapshotWithSurface(bool managedAttachAllowed, bool includeCanvas = false) => new
    {
        protocolVersion = BridgeProtocolClient.ProtocolVersion,
        revision = 1UL,
        editorState = 1,
        playModeState = 0,
        sceneDirty = false,
        canUndo = false,
        canRedo = false,
        activeScenePath = "editor_scene.uvescene",
        selectedEntities = Array.Empty<object>(),
        selectedEntitiesTruncated = false,
        activeEntity = (object?)null,
        hierarchy = new { filter = string.Empty, filterActive = false, truncated = false, entries = Array.Empty<object>() },
        inspector = new { mode = 0, selectedEntitiesTruncated = false, selectedEntities = Array.Empty<object>(), activeEntity = (object?)null, parent = (object?)null, ancestry = Array.Empty<object>(), eligibleDrawerIds = Array.Empty<string>(), canEditSelectedName = false },
        contentBrowser = new { contentRoot = "assets", currentDirectory = string.Empty, filter = string.Empty, typeFocus = "All", breadcrumbs = Array.Empty<string>(), refreshGeneration = 0UL, visibleEntryCount = 0, directEntryCount = 0, contentRootExists = true, initialized = false, lastRefreshSucceeded = true, truncated = false, entries = Array.Empty<object>(), selectedEntry = (object?)null },
        viewportSurface = new { state = 0, generation = 0UL, width = 0U, height = 0U, nativeRendererOwnsSurface = true, managedAttachAllowed, reason = "No managed viewport surface transport is available in this headless bridge session." },
        visualScripting = new
        {
            available = false,
            graphRevision = 0UL,
            nodeCount = 0,
            linkCount = 0,
            canEdit = false,
            reason = "Native visual-scripting presentation is unavailable in this headless bridge session.",
            debugger = new
            {
                available = includeCanvas,
                state = includeCanvas ? 2 : 0,
                instructionIndex = includeCanvas ? 1UL : 0UL,
                sourceNodeId = includeCanvas ? 20U : 0U,
                executedInstructions = includeCanvas ? 1UL : 0UL,
                pauseReason = includeCanvas ? "Breakpoint reached." : string.Empty,
                breakpointNodeIds = includeCanvas ? new[] { 20U } : Array.Empty<uint>(),
                reason = includeCanvas ? "The native visual-scripting debugger snapshot is available as copied read-only state." : "No visual-scripting debugger is attached to this bridge session.",
            },
            canvas = includeCanvas ? new
            {
                revision = 1UL,
                graphRevision = 1UL,
                pan = new { x = 2F, y = -3F },
                zoom = 1.5F,
                nodesTruncated = false,
                linksTruncated = false,
                paletteTruncated = false,
                diagnosticsTruncated = false,
                nodes = new[] { new
                {
                    id = 1U,
                    typeId = "test.source",
                    displayName = "Test Source",
                    x = 10F,
                    y = 20F,
                    selected = true,
                    pins = new[] { new { name = "Out", direction = 1, type = 2 } },
                } },
                links = new[] { new
                {
                    output = new { nodeId = 1U, pinName = "Out" },
                    input = new { nodeId = 2U, pinName = "In" },
                } },
                selectedNodeIds = new[] { 1U },
                paletteNodeTypeIds = new[] { "test.source" },
                diagnostics = new[] { new
                {
                    code = 0,
                    nodeId = 1U,
                    pinName = "",
                    message = "diagnostic",
                } },
            } : null,
        },
        developerConsole = new
        {
            generation = 3UL,
            available = true,
            developmentOnly = true,
            severityFilter = 0,
            historyCursor = 0,
            historyEntry = "help",
            outputTruncated = false,
            historyTruncated = false,
            cvarsTruncated = false,
            completionTruncated = false,
            output = new[] { new { severity = 0, text = "ready" } },
            history = new[] { "help" },
            cvars = new[] { new { name = "r.vsync", value = "1", readOnly = true } },
            completions = new[] { new { identifier = "help", help = "List registered native commands." } },
        },
        scriptRuntime = new
        {
            available = true,
            instanceCount = 2,
            entriesTruncated = false,
            reason = "The native ScriptRuntime snapshot is available as copied read-only state.",
            entries = new[]
            {
                new
                {
                    entityIndex = 7U,
                    entityGeneration = 3U,
                    generation = 9UL,
                    programVersion = 4U,
                    instructionCount = 12,
                    stateValueCount = 2,
                    enabled = true,
                },
                new
                {
                    entityIndex = 8U,
                    entityGeneration = 1U,
                    generation = 10UL,
                    programVersion = 5U,
                    instructionCount = 3,
                    stateValueCount = 0,
                    enabled = false,
                },
            },
        },
        dataTableCatalog = new
        {
            generation = 4UL,
            entriesTruncated = false,
            entries = new[] { new { name = "weapons", generation = 2UL, columnCount = 1, rowCount = 1, valid = true } },
        },
        dataTablePreview = new
        {
            available = true,
            generation = 2UL,
            name = "weapons",
            totalColumnCount = 1,
            totalRowCount = 1,
            columnsTruncated = false,
            rowsTruncated = false,
            valuesTruncated = false,
            reason = "Native table preview is available as copied read-only facts.",
            columns = new[] { new { name = "damage", type = 1 } },
            rows = new[] { new { identifier = "pistol", values = new[] { "25" } } },
        },
        capabilities = Array.Empty<int>(),
    };

    private static object Snapshot(bool sceneDirty, bool includeCanvas = false) => new
    {
        protocolVersion = BridgeProtocolClient.ProtocolVersion,
        revision = 1UL,
        editorState = 1,
        playModeState = 0,
        sceneDirty,
        canUndo = false,
        canRedo = false,
        activeScenePath = "editor_scene.uvescene",
        selectedEntities = Array.Empty<object>(),
        selectedEntitiesTruncated = false,
        activeEntity = (object?)null,
        hierarchy = new
        {
            filter = string.Empty,
            filterActive = false,
            truncated = false,
            entries = Array.Empty<object>(),
        },
        inspector = new
        {
            mode = 0,
            selectedEntitiesTruncated = false,
            selectedEntities = Array.Empty<object>(),
            activeEntity = (object?)null,
            parent = (object?)null,
            ancestry = Array.Empty<object>(),
            eligibleDrawerIds = Array.Empty<string>(),
            canEditSelectedName = false,
        },
        contentBrowser = new
        {
            contentRoot = "assets",
            currentDirectory = string.Empty,
            filter = string.Empty,
            typeFocus = "All",
            breadcrumbs = Array.Empty<string>(),
            refreshGeneration = 0UL,
            visibleEntryCount = 0,
            directEntryCount = 0,
            contentRootExists = true,
            initialized = false,
            lastRefreshSucceeded = true,
            truncated = false,
            entries = Array.Empty<object>(),
            selectedEntry = (object?)null,
        },
        viewportSurface = new
        {
            state = 0,
            generation = 0UL,
            width = 0U,
            height = 0U,
            nativeRendererOwnsSurface = true,
            managedAttachAllowed = false,
            reason = "No managed viewport surface transport is available in this headless bridge session.",
        },
        visualScripting = new
        {
            available = false,
            graphRevision = 0UL,
            nodeCount = 0,
            linkCount = 0,
            canEdit = false,
            reason = "Native visual-scripting presentation is unavailable in this headless bridge session.",
            debugger = new
            {
                available = includeCanvas,
                state = includeCanvas ? 2 : 0,
                instructionIndex = includeCanvas ? 1UL : 0UL,
                sourceNodeId = includeCanvas ? 20U : 0U,
                executedInstructions = includeCanvas ? 1UL : 0UL,
                pauseReason = includeCanvas ? "Breakpoint reached." : string.Empty,
                breakpointNodeIds = includeCanvas ? new[] { 20U } : Array.Empty<uint>(),
                reason = includeCanvas ? "The native visual-scripting debugger snapshot is available as copied read-only state." : "No visual-scripting debugger is attached to this bridge session.",
            },
            canvas = includeCanvas ? new
            {
                revision = 1UL,
                graphRevision = 1UL,
                pan = new { x = 2F, y = -3F },
                zoom = 1.5F,
                nodesTruncated = false,
                linksTruncated = false,
                paletteTruncated = false,
                diagnosticsTruncated = false,
                nodes = new[] { new
                {
                    id = 1U,
                    typeId = "test.source",
                    displayName = "Test Source",
                    x = 10F,
                    y = 20F,
                    selected = true,
                    pins = new[] { new { name = "Out", direction = 1, type = 2 } },
                } },
                links = new[] { new
                {
                    output = new { nodeId = 1U, pinName = "Out" },
                    input = new { nodeId = 2U, pinName = "In" },
                } },
                selectedNodeIds = new[] { 1U },
                paletteNodeTypeIds = new[] { "test.source" },
                diagnostics = new[] { new
                {
                    code = 0,
                    nodeId = 1U,
                    pinName = "",
                    message = "diagnostic",
                } },
            } : null,
        },
        developerConsole = new
        {
            generation = 3UL,
            available = true,
            developmentOnly = true,
            severityFilter = 0,
            historyCursor = 0,
            historyEntry = "help",
            outputTruncated = false,
            historyTruncated = false,
            cvarsTruncated = false,
            completionTruncated = false,
            output = new[] { new { severity = 0, text = "ready" } },
            history = new[] { "help" },
            cvars = new[] { new { name = "r.vsync", value = "1", readOnly = true } },
            completions = new[] { new { identifier = "help", help = "List registered native commands." } },
        },
        scriptRuntime = new
        {
            available = true,
            instanceCount = 2,
            entriesTruncated = false,
            reason = "The native ScriptRuntime snapshot is available as copied read-only state.",
            entries = new[]
            {
                new
                {
                    entityIndex = 7U,
                    entityGeneration = 3U,
                    generation = 9UL,
                    programVersion = 4U,
                    instructionCount = 12,
                    stateValueCount = 2,
                    enabled = true,
                },
                new
                {
                    entityIndex = 8U,
                    entityGeneration = 1U,
                    generation = 10UL,
                    programVersion = 5U,
                    instructionCount = 3,
                    stateValueCount = 0,
                    enabled = false,
                },
            },
        },
        dataTableCatalog = new
        {
            generation = 4UL,
            entriesTruncated = false,
            entries = new[] { new { name = "weapons", generation = 2UL, columnCount = 1, rowCount = 1, valid = true } },
        },
        dataTablePreview = new
        {
            available = true,
            generation = 2UL,
            name = "weapons",
            totalColumnCount = 1,
            totalRowCount = 1,
            columnsTruncated = false,
            rowsTruncated = false,
            valuesTruncated = false,
            reason = "Native table preview is available as copied read-only facts.",
            columns = new[] { new { name = "damage", type = 1 } },
            rows = new[] { new { identifier = "pistol", values = new[] { "25" } } },
        },
        capabilities = Array.Empty<int>(),
    };
}
