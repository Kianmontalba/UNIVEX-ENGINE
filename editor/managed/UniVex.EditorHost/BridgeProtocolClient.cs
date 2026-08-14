// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

using System.Buffers.Binary;
using System.Text;
using System.Text.Json;

namespace UniVex.EditorHost;

/// <summary>
/// Typed, single-request-at-a-time client for the local length-prefixed C++ bridge protocol.
/// It owns no engine memory, never exposes native handles, and is safe only while its paired
/// backend stream remains alive.
/// </summary>
public sealed class BridgeProtocolClient : IAsyncDisposable
{
    public const uint ProtocolVersion = 1;
    public const int MaximumFrameBytes = 1024 * 1024;

    private readonly Stream input;
    private readonly Stream output;
    private long nextRequestId = 1;
    private bool disposed;

    public BridgeProtocolClient(Stream input, Stream output)
    {
        this.input = input ?? throw new ArgumentNullException(nameof(input));
        this.output = output ?? throw new ArgumentNullException(nameof(output));
    }

    public async Task<BridgeHelloResult> HelloAsync(CancellationToken cancellationToken)
    {
        using JsonDocument response = await InvokeAsync(
            "bridge.hello",
            new { protocolVersion = ProtocolVersion },
            cancellationToken).ConfigureAwait(false);
        JsonElement result = GetResultOrThrow(response.RootElement);
        bool compatible = result.GetProperty("compatible").GetBoolean();
        uint backendProtocolVersion = result.GetProperty("protocolVersion").GetUInt32();
        string code = result.GetProperty("code").GetString() ?? "bridge.protocol.invalid";
        JsonElement snapshot = result.GetProperty("snapshot").Clone();
        return new BridgeHelloResult(compatible, backendProtocolVersion, code, snapshot);
    }

    public async Task<JsonElement> GetSnapshotAsync(CancellationToken cancellationToken)
    {
        using JsonDocument response = await InvokeAsync("bridge.getSnapshot", new { }, cancellationToken)
            .ConfigureAwait(false);
        return GetResultOrThrow(response.RootElement).GetProperty("snapshot").Clone();
    }

    public async ValueTask DisposeAsync()
    {
        if (disposed)
        {
            return;
        }
        disposed = true;
        await input.DisposeAsync().ConfigureAwait(false);
        await output.DisposeAsync().ConfigureAwait(false);
    }

    private async Task<JsonDocument> InvokeAsync(string method, object parameters, CancellationToken cancellationToken)
    {
        ObjectDisposedException.ThrowIf(disposed, this);
        long requestId = nextRequestId++;
        byte[] requestBody = JsonSerializer.SerializeToUtf8Bytes(new
        {
            jsonrpc = "2.0",
            id = requestId,
            method,
            @params = parameters,
        });
        await WriteFrameAsync(requestBody, cancellationToken).ConfigureAwait(false);
        byte[] responseBody = await ReadFrameAsync(cancellationToken).ConfigureAwait(false);
        JsonDocument response = JsonDocument.Parse(responseBody);
        if (!response.RootElement.TryGetProperty("id", out JsonElement responseId) ||
            responseId.ValueKind != JsonValueKind.Number || responseId.GetInt64() != requestId)
        {
            response.Dispose();
            throw new BridgeProtocolException("bridge.transport.response_id_mismatch",
                "The backend returned a response with an unexpected request identifier.");
        }
        return response;
    }

    private async Task WriteFrameAsync(byte[] body, CancellationToken cancellationToken)
    {
        if (body.Length == 0 || body.Length > MaximumFrameBytes)
        {
            throw new BridgeProtocolException("bridge.transport.frame.invalid",
                "The outgoing bridge frame length is outside the supported bound.");
        }

        byte[] header = new byte[sizeof(uint)];
        BinaryPrimitives.WriteUInt32BigEndian(header, checked((uint)body.Length));
        await output.WriteAsync(header, cancellationToken).ConfigureAwait(false);
        await output.WriteAsync(body, cancellationToken).ConfigureAwait(false);
        await output.FlushAsync(cancellationToken).ConfigureAwait(false);
    }

    private async Task<byte[]> ReadFrameAsync(CancellationToken cancellationToken)
    {
        byte[] header = await ReadExactlyAsync(sizeof(uint), cancellationToken).ConfigureAwait(false);
        uint length = BinaryPrimitives.ReadUInt32BigEndian(header);
        if (length == 0 || length > MaximumFrameBytes)
        {
            throw new BridgeProtocolException("bridge.transport.frame.invalid",
                "The backend returned a frame length outside the supported bound.");
        }
        return await ReadExactlyAsync(checked((int)length), cancellationToken).ConfigureAwait(false);
    }

    private async Task<byte[]> ReadExactlyAsync(int length, CancellationToken cancellationToken)
    {
        byte[] buffer = new byte[length];
        int total = 0;
        while (total < buffer.Length)
        {
            int count = await input.ReadAsync(buffer.AsMemory(total), cancellationToken).ConfigureAwait(false);
            if (count == 0)
            {
                throw new BridgeProtocolException("bridge.transport.eof",
                    "The backend process closed its protocol stream before the response completed.");
            }
            total += count;
        }
        return buffer;
    }

    private static JsonElement GetResultOrThrow(JsonElement response)
    {
        if (response.TryGetProperty("error", out JsonElement error))
        {
            string code = error.TryGetProperty("data", out JsonElement data) &&
                          data.TryGetProperty("code", out JsonElement codeValue)
                ? codeValue.GetString() ?? "bridge.request.invalid"
                : "bridge.request.invalid";
            string message = error.GetProperty("message").GetString() ?? "The backend rejected the bridge request.";
            throw new BridgeProtocolException(code, message);
        }
        if (!response.TryGetProperty("result", out JsonElement result))
        {
            throw new BridgeProtocolException("bridge.transport.response.invalid",
                "The backend response contains neither a result nor an error.");
        }
        return result;
    }
}

public sealed record BridgeHelloResult(bool Compatible, uint BackendProtocolVersion, string Code, JsonElement Snapshot);

public sealed class BridgeProtocolException : Exception
{
    public BridgeProtocolException(string code, string message) : base(message)
    {
        Code = code;
    }

    public string Code { get; }
}
