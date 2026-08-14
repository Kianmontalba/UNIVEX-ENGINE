// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

using System.Diagnostics;
using System.Text;
using System.Text.Json;

namespace UniVex.EditorHost;

/// <summary>
/// Owns one headless C++ bridge child. A new instance always means a new native editor session;
/// callers must obtain explicit user acknowledgement before replacing a failed session.
/// </summary>
public sealed class BridgeBackendSession : IAsyncDisposable
{
    private readonly Process process;
    private readonly BridgeProtocolClient client;
    private Task stderrDrain = Task.CompletedTask;
    private readonly StringBuilder stderr = new();
    private bool disposed;

    private BridgeBackendSession(Process process, BridgeProtocolClient client)
    {
        this.process = process;
        this.client = client;
    }

    public event EventHandler? Exited;

    public bool HasExited => process.HasExited;

    public int? ExitCode => process.HasExited ? process.ExitCode : null;

    public string StandardError => stderr.ToString();

    public JsonElement? LastSnapshot { get; private set; }

    public static async Task<BridgeBackendSession> StartAsync(
        string executablePath,
        string? scenePath,
        CancellationToken cancellationToken)
    {
        if (string.IsNullOrWhiteSpace(executablePath))
        {
            throw new ArgumentException("A native uve_editor executable path is required.", nameof(executablePath));
        }

        ProcessStartInfo startInfo = new(executablePath)
        {
            UseShellExecute = false,
            RedirectStandardInput = true,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            CreateNoWindow = true,
        };
        startInfo.ArgumentList.Add("--bridge-stdio");
        if (!string.IsNullOrWhiteSpace(scenePath))
        {
            startInfo.ArgumentList.Add("--scene");
            startInfo.ArgumentList.Add(scenePath);
        }

        Process process = new() { StartInfo = startInfo, EnableRaisingEvents = true };
        if (!process.Start())
        {
            process.Dispose();
            throw new BridgeProtocolException("bridge.backend.launch.failed", "The native bridge backend did not start.");
        }

        BridgeProtocolClient client = new(process.StandardOutput.BaseStream, process.StandardInput.BaseStream);
        BridgeBackendSession session = new(process, client);
        session.process.Exited += (_, _) => session.Exited?.Invoke(session, EventArgs.Empty);
        session.stderrDrain = session.DrainStandardErrorAsync();

        try
        {
            BridgeHelloResult hello = await session.client.HelloAsync(cancellationToken).ConfigureAwait(false);
            session.LastSnapshot = hello.Snapshot;
            if (!hello.Compatible || hello.BackendProtocolVersion != BridgeProtocolClient.ProtocolVersion)
            {
                throw new BridgeProtocolException("bridge.protocol.unsupported",
                    $"The backend reports protocol {hello.BackendProtocolVersion}, but this host requires {BridgeProtocolClient.ProtocolVersion}.");
            }
            return session;
        }
        catch
        {
            await session.DisposeAsync().ConfigureAwait(false);
            throw;
        }
    }

    public async Task<JsonElement> RefreshSnapshotAsync(CancellationToken cancellationToken)
    {
        JsonElement snapshot = await client.GetSnapshotAsync(cancellationToken).ConfigureAwait(false);
        LastSnapshot = snapshot;
        return snapshot;
    }

    public bool LastKnownSceneDirty()
    {
        return LastSnapshot.HasValue && LastSnapshot.Value.TryGetProperty("sceneDirty", out JsonElement dirty) &&
               dirty.ValueKind == JsonValueKind.True;
    }

    public async ValueTask DisposeAsync()
    {
        if (disposed)
        {
            return;
        }
        disposed = true;

        await client.DisposeAsync().ConfigureAwait(false);
        if (!process.HasExited)
        {
            using CancellationTokenSource timeout = new(TimeSpan.FromSeconds(2));
            try
            {
                await process.WaitForExitAsync(timeout.Token).ConfigureAwait(false);
            }
            catch (OperationCanceledException)
            {
                process.Kill(entireProcessTree: true);
                await process.WaitForExitAsync().ConfigureAwait(false);
            }
        }
        await stderrDrain.ConfigureAwait(false);
        process.Dispose();
    }

    private async Task DrainStandardErrorAsync()
    {
        char[] buffer = new char[1024];
        while (true)
        {
            int count = await process.StandardError.ReadAsync(buffer).ConfigureAwait(false);
            if (count == 0)
            {
                return;
            }
            lock (stderr)
            {
                stderr.Append(buffer, 0, count);
            }
        }
    }
}
