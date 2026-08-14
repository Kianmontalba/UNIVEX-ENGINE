// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

namespace UniVex.EditorHost;

/// <summary>
/// Identifies the managed shell workspace selected by the user. This is presentation state only.
/// </summary>
public enum DockShellWorkspace
{
    Library,
    Asset,
    Scripting,
    Debug,
    Plugin,
}

/// <summary>
/// Identifies a fixed region in the managed UniVex shell. Increment 71 does not support floating docks.
/// </summary>
public enum DockShellRegion
{
    Left,
    Center,
    Right,
    Bottom,
}

/// <summary>
/// A normalized, managed-only description of the fixed-region shell layout. It owns neither native
/// engine state nor a backend process and is safe to persist only through <see cref="DockShellPreferencesStore"/>.
/// </summary>
public sealed record DockShellLayoutState(
    double LeftDockWidth,
    double RightDockWidth,
    double BottomDockHeight,
    DockShellWorkspace ActiveWorkspace,
    string LeftTab,
    string CenterTab,
    string RightTab,
    string BottomTab,
    bool IsLeftDockVisible,
    bool IsRightDockVisible,
    bool IsBottomDockVisible)
{
    public const int CurrentSchemaVersion = 1;

    public const double MinimumSideDockWidth = 180.0;
    public const double MaximumSideDockWidth = 560.0;
    public const double MinimumBottomDockHeight = 140.0;
    public const double MaximumBottomDockHeight = 440.0;

    public static DockShellLayoutState Default { get; } = new(
        LeftDockWidth: 260.0,
        RightDockWidth: 320.0,
        BottomDockHeight: 220.0,
        ActiveWorkspace: DockShellWorkspace.Library,
        LeftTab: "Scene",
        CenterTab: "Viewport",
        RightTab: "Inspector",
        BottomTab: "FileSystem",
        IsLeftDockVisible: true,
        IsRightDockVisible: true,
        IsBottomDockVisible: true);

    public DockShellLayoutState Normalize()
    {
        return this with
        {
            LeftDockWidth = NormalizeRange(LeftDockWidth, MinimumSideDockWidth, MaximumSideDockWidth,
                Default.LeftDockWidth),
            RightDockWidth = NormalizeRange(RightDockWidth, MinimumSideDockWidth, MaximumSideDockWidth,
                Default.RightDockWidth),
            BottomDockHeight = NormalizeRange(BottomDockHeight, MinimumBottomDockHeight, MaximumBottomDockHeight,
                Default.BottomDockHeight),
            ActiveWorkspace = Enum.IsDefined(ActiveWorkspace) ? ActiveWorkspace : Default.ActiveWorkspace,
            LeftTab = NormalizeTab(LeftTab, DockShellPanelCatalog.LeftTabs, Default.LeftTab),
            CenterTab = NormalizeTab(CenterTab, DockShellPanelCatalog.CenterTabs, Default.CenterTab),
            RightTab = NormalizeTab(RightTab, DockShellPanelCatalog.RightTabs, Default.RightTab),
            BottomTab = NormalizeTab(BottomTab, DockShellPanelCatalog.BottomTabs, Default.BottomTab),
        };
    }

    private static double NormalizeRange(double value, double minimum, double maximum, double fallback)
    {
        if (double.IsNaN(value) || double.IsInfinity(value))
        {
            return fallback;
        }
        return Math.Clamp(value, minimum, maximum);
    }

    private static string NormalizeTab(string? tab, IReadOnlySet<string> knownTabs, string fallback)
    {
        return tab is not null && knownTabs.Contains(tab) ? tab : fallback;
    }
}

/// <summary>
/// Tracks unsaved managed layout changes without creating a native bridge request or touching scene state.
/// </summary>
public sealed class DockShellLayoutSession
{
    public DockShellLayoutSession(DockShellLayoutState initialLayout)
    {
        PersistedLayout = initialLayout.Normalize();
        CurrentLayout = PersistedLayout;
    }

    public DockShellLayoutState PersistedLayout { get; private set; }

    public DockShellLayoutState CurrentLayout { get; private set; }

    public bool IsDirty => CurrentLayout != PersistedLayout;

    public void Apply(DockShellLayoutState layout)
    {
        CurrentLayout = layout.Normalize();
    }

    public void MarkSaved()
    {
        PersistedLayout = CurrentLayout;
    }
}
