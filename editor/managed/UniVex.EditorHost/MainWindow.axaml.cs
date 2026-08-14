// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

using System.Text.Json;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Threading;

namespace UniVex.EditorHost;

public partial class MainWindow : Window
{
    private BridgeBackendSession? session;
    private HostSessionState state = HostSessionState.Disconnected;
    private bool closeConfirmed;

    public MainWindow()
    {
        InitializeComponent();
        BackendPathTextBox.Text = Environment.GetEnvironmentVariable("UVE_EDITOR_BACKEND") ?? "uve_editor";
        DetailsTextBlock.Text = "Choose the native uve_editor executable, then launch a separate --bridge-stdio headless backend.";
        Closing += MainWindow_OnClosing;
        Closed += MainWindow_OnClosed;
    }

    private async void ConnectButton_OnClick(object? sender, RoutedEventArgs e)
    {
        await StartBackendAsync().ConfigureAwait(true);
    }

    private async void RefreshButton_OnClick(object? sender, RoutedEventArgs e)
    {
        if (session is null || state != HostSessionState.Connected)
        {
            return;
        }

        try
        {
            JsonElement snapshot = await session.RefreshSnapshotAsync(CancellationToken.None).ConfigureAwait(true);
            DisplayConnectedSnapshot(snapshot);
        }
        catch (Exception exception)
        {
            EnterFailure("bridge.snapshot.failed", exception.Message);
        }
    }

    private void StartNewSessionButton_OnClick(object? sender, RoutedEventArgs e)
    {
        if (state != HostSessionState.Failed)
        {
            return;
        }

        state = HostSessionState.ConfirmFreshSession;
        WarningTextBlock.Text = SessionLossPolicy.DescribeFreshSessionLoss(LastKnownDirtyEvidence());
        WarningBorder.IsVisible = true;
        StartNewSessionButton.IsEnabled = false;
    }

    private async void ConfirmWarningButton_OnClick(object? sender, RoutedEventArgs e)
    {
        if (state == HostSessionState.ConfirmFreshSession)
        {
            WarningBorder.IsVisible = false;
            await DisposeSessionAsync().ConfigureAwait(true);
            state = HostSessionState.Disconnected;
            await StartBackendAsync().ConfigureAwait(true);
            return;
        }

        if (state == HostSessionState.ConfirmDiscardDirtySession)
        {
            closeConfirmed = true;
            await DisposeSessionAsync().ConfigureAwait(true);
            Close();
        }
    }

    private void CancelWarningButton_OnClick(object? sender, RoutedEventArgs e)
    {
        WarningBorder.IsVisible = false;
        if (state == HostSessionState.ConfirmFreshSession)
        {
            state = HostSessionState.Failed;
            StartNewSessionButton.IsEnabled = true;
        }
        else if (state == HostSessionState.ConfirmDiscardDirtySession)
        {
            state = HostSessionState.Connected;
        }
    }

    private async Task StartBackendAsync()
    {
        if (state == HostSessionState.Connecting)
        {
            return;
        }

        string executablePath = BackendPathTextBox.Text?.Trim() ?? string.Empty;
        string? scenePath = string.IsNullOrWhiteSpace(ScenePathTextBox.Text) ? null : ScenePathTextBox.Text.Trim();
        state = HostSessionState.Connecting;
        SetControlsEnabled(false);
        StatusTextBlock.Text = "Connecting to a separate headless bridge backend…";
        DetailsTextBlock.Text = executablePath;

        using CancellationTokenSource timeout = new(TimeSpan.FromSeconds(10));
        try
        {
            BridgeBackendSession started = await BridgeBackendSession.StartAsync(executablePath, scenePath, timeout.Token)
                .ConfigureAwait(true);
            session = started;
            session.Exited += Session_OnExited;
            DisplayConnectedSnapshot(started.LastSnapshot!.Value);
        }
        catch (BridgeProtocolException exception)
        {
            EnterFailure(exception.Code, exception.Message);
        }
        catch (OperationCanceledException)
        {
            EnterFailure("bridge.backend.timeout", "The backend did not complete the protocol handshake within 10 seconds.");
        }
        catch (Exception exception)
        {
            EnterFailure("bridge.backend.launch.failed", exception.Message);
        }
    }

