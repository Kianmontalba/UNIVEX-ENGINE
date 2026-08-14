// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

using System.Text.Json;

namespace UniVex.EditorHost.Tests;

public sealed class DockShellLayoutTests : IDisposable
{
    private readonly string temporaryDirectory = Path.Combine(Path.GetTempPath(), $"uve-shell-tests-{Guid.NewGuid():N}");

    [Fact]
    public void Normalize_ClampsRegionsAndFallsBackToKnownTabs()
    {
        DockShellLayoutState input = DockShellLayoutState.Default with
        {
            LeftDockWidth = -10.0,
            RightDockWidth = 10000.0,
            BottomDockHeight = double.NaN,
            LeftTab = "Unknown",
            CenterTab = "Unknown",
            RightTab = "Unknown",
            BottomTab = "Unknown",
        };

        DockShellLayoutState normalized = input.Normalize();

        Assert.Equal(DockShellLayoutState.MinimumSideDockWidth, normalized.LeftDockWidth);
        Assert.Equal(DockShellLayoutState.MaximumSideDockWidth, normalized.RightDockWidth);
        Assert.Equal(DockShellLayoutState.Default.BottomDockHeight, normalized.BottomDockHeight);
        Assert.Equal("Scene", normalized.LeftTab);
        Assert.Equal("Viewport", normalized.CenterTab);
        Assert.Equal("Inspector", normalized.RightTab);
        Assert.Equal("FileSystem", normalized.BottomTab);
    }

    [Fact]
    public async Task LoadAsync_MigratesRecognizedUnversionedV0WithoutWritingBack()
    {
        string path = CreatePath("v0-layout.json");
        string v0Json = """
            {
              "leftDockWidth": 241,
              "rightDockWidth": 333,
              "bottomDockHeight": 199,
              "activeWorkspace": "Asset",
              "leftTab": "Scene",
              "centerTab": "Viewport",
              "rightTab": "Signals",
              "bottomTab": "Animator"
            }
            """;
        await File.WriteAllTextAsync(path, v0Json);
        DateTime writtenAt = File.GetLastWriteTimeUtc(path);
        DockShellPreferencesStore store = new(path);

        DockShellLayoutState migrated = await store.LoadAsync(CancellationToken.None);

        Assert.Equal(241.0, migrated.LeftDockWidth);
        Assert.Equal(333.0, migrated.RightDockWidth);
        Assert.Equal(199.0, migrated.BottomDockHeight);
        Assert.Equal(DockShellWorkspace.Asset, migrated.ActiveWorkspace);
        Assert.Equal("Signals", migrated.RightTab);
        Assert.Equal("Animator", migrated.BottomTab);
        Assert.True(migrated.IsLeftDockVisible);
        Assert.True(migrated.IsRightDockVisible);
        Assert.True(migrated.IsBottomDockVisible);
        Assert.Equal(v0Json, await File.ReadAllTextAsync(path));
        Assert.Equal(writtenAt, File.GetLastWriteTimeUtc(path));
    }

    [Fact]
    public async Task LoadAsync_RecognizesV0VersionAndNormalizesLegacyBounds()
    {
        string path = CreatePath("versioned-v0-layout.json");
        await File.WriteAllTextAsync(path, """
            {
              "schemaVersion": 0,
              "leftDockWidth": 50,
              "rightDockWidth": 700,
              "bottomDockHeight": 100,
              "activeWorkspace": "Library",
              "leftTab": "Scene",
              "centerTab": "Viewport",
              "rightTab": "Inspector",
              "bottomTab": "FileSystem"
            }
            """);
        DockShellPreferencesStore store = new(path);

        DockShellLayoutState migrated = await store.LoadAsync(CancellationToken.None);

        Assert.Equal(DockShellLayoutState.MinimumSideDockWidth, migrated.LeftDockWidth);
        Assert.Equal(DockShellLayoutState.MaximumSideDockWidth, migrated.RightDockWidth);
        Assert.Equal(DockShellLayoutState.MinimumBottomDockHeight, migrated.BottomDockHeight);
    }

