// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

namespace UniVex.EditorHost;

/// <summary>
/// Provides fixed-region tab identifiers and honest Increment 71 availability descriptions.
/// This catalog is managed presentation data only and never dispatches a bridge command.
/// </summary>
public static class DockShellPanelCatalog
{
    public static IReadOnlySet<string> LeftTabs { get; } = new HashSet<string>(StringComparer.Ordinal)
    {
        "Scene",
    };

    public static IReadOnlySet<string> CenterTabs { get; } = new HashSet<string>(StringComparer.Ordinal)
    {
        "Viewport",
    };

    public static IReadOnlySet<string> RightTabs { get; } = new HashSet<string>(StringComparer.Ordinal)
    {
        "Inspector",
        "Import",
        "Signals",
    };

    public static IReadOnlySet<string> BottomTabs { get; } = new HashSet<string>(StringComparer.Ordinal)
    {
        "FileSystem",
        "Debugger",
        "Animator",
        "AI Toolbar",
    };

    public static DockShellPanelPresentation Describe(string panel, HostSessionState sessionState)
    {
        bool connected = sessionState == HostSessionState.Connected;
        return panel switch
        {
            "Scene" => Deferred(panel, connected,
                "C# Hierarchy presentation is planned for Increment 72. No entity selection or mutation is available here."),
            "Viewport" => Deferred(panel, connected,
                "Native viewport surface hosting is deferred to Increment 74. C# owns no OpenGL context or renderer resource."),
            "Inspector" => Deferred(panel, connected,
                "C# Inspector presentation and property editing are planned for Increment 72. No property mutation is available here."),
            "Import" => Deferred(panel, connected,
                "C# import review is planned for Increment 76. This shell does not enqueue, retry, or mutate imports."),
            "Signals" => Deferred(panel, connected,
                "Managed scripting signal presentation is not available in Increment 71."),
            "FileSystem" => Deferred(panel, connected,
                "C# Content Browser integration is planned for Increment 72. This tab does not enumerate or modify project files."),
            "Debugger" => Deferred(panel, connected,
                "Managed debugger presentation is deferred; this shell does not attach to or control a runtime."),
            "Animator" => Deferred(panel, connected,
                "Animation tooling is deferred; this shell owns no animation state."),
            "AI Toolbar" => Deferred(panel, connected,
                "AI tooling is deferred; this shell owns no background jobs or asset mutations."),
            _ => new DockShellPanelPresentation(panel, false, "Unknown fixed shell panel."),
        };
    }

    private static DockShellPanelPresentation Deferred(string title, bool connected, string deferredMessage)
    {
        string connectionPrefix = connected
            ? "Backend connected. "
            : "Backend unavailable. ";
        return new DockShellPanelPresentation(title, false, connectionPrefix + deferredMessage);
    }
}

/// <summary>
/// Copied presentation facts for one managed shell tab. Availability never grants C# ownership of a backend feature.
/// </summary>
public sealed record DockShellPanelPresentation(string Title, bool IsAvailable, string Message);