    private void Session_OnExited(object? sender, EventArgs e)
    {
        Dispatcher.UIThread.Post(() =>
        {
            if (ReferenceEquals(sender, session))
            {
                string? details = session?.StandardError;
                EnterFailure("bridge.backend.exited", string.IsNullOrWhiteSpace(details)
                    ? "The headless backend process exited or closed its protocol stream."
                    : details.Trim());
            }
        });
    }

    private void DisplayConnectedSnapshot(JsonElement snapshot)
    {
        state = HostSessionState.Connected;
        WarningBorder.IsVisible = false;
        SetControlsEnabled(true);
        RefreshButton.IsEnabled = true;
        StartNewSessionButton.IsEnabled = false;
        StatusTextBlock.Text = "Connected to a headless C++ bridge backend.";
        DetailsTextBlock.Text = DescribeSnapshot(snapshot);
    }

    private void EnterFailure(string code, string detail)
    {
        state = HostSessionState.Failed;
        WarningBorder.IsVisible = false;
        bool requiresFreshSessionAcknowledgement = SessionLossPolicy.RequiresFreshSessionAcknowledgement(session is not null);
        SetControlsEnabled(!requiresFreshSessionAcknowledgement);
        RefreshButton.IsEnabled = false;
        StartNewSessionButton.IsEnabled = requiresFreshSessionAcknowledgement;
        StatusTextBlock.Text = $"Backend unavailable — {code}";
        DetailsTextBlock.Text = detail;
    }

    private void SetControlsEnabled(bool enabled)
    {
        ConnectButton.IsEnabled = enabled;
        BackendPathTextBox.IsEnabled = enabled;
        ScenePathTextBox.IsEnabled = enabled;
    }

    private bool? LastKnownDirtyEvidence()
    {
        if (session is null || session.LastSnapshot is null)
        {
            return null;
        }
        return session.LastKnownSceneDirty();
    }

    private static string DescribeSnapshot(JsonElement snapshot)
    {
        uint protocolVersion = snapshot.GetProperty("protocolVersion").GetUInt32();
        ulong revision = snapshot.GetProperty("revision").GetUInt64();
        bool dirty = snapshot.GetProperty("sceneDirty").GetBoolean();
        int selected = snapshot.GetProperty("selectedEntities").GetArrayLength();
        int capabilities = snapshot.GetProperty("capabilities").GetArrayLength();
        string scenePath = snapshot.GetProperty("activeScenePath").GetString() ?? "";
        return $"Protocol {protocolVersion}; revision {revision}; dirty={dirty}; selected entities={selected}; " +
               $"advertised capabilities={capabilities}; scene='{scenePath}'.";
    }

    private void MainWindow_OnClosing(object? sender, WindowClosingEventArgs e)
    {
        if (closeConfirmed || session is null || state != HostSessionState.Connected || !session.LastKnownSceneDirty())
        {
            return;
        }

        e.Cancel = true;
        state = HostSessionState.ConfirmDiscardDirtySession;
        WarningTextBlock.Text = SessionLossPolicy.DescribeDirtyCloseLoss();
        WarningBorder.IsVisible = true;
        SetControlsEnabled(false);
    }

    private async void MainWindow_OnClosed(object? sender, EventArgs e)
    {
        await DisposeSessionAsync().ConfigureAwait(false);
    }

    private async Task DisposeSessionAsync()
    {
        if (session is null)
        {
            return;
        }

        BridgeBackendSession disposing = session;
        session = null;
        disposing.Exited -= Session_OnExited;
        await disposing.DisposeAsync().ConfigureAwait(true);
    }
}
