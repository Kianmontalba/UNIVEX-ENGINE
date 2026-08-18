// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

using System.Buffers.Binary;
using System.Text;
using System.Text.Json;
using System.Linq;

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

    private enum FrameReadStage
    {
        Header,
        Body,
    }

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
        BridgeEditorSnapshot snapshot = BridgeSnapshotParser.Parse(result.GetProperty("snapshot"));
        return new BridgeHelloResult(compatible, backendProtocolVersion, code, snapshot);
    }

    public async Task<BridgeEditorSnapshot> GetSnapshotAsync(CancellationToken cancellationToken)
    {
        using JsonDocument response = await InvokeAsync("bridge.getSnapshot", new { }, cancellationToken)
            .ConfigureAwait(false);
        return BridgeSnapshotParser.Parse(GetResultOrThrow(response.RootElement).GetProperty("snapshot"));
    }

    public async Task<BridgeCommandResult> DispatchAsync(BridgeCommand command, CancellationToken cancellationToken)
    {
        ArgumentNullException.ThrowIfNull(command);
        if (string.IsNullOrWhiteSpace(command.Kind))
        {
            throw new ArgumentException("A named bridge command kind is required.", nameof(command));
        }

        using JsonDocument response = await InvokeAsync("bridge.dispatch", new
        {
            protocolVersion = ProtocolVersion,
            requestId = checked((ulong)nextRequestId),
            expectedRevision = command.ExpectedRevision,
            kind = command.Kind,
            entity = command.Entity is null ? null : new { index = command.Entity.Index, generation = command.Entity.Generation },
            entityName = command.EntityName,
            entityKind = (string?)null,
            hierarchyFilter = command.HierarchyFilter,
            contentDirectory = command.ContentDirectory,
            contentFilter = command.ContentFilter,
            contentFocus = command.ContentFocus,
            contentEntryPath = command.ContentEntryPath,
            contentImportDestinationPath = command.ContentImportDestinationPath,
            visualScriptNodeId = command.VisualScriptNodeId,
            visualScriptNode = command.VisualScriptNode is null ? null : new
            {
                id = command.VisualScriptNode.Id,
                typeId = command.VisualScriptNode.TypeId,
            },
            visualScriptNodeTypeId = command.VisualScriptNodeTypeId,
            visualScriptPosition = command.VisualScriptPosition is null ? null : new
            {
                x = command.VisualScriptPosition.X,
                y = command.VisualScriptPosition.Y,
            },
            visualScriptLink = command.VisualScriptLink is null ? null : new
            {
                output = new
                {
                    nodeId = command.VisualScriptLink.Output.NodeId,
                    pinName = command.VisualScriptLink.Output.PinName,
                },
                input = new
                {
                    nodeId = command.VisualScriptLink.Input.NodeId,
                    pinName = command.VisualScriptLink.Input.PinName,
                },
            },
            visualScriptSelection = command.VisualScriptSelection,
            visualScriptView = command.VisualScriptView is null ? null : new
            {
                pan = new
                {
                    x = command.VisualScriptView.Pan.X,
                    y = command.VisualScriptView.Pan.Y,
                },
                zoom = command.VisualScriptView.Zoom,
            },
            visualScriptGraphSchema = command.VisualScriptGraphSchema,
            visualScriptPinName = command.VisualScriptPinName,
            visualScriptDefaultValue = command.VisualScriptDefaultValue,
            dataTableName = command.DataTableName,
            developerConsoleCommand = command.DeveloperConsoleCommand,
            developerConsoleSeverityFilter = command.DeveloperConsoleSeverityFilter,
            developerConsoleCompletionPrefix = command.DeveloperConsoleCompletionPrefix,
            developerConsoleHistoryDelta = command.DeveloperConsoleHistoryDelta,
            motionQueryCommand = command.MotionQueryCommandKind is null ? null : new
            {
                protocolVersion = BridgeProtocolClient.ProtocolVersion,
                expectedRevision = command.MotionQueryCommandExpectedRevision ?? command.ExpectedRevision,
                kind = command.MotionQueryCommandKind,
                resource = command.MotionQueryResource is null ? null : new
                {
                    guid = command.MotionQueryResource.Guid,
                    generation = command.MotionQueryResource.Generation,
                },
                text = command.MotionQueryText,
                candidateIndex = command.MotionQueryCandidateIndex,
                database = SerializeMotionQueryDatabase(command.MotionQueryDatabase),
                candidate = SerializeOptionalMotionQueryCandidate(command.MotionQueryCandidate),
                pasteTarget = command.MotionQueryPasteTarget is null ? null : new
                {
                    resource = new
                    {
                        guid = command.MotionQueryPasteTarget.Resource.Guid,
                        generation = command.MotionQueryPasteTarget.Resource.Generation,
                    },
                    displayName = command.MotionQueryPasteTarget.DisplayName,
                    context = new
                    {
                        databaseId = command.MotionQueryPasteTarget.DatabaseId,
                        generation = command.MotionQueryPasteTarget.Generation,
                    },
                },
            },
            motionQueryDebugCommand = command.MotionQueryDebugCommandKind is null ? null : new
            {
                protocolVersion = BridgeProtocolClient.ProtocolVersion,
                expectedGeneration = command.MotionQueryDebugExpectedGeneration ?? 0UL,
                kind = command.MotionQueryDebugCommandKind,
                database = command.MotionQueryDebugDatabase is null ? null : new
                {
                    guid = command.MotionQueryDebugDatabase.Guid,
                    generation = command.MotionQueryDebugDatabase.Generation,
                },
                filter = command.MotionQueryDebugFilter,
                payload = command.MotionQueryDebugPayload,
                eventSequence = command.MotionQueryDebugEventSequence,
            },
            motionQueryReplayBaselineName = command.MotionQueryReplayBaselineName,
            motionQueryReplayFixturePayload = command.MotionQueryReplayFixturePayload,
            motionQueryReplayBaselineEnvelopePayload = command.MotionQueryReplayBaselineEnvelopePayload,
            motionQueryReplayBaselineNewName = command.MotionQueryReplayBaselineNewName,
        }, cancellationToken).ConfigureAwait(false);
        JsonElement result = GetResultOrThrow(response.RootElement);
        BridgeEntityRef? createdEntity = result.GetProperty("createdEntity").ValueKind == JsonValueKind.Null
            ? null
            : ParseEntityRef(result.GetProperty("createdEntity"));
        BridgeVisualScriptGraphSchema? graphSchema = result.TryGetProperty("graphSchema", out JsonElement graphSchemaValue) &&
                                                       graphSchemaValue.ValueKind != JsonValueKind.Null
            ? BridgeSnapshotParser.ParseVisualScriptGraphSchema(graphSchemaValue)
            : null;
        string? envelopePayload = result.TryGetProperty("motionQueryReplayBaselineEnvelopePayload", out JsonElement envelopeValue) &&
                                   envelopeValue.ValueKind == JsonValueKind.String
            ? envelopeValue.GetString()
            : null;
        string? liveDebugTracePayload = result.TryGetProperty("motionQueryLiveDebugTracePayload", out JsonElement liveTraceValue) &&
                                        liveTraceValue.ValueKind == JsonValueKind.String
            ? liveTraceValue.GetString()
            : null;
        ulong? contentImportJobId = result.TryGetProperty("contentImportJobId", out JsonElement contentImportJobValue) &&
                                    contentImportJobValue.ValueKind == JsonValueKind.Number
            ? contentImportJobValue.GetUInt64()
            : null;
        return new BridgeCommandResult(
            result.GetProperty("applied").GetBoolean(),
            result.GetProperty("code").GetString() ?? "bridge.response.invalid",
            result.GetProperty("message").GetString() ?? "The backend returned no bridge command message.",
            BridgeSnapshotParser.Parse(result.GetProperty("snapshot")),
            createdEntity,
            graphSchema,
            envelopePayload,
            liveDebugTracePayload,
            contentImportJobId);
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

    private static object SerializeMotionQueryVector3(BridgeMotionQueryVector3 value) =>
        new { x = value.X, y = value.Y, z = value.Z };

    private static object SerializeMotionQueryQuaternion(BridgeMotionQueryQuaternion value) =>
        new { x = value.X, y = value.Y, z = value.Z, w = value.W };

    private static object SerializeMotionQueryTransform(BridgeMotionQueryTransform value) =>
        new
        {
            position = SerializeMotionQueryVector3(value.Position),
            rotation = SerializeMotionQueryQuaternion(value.Rotation),
            scale = SerializeMotionQueryVector3(value.Scale),
        };

    private static object SerializeMotionQueryTimeState(BridgeMotionQueryTimeState value) =>
        new
        {
            frameNumber = value.FrameNumber,
            realTimeSeconds = value.RealTimeSeconds,
            gameTimeSeconds = value.GameTimeSeconds,
            fixedTimeSeconds = value.FixedTimeSeconds,
            animationTimeSeconds = value.AnimationTimeSeconds,
            fixedAccumulatorSeconds = value.FixedAccumulatorSeconds,
            realDeltaSeconds = value.RealDeltaSeconds,
            gameDeltaSeconds = value.GameDeltaSeconds,
            animationDeltaSeconds = value.AnimationDeltaSeconds,
            paused = value.Paused,
        };

    private static object SerializeMotionQueryFeature(BridgeMotionQueryFeature value) =>
        new
        {
            rootVelocity = SerializeMotionQueryVector3(value.RootVelocity),
            facingDirection = SerializeMotionQueryVector3(value.FacingDirection),
            trajectory = value.Trajectory.Select(sample => new
            {
                offsetSeconds = sample.OffsetSeconds,
                relativePosition = SerializeMotionQueryVector3(sample.RelativePosition),
            }).ToArray(),
            skeleton = new
            {
                skeletonId = value.Skeleton.SkeletonId,
                joints = value.Skeleton.Joints.Select(joint => new
                {
                    jointId = joint.JointId,
                    parentJointId = joint.ParentJointId,
                }).ToArray(),
            },
            pose = new
            {
                skeletonId = value.Pose.SkeletonId,
                localJoints = value.Pose.LocalJoints.Select(SerializeMotionQueryTransform).ToArray(),
            },
            evaluationContext = new
            {
                time = SerializeMotionQueryTimeState(value.EvaluationContext.Time),
                sampleTimeSeconds = value.EvaluationContext.SampleTimeSeconds,
            },
        };

    private static object SerializeMotionQueryCandidate(BridgeMotionQueryCandidate value) =>
        new
        {
            candidateId = value.CandidateId,
            sourceClipId = value.SourceClipId,
            sampleTimeSeconds = value.SampleTimeSeconds,
            feature = SerializeMotionQueryFeature(value.Feature),
        };

    private static object? SerializeOptionalMotionQueryCandidate(BridgeMotionQueryCandidate? value) =>
        value is null ? null : SerializeMotionQueryCandidate(value);

    private static object? SerializeMotionQueryDatabase(BridgeMotionQueryDatabaseEntry? value) =>
        value is null
            ? null
            : new
            {
                resource = new { guid = value.Resource.Guid, generation = value.Resource.Generation },
                displayName = value.DisplayName,
                dirty = value.Dirty,
                contract = new
                {
                    context = new
                    {
                        databaseId = value.Contract.Context.DatabaseId,
                        generation = value.Contract.Context.Generation,
                    },
                    schema = new
                    {
                        version = value.Contract.Schema.Version,
                        schemaId = value.Contract.Schema.SchemaId,
                        trajectoryOffsets = value.Contract.Schema.TrajectoryOffsets,
                        featureChannelIds = value.Contract.Schema.FeatureChannelIds,
                    },
                    settings = new
                    {
                        maximumCandidates = value.Contract.Settings.MaximumCandidates,
                        requireTrajectorySchema = value.Contract.Settings.RequireTrajectorySchema,
                    },
                    candidates = value.Contract.Candidates.Select(SerializeMotionQueryCandidate).ToArray(),
                    events = value.Contract.Events.Select(entry => new
                    {
                        kind = entry.Kind,
                        sequence = entry.Sequence,
                        candidateId = entry.CandidateId,
                        message = entry.Message,
                    }).ToArray(),
                },
            };

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
        JsonDocument response;
        try
        {
            response = JsonDocument.Parse(responseBody);
        }
        catch (JsonException exception)
        {
            throw new BridgeProtocolException("bridge.transport.json.invalid",
                $"The backend returned invalid JSON: {exception.Message}");
        }
        if (response.RootElement.ValueKind != JsonValueKind.Object ||
            !response.RootElement.TryGetProperty("jsonrpc", out JsonElement jsonRpcVersion) ||
            jsonRpcVersion.ValueKind != JsonValueKind.String || jsonRpcVersion.GetString() != "2.0")
        {
            response.Dispose();
            throw new BridgeProtocolException("bridge.transport.response.invalid",
                "The backend returned an invalid JSON-RPC response envelope.");
        }
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
        byte[] header = await ReadExactlyAsync(sizeof(uint), FrameReadStage.Header, cancellationToken).ConfigureAwait(false);
        uint length = BinaryPrimitives.ReadUInt32BigEndian(header);
        if (length == 0)
        {
            throw new BridgeProtocolException("bridge.transport.frame.zero_length",
                "The backend returned a zero-length response frame.");
        }
        if (length > MaximumFrameBytes)
        {
            throw new BridgeProtocolException("bridge.transport.frame.oversized",
                "The backend returned a response frame larger than the supported bound.");
        }
        return await ReadExactlyAsync(checked((int)length), FrameReadStage.Body, cancellationToken).ConfigureAwait(false);
    }

    private async Task<byte[]> ReadExactlyAsync(int length, FrameReadStage stage, CancellationToken cancellationToken)
    {
        byte[] buffer = new byte[length];
        int total = 0;
        while (total < buffer.Length)
        {
            int count = await input.ReadAsync(buffer.AsMemory(total), cancellationToken).ConfigureAwait(false);
            if (count == 0)
            {
                if (total == 0 && stage == FrameReadStage.Header)
                {
                    throw new BridgeProtocolException("bridge.transport.eof",
                        "The backend process closed its protocol stream before sending a response.");
                }

                string code = stage == FrameReadStage.Header
                    ? "bridge.transport.frame.truncated_header"
                    : "bridge.transport.frame.truncated_body";
                throw new BridgeProtocolException(code,
                    "The backend process closed its protocol stream before a complete response frame was received.");
            }
            total += count;
        }
        return buffer;
    }

    private static BridgeEntityRef ParseEntityRef(JsonElement value)
    {
        if (value.ValueKind != JsonValueKind.Object)
        {
            throw new BridgeProtocolException("bridge.transport.response.invalid",
                "The backend returned an invalid created entity identity.");
        }
        try
        {
            return new BridgeEntityRef(value.GetProperty("index").GetUInt32(), value.GetProperty("generation").GetUInt32());
        }
        catch (Exception exception) when (exception is KeyNotFoundException or InvalidOperationException or FormatException)
        {
            throw new BridgeProtocolException("bridge.transport.response.invalid",
                $"The backend returned an invalid created entity identity: {exception.Message}");
        }
    }

    private static JsonElement GetResultOrThrow(JsonElement response)
    {
        bool hasError = response.TryGetProperty("error", out JsonElement error);
        bool hasResult = response.TryGetProperty("result", out JsonElement result);
        if (hasError == hasResult)
        {
            throw new BridgeProtocolException("bridge.transport.response.invalid",
                "The backend response must contain exactly one result or error payload.");
        }
        if (hasError)
        {
            if (error.ValueKind != JsonValueKind.Object ||
                !error.TryGetProperty("message", out JsonElement messageValue) ||
                messageValue.ValueKind != JsonValueKind.String)
            {
                throw new BridgeProtocolException("bridge.transport.response.invalid",
                    "The backend returned an invalid JSON-RPC error payload.");
            }
            string code = error.TryGetProperty("data", out JsonElement data) &&
                          data.ValueKind == JsonValueKind.Object &&
                          data.TryGetProperty("code", out JsonElement codeValue) &&
                          codeValue.ValueKind == JsonValueKind.String
                ? codeValue.GetString() ?? "bridge.request.invalid"
                : "bridge.request.invalid";
            throw new BridgeProtocolException(code,
                messageValue.GetString() ?? "The backend rejected the bridge request.");
        }
        return result;
    }
}

public sealed record BridgeHelloResult(bool Compatible, uint BackendProtocolVersion, string Code, BridgeEditorSnapshot Snapshot);

public sealed class BridgeProtocolException : Exception
{
    public BridgeProtocolException(string code, string message) : base(message)
    {
        Code = code;
    }

    public string Code { get; }
}
