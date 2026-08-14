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
        activeEntity = (object?)null,
        capabilities = Array.Empty<int>(),
    };
}
