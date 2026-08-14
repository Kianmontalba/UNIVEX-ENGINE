// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

using System.Text.Json;
using System.Threading;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Threading;

namespace UniVex.EditorHost;

public partial class MainWindow : Window
{
    private readonly SemaphoreSlim lifecycleGate = new(1, 1);
    private readonly DockShellPreferencesStore layoutStore;
    private DockShellLayoutSession layoutSession = new(DockShellLayoutState.Default);
    private BridgeBackendSession? session;
    private HostSessionState state = HostSessionState.Disconnected;
    private bool closeConfirmed;
    private bool applyingLayout;
    private bool shellInitialized;

    public MainWindow()
    {
        InitializeComponent();
        shellInitialized = true;
        layoutStore = new DockShellPreferencesStore(BuildLayoutPath());
        BackendPathTextBox.Text = Environment.GetEnvironmentVariable("UVE_EDITOR_BACKEND") ?? "uve_editor";
        DetailsTextBlock.Text = "Choose the native uve_editor executable, then launch a separate --bridge-stdio headless backend.";
        Opened += MainWindow_OnOpened;
        Closing += MainWindow_OnClosing;
        Closed += MainWindow_OnClosed;
        RenderShellLayout(layoutSession.CurrentLayout);
        UpdateShellPresentation();
    }

    private async void MainWindow_OnOpened(object? sender, EventArgs e)
    {
        DockShellLayoutState loaded = await layoutStore.LoadAsync(CancellationToken.None).ConfigureAwait(true);
        layoutSession = new DockShellLayoutSession(loaded);
        RenderShellLayout(layoutSession.CurrentLayout);
        UpdateShellPresentation();
    }

    private async void SaveShellLayoutButton_OnClick(object? sender, RoutedEventArgs e)
    {
        try
        {
            await layoutStore.SaveAsync(layoutSession.CurrentLayout, CancellationToken.None).ConfigureAwait(true);
            layoutSession.MarkSaved();
            UpdateShellPresentation();
        }
        catch (Exception exception) when (exception is IOException or UnauthorizedAccessException or InvalidOperationException)
        {
            StatusTextBlock.Text = "Shell layout was not saved.";
            DetailsTextBlock.Text = exception.Message;
        }
    }

    private void ToggleLeftDockButton_OnClick(object? sender, RoutedEventArgs e)
    {
        UpdateShellLayout(layoutSession.CurrentLayout with
        {
            IsLeftDockVisible = !layoutSession.CurrentLayout.IsLeftDockVisible,
        });
    }

    private void ToggleRightDockButton_OnClick(object? sender, RoutedEventArgs e)
    {
        UpdateShellLayout(layoutSession.CurrentLayout with
        {
            IsRightDockVisible = !layoutSession.CurrentLayout.IsRightDockVisible,
        });
    }

    private void ToggleBottomDockButton_OnClick(object? sender, RoutedEventArgs e)
    {
        UpdateShellLayout(layoutSession.CurrentLayout with
        {
            IsBottomDockVisible = !layoutSession.CurrentLayout.IsBottomDockVisible,
        });
    }

    private void WorkspaceButton_OnClick(object? sender, RoutedEventArgs e)
    {
        if (sender is not Button { Tag: string workspaceText } ||
            !Enum.TryParse(workspaceText, ignoreCase: false, out DockShellWorkspace workspace))
        {
            return;
        }
        UpdateShellLayout(layoutSession.CurrentLayout with { ActiveWorkspace = workspace });
    }

    private void DockTabControl_OnSelectionChanged(object? sender, SelectionChangedEventArgs e)
    {
        if (!shellInitialized || applyingLayout || sender is not TabControl tabControl || tabControl.SelectedItem is not TabItem tabItem ||
            tabItem.Header is not string tabName)
        {
            return;
        }

        DockShellLayoutState current = layoutSession.CurrentLayout;
        DockShellLayoutState updated = ReferenceEquals(tabControl, LeftTabControl)
            ? current with { LeftTab = tabName }
            : ReferenceEquals(tabControl, CenterTabControl)
                ? current with { CenterTab = tabName }
                : ReferenceEquals(tabControl, RightTabControl)
                    ? current with { RightTab = tabName }
                    : ReferenceEquals(tabControl, BottomTabControl)
                        ? current with { BottomTab = tabName }
                        : current;
        UpdateShellLayout(updated);
    }

    private void DockSplitter_OnPointerReleased(object? sender, PointerReleasedEventArgs e)
    {
        if (!shellInitialized || applyingLayout)
        {
            return;
        }

        UpdateShellLayout(layoutSession.CurrentLayout with
        {
            LeftDockWidth = ShellGrid.ColumnDefinitions[0].Width.Value,
            RightDockWidth = ShellGrid.ColumnDefinitions[4].Width.Value,
            BottomDockHeight = ShellGrid.RowDefinitions[2].Height.Value,
        });
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
        UpdateShellPresentation();
    }

    private async void ConfirmWarningButton_OnClick(object? sender, RoutedEventArgs e)
    {
        if (state == HostSessionState.ConfirmFreshSession)
        {
            WarningBorder.IsVisible = false;
            await DisposeSessionAsync().ConfigureAwait(true);
            state = HostSessionState.Disconnected;
            UpdateShellPresentation();
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
            SetControlsEnabled(true);
            RefreshButton.IsEnabled = true;
        }
        UpdateShellPresentation();
    }

