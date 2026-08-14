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
            capabilities = Array.Empty<int>(),
        }));

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

    private static object Snapshot(bool sceneDirty) => new
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
        capabilities = Array.Empty<int>(),
    };
}
