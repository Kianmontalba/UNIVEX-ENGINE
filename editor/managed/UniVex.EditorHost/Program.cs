// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

using Avalonia;

namespace UniVex.EditorHost;

internal static class Program
{
    [STAThread]
    public static int Main(string[] args)
    {
        if (args.Length == 2 && args[0] == "--probe")
        {
            return ProbeBackendAsync(args[1]).GetAwaiter().GetResult();
        }
        BuildAvaloniaApp().StartWithClassicDesktopLifetime(args);
        return 0;
    }

    public static AppBuilder BuildAvaloniaApp() => AppBuilder.Configure<App>()
        .UsePlatformDetect()
        .WithInterFont()
        .LogToTrace();

    private static async Task<int> ProbeBackendAsync(string backendExecutablePath)
    {
        try
        {
            await using BridgeBackendSession session = await BridgeBackendSession.StartAsync(
                backendExecutablePath, scenePath: null, CancellationToken.None).ConfigureAwait(false);
            BridgeEditorSnapshot initial = await session.RefreshSnapshotAsync(CancellationToken.None).ConfigureAwait(false);

            // Exercises a real bridge.dispatch round trip against the native backend: the Motion
            // Query authoring command this probe used to exercise no longer exists on the native
            // side (the Animation plugin, including its bridge dispatch surface, was removed), so
            // this instead dispatches the always-available clearSelection command to prove the
            // process spawn, JSON-RPC handshake, and typed BridgeCommand/BridgeCommandResult
            // marshaling still round-trip correctly through the real executable.
            BridgeCommandResult cleared = await session.DispatchAsync(
                new BridgeCommand(initial.Revision, "clearSelection"), CancellationToken.None)
                .ConfigureAwait(false);
            if (!cleared.Applied)
            {
                throw new BridgeProtocolException("bridge.probe.clear_selection.invalid",
                    $"The native bridge did not apply the clearSelection probe command: {cleared.Code}: {cleared.Message}");
            }
            return 0;
        }
        catch (BridgeProtocolException exception)
        {
            await Console.Error.WriteLineAsync($"{exception.Code}: {exception.Message}").ConfigureAwait(false);
            return 2;
        }
        catch (Exception exception)
        {
            await Console.Error.WriteLineAsync($"bridge.host.probe.failed: {exception.Message}").ConfigureAwait(false);
            return 3;
        }
    }
}
