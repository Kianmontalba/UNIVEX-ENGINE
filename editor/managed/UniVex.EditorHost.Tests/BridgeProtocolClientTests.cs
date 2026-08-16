// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

using System.Buffers.Binary;
using System.Text.Json;
using System.Text.Json.Nodes;

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
    public async Task DispatchAsync_WritesSetVisualScriptPinDefaultPayload()
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
                message = "The native visual-script pin default was updated.",
                snapshot = Snapshot(sceneDirty: false),
                createdEntity = (object?)null,
            },
        });
        await using MemoryStream output = new();
        await using BridgeProtocolClient client = new(input, output);

        await client.DispatchAsync(new BridgeCommand(7UL, "setVisualScriptPinDefault",
            VisualScriptNodeId: 7U,
            VisualScriptPinName: "Value",
            VisualScriptDefaultValue: "3.14"), CancellationToken.None);

        output.Position = 0;
        using JsonDocument request = JsonDocument.Parse(await ReadFrameAsync(output));
        JsonElement parameters = request.RootElement.GetProperty("params");
        Assert.Equal("setVisualScriptPinDefault", parameters.GetProperty("kind").GetString());
        Assert.Equal(7U, parameters.GetProperty("visualScriptNodeId").GetUInt32());
        Assert.Equal("Value", parameters.GetProperty("visualScriptPinName").GetString());
        Assert.Equal("3.14", parameters.GetProperty("visualScriptDefaultValue").GetString());
    }

    [Fact]
    public void MotionQuerySnapshot_ParsesAuthoringDebuggerAndTraceFacts()
    {
        JsonObject snapshot = JsonNode.Parse(JsonSerializer.Serialize(Snapshot(sceneDirty: false)))!.AsObject();
        snapshot["motionQuery"] = new JsonObject
        {
            ["authoring"] = new JsonObject
            {
                ["revision"] = 4UL,
                ["selectedResource"] = new JsonObject { ["guid"] = 77UL, ["generation"] = 1UL },
                ["diagnostic"] = "native",
                ["databases"] = new JsonArray
                {
                    new JsonObject
                    {
                        ["resource"] = new JsonObject { ["guid"] = 77UL, ["generation"] = 1UL },
                        ["displayName"] = "Bridge DB",
                        ["databaseId"] = "bridge-db",
                        ["generation"] = 1UL,
                        ["schemaVersion"] = 1U,
                        ["schemaId"] = "schema",
                        ["candidateCount"] = 1,
                        ["maximumCandidates"] = 4,
                        ["valid"] = true,
                        ["selected"] = true,
                        ["dirty"] = false,
                    },
                },
            },
            ["debugger"] = new JsonObject
            {
                ["attached"] = true,
                ["generation"] = 2UL,
                ["database"] = new JsonObject { ["guid"] = 77UL, ["generation"] = 1UL },
                ["selectedCandidateIndex"] = 0UL,
                ["candidateCount"] = 1,
                ["candidatesEvaluated"] = 1,
                ["selectedCost"] = 0.5F,
                ["selectedCandidateId"] = "candidate",
                ["selectedSourceClipId"] = "walk",
                ["qualityTier"] = 2,
                ["continuityCode"] = 1,
                ["continuityApplied"] = true,
                ["transitionCode"] = 2,
                ["transitionHeldPrevious"] = false,
                ["telemetryCode"] = 0,
                ["telemetryIndexEntryCount"] = 32UL,
                ["telemetryCandidatesConsidered"] = 1UL,
                ["telemetryBudgetSaturated"] = false,
                ["provenance"] = "continuity_applied",
                ["message"] = "matched",
            },
            ["trace"] = new JsonObject
            {
                ["generation"] = 3UL,
                ["truncated"] = false,
                ["events"] = new JsonArray
                {
                    new JsonObject
                    {
                        ["sequence"] = 1UL,
                        ["timestampNanoseconds"] = 10UL,
                        ["frameNumber"] = 1UL,
                        ["kind"] = "match",
                        ["database"] = new JsonObject { ["guid"] = 77UL, ["generation"] = 1UL },
                        ["candidatesConsidered"] = 1,
                        ["candidatesEvaluated"] = 1,
                        ["cost"] = 0.5F,
                        ["selectedCandidateIndex"] = 0UL,
                        ["qualityTier"] = 2,
                        ["continuityCode"] = 1,
                        ["continuityApplied"] = true,
                        ["transitionCode"] = 2,
                        ["transitionHeldPrevious"] = false,
                        ["telemetryCode"] = 0,
                        ["telemetryIndexEntryCount"] = 32UL,
                        ["telemetryCandidatesConsidered"] = 1UL,
                        ["telemetryBudgetSaturated"] = false,
                        ["provenance"] = "continuity_applied",
                        ["message"] = "matched",
                    },
                },
            },
            ["liveDebugActive"] = true,
            ["liveDebugGeneration"] = 4UL,
            ["liveDebugDatabase"] = new JsonObject { ["guid"] = 77UL, ["generation"] = 1UL },
            ["liveDebugFilter"] = "match",
            ["liveDebugTotalTraceEventCount"] = 1,
            ["liveDebugVisibleTraceEventCount"] = 1,
            ["liveDebugTraceTruncated"] = false,
            ["liveDebugDiagnostic"] = "active",
            ["replayComparison"] = new JsonObject
            {
                ["available"] = true,
                ["code"] = 0,
                ["comparisonCode"] = 0,
                ["comparedEventCount"] = 1UL,
                ["mismatchIndex"] = 0UL,
                ["fixtureTruncated"] = false,
                ["snapshotTruncated"] = false,
                ["mismatchFieldMask"] = 0U,
                ["message"] = "replay fixture matches trace",
                ["diagnosticSummary"] = "",
            },
        };
        using JsonDocument document = JsonDocument.Parse(snapshot.ToJsonString());
        BridgeEditorSnapshot parsed = BridgeSnapshotParser.Parse(document.RootElement);
        Assert.Equal(4UL, parsed.MotionQuery.Authoring.Revision);
        Assert.Equal(77UL, parsed.MotionQuery.Authoring.SelectedResource!.Guid);
        Assert.Equal("Bridge DB", parsed.MotionQuery.Authoring.Databases[0].DisplayName);
        Assert.True(parsed.MotionQuery.Debugger.IsAttached);
        Assert.Equal("candidate", parsed.MotionQuery.Debugger.SelectedCandidateId);
        Assert.Equal((byte)2, parsed.MotionQuery.Debugger.QualityTier);
        Assert.Equal((byte)1, parsed.MotionQuery.Debugger.ContinuityCode);
        Assert.True(parsed.MotionQuery.Debugger.ContinuityApplied);
        Assert.Equal((byte)2, parsed.MotionQuery.Debugger.TransitionCode);
        Assert.Equal("continuity_applied", parsed.MotionQuery.Debugger.Provenance);
        Assert.Equal((byte)0, parsed.MotionQuery.Debugger.TelemetryCode);
        Assert.Equal(32UL, parsed.MotionQuery.Debugger.TelemetryIndexEntryCount);
        Assert.Equal(1UL, parsed.MotionQuery.Debugger.TelemetryCandidatesConsidered);
        Assert.False(parsed.MotionQuery.Debugger.TelemetryBudgetSaturated);
        Assert.Equal(1, parsed.MotionQuery.Trace.Events.Count);
        Assert.Equal("match", parsed.MotionQuery.Trace.Events[0].Kind);
        Assert.Equal(0UL, parsed.MotionQuery.Trace.Events[0].SelectedCandidateIndex);
        Assert.Equal((byte)2, parsed.MotionQuery.Trace.Events[0].QualityTier);
        Assert.True(parsed.MotionQuery.Trace.Events[0].ContinuityApplied);
        Assert.Equal("continuity_applied", parsed.MotionQuery.Trace.Events[0].Provenance);
        Assert.Equal((byte)0, parsed.MotionQuery.Trace.Events[0].TelemetryCode);
        Assert.Equal(32UL, parsed.MotionQuery.Trace.Events[0].TelemetryIndexEntryCount);
        Assert.False(parsed.MotionQuery.Trace.Events[0].TelemetryBudgetSaturated);
        Assert.True(parsed.MotionQuery.LiveDebugActive);
        Assert.Equal(4UL, parsed.MotionQuery.LiveDebugGeneration);
        Assert.Equal(77UL, parsed.MotionQuery.LiveDebugDatabase!.Guid);
        Assert.Equal("match", parsed.MotionQuery.LiveDebugFilter);
        Assert.Equal(1, parsed.MotionQuery.LiveDebugVisibleTraceEventCount);
        Assert.True(parsed.MotionQuery.ReplayComparison.Available);
        Assert.Equal((byte)0, parsed.MotionQuery.ReplayComparison.Code);
        Assert.Equal((byte)0, parsed.MotionQuery.ReplayComparison.ComparisonCode);
        Assert.Equal(1UL, parsed.MotionQuery.ReplayComparison.ComparedEventCount);
        Assert.Equal(0U, parsed.MotionQuery.ReplayComparison.MismatchFieldMask);
        Assert.Equal("replay fixture matches trace", parsed.MotionQuery.ReplayComparison.Message);
        Assert.Equal(string.Empty, parsed.MotionQuery.ReplayComparison.DiagnosticSummary);
    }

    [Fact]
    public void MotionQuerySnapshot_RejectsNonMonotonicTraceEvents()
    {
        JsonObject snapshot = JsonNode.Parse(JsonSerializer.Serialize(Snapshot(sceneDirty: false)))!.AsObject();
        snapshot["motionQuery"] = new JsonObject
        {
            ["authoring"] = new JsonObject
            {
                ["revision"] = 0UL,
                ["selectedResource"] = null,
                ["diagnostic"] = "native",
                ["databases"] = new JsonArray(),
            },
            ["debugger"] = new JsonObject
            {
                ["attached"] = false,
                ["generation"] = 0UL,
                ["database"] = null,
                ["selectedCandidateIndex"] = null,
                ["candidateCount"] = 0,
                ["candidatesEvaluated"] = 0,
                ["selectedCost"] = 0F,
                ["selectedCandidateId"] = string.Empty,
                ["selectedSourceClipId"] = string.Empty,
                ["message"] = "none",
            },
            ["trace"] = new JsonObject
            {
                ["generation"] = 1UL,
                ["truncated"] = false,
                ["events"] = new JsonArray
                {
                    new JsonObject
                    {
                        ["sequence"] = 2UL,
                        ["timestampNanoseconds"] = 10UL,
                        ["frameNumber"] = 1UL,
                        ["kind"] = "match",
                        ["database"] = null,
                        ["candidatesConsidered"] = 1,
                        ["candidatesEvaluated"] = 1,
                        ["cost"] = 0F,
                        ["message"] = string.Empty,
                    },
                    new JsonObject
                    {
                        ["sequence"] = 1UL,
                        ["timestampNanoseconds"] = 11UL,
                        ["frameNumber"] = 1UL,
                        ["kind"] = "match",
                        ["database"] = null,
                        ["candidatesConsidered"] = 1,
                        ["candidatesEvaluated"] = 1,
                        ["cost"] = 0F,
                        ["message"] = string.Empty,
                    },
                },
            },
        };
        using JsonDocument document = JsonDocument.Parse(snapshot.ToJsonString());
        BridgeProtocolException exception = Assert.Throws<BridgeProtocolException>(
            () => BridgeSnapshotParser.Parse(document.RootElement));
        Assert.Contains("monotonic", exception.Message, StringComparison.OrdinalIgnoreCase);
    }

    [Fact]
    public async Task DispatchAsync_WritesMotionQueryNamedCommandPayload()
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
                code = "bridge.motion_query.command.applied",
                message = "applied",
                snapshot = Snapshot(sceneDirty: false),
                createdEntity = (object?)null,
            },
        });
        await using MemoryStream output = new();
        await using BridgeProtocolClient client = new(input, output);
        BridgeCommand command = new(7UL, "dispatchMotionQueryCommand")
        {
            MotionQueryCommandKind = "selectDatabase",
            MotionQueryCommandExpectedRevision = 3UL,
            MotionQueryResource = new BridgeMotionQueryResourceHandle(77UL, 1UL),
        };
        await client.DispatchAsync(command, CancellationToken.None);
        output.Position = 0;
        using JsonDocument request = JsonDocument.Parse(await ReadFrameAsync(output));
        JsonElement payload = request.RootElement.GetProperty("params").GetProperty("motionQueryCommand");
        Assert.Equal("selectDatabase", payload.GetProperty("kind").GetString());
        Assert.Equal(3UL, payload.GetProperty("expectedRevision").GetUInt64());
        Assert.Equal(77UL, payload.GetProperty("resource").GetProperty("guid").GetUInt64());
    }

    [Fact]
    public async Task DispatchAsync_WritesMotionQueryLiveDebugCommandPayload()
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
                code = "bridge.motion_query.debug.command.applied",
                message = "filter updated",
                snapshot = Snapshot(sceneDirty: false),
                createdEntity = (object?)null,
            },
        });
        await using MemoryStream output = new();
        await using BridgeProtocolClient client = new(input, output);
        BridgeCommand command = new(8UL, "dispatchMotionQueryDebugCommand")
        {
            MotionQueryDebugCommandKind = "setFilter",
            MotionQueryDebugExpectedGeneration = 3UL,
            MotionQueryDebugFilter = "accepted",
        };
        await client.DispatchAsync(command, CancellationToken.None);
        output.Position = 0;
        using JsonDocument request = JsonDocument.Parse(await ReadFrameAsync(output));
        JsonElement payload = request.RootElement.GetProperty("params").GetProperty("motionQueryDebugCommand");
        Assert.Equal("setFilter", payload.GetProperty("kind").GetString());
        Assert.Equal(3UL, payload.GetProperty("expectedGeneration").GetUInt64());
        Assert.Equal("accepted", payload.GetProperty("filter").GetString());
    }

    [Fact]
    public void ReadScriptRuntimeCapability_UsesAppendOnlyProtocolId()
    {
        Assert.Equal((byte)76, BridgeSnapshotParser.ReadScriptRuntimeCapability);
        Assert.Equal((byte)77, BridgeSnapshotParser.ReadScriptRuntimeTickDiagnosticsCapability);
    }

    [Fact]
    public void ScriptRuntimeTickSummary_ParsesValidSummaryAndFallsBackToEmpty()
    {
        using JsonDocument validDocument = JsonDocument.Parse(JsonSerializer.Serialize(new
        {
            available = true,
            reason = "The native ScriptRuntime diagnostic tick completed.",
            enabledInstanceCount = 3,
            completedCount = 2,
            instructionBudgetExceededCount = 1,
            invalidInstructionCount = 0,
            diagnosticCount = 1,
        }));
        BridgeScriptRuntimeTickSummary valid = BridgeSnapshotParser.ParseTickSummaryForResponse(validDocument.RootElement);
        Assert.True(valid.IsAvailable);
        Assert.Equal("The native ScriptRuntime diagnostic tick completed.", valid.Reason);
        Assert.Equal(3, valid.EnabledInstanceCount);
        Assert.Equal(2, valid.CompletedCount);
        Assert.Equal(1, valid.InstructionBudgetExceededCount);
        Assert.Equal(0, valid.InvalidInstructionCount);
        Assert.Equal(1, valid.DiagnosticCount);

        using JsonDocument snapshotDocument = JsonDocument.Parse(JsonSerializer.Serialize(Snapshot(sceneDirty: false)));
        BridgeEditorSnapshot fallback = BridgeSnapshotParser.Parse(snapshotDocument.RootElement);
        Assert.False(fallback.ScriptRuntimeTickSummary.IsAvailable);
        Assert.Equal(0, fallback.ScriptRuntimeTickSummary.EnabledInstanceCount);
        Assert.Contains("has been requested", fallback.ScriptRuntimeTickSummary.Reason);
    }

    [Fact]
    public void ScriptRuntimeTickHistory_ParsesOrderedBoundedEntries()
    {
        JsonObject snapshot = JsonNode.Parse(JsonSerializer.Serialize(Snapshot(sceneDirty: false)))!.AsObject();
        snapshot["scriptRuntimeTickHistoryTruncated"] = true;
        snapshot["scriptRuntimeTickHistory"] = new JsonArray(
            new JsonObject
            {
                ["sequence"] = 3UL,
                ["summary"] = new JsonObject
                {
                    ["available"] = true,
                    ["reason"] = "tick",
                    ["enabledInstanceCount"] = 2,
                    ["completedCount"] = 2,
                    ["instructionBudgetExceededCount"] = 0,
                    ["invalidInstructionCount"] = 0,
                    ["diagnosticCount"] = 0,
                },
            },
            new JsonObject
            {
                ["sequence"] = 4UL,
                ["summary"] = new JsonObject
                {
                    ["available"] = true,
                    ["reason"] = "tick",
                    ["enabledInstanceCount"] = 2,
                    ["completedCount"] = 1,
                    ["instructionBudgetExceededCount"] = 1,
                    ["invalidInstructionCount"] = 0,
                    ["diagnosticCount"] = 1,
                },
            });
        using JsonDocument document = JsonDocument.Parse(snapshot.ToJsonString());
        BridgeEditorSnapshot parsed = BridgeSnapshotParser.Parse(document.RootElement);
        Assert.True(parsed.ScriptRuntimeTickHistoryTruncated);
        Assert.Equal(2, parsed.ScriptRuntimeTickHistory.Count);
        Assert.Equal(3UL, parsed.ScriptRuntimeTickHistory[0].Sequence);
        Assert.Equal(4UL, parsed.ScriptRuntimeTickHistory[1].Sequence);
        Assert.Equal(1, parsed.ScriptRuntimeTickHistory[1].Summary.InstructionBudgetExceededCount);
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
        Assert.Equal("EVENT", snapshot.VisualScripting.Canvas.Nodes[0].Category);
        Assert.Equal("node.event", snapshot.VisualScripting.Canvas.Nodes[0].IconId);
        Assert.Equal(10U, snapshot.VisualScripting.Canvas.Nodes[0].DisplayOrder);
        Assert.Equal(1U, snapshot.VisualScripting.Canvas.Nodes[0].PresentationFlags);
        Assert.True(snapshot.VisualScripting.Canvas.Dirty);
        Assert.True(snapshot.VisualScripting.Canvas.CanUndo);
        Assert.False(snapshot.VisualScripting.Canvas.CanRedo);
        Assert.Equal((byte)1, snapshot.VisualScripting.Canvas.Nodes[0].Pins[0].Role);
        Assert.Equal("0.016", snapshot.VisualScripting.Canvas.Nodes[0].Pins[0].DefaultValue);
        Assert.Equal(1, snapshot.VisualScripting.Canvas.Links.Count);
        Assert.Equal(new uint[] { 1U }, snapshot.VisualScripting.Canvas.SelectedNodeIds);
        Assert.Equal(1.5F, snapshot.VisualScripting.Canvas.View.Zoom);
        Assert.Single(snapshot.VisualScripting.Canvas.Diagnostics);
        Assert.Equal((byte)2, snapshot.VisualScripting.Canvas.Diagnostics[0].Severity);
        Assert.Equal("Node #1 / pin Out", snapshot.VisualScripting.Canvas.Diagnostics[0].SourceContext);
        Assert.Equal("Node #1 / pin Out: diagnostic", snapshot.VisualScripting.Canvas.Diagnostics[0].DisplayText);
        Assert.Equal(new BridgeVisualScriptEndpoint(2U, "In"),
                     snapshot.VisualScripting.Canvas.Diagnostics[0].RelatedEndpoint);
        Assert.True(snapshot.VisualScripting.Debugger.Available);
        Assert.Equal((byte)2, snapshot.VisualScripting.Debugger.State);
        Assert.Equal(1UL, snapshot.VisualScripting.Debugger.InstructionIndex);
        Assert.Equal(20U, snapshot.VisualScripting.Debugger.SourceNodeId);
        Assert.Equal("Breakpoint reached.", snapshot.VisualScripting.Debugger.PauseReason);
        Assert.Equal(new uint[] { 20U }, snapshot.VisualScripting.Debugger.BreakpointNodeIds);
        Assert.Single(snapshot.VisualScripting.Canvas.PaletteDescriptors);
        Assert.Equal("EVENT", snapshot.VisualScripting.Canvas.PaletteDescriptors[0].Category);
        Assert.Equal("node.event", snapshot.VisualScripting.Canvas.PaletteDescriptors[0].IconId);
        Assert.Single(snapshot.VisualScripting.Canvas.PaletteDescriptors[0].Pins);
    }

    [Fact]
    public void SnapshotParser_RejectsVisualScriptDiagnosticSeverityBeyondBound()
    {
        JsonObject root = JsonNode.Parse(JsonSerializer.Serialize(Snapshot(sceneDirty: false, includeCanvas: true)))!.AsObject();
        JsonObject canvas = root["visualScripting"]!["canvas"]!.AsObject();
        canvas["diagnostics"]!.AsArray()[0]!["severity"] = 3;

        using JsonDocument document = JsonDocument.Parse(root.ToJsonString());
        BridgeProtocolException exception = Assert.Throws<BridgeProtocolException>(() =>
            BridgeSnapshotParser.Parse(document.RootElement));

        Assert.Equal("bridge.snapshot.invalid", exception.Code);
    }

    [Fact]
    public void SnapshotParser_AcceptsVersionedVisualScriptGraphSchemaDto()
    {
        using JsonDocument document = JsonDocument.Parse(JsonSerializer.Serialize(new
        {
            schemaVersion = 1U,
            nodes = new[]
            {
                new { id = 2U, typeId = "test.sink" },
                new { id = 1U, typeId = "test.source" },
            },
            links = new[]
            {
                new
                {
                    output = new { nodeId = 1U, pinName = "Out" },
                    input = new { nodeId = 2U, pinName = "In" },
                },
            },
            layout = new[]
            {
                new { nodeId = 2U, x = 30F, y = 40F },
                new { nodeId = 1U, x = 10F, y = 20F },
            },
            metadata = new Dictionary<string, string> { ["assetType"] = "visual-script-graph" },
        }));

        BridgeVisualScriptGraphSchema schema = BridgeSnapshotParser.ParseVisualScriptGraphSchema(document.RootElement);

        Assert.Equal(1U, schema.SchemaVersion);
        Assert.Equal(2, schema.Nodes.Count);
        Assert.Equal(new BridgeVisualScriptGraphNode(2U, "test.sink", new BridgeVisualScriptPoint(30F, 40F)),
                     schema.Nodes[0]);
        Assert.Equal(new BridgeVisualScriptGraphNode(1U, "test.source", new BridgeVisualScriptPoint(10F, 20F)),
                     schema.Nodes[1]);
        Assert.Single(schema.Links);
        Assert.Equal(new BridgeVisualScriptEndpoint(2U, "In"), schema.Links[0].Input);
        Assert.Equal("visual-script-graph", schema.Metadata["assetType"]);
    }

    [Fact]
    public void SnapshotParser_RejectsUnsupportedVisualScriptGraphSchemaVersion()
    {
        using JsonDocument document = JsonDocument.Parse("{\"schemaVersion\":99,\"nodes\":[],\"links\":[]}");

        BridgeProtocolException exception = Assert.Throws<BridgeProtocolException>(() =>
            BridgeSnapshotParser.ParseVisualScriptGraphSchema(document.RootElement));

        Assert.Equal("bridge.snapshot.invalid", exception.Code);
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
                dirty = true,
                canUndo = true,
                canRedo = false,
                nodes = new[] { new
                {
                    id = 1U,
                    typeId = "test.source",
                    displayName = "Test Source",
                    category = "EVENT",
                    iconId = "node.event",
                    displayOrder = 10U,
                    presentationFlags = 1U,
                    x = 10F,
                    y = 20F,
                    selected = true,
                    pins = new[] { new { name = "Out", direction = 1, type = 2, role = 1, defaultValue = "0.016" } },
                } },
                links = new[] { new
                {
                    output = new { nodeId = 1U, pinName = "Out" },
                    input = new { nodeId = 2U, pinName = "In" },
                } },
                selectedNodeIds = new[] { 1U },
                paletteNodeTypeIds = new[] { "test.source" },
                paletteDescriptors = new[] { new
                {
                    typeId = "test.source",
                    displayName = "Test Source",
                    category = "EVENT",
                    iconId = "node.event",
                    displayOrder = 10U,
                    presentationFlags = 1U,
                    pins = new[] { new { name = "Out", direction = 1, type = 2, role = 1, defaultValue = "0.016" } },
                } },
                diagnostics = new[] { new
                {
                    code = 0,
                    nodeId = 1U,
                    pinName = "",
                    message = "diagnostic",
                    severity = 2,
                    sourceContext = "Node #1 / pin Out",
                    relatedEndpoint = new { nodeId = 2U, pinName = "In" },
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
                dirty = true,
                canUndo = true,
                canRedo = false,
                nodes = new[] { new
                {
                    id = 1U,
                    typeId = "test.source",
                    displayName = "Test Source",
                    category = "EVENT",
                    iconId = "node.event",
                    displayOrder = 10U,
                    presentationFlags = 1U,
                    x = 10F,
                    y = 20F,
                    selected = true,
                    pins = new[] { new { name = "Out", direction = 1, type = 2, role = 1, defaultValue = "0.016" } },
                } },
                links = new[] { new
                {
                    output = new { nodeId = 1U, pinName = "Out" },
                    input = new { nodeId = 2U, pinName = "In" },
                } },
                selectedNodeIds = new[] { 1U },
                paletteNodeTypeIds = new[] { "test.source" },
                paletteDescriptors = new[] { new
                {
                    typeId = "test.source",
                    displayName = "Test Source",
                    category = "EVENT",
                    iconId = "node.event",
                    displayOrder = 10U,
                    presentationFlags = 1U,
                    pins = new[] { new { name = "Out", direction = 1, type = 2, role = 1, defaultValue = "0.016" } },
                } },
                diagnostics = new[] { new
                {
                    code = 0,
                    nodeId = 1U,
                    pinName = "",
                    message = "diagnostic",
                    severity = 2,
                    sourceContext = "Node #1 / pin Out",
                    relatedEndpoint = new { nodeId = 2U, pinName = "In" },
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
