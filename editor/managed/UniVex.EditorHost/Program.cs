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
            _ = await session.RefreshSnapshotAsync(CancellationToken.None).ConfigureAwait(false);
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
