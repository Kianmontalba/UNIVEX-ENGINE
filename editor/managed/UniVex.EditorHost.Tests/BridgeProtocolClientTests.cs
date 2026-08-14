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
        Assert.False(result.Snapshot.GetProperty("sceneDirty").GetBoolean());

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
        activeEntity = (object?)null,
        capabilities = Array.Empty<int>(),
    };
}
