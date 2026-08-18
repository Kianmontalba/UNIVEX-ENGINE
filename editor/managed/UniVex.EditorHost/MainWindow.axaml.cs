// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.Primitives;
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
    private BridgeHierarchyEntry? selectedHierarchyEntry;
    private BridgeContentBrowserEntry? selectedContentEntry;
    private BridgeDataTableCatalogEntry? selectedDataTableEntry;
    private BridgeScriptRuntimeInstanceEntry? selectedScriptRuntimeInstance;
    private IReadOnlyList<BridgeVisualScriptPaletteEntry> visualScriptPalette = Array.Empty<BridgeVisualScriptPaletteEntry>();
    private BridgeVisualScriptNode? selectedVisualScriptNode;
    private BridgeVisualScriptPin? selectedVisualScriptPin;
    private bool closeConfirmed;
    private bool applyingLayout;
    private bool applyingSnapshot;
    private bool scriptRuntimeRequestAvailable;
    private bool scriptRuntimeTickAvailable;
    private BridgeScriptRuntimeSnapshot? scriptRuntimeSnapshot;
    private bool shellInitialized;
    private Popup? visualScriptNodeSearchPopup;
    private TextBox? visualScriptNodeSearchTextBox;
    private ListBox? visualScriptNodeSearchListBox;
    private BridgeVisualScriptPoint visualScriptNodeSearchPosition = new(0F, 0F);
    private bool applyingVisualScriptNodeSearchResults;

    public MainWindow()
    {
        InitializeComponent();
        VisualScriptCanvas.CommandRequested += VisualScriptCanvas_OnCommandRequested;
        VisualScriptCanvas.NodeSearchRequested += VisualScriptCanvas_OnNodeSearchRequested;
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

    private void ToggleLeftDockButton_OnClick(object? sender, RoutedEventArgs e) =>
        UpdateShellLayout(layoutSession.CurrentLayout with { IsLeftDockVisible = !layoutSession.CurrentLayout.IsLeftDockVisible });

    private void ToggleRightDockButton_OnClick(object? sender, RoutedEventArgs e) =>
        UpdateShellLayout(layoutSession.CurrentLayout with { IsRightDockVisible = !layoutSession.CurrentLayout.IsRightDockVisible });

    private void ToggleBottomDockButton_OnClick(object? sender, RoutedEventArgs e) =>
        UpdateShellLayout(layoutSession.CurrentLayout with { IsBottomDockVisible = !layoutSession.CurrentLayout.IsBottomDockVisible });

    private void VisualScriptCanvas_OnCommandRequested(object? sender, VisualScriptCanvasCommandEventArgs e) =>
        DispatchCurrentCommand(e.Command);

    private void VisualScriptCanvas_OnNodeSearchRequested(object? sender, VisualScriptNodeSearchRequestedEventArgs e)
    {
        visualScriptNodeSearchPosition = VisualScriptCanvas.ScreenToCanvasPoint(e.Position);
        OpenVisualScriptNodeSearchPopup();
    }

    private void OpenVisualScriptNodeSearchPopup()
    {
        CloseVisualScriptNodeSearchPopup();

        TextBox searchBox = new()
        {
            Width = 300D,
            Watermark = "Search nodes…",
            FontSize = 13D,
            Margin = new Avalonia.Thickness(8D),
        };
        ListBox results = new()
        {
            Width = 300D,
            MaxHeight = 280D,
            SelectionMode = SelectionMode.Single,
            Margin = new Avalonia.Thickness(8D, 0D, 8D, 8D),
        };
        StackPanel content = new()
        {
            Spacing = 0D,
            Background = Avalonia.Media.Brushes.Transparent,
            Children = { searchBox, results },
        };
        Border surface = new()
        {
            Child = content,
            Background = Avalonia.Media.Brushes.Black,
            BorderBrush = Avalonia.Media.Brushes.Gray,
            BorderThickness = new Avalonia.Thickness(1D),
            CornerRadius = new Avalonia.CornerRadius(4D),
            Padding = new Avalonia.Thickness(1D),
        };
        Popup popup = new()
        {
            PlacementTarget = VisualScriptCanvas,
            Placement = PlacementMode.Pointer,
            IsLightDismissEnabled = true,
            Child = surface,
        };

        visualScriptNodeSearchPopup = popup;
        visualScriptNodeSearchTextBox = searchBox;
        visualScriptNodeSearchListBox = results;
        searchBox.TextChanged += VisualScriptNodeSearchTextBox_OnTextChanged;
        searchBox.KeyDown += VisualScriptNodeSearchTextBox_OnKeyDown;
        results.SelectionChanged += VisualScriptNodeSearchListBox_OnSelectionChanged;
        popup.Closed += VisualScriptNodeSearchPopup_OnClosed;
        UpdateVisualScriptNodeSearchResults();
        popup.IsOpen = true;
        searchBox.Focus();
    }

    private void VisualScriptNodeSearchTextBox_OnTextChanged(object? sender, TextChangedEventArgs e) =>
        UpdateVisualScriptNodeSearchResults();

    private void UpdateVisualScriptNodeSearchResults()
    {
        if (visualScriptNodeSearchListBox is null)
        {
            return;
        }
        applyingVisualScriptNodeSearchResults = true;
        try
        {
            IReadOnlyList<BridgeVisualScriptPaletteEntry> matches = VisualScriptCanvasControl.FilterPalette(
                visualScriptPalette, visualScriptNodeSearchTextBox?.Text);
            visualScriptNodeSearchListBox.ItemsSource = matches;
            visualScriptNodeSearchListBox.SelectedIndex = matches.Count == 0 ? -1 : 0;
        }
        finally
        {
            applyingVisualScriptNodeSearchResults = false;
        }
    }

    private void VisualScriptNodeSearchTextBox_OnKeyDown(object? sender, KeyEventArgs e)
    {
        if (e.Key == Key.Escape)
        {
            CloseVisualScriptNodeSearchPopup();
            e.Handled = true;
        }
        else if (e.Key is Key.Down or Key.Up && visualScriptNodeSearchListBox is { } results &&
                 results.ItemCount > 0)
        {
            int delta = e.Key == Key.Down ? 1 : -1;
            int nextIndex = results.SelectedIndex < 0 ? 0 : results.SelectedIndex + delta;
            results.SelectedIndex = Math.Clamp(nextIndex, 0, results.ItemCount - 1);
            e.Handled = true;
        }
        else if (e.Key == Key.Enter)
        {
            CommitVisualScriptNodeSearchSelection();
            e.Handled = true;
        }
    }

    private void VisualScriptNodeSearchListBox_OnSelectionChanged(object? sender, SelectionChangedEventArgs e)
    {
        if (!applyingVisualScriptNodeSearchResults && e.AddedItems.Count > 0)
        {
            CommitVisualScriptNodeSearchSelection();
        }
    }

    private void CommitVisualScriptNodeSearchSelection()
    {
        if (visualScriptNodeSearchListBox?.SelectedItem is BridgeVisualScriptPaletteEntry entry)
        {
            VisualScriptCanvas.RequestPaletteInsertion(entry, visualScriptNodeSearchPosition);
            CloseVisualScriptNodeSearchPopup();
        }
    }

    private void VisualScriptNodeSearchPopup_OnClosed(object? sender, EventArgs e)
    {
        if (ReferenceEquals(sender, visualScriptNodeSearchPopup))
        {
            visualScriptNodeSearchPopup = null;
            visualScriptNodeSearchTextBox = null;
            visualScriptNodeSearchListBox = null;
        }
    }

    private void CloseVisualScriptNodeSearchPopup()
    {
        if (visualScriptNodeSearchPopup is { } popup)
        {
            popup.Closed -= VisualScriptNodeSearchPopup_OnClosed;
            popup.IsOpen = false;
            popup.Child = null;
        }
        visualScriptNodeSearchPopup = null;
        visualScriptNodeSearchTextBox = null;
        visualScriptNodeSearchListBox = null;
    }

    private void VisualScriptUndoButton_OnClick(object? sender, RoutedEventArgs e) =>
        VisualScriptCanvas.RequestUndo();

    private void VisualScriptRedoButton_OnClick(object? sender, RoutedEventArgs e) =>
        VisualScriptCanvas.RequestRedo();

    private void VisualScriptPaletteFilterTextBox_OnTextChanged(object? sender, TextChangedEventArgs e)
    {
        if (!applyingSnapshot)
        {
            ApplyVisualScriptPaletteFilter();
        }
    }

    private void VisualScriptPropertiesListBox_OnSelectionChanged(object? sender, SelectionChangedEventArgs e)
    {
        if (applyingSnapshot || VisualScriptPropertiesListBox.SelectedItem is not BridgeVisualScriptPin pin)
        {
            return;
        }
        selectedVisualScriptPin = pin;
        VisualScriptDefaultValueTextBox.Text = pin.DefaultValue ?? string.Empty;
        VisualScriptApplyDefaultButton.IsEnabled = selectedVisualScriptNode is not null;
    }

    private void VisualScriptApplyDefaultButton_OnClick(object? sender, RoutedEventArgs e)
    {
        if (selectedVisualScriptNode is null || selectedVisualScriptPin is null)
        {
            return;
        }
        DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "setVisualScriptPinDefault",
            VisualScriptNodeId: selectedVisualScriptNode.Id,
            VisualScriptPinName: selectedVisualScriptPin.Name,
            VisualScriptDefaultValue: VisualScriptDefaultValueTextBox.Text ?? string.Empty));
    }

    private void VisualScriptPaletteListBox_OnSelectionChanged(object? sender, SelectionChangedEventArgs e)
    {
        if (applyingSnapshot || VisualScriptPaletteListBox.SelectedItem is not BridgeVisualScriptPaletteEntry entry)
        {
            return;
        }
        VisualScriptPaletteListBox.SelectedItem = null;
        DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "addVisualScriptNodeType",
            VisualScriptNodeTypeId: entry.TypeId,
            VisualScriptPosition: new BridgeVisualScriptPoint(0F, 0F)));
    }

    private void ScriptRuntimeRefreshButton_OnClick(object? sender, RoutedEventArgs e) =>
        DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "readScriptRuntime"));

    private void ScriptRuntimeTickButton_OnClick(object? sender, RoutedEventArgs e) =>
        DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "readScriptRuntimeTickDiagnostics"));

    private void ScriptRuntimeFilterTextBox_OnTextChanged(object? sender, TextChangedEventArgs e)
    {
        if (!applyingSnapshot)
        {
            ApplyScriptRuntimeFilter();
        }
    }

    private void ScriptRuntimeClearFilterButton_OnClick(object? sender, RoutedEventArgs e) =>
        ScriptRuntimeFilterTextBox.Text = string.Empty;

    private void ScriptRuntimeInstancesListBox_OnSelectionChanged(object? sender, SelectionChangedEventArgs e)
    {
        if (applyingSnapshot)
        {
            return;
        }

        selectedScriptRuntimeInstance = ScriptRuntimeInstancesListBox.SelectedItem as BridgeScriptRuntimeInstanceEntry;
        RenderScriptRuntimeSelection(selectedScriptRuntimeInstance);
    }

    private void DeveloperConsoleSubmitButton_OnClick(object? sender, RoutedEventArgs e)
    {
        string command = DeveloperConsoleCommandTextBox.Text?.Trim() ?? string.Empty;
        if (command.Length == 0)
        {
            return;
        }
        DeveloperConsoleCommandTextBox.Text = string.Empty;
        DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "submitDeveloperConsoleCommand",
            DeveloperConsoleCommand: command));
    }

    private void DeveloperConsoleClearButton_OnClick(object? sender, RoutedEventArgs e) =>
        DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "clearDeveloperConsole"));

    private void DeveloperConsoleDiscoverButton_OnClick(object? sender, RoutedEventArgs e)
    {
        byte filter = checked((byte)Math.Clamp(DeveloperConsoleSeverityFilterComboBox.SelectedIndex, 0, 3));
        DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "setDeveloperConsoleSeverityFilter",
            DeveloperConsoleSeverityFilter: filter,
            DeveloperConsoleCompletionPrefix: DeveloperConsoleCompletionPrefixTextBox.Text ?? string.Empty));
    }

    private void DeveloperConsoleHistoryUpButton_OnClick(object? sender, RoutedEventArgs e) =>
        DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "moveDeveloperConsoleHistory",
            DeveloperConsoleHistoryDelta: -1));

    private void DeveloperConsoleHistoryDownButton_OnClick(object? sender, RoutedEventArgs e) =>
        DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "moveDeveloperConsoleHistory",
            DeveloperConsoleHistoryDelta: 1));

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
            DisplayConnectedSnapshot(await session.RefreshSnapshotAsync(CancellationToken.None).ConfigureAwait(true));
        }
        catch (Exception exception)
        {
            EnterFailure("bridge.snapshot.failed", exception.Message);
        }
    }

    private void ApplyHierarchyFilterButton_OnClick(object? sender, RoutedEventArgs e) =>
        DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "setHierarchyFilter",
            HierarchyFilter: HierarchyFilterTextBox.Text ?? string.Empty));

    private void ClearHierarchyFilterButton_OnClick(object? sender, RoutedEventArgs e)
    {
        HierarchyFilterTextBox.Text = string.Empty;
        DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "setHierarchyFilter", HierarchyFilter: string.Empty));
    }

    private void HierarchyListBox_OnSelectionChanged(object? sender, SelectionChangedEventArgs e)
    {
        if (!applyingSnapshot)
        {
            selectedHierarchyEntry = HierarchyListBox.SelectedItem as BridgeHierarchyEntry;
        }
    }

    private void SelectHierarchyEntityButton_OnClick(object? sender, RoutedEventArgs e)
    {
        if (selectedHierarchyEntry is not null)
        {
            DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "selectEntity", Entity: selectedHierarchyEntry.Entity));
        }
    }

    private void ToggleHierarchyEntityButton_OnClick(object? sender, RoutedEventArgs e)
    {
        if (selectedHierarchyEntry is not null)
        {
            DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "toggleEntitySelection", Entity: selectedHierarchyEntry.Entity));
        }
    }

    private void ClearSelectionButton_OnClick(object? sender, RoutedEventArgs e) =>
        DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "clearSelection"));

    private void RenameSelectedEntityButton_OnClick(object? sender, RoutedEventArgs e)
    {
        string name = InspectorNameTextBox.Text?.Trim() ?? string.Empty;
        if (!string.IsNullOrWhiteSpace(name))
        {
            DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "setSelectedEntityName", EntityName: name));
        }
    }

    private void RefreshContentBrowserButton_OnClick(object? sender, RoutedEventArgs e) =>
        DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "refreshContentBrowser"));

    private void ContentRootButton_OnClick(object? sender, RoutedEventArgs e) =>
        DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "setContentBrowserDirectory", ContentDirectory: string.Empty));

    private void ContentUpButton_OnClick(object? sender, RoutedEventArgs e)
    {
        string currentDirectory = session?.LastSnapshot?.ContentBrowser.CurrentDirectory ?? string.Empty;
        int separator = currentDirectory.LastIndexOf('/');
        string parent = separator < 0 ? string.Empty : currentDirectory[..separator];
        DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "setContentBrowserDirectory", ContentDirectory: parent));
    }

    private void OpenSelectedFolderButton_OnClick(object? sender, RoutedEventArgs e)
    {
        if (selectedContentEntry is { IsDirectory: true })
        {
            DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "setContentBrowserDirectory",
                ContentDirectory: selectedContentEntry.RelativePath));
        }
    }

    private void ApplyContentFiltersButton_OnClick(object? sender, RoutedEventArgs e)
    {
        DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "setContentBrowserFilter",
            ContentFilter: ContentFilterTextBox.Text ?? string.Empty));
        DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "setContentBrowserFocus",
            ContentFocus: SelectedContentFocusTag()));
    }

    private void ClearContentFiltersButton_OnClick(object? sender, RoutedEventArgs e)
    {
        ContentFilterTextBox.Text = string.Empty;
        ContentFocusComboBox.SelectedIndex = 0;
        DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "setContentBrowserFilter", ContentFilter: string.Empty));
        DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "setContentBrowserFocus", ContentFocus: "all"));
    }

    private void DataTableCatalogListBox_OnSelectionChanged(object? sender, SelectionChangedEventArgs e)
    {
        if (applyingSnapshot)
        {
            return;
        }
        selectedDataTableEntry = DataTableCatalogListBox.SelectedItem as BridgeDataTableCatalogEntry;
        if (selectedDataTableEntry is not null)
        {
            DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "selectDataTablePreview",
                DataTableName: selectedDataTableEntry.Name));
        }
    }

    private void ContentBrowserListBox_OnSelectionChanged(object? sender, SelectionChangedEventArgs e)
    {
        if (applyingSnapshot)
        {
            return;
        }
        selectedContentEntry = ContentBrowserListBox.SelectedItem as BridgeContentBrowserEntry;
        if (selectedContentEntry is not null)
        {
            DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "selectContentBrowserEntry",
                ContentEntryPath: selectedContentEntry.RelativePath));
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
                DisplayConnectedSnapshot(started.LastSnapshot!);
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

    private async void DispatchCurrentCommand(BridgeCommand command)
    {
        BridgeCommandResult? result = await DispatchCommandAsync(command).ConfigureAwait(true);
        if (result != null && !result.Applied)
        {
            StatusTextBlock.Text = $"Native command not applied — {result.Code}";
            DetailsTextBlock.Text = result.Message;
        }
    }

    private async Task<BridgeCommandResult?> DispatchCommandAsync(BridgeCommand command)
    {
        if (session is null || state != HostSessionState.Connected)
        {
            return null;
        }

        try
        {
            BridgeCommandResult result = await session.DispatchAsync(command, CancellationToken.None).ConfigureAwait(true);
            DisplayConnectedSnapshot(result.Snapshot);
            return result;
        }
        catch (BridgeProtocolException exception)
        {
            EnterFailure(exception.Code, exception.Message);
            return null;
        }
        catch (Exception exception)
        {
            EnterFailure("bridge.command.failed", exception.Message);
            return null;
        }
    }

    private void DisplayConnectedSnapshot(BridgeEditorSnapshot snapshot)
    {
        state = HostSessionState.Connected;
        WarningBorder.IsVisible = false;
        SetControlsEnabled(true);
        RefreshButton.IsEnabled = true;
        StartNewSessionButton.IsEnabled = false;
        StatusTextBlock.Text = "Connected to a headless C++ bridge backend.";
        DetailsTextBlock.Text = DescribeSnapshot(snapshot);
        RenderPanels(snapshot);
        UpdateShellPresentation();
    }

    private void RenderPanels(BridgeEditorSnapshot snapshot)
    {
        applyingSnapshot = true;
        try
        {
            RenderHierarchy(snapshot.Hierarchy);
            RenderInspector(snapshot.Inspector);
            RenderContentBrowser(snapshot.ContentBrowser);
            RenderViewportSurface(snapshot.ViewportSurface);
            RenderVisualScripting(snapshot.VisualScripting, snapshot.Revision);
            RenderScriptRuntime(snapshot.ScriptRuntime,
                snapshot.Capabilities.Contains(BridgeSnapshotParser.ReadScriptRuntimeCapability),
                snapshot.Capabilities.Contains(BridgeSnapshotParser.ReadScriptRuntimeTickDiagnosticsCapability),
                snapshot.Revision,
                snapshot.ScriptRuntimeTickSummary,
                snapshot.ScriptRuntimeTickHistory,
                snapshot.ScriptRuntimeTickHistoryTruncated);
            RenderDeveloperConsole(snapshot.DeveloperConsole);
            RenderDataTableCatalog(snapshot.DataTableCatalog, snapshot.DataTablePreview);
            RenderDataTablePreview(snapshot.DataTablePreview);
            RenderMotionQuery(snapshot.MotionQuery);
        }
        finally
        {
            applyingSnapshot = false;
        }
    }

    private static string DescribeViewportSurface(BridgeViewportSurfaceSnapshot surface)
    {
        string dimensions = surface.Width == 0U || surface.Height == 0U
            ? "no native dimensions"
            : $"{surface.Width}×{surface.Height}";
        return $"C++ surface state: {surface.State}; generation {surface.Generation}; {dimensions}. {surface.Reason}";
    }

    private void RenderViewportSurface(BridgeViewportSurfaceSnapshot surface)
    {
        ViewportSurfaceStatusTextBlock.Text = DescribeViewportSurface(surface);
    }

    private static string DescribeVisualScripting(BridgeVisualScriptingSnapshot scripting)
    {
        string availability = scripting.IsAvailable ? "available" : "unavailable";
        return $"Native presentation {availability}; graph revision {scripting.GraphRevision}; " +
               $"{scripting.NodeCount} nodes, {scripting.LinkCount} links; managed edit capability: {scripting.CanEdit}. " +
               scripting.Reason;
    }

    private static string DescribeVisualScriptDebugger(BridgeVisualScriptDebuggerSnapshot debugger,
                                                         BridgeVisualScriptCanvasSnapshot canvas)
    {
        if (!debugger.Available)
        {
            return $"Debugger unavailable. {debugger.Reason}";
        }
        string sourceNode = debugger.SourceNodeId == 0U
            ? "none"
            : canvas.Nodes.FirstOrDefault(node => node.Id == debugger.SourceNodeId) is { } node
                ? $"{node.DisplayName} (#{node.Id})"
                : $"#{debugger.SourceNodeId} (not in copied canvas view)";
        string pauseReason = string.IsNullOrWhiteSpace(debugger.PauseReason)
            ? string.Empty
            : $" Pause: {debugger.PauseReason}";
        string traceSummary = $" Trace: {debugger.Trace.Count} event(s)" +
                              (debugger.TraceTruncated ? ", truncated." : ".");
        return $"Debugger state {debugger.State}; instruction {debugger.InstructionIndex}; source node {sourceNode}; " +
               $"{debugger.ExecutedInstructions} executed; {debugger.BreakpointNodeIds.Count} breakpoint(s)." +
               $"{traceSummary}{pauseReason} {debugger.Reason}";
    }

    private void RenderScriptRuntime(BridgeScriptRuntimeSnapshot runtime, bool requestAvailable,
                                     bool tickAvailable, ulong bridgeRevision,
                                     BridgeScriptRuntimeTickSummary tickSummary,
                                     IReadOnlyList<BridgeScriptRuntimeTickHistoryEntry> tickHistory,
                                     bool tickHistoryTruncated)
    {
        scriptRuntimeRequestAvailable = requestAvailable;
        scriptRuntimeTickAvailable = tickAvailable;
        ScriptRuntimeRefreshButton.IsEnabled = state == HostSessionState.Connected && requestAvailable;
        ScriptRuntimeTickButton.IsEnabled = state == HostSessionState.Connected && tickAvailable;
        scriptRuntimeSnapshot = runtime;
        string tickStatus = tickSummary.IsAvailable
            ? $" Last diagnostic tick: {tickSummary.EnabledInstanceCount} enabled, {tickSummary.CompletedCount} completed, " +
              $"{tickSummary.InstructionBudgetExceededCount} budget-exceeded, {tickSummary.InvalidInstructionCount} invalid, " +
              $"{tickSummary.DiagnosticCount} diagnostic(s)."
            : $" {tickSummary.Reason}";
        string historyStatus = tickHistory.Count == 0
            ? "No diagnostic tick history is available."
            : $"Recent native diagnostic ticks: {string.Join(" · ", tickHistory.Select(entry =>
                $"#{entry.Sequence} {entry.Summary.CompletedCount}/{entry.Summary.EnabledInstanceCount} completed, " +
                $"{entry.Summary.DiagnosticCount} diagnostic(s)"))}" +
              (tickHistoryTruncated ? " · older entries truncated" : string.Empty);
        ScriptRuntimeHistoryTextBlock.Text = historyStatus;
        if (!runtime.IsAvailable)
        {
            ScriptRuntimeStatusTextBlock.Text = $"Bridge revision {bridgeRevision}: {runtime.Reason}{tickStatus}";
        }
        else
        {
            string truncation = runtime.EntriesTruncated ? " The native bridge truncated the copied instance list." : string.Empty;
            ScriptRuntimeStatusTextBlock.Text =
                $"{runtime.InstanceCount} native ScriptRuntime instance(s); {runtime.Entries.Count} visible copied row(s) " +
                $"({runtime.VisibleEnabledInstanceCount} enabled, {runtime.VisibleDisabledInstanceCount} disabled). " +
                $"Instance rows include generational identity, program version, instruction/state counts, and enabled state. " +
                $"Bridge revision {bridgeRevision}. {runtime.Reason}{truncation}{tickStatus}";
        }
        ApplyScriptRuntimeFilter();
    }

    private void ApplyScriptRuntimeFilter()
    {
        if (scriptRuntimeSnapshot is not { } runtime)
        {
            return;
        }

        string filter = ScriptRuntimeFilterTextBox.Text?.Trim() ?? string.Empty;
        IReadOnlyList<BridgeScriptRuntimeInstanceEntry> entries = filter.Length == 0
            ? runtime.Entries
            : runtime.Entries.Where(entry => entry.MatchesFilter(filter)).ToArray();
        ScriptRuntimeInstancesListBox.ItemsSource = entries;
        ScriptRuntimeInstancesListBox.IsVisible = entries.Count > 0;
        ScriptRuntimeEmptyTextBlock.IsVisible = entries.Count == 0;
        ScriptRuntimeEmptyTextBlock.Text = runtime.Entries.Count == 0
            ? runtime.IsAvailable
                ? "Native ScriptRuntime is attached, but no active instances are in the copied snapshot."
                : runtime.Reason
            : $"No copied ScriptRuntime instances match '{filter}'. The filter is local to this bounded snapshot.";
        if (runtime.IsAvailable && filter.Length > 0)
        {
            ScriptRuntimeStatusTextBlock.Text +=
                $" Filter '{filter}' matches {entries.Count} of {runtime.Entries.Count} copied row(s).";
        }
        selectedScriptRuntimeInstance = entries.FirstOrDefault(entry =>
            selectedScriptRuntimeInstance is not null &&
            entry.EntityIndex == selectedScriptRuntimeInstance.EntityIndex &&
            entry.EntityGeneration == selectedScriptRuntimeInstance.EntityGeneration &&
            entry.Generation == selectedScriptRuntimeInstance.Generation);
        ScriptRuntimeInstancesListBox.SelectedItem = selectedScriptRuntimeInstance;
        RenderScriptRuntimeSelection(selectedScriptRuntimeInstance);
    }

    private void RenderScriptRuntimeSelection(BridgeScriptRuntimeInstanceEntry? instance)
    {
        ScriptRuntimeSelectionDetailsTextBlock.Text = instance is null
            ? "Select a copied ScriptRuntime instance for details."
            : $"Selected copied instance: {instance.DisplayText}. Runtime control remains native-owned and read-only here.";
    }

    private void RenderDeveloperConsole(BridgeDeveloperConsoleSnapshot console)
    {
        DeveloperConsoleOutputListBox.ItemsSource = console.Output
            .Select(entry => $"[{entry.Severity}] {entry.Text}")
            .ToArray();
        string truncation = console.OutputTruncated || console.HistoryTruncated || console.CVarsTruncated
            ? " · bounded/truncated"
            : string.Empty;
        DeveloperConsoleCompletionListBox.ItemsSource = console.Completions
            .Select(completion => $"{completion.Identifier} — {completion.Help}")
            .ToArray();
        applyingSnapshot = true;
        try
        {
            DeveloperConsoleSeverityFilterComboBox.SelectedIndex = Math.Clamp(console.SeverityFilter, (byte)0, (byte)3);
            if (console.HistoryCursor >= 0)
            {
                DeveloperConsoleCommandTextBox.Text = console.HistoryEntry;
            }
        }
        finally
        {
            applyingSnapshot = false;
        }
        string availability = console.IsAvailable ? "available" : "shipping-disabled";
        string cursor = console.HistoryCursor < 0 ? "draft" : $"history {console.HistoryCursor}";
        string discoveryTruncation = console.CompletionTruncated ? " · completion list truncated" : string.Empty;
        DeveloperConsoleStatusTextBlock.Text =
            $"Generation {console.Generation}; {availability}; filter {console.SeverityFilter}; {cursor}; " +
            $"{console.Output.Count} output entr(y/ies), {console.History.Count} history item(s), " +
            $"{console.CVars.Count} cvar(s){truncation}{discoveryTruncation}";
    }

    private void RenderDataTableCatalog(BridgeDataTableCatalogSnapshot catalog,
                                        BridgeDataTablePreviewSnapshot preview)
    {
        DataTableCatalogListBox.ItemsSource = catalog.Entries;
        selectedDataTableEntry = preview.IsAvailable
            ? catalog.Entries.FirstOrDefault(entry => entry.Name == preview.Name)
            : null;
        DataTableCatalogListBox.SelectedItem = selectedDataTableEntry;
        string truncation = catalog.EntriesTruncated ? " · bounded/truncated" : string.Empty;
        DataTableCatalogStatusTextBlock.Text =
            $"Generation {catalog.Generation}; {catalog.Entries.Count} native table descriptor(s){truncation}. " +
            "Read-only copied facts; table rows and asset ownership remain native.";
    }

    private void RenderDataTablePreview(BridgeDataTablePreviewSnapshot preview)
    {
        DataTablePreviewListBox.ItemsSource = preview.Rows;
        string truncation = preview.ColumnsTruncated || preview.RowsTruncated || preview.ValuesTruncated
            ? " · bounded/truncated"
            : string.Empty;
        DataTablePreviewStatusTextBlock.Text = preview.IsAvailable
            ? $"{preview.Name}; generation {preview.Generation}; {preview.TotalColumnCount} column(s), " +
              $"{preview.TotalRowCount} row(s); {preview.Columns.Count} visible column descriptor(s){truncation}. " +
              preview.Reason
            : preview.Reason;
    }

    private void RenderMotionQuery(BridgeMotionQuerySnapshot motionQuery)
    {
        bool connected = state == HostSessionState.Connected;
        MotionQueryAuthoringDatabasesListBox.ItemsSource = motionQuery.Authoring.Databases;
        MotionQueryAuthoringCommandMetadataListBox.ItemsSource = motionQuery.Authoring.CommandMetadata;
        MotionQueryAuthoringDatabasesListBox.IsEnabled = connected && motionQuery.Authoring.Databases.Count > 0;
        MotionQueryAuthoringCommandMetadataListBox.IsEnabled = connected && motionQuery.Authoring.CommandMetadata.Count > 0;
        MotionQueryPasteDatabaseButton.IsEnabled = connected;
        MotionQueryAuthoringStatusTextBlock.Text = connected
            ? $"Authoring revision {motionQuery.Authoring.Revision}; {motionQuery.Authoring.Databases.Count} native database(s). {motionQuery.Authoring.Diagnostic}"
            : motionQuery.Authoring.Diagnostic;

        MotionQueryExportRegistryButton.IsEnabled = connected;
        MotionQueryImportRegistryButton.IsEnabled = state == HostSessionState.Connected;
        MotionQueryRunBatchButton.IsEnabled = state == HostSessionState.Connected;
        MotionQueryClearReplayButton.IsEnabled = state == HostSessionState.Connected;
        MotionQueryExportTraceButton.IsEnabled = state == HostSessionState.Connected && motionQuery.Trace.Events.Count > 0;
        MotionQueryImportTraceButton.IsEnabled = state == HostSessionState.Connected;
        MotionQueryExportEvidenceButton.IsEnabled = state == HostSessionState.Connected && motionQuery.Trace.Events.Count > 0;

        MotionQueryStatusTextBlock.Text = motionQuery.ReplayWorkflow.ReadyForComparison
            ? $"Ready for regression. Registry generation {motionQuery.ReplayWorkflow.RegistryGeneration}, {motionQuery.ReplayWorkflow.BaselineCount} baseline(s)."
            : $"Not ready for regression: {motionQuery.ReplayWorkflow.Diagnostic}";

        MotionQuerySessionFactsTextBlock.Text =
            $"{motionQuery.ReplaySessionFacts.TotalIndividualComparisons} individual comparisons | " +
            $"{motionQuery.ReplaySessionFacts.TotalBatchRuns} batch runs | " +
            $"{motionQuery.ReplaySessionFacts.TotalBaselinesEvaluated} baselines evaluated | " +
            $"{motionQuery.ReplaySessionFacts.TotalMatchesFound} matches | " +
            $"{motionQuery.ReplaySessionFacts.TotalMismatchesFound} mismatches";

        MotionQueryBatchHistoryListBox.ItemsSource = motionQuery.ReplayBatchHistory;
        MotionQueryIndividualHistoryListBox.ItemsSource = motionQuery.ReplayDiagnostics.History;
        MotionQueryBaselinesListBox.ItemsSource = motionQuery.ReplayDiagnostics.Baselines;
        MotionQueryTraceListBox.ItemsSource = motionQuery.Trace.Events;

        MotionQueryDiagnosticsTextBlock.Text = motionQuery.ReplayDiagnostics.HasActiveComparison
            ? motionQuery.ReplayDiagnostics.IsMatch
                ? "Match: No regression detected."
                : $"Mismatch: Comparison code {motionQuery.ReplayDiagnostics.ComparisonCode}, Field mask {motionQuery.ReplayDiagnostics.MismatchFieldMask}, Compatibility mask {motionQuery.ReplayDiagnostics.CompatibilityMismatchMask}."
            : "No active comparison.";
    }

    private static string? ReadMotionQueryAuthoringText(Button button, int textBoxIndex)
    {
        return button.Parent is Grid grid
            ? grid.Children.OfType<TextBox>().ElementAtOrDefault(textBoxIndex)?.Text?.Trim()
            : null;
    }

    private void MotionQueryCopyDatabaseButton_OnClick(object? sender, RoutedEventArgs e)
    {
        if (sender is Button { DataContext: BridgeMotionQueryDatabaseRow row })
        {
            DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "dispatchMotionQueryCommand")
            {
                MotionQueryCommandKind = "copyDatabase",
                MotionQueryResource = row.Resource,
            });
        }
    }

    private void MotionQueryPasteDatabaseButton_OnClick(object? sender, RoutedEventArgs e)
    {
        if (!ulong.TryParse(MotionQueryPasteResourceGuidTextBox.Text?.Trim(), out ulong resourceGuid) || resourceGuid == 0UL ||
            !ulong.TryParse(MotionQueryPasteResourceGenerationTextBox.Text?.Trim(), out ulong resourceGeneration) || resourceGeneration == 0UL ||
            !ulong.TryParse(MotionQueryPasteContextGenerationTextBox.Text?.Trim(), out ulong contextGeneration) || contextGeneration == 0UL)
        {
            StatusTextBlock.Text = "Motion Query database was not pasted.";
            DetailsTextBlock.Text = "Enter positive resource and context generations before pasting.";
            return;
        }
        string displayName = MotionQueryPasteDisplayNameTextBox.Text?.Trim() ?? string.Empty;
        string databaseId = MotionQueryPasteDatabaseIdTextBox.Text?.Trim() ?? string.Empty;
        if (displayName.Length == 0 || displayName.Length > 128 || databaseId.Length == 0 || databaseId.Length > 128)
        {
            StatusTextBlock.Text = "Motion Query database was not pasted.";
            DetailsTextBlock.Text = "Display name and database ID must be bounded, non-empty text.";
            return;
        }
        DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "dispatchMotionQueryCommand")
        {
            MotionQueryCommandKind = "pasteDatabase",
            MotionQueryPasteTarget = new BridgeMotionQueryPasteTarget(
                new BridgeMotionQueryResourceHandle(resourceGuid, resourceGeneration),
                displayName,
                databaseId,
                contextGeneration),
        });
    }

    private void MotionQuerySelectDatabaseButton_OnClick(object? sender, RoutedEventArgs e)
    {
        if (sender is Button { DataContext: BridgeMotionQueryDatabaseRow row })
        {
            DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "dispatchMotionQueryCommand")
            {
                MotionQueryCommandKind = "selectDatabase",
                MotionQueryResource = row.Resource,
            });
        }
    }

    private void MotionQueryRemoveDatabaseButton_OnClick(object? sender, RoutedEventArgs e)
    {
        if (sender is Button { DataContext: BridgeMotionQueryDatabaseRow row })
        {
            DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "dispatchMotionQueryCommand")
            {
                MotionQueryCommandKind = "removeDatabase",
                MotionQueryResource = row.Resource,
            });
        }
    }

    private void MotionQuerySetDisplayNameButton_OnClick(object? sender, RoutedEventArgs e)
    {
        if (sender is Button button && button.DataContext is BridgeMotionQueryDatabaseRow row)
        {
            DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "dispatchMotionQueryCommand")
            {
                MotionQueryCommandKind = "setDisplayName",
                MotionQueryResource = row.Resource,
                MotionQueryText = ReadMotionQueryAuthoringText(button, 0),
            });
        }
    }

    private void MotionQuerySetSchemaIdButton_OnClick(object? sender, RoutedEventArgs e)
    {
        if (sender is Button button && button.DataContext is BridgeMotionQueryDatabaseRow row)
        {
            DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "dispatchMotionQueryCommand")
            {
                MotionQueryCommandKind = "setSchemaId",
                MotionQueryResource = row.Resource,
                MotionQueryText = ReadMotionQueryAuthoringText(button, 1),
            });
        }
    }

    private void MotionQuerySetMaximumCandidatesButton_OnClick(object? sender, RoutedEventArgs e)
    {
        if (sender is not Button button || button.DataContext is not BridgeMotionQueryDatabaseRow row)
        {
            return;
        }
        string? text = ReadMotionQueryAuthoringText(button, 2);
        if (!uint.TryParse(text, out uint maximumCandidates) || maximumCandidates == 0U || maximumCandidates > 4096U)
        {
            StatusTextBlock.Text = "Maximum candidates was not changed.";
            DetailsTextBlock.Text = "Enter a positive bounded integer before applying the native Motion Query limit.";
            return;
        }
        DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "dispatchMotionQueryCommand")
        {
            MotionQueryCommandKind = "setMaximumCandidates",
            MotionQueryResource = row.Resource,
            MotionQueryCandidateIndex = maximumCandidates,
        });
    }

    private void MotionQueryValidateDatabaseButton_OnClick(object? sender, RoutedEventArgs e)
    {
        if (sender is Button { DataContext: BridgeMotionQueryDatabaseRow row })
        {
            DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "dispatchMotionQueryCommand")
            {
                MotionQueryCommandKind = "validateDatabase",
                MotionQueryResource = row.Resource,
            });
        }
    }

    private async void MotionQueryExportRegistryButton_OnClick(object? sender, RoutedEventArgs e)
    {
        var result = await DispatchCommandAsync(new BridgeCommand(CurrentRevision(), "exportMotionQueryReplayBaselineRegistry")).ConfigureAwait(true);
        if (result != null && result.Applied && result.MotionQueryReplayBaselineEnvelopePayload != null)
        {
            var topLevel = TopLevel.GetTopLevel(this);
            var clipboard = topLevel?.Clipboard;
            if (clipboard != null)
            {
                await clipboard.SetTextAsync(result.MotionQueryReplayBaselineEnvelopePayload).ConfigureAwait(true);
                MotionQueryStatusTextBlock.Text = "Registry exported to clipboard.";
            }
        }
    }

    private async void MotionQueryImportRegistryButton_OnClick(object? sender, RoutedEventArgs e)
    {
        var topLevel = TopLevel.GetTopLevel(this);
        var clipboard = topLevel?.Clipboard;
        if (clipboard != null)
        {
#pragma warning disable CS0618
            var payload = await clipboard.GetTextAsync().ConfigureAwait(true);
#pragma warning restore CS0618
            if (!string.IsNullOrWhiteSpace(payload))
            {
                DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "importMotionQueryReplayBaselineRegistry")
                {
                    MotionQueryReplayBaselineEnvelopePayload = payload
                });
            }
        }
    }

    private void MotionQueryRunBatchButton_OnClick(object? sender, RoutedEventArgs e) =>
        DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "runMotionQueryReplayBaselineBatch"));

    private void MotionQueryClearReplayButton_OnClick(object? sender, RoutedEventArgs e) =>
        DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "clearMotionQueryReplayBaseline"));

    private void MotionQueryLoadBaselineButton_OnClick(object? sender, RoutedEventArgs e)
    {
        if (sender is Button { DataContext: BridgeMotionQueryReplayBaselineEntry baseline })
        {
            DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "loadMotionQueryReplayBaseline")
            {
                MotionQueryReplayBaselineName = baseline.Name
            });
        }
    }

    private void MotionQueryRemoveBaselineButton_OnClick(object? sender, RoutedEventArgs e)
    {
        if (sender is Button { DataContext: BridgeMotionQueryReplayBaselineEntry baseline })
        {
            DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "clearMotionQueryReplayBaseline")
            {
                MotionQueryReplayBaselineName = baseline.Name
            });
        }
    }

    private void MotionQueryRenameBaselineButton_OnClick(object? sender, RoutedEventArgs e)
    {
        if (sender is Button button && button.DataContext is BridgeMotionQueryReplayBaselineEntry baseline &&
            button.Parent is Grid grid)
        {
            var textBox = grid.Children.OfType<TextBox>().FirstOrDefault();
            string newName = textBox?.Text?.Trim() ?? string.Empty;
            if (!string.IsNullOrWhiteSpace(newName))
            {
                DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "renameMotionQueryReplayBaseline")
                {
                    MotionQueryReplayBaselineName = baseline.Name,
                    MotionQueryReplayBaselineNewName = newName
                });
            }
        }
    }

    private void MotionQueryTraceListBox_OnSelectionChanged(object? sender, SelectionChangedEventArgs e)
    {
        if (applyingSnapshot || MotionQueryTraceListBox.SelectedItem is not BridgeMotionQueryTraceEvent traceEvent)
        {
            return;
        }

        if (session?.LastSnapshot != null)
        {
            DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "dispatchMotionQueryDebugCommand")
            {
                MotionQueryDebugCommandKind = "selectEvent",
                MotionQueryDebugExpectedGeneration = session.LastSnapshot.MotionQuery.LiveDebugGeneration,
                MotionQueryDebugEventSequence = traceEvent.Sequence
            });
        }
    }

    private void MotionQueryRemoveTraceEventButton_OnClick(object? sender, RoutedEventArgs e)
    {
        if (sender is Button { DataContext: BridgeMotionQueryTraceEvent traceEvent } &&
            session?.LastSnapshot != null)
        {
            DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "dispatchMotionQueryDebugCommand")
            {
                MotionQueryDebugCommandKind = "removeEvent",
                MotionQueryDebugExpectedGeneration = session.LastSnapshot.MotionQuery.LiveDebugGeneration,
                MotionQueryDebugEventSequence = traceEvent.Sequence
            });
        }
    }

    private void MotionQueryToggleTraceEventPinButton_OnClick(object? sender, RoutedEventArgs e)
    {
        if (sender is Button { DataContext: BridgeMotionQueryTraceEvent traceEvent } &&
            session?.LastSnapshot != null)
        {
            DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "dispatchMotionQueryDebugCommand")
            {
                MotionQueryDebugCommandKind = "toggleTraceEventPin",
                MotionQueryDebugExpectedGeneration = session.LastSnapshot.MotionQuery.LiveDebugGeneration,
                MotionQueryDebugEventSequence = traceEvent.Sequence
            });
        }
    }

    private void MotionQuerySetTraceEventCommentButton_OnClick(object? sender, RoutedEventArgs e)
    {
        if (sender is Button button && button.DataContext is BridgeMotionQueryTraceEvent traceEvent &&
            button.Parent is Grid grid && session?.LastSnapshot != null)
        {
            var textBox = grid.Children.OfType<TextBox>().FirstOrDefault();
            string comment = textBox?.Text?.Trim() ?? string.Empty;
            DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "dispatchMotionQueryDebugCommand")
            {
                MotionQueryDebugCommandKind = "setTraceEventComment",
                MotionQueryDebugExpectedGeneration = session.LastSnapshot.MotionQuery.LiveDebugGeneration,
                MotionQueryDebugEventSequence = traceEvent.Sequence,
                MotionQueryDebugFilter = comment // Reusing filter field for comment
            });
        }
    }

    private void MotionQuerySetTraceEventCategoryButton_OnClick(object? sender, RoutedEventArgs e)
    {
        if (sender is Button button && button.DataContext is BridgeMotionQueryTraceEvent traceEvent &&
            button.Parent is Grid grid && session?.LastSnapshot != null)
        {
            var textBox = grid.Children.OfType<TextBox>().FirstOrDefault();
            string category = textBox?.Text?.Trim() ?? string.Empty;
            DispatchCurrentCommand(new BridgeCommand(CurrentRevision(), "dispatchMotionQueryDebugCommand")
            {
                MotionQueryDebugCommandKind = "setTraceEventCategory",
                MotionQueryDebugExpectedGeneration = session.LastSnapshot.MotionQuery.LiveDebugGeneration,
                MotionQueryDebugEventSequence = traceEvent.Sequence,
                MotionQueryDebugFilter = category // Reusing filter field for category
            });
        }
    }

    private async void MotionQueryExportTraceButton_OnClick(object? sender, RoutedEventArgs e)
    {
        var result = await DispatchCommandAsync(new BridgeCommand(CurrentRevision(), "dispatchMotionQueryDebugCommand")
        {
            MotionQueryDebugCommandKind = "exportTrace",
            MotionQueryDebugExpectedGeneration = session?.LastSnapshot?.MotionQuery.LiveDebugGeneration ?? 0UL,
        }).ConfigureAwait(true);
        if (result != null && result.Applied && result.MotionQueryLiveDebugTracePayload != null)
        {
            MotionQueryTracePersistencePayloadTextBox.Text = result.MotionQueryLiveDebugTracePayload;
            var clipboard = TopLevel.GetTopLevel(this)?.Clipboard;
            if (clipboard != null)
            {
                await clipboard.SetTextAsync(result.MotionQueryLiveDebugTracePayload).ConfigureAwait(false);
            }
            MotionQueryStatusTextBlock.Text = "Live debug trace exported to the persistence field and clipboard.";
        }
    }

    private async void MotionQueryImportTraceButton_OnClick(object? sender, RoutedEventArgs e)
    {
        string payload = MotionQueryTracePersistencePayloadTextBox.Text?.Trim() ?? string.Empty;
        if (payload.Length == 0)
        {
            MotionQueryStatusTextBlock.Text = "Paste a persisted live debug trace JSON payload first.";
            return;
        }
        var result = await DispatchCommandAsync(new BridgeCommand(CurrentRevision(), "dispatchMotionQueryDebugCommand")
        {
            MotionQueryDebugCommandKind = "importTrace",
            MotionQueryDebugExpectedGeneration = session?.LastSnapshot?.MotionQuery.LiveDebugGeneration ?? 0UL,
            MotionQueryDebugPayload = payload,
        }).ConfigureAwait(true);
        if (result != null && result.Applied)
        {
            MotionQueryStatusTextBlock.Text = "Live debug trace imported for offline inspection.";
        }
    }

    private async void MotionQueryExportEvidenceButton_OnClick(object? sender, RoutedEventArgs e)
    {
        var result = await DispatchCommandAsync(new BridgeCommand(CurrentRevision(), "exportMotionQueryReplayEvidence")).ConfigureAwait(true);
        if (result != null && result.Applied && result.MotionQueryReplayBaselineEnvelopePayload != null)
        {
            var topLevel = TopLevel.GetTopLevel(this);
            var clipboard = topLevel?.Clipboard;
            if (clipboard != null)
            {
                await clipboard.SetTextAsync(result.MotionQueryReplayBaselineEnvelopePayload).ConfigureAwait(false);
                MotionQueryStatusTextBlock.Text = "Trace exported as evidence to clipboard.";
            }
        }
    }

    private void RenderVisualScripting(BridgeVisualScriptingSnapshot scripting, ulong bridgeRevision)
    {
        VisualScriptingStatusTextBlock.Text = DescribeVisualScripting(scripting);
        VisualScriptingDebuggerTextBlock.Text = DescribeVisualScriptDebugger(scripting.Debugger, scripting.Canvas);
        VisualScriptingBreakpointsListBox.ItemsSource = scripting.Debugger.Available
            ? scripting.Debugger.BreakpointNodeIds.Select(nodeId => $"Node {nodeId}").ToArray()
            : Array.Empty<string>();
        string truncation = scripting.Canvas.NodesTruncated || scripting.Canvas.LinksTruncated ||
            scripting.Canvas.PaletteTruncated || scripting.Canvas.DiagnosticsTruncated
            ? " · truncated"
            : string.Empty;
        VisualScriptingDiagnosticsTextBlock.Text =
            $"Canvas rev {scripting.Canvas.Revision}; {scripting.Canvas.Diagnostics.Count} diagnostic(s){truncation}";
        visualScriptPalette = scripting.Canvas.PaletteDescriptors.Count > 0
            ? scripting.Canvas.PaletteDescriptors
            : scripting.Canvas.PaletteNodeTypeIds.Select(typeId => new BridgeVisualScriptPaletteEntry(
                typeId, typeId, "Uncategorized", "node.default", 0U, 0U,
                Array.Empty<BridgeVisualScriptPin>())).ToArray();
        ApplyVisualScriptPaletteFilter();
        VisualScriptCanvas.ApplySnapshot(scripting.Canvas, bridgeRevision);
        RenderVisualScriptProperties(scripting.Canvas);
    }

    private void RenderVisualScriptProperties(BridgeVisualScriptCanvasSnapshot canvas)
    {
        selectedVisualScriptNode = canvas.SelectedNodeIds.Count == 1
            ? canvas.Nodes.FirstOrDefault(node => node.Id == canvas.SelectedNodeIds[0])
            : canvas.Nodes.FirstOrDefault(node => node.IsSelected);
        IReadOnlyList<BridgeVisualScriptPin> editablePins = selectedVisualScriptNode?.Pins
            .Where(pin => pin.Direction == 0 && pin.Role != 0)
            .ToArray() ?? Array.Empty<BridgeVisualScriptPin>();
        VisualScriptPropertiesListBox.ItemsSource = editablePins;
        selectedVisualScriptPin = editablePins.FirstOrDefault();
        VisualScriptPropertiesListBox.SelectedItem = selectedVisualScriptPin;
        VisualScriptDefaultValueTextBox.Text = selectedVisualScriptPin?.DefaultValue ?? string.Empty;
        VisualScriptApplyDefaultButton.IsEnabled = selectedVisualScriptNode is not null && selectedVisualScriptPin is not null;
        VisualScriptPropertyStatusTextBlock.Text = selectedVisualScriptNode is null
            ? "Select a single visual-script node to inspect copied pin defaults."
            : $"Node {selectedVisualScriptNode.DisplayName} #{selectedVisualScriptNode.Id}: {editablePins.Count} editable copied input pin(s).";
    }

    private void ApplyVisualScriptPaletteFilter()
    {
        VisualScriptPaletteListBox.ItemsSource = VisualScriptCanvasControl.FilterPalette(
            visualScriptPalette, VisualScriptPaletteFilterTextBox.Text);
    }

    private void RenderHierarchy(BridgeHierarchySnapshot hierarchy)
    {
        HierarchyFilterTextBox.Text = hierarchy.Filter;
        HierarchyListBox.ItemsSource = hierarchy.Entries;
        selectedHierarchyEntry = null;
        foreach (BridgeHierarchyEntry entry in hierarchy.Entries)
        {
            if (entry.IsActive)
            {
                selectedHierarchyEntry = entry;
                HierarchyListBox.SelectedItem = entry;
                break;
            }
        }
        string suffix = hierarchy.IsTruncated ? " The native bridge truncated this copied view." : string.Empty;
        HierarchyStatusTextBlock.Text = $"{hierarchy.Entries.Count} native hierarchy row(s).{suffix}";
    }

    private void RenderInspector(BridgeInspectorSnapshot inspector)
    {
        switch (inspector.Mode)
        {
            case BridgeInspectorMode.NoSelection:
                InspectorTitleTextBlock.Text = "No entity selection";
                InspectorContextTextBlock.Text = "Select a hierarchy entity to inspect copied native context.";
                InspectorNameTextBox.Text = string.Empty;
                InspectorNameTextBox.IsEnabled = false;
                RenameSelectedEntityButton.IsEnabled = false;
                InspectorDrawersTextBlock.Text = "Native drawer facts: none";
                InspectorSelectionTextBlock.Text = string.Empty;
                InspectorAssetBindingTextBlock.Text = string.Empty;
                break;
            case BridgeInspectorMode.MultiSelection:
                InspectorTitleTextBlock.Text = $"{inspector.SelectedEntities.Count} entities selected";
                InspectorContextTextBlock.Text = inspector.ActiveEntity is null
                    ? "Multi-selection is read-only."
                    : $"Active native selection: {inspector.ActiveEntity.DisplayLabel}. Single-entity editing is unavailable.";
                InspectorNameTextBox.Text = string.Empty;
                InspectorNameTextBox.IsEnabled = false;
                RenameSelectedEntityButton.IsEnabled = false;
                InspectorDrawersTextBlock.Text = "Native drawer facts are available only for single selection.";
                InspectorSelectionTextBlock.Text = string.Join("\n", inspector.SelectedEntities.Select(entity => entity.DisplayLabel)) +
                                                      (inspector.SelectedEntitiesTruncated ? "\nNative selection list truncated." : string.Empty);
                InspectorAssetBindingTextBlock.Text = "Asset binding facts are available only for single selection.";
                break;
            case BridgeInspectorMode.SingleSelection:
                BridgeEntitySnapshot active = inspector.ActiveEntity!;
                InspectorTitleTextBlock.Text = active.DisplayLabel;
                InspectorContextTextBlock.Text = inspector.Parent is null
                    ? "Native hierarchy parent: Root"
                    : $"Native hierarchy parent: {inspector.Parent.DisplayLabel}";
                InspectorNameTextBox.Text = active.DisplayLabel;
                InspectorNameTextBox.IsEnabled = inspector.CanEditSelectedName;
                RenameSelectedEntityButton.IsEnabled = inspector.CanEditSelectedName;
                InspectorDrawersTextBlock.Text = inspector.EligibleDrawerIds.Count == 0
                    ? "Native drawer facts: none"
                    : $"Native drawer facts: {string.Join(", ", inspector.EligibleDrawerIds)}";
                InspectorSelectionTextBlock.Text = inspector.Ancestry.Count == 0
                    ? "Native ancestry: unavailable"
                    : $"Native ancestry: {string.Join(" > ", inspector.Ancestry.Select(entity => entity.DisplayLabel))}";
                InspectorAssetBindingTextBlock.Text = inspector.AssetBinding is null
                    ? "Asset binding facts: none"
                    : $"Asset binding: {inspector.AssetBinding.DisplayText}";
                break;
        }
    }

    private void RenderContentBrowser(BridgeContentBrowserSnapshot contentBrowser)
    {
        ContentFilterTextBox.Text = contentBrowser.Filter;
        SelectContentFocus(contentBrowser.TypeFocus);
        ContentBrowserListBox.ItemsSource = contentBrowser.Entries;
        selectedContentEntry = contentBrowser.SelectedEntry;
        ContentBrowserListBox.SelectedItem = selectedContentEntry;
        string breadcrumbs = contentBrowser.Breadcrumbs.Count == 0
            ? "Content"
            : $"Content > {string.Join(" > ", contentBrowser.Breadcrumbs)}";
        ContentBreadcrumbTextBlock.Text = breadcrumbs;
        string status = $"{contentBrowser.Entries.Count} / {contentBrowser.VisibleEntryCount} visible copied row(s); " +
                        $"{contentBrowser.DirectEntryCount} direct native entry(ies); snapshot #{contentBrowser.RefreshGeneration}.";
        if (!contentBrowser.IsInitialized)
        {
            status += " Native cache has not been explicitly refreshed yet.";
        }
        if (!contentBrowser.LastRefreshSucceeded)
        {
            status += " Last native refresh failed; the prior successful snapshot is retained.";
        }
        if (!contentBrowser.ContentRootExists)
        {
            status += " Native content root does not exist.";
        }
        if (contentBrowser.IsTruncated)
        {
            status += " The native bridge truncated this copied view.";
        }
        ContentStatusTextBlock.Text = status;
        ContentImportActionTextBlock.Text = contentBrowser.ImportAction?.DisplayText ??
                                            "Import actions: backend capability field unavailable (legacy snapshot).";
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
        ScriptRuntimeRefreshButton.IsEnabled = enabled && state == HostSessionState.Connected && scriptRuntimeRequestAvailable;
        ScriptRuntimeTickButton.IsEnabled = enabled && state == HostSessionState.Connected && scriptRuntimeTickAvailable;
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

    private bool? LastKnownDirtyEvidence() => session?.LastSnapshot?.SceneDirty;

    private ulong CurrentRevision() => session?.LastSnapshot?.Revision ?? 0UL;

    private string SelectedContentFocusTag() =>
        ContentFocusComboBox.SelectedItem is ComboBoxItem { Tag: string tag } ? tag : "all";

    private void SelectContentFocus(string nativeFocusLabel)
    {
        for (int index = 0; index < ContentFocusComboBox.Items.Count; ++index)
        {
            if (ContentFocusComboBox.Items[index] is ComboBoxItem { Content: string content } && content == nativeFocusLabel)
            {
                ContentFocusComboBox.SelectedIndex = index;
                return;
            }
        }
        ContentFocusComboBox.SelectedIndex = 0;
    }

    private static string DescribeSnapshot(BridgeEditorSnapshot snapshot) =>
        $"Protocol {snapshot.ProtocolVersion}; revision {snapshot.Revision}; dirty={snapshot.SceneDirty}; " +
        $"hierarchy rows={snapshot.Hierarchy.Entries.Count}; content rows={snapshot.ContentBrowser.Entries.Count}; " +
        $"viewport={snapshot.ViewportSurface.State}; advertised capabilities={snapshot.Capabilities.Count}; scene='{snapshot.ActiveScenePath}'.";

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