    [Fact]
    public async Task LoadAsync_UsesSafeDefaultsForCorruptUnknownAndUnrecognizedDataWithoutOverwrite()
    {
        string corruptPath = CreatePath("corrupt-layout.json");
        await File.WriteAllTextAsync(corruptPath, "{");
        string futurePath = CreatePath("future-layout.json");
        string futureJson = """{"schemaVersion": 99, "futureValue": "preserve"}""";
        await File.WriteAllTextAsync(futurePath, futureJson);
        string unrecognizedPath = CreatePath("unrecognized-layout.json");
        string unrecognizedJson = """{"theme": "custom"}""";
        await File.WriteAllTextAsync(unrecognizedPath, unrecognizedJson);

        Assert.Equal(DockShellLayoutState.Default, await new DockShellPreferencesStore(corruptPath).LoadAsync(CancellationToken.None));
        Assert.Equal(DockShellLayoutState.Default, await new DockShellPreferencesStore(futurePath).LoadAsync(CancellationToken.None));
        Assert.Equal(DockShellLayoutState.Default, await new DockShellPreferencesStore(unrecognizedPath).LoadAsync(CancellationToken.None));
        Assert.Equal(futureJson, await File.ReadAllTextAsync(futurePath));
        Assert.Equal(unrecognizedJson, await File.ReadAllTextAsync(unrecognizedPath));
    }

    [Fact]
    public async Task SaveAsync_WritesCurrentV1LayoutOnlyAfterExplicitRequest()
    {
        string path = CreatePath("explicit-save-layout.json");
        DockShellPreferencesStore store = new(path);
        DockShellLayoutSession session = new(DockShellLayoutState.Default);
        session.Apply(session.CurrentLayout with { ActiveWorkspace = DockShellWorkspace.Debug, RightTab = "Signals" });

        Assert.True(session.IsDirty);
        Assert.False(File.Exists(path));

        await store.SaveAsync(session.CurrentLayout, CancellationToken.None);
        session.MarkSaved();
        Assert.False(session.IsDirty);
        Assert.True(File.Exists(path));

        using JsonDocument saved = JsonDocument.Parse(await File.ReadAllTextAsync(path));
        Assert.Equal(DockShellLayoutState.CurrentSchemaVersion, saved.RootElement.GetProperty("schemaVersion").GetInt32());
        Assert.Equal("Debug", saved.RootElement.GetProperty("activeWorkspace").GetString());
        Assert.Equal("Signals", saved.RootElement.GetProperty("rightTab").GetString());

        session.Apply(session.CurrentLayout with { BottomTab = "Animator" });
        Assert.True(session.IsDirty);
        Assert.Equal("FileSystem", saved.RootElement.GetProperty("bottomTab").GetString());
        Assert.Equal("FileSystem", (await store.LoadAsync(CancellationToken.None)).BottomTab);
    }

    [Fact]
    public void PanelCatalog_ReportsDeferredPresentationWithoutBackendMutationClaims()
    {
        DockShellPanelPresentation viewport = DockShellPanelCatalog.Describe("Viewport", HostSessionState.Connected);
        DockShellPanelPresentation inspector = DockShellPanelCatalog.Describe("Inspector", HostSessionState.Failed);

        Assert.False(viewport.IsAvailable);
        Assert.Contains("no OpenGL context", viewport.Message, StringComparison.Ordinal);
        Assert.False(inspector.IsAvailable);
        Assert.Contains("Backend unavailable", inspector.Message, StringComparison.Ordinal);
        Assert.Contains("No property mutation", inspector.Message, StringComparison.Ordinal);
    }

    [Fact]
    public void LayoutActions_DoNotWidenExistingBackendLaunchAdmission()
    {
        Assert.False(SessionLossPolicy.CanStartBackend(HostSessionState.Connecting, hasOwnedBackend: false));
        Assert.False(SessionLossPolicy.CanStartBackend(HostSessionState.Connected, hasOwnedBackend: true));
        Assert.False(SessionLossPolicy.CanStartBackend(HostSessionState.Failed, hasOwnedBackend: true));
        Assert.True(SessionLossPolicy.CanStartBackend(HostSessionState.Failed, hasOwnedBackend: false));
    }

    public void Dispose()
    {
        if (Directory.Exists(temporaryDirectory))
        {
            Directory.Delete(temporaryDirectory, recursive: true);
        }
    }

    private string CreatePath(string fileName)
    {
        Directory.CreateDirectory(temporaryDirectory);
        return Path.Combine(temporaryDirectory, fileName);
    }
}