    private async Task StartBackendAsync()
    {
        if (!SessionLossPolicy.CanStartBackend(state, session is not null))
        {
            return;
        }

        await lifecycleGate.WaitAsync().ConfigureAwait(true);
        try
        {
            if (!SessionLossPolicy.CanStartBackend(state, session is not null))
            {
                return;
            }

            string executablePath = BackendPathTextBox.Text?.Trim() ?? string.Empty;
            string? scenePath = string.IsNullOrWhiteSpace(ScenePathTextBox.Text) ? null : ScenePathTextBox.Text.Trim();
            state = HostSessionState.Connecting;
            SetControlsEnabled(false);
            StatusTextBlock.Text = "Connecting to a separate headless bridge backend…";
            DetailsTextBlock.Text = executablePath;
            UpdateShellPresentation();

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
        finally
        {
            lifecycleGate.Release();
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
        UpdateShellPresentation();
    }

    private void EnterFailure(string code, string detail)
    {
        if (SessionLossPolicy.IsTerminalFailureState(state))
        {
            return;
        }

        state = HostSessionState.Failed;
        WarningBorder.IsVisible = false;
        bool requiresFreshSessionAcknowledgement = SessionLossPolicy.RequiresFreshSessionAcknowledgement(session is not null);
        SetControlsEnabled(!requiresFreshSessionAcknowledgement);
        RefreshButton.IsEnabled = false;
        StartNewSessionButton.IsEnabled = requiresFreshSessionAcknowledgement;
        StatusTextBlock.Text = $"Backend unavailable — {code}";
        DetailsTextBlock.Text = detail;
        UpdateShellPresentation();
    }

    private void SetControlsEnabled(bool enabled)
    {
        ConnectButton.IsEnabled = enabled && SessionLossPolicy.CanStartBackend(state, session is not null);
        BackendPathTextBox.IsEnabled = enabled;
        ScenePathTextBox.IsEnabled = enabled;
    }

    private void UpdateShellLayout(DockShellLayoutState layout)
    {
        layoutSession.Apply(layout);
        RenderShellLayout(layoutSession.CurrentLayout);
        UpdateShellPresentation();
    }

    private void RenderShellLayout(DockShellLayoutState layout)
    {
        applyingLayout = true;
        try
        {
            LeftDockBorder.IsVisible = layout.IsLeftDockVisible;
            LeftDockSplitter.IsVisible = layout.IsLeftDockVisible;
            ShellGrid.ColumnDefinitions[0].Width = layout.IsLeftDockVisible
                ? new GridLength(layout.LeftDockWidth)
                : new GridLength(0.0);

            RightDockBorder.IsVisible = layout.IsRightDockVisible;
            RightDockSplitter.IsVisible = layout.IsRightDockVisible;
            ShellGrid.ColumnDefinitions[4].Width = layout.IsRightDockVisible
                ? new GridLength(layout.RightDockWidth)
                : new GridLength(0.0);

            BottomDockBorder.IsVisible = layout.IsBottomDockVisible;
            BottomDockSplitter.IsVisible = layout.IsBottomDockVisible;
            ShellGrid.RowDefinitions[2].Height = layout.IsBottomDockVisible
                ? new GridLength(layout.BottomDockHeight)
                : new GridLength(0.0);

            SelectTab(LeftTabControl, layout.LeftTab);
            SelectTab(CenterTabControl, layout.CenterTab);
            SelectTab(RightTabControl, layout.RightTab);
            SelectTab(BottomTabControl, layout.BottomTab);
        }
        finally
        {
            applyingLayout = false;
        }
    }

    private void UpdateShellPresentation()
    {
        WorkspaceTextBlock.Text = $"Active: {layoutSession.CurrentLayout.ActiveWorkspace}";
        LayoutDirtyTextBlock.Text = layoutSession.IsDirty ? "Layout has unsaved changes" : "Layout saved";
        ConnectionIndicatorTextBlock.Text = state switch
        {
            HostSessionState.Connected => "BACKEND CONNECTED",
            HostSessionState.Connecting => "BACKEND CONNECTING",
            HostSessionState.Failed or HostSessionState.ConfirmFreshSession => "BACKEND UNAVAILABLE",
            HostSessionState.ConfirmDiscardDirtySession => "BACKEND CLOSE CONFIRMATION",
            _ => "BACKEND OFFLINE",
        };
    }

    private static void SelectTab(TabControl tabControl, string tabName)
    {
        for (int index = 0; index < tabControl.Items.Count; ++index)
        {
            if (tabControl.Items[index] is TabItem { Header: string header } && header == tabName)
            {
                tabControl.SelectedIndex = index;
                return;
            }
        }
        tabControl.SelectedIndex = 0;
    }

    private static string BuildLayoutPath()
    {
        string root = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
        return Path.Combine(root, "UniVex", "editor_host_layout.json");
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
        UpdateShellPresentation();
    }

    private async void MainWindow_OnClosed(object? sender, EventArgs e)
    {
        // Layout is intentionally not written here. Increment 71 uses explicit Save Shell Layout only.
        await DisposeSessionAsync().ConfigureAwait(false);
    }

    private async Task DisposeSessionAsync()
    {
        await lifecycleGate.WaitAsync().ConfigureAwait(true);
        try
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
        finally
        {
            lifecycleGate.Release();
        }
    }
}
