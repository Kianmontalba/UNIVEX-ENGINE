// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

using System.Text.Json;

namespace UniVex.EditorHost;

/// <summary>
/// Persists managed shell presentation preferences only. It never reads or writes project, scene,
/// asset, or native-editor settings files. Writes occur only through an explicit caller request.
/// </summary>
public sealed class DockShellPreferencesStore
{
    private const string SchemaVersionProperty = "schemaVersion";
    private static readonly JsonSerializerOptions SerializationOptions = new(JsonSerializerDefaults.Web)
    {
        WriteIndented = true,
    };

    public DockShellPreferencesStore(string filePath)
    {
        if (string.IsNullOrWhiteSpace(filePath))
        {
            throw new ArgumentException("A managed shell layout file path is required.", nameof(filePath));
        }
        FilePath = filePath;
    }

    public string FilePath { get; }

    public async Task<DockShellLayoutState> LoadAsync(CancellationToken cancellationToken)
    {
        if (!File.Exists(FilePath))
        {
            return DockShellLayoutState.Default;
        }

        try
        {
            await using FileStream input = new(FilePath, FileMode.Open, FileAccess.Read, FileShare.Read);
            using JsonDocument document = await JsonDocument.ParseAsync(input, cancellationToken: cancellationToken)
                .ConfigureAwait(false);
            return Parse(document.RootElement);
        }
        catch (IOException)
        {
            return DockShellLayoutState.Default;
        }
        catch (JsonException)
        {
            return DockShellLayoutState.Default;
        }
        catch (UnauthorizedAccessException)
        {
            return DockShellLayoutState.Default;
        }
    }

    public async Task SaveAsync(DockShellLayoutState layout, CancellationToken cancellationToken)
    {
        ArgumentNullException.ThrowIfNull(layout);
        string? directory = Path.GetDirectoryName(FilePath);
        if (string.IsNullOrWhiteSpace(directory))
        {
            throw new InvalidOperationException("The managed shell layout path must include a directory.");
        }

        Directory.CreateDirectory(directory);
        DockShellLayoutState normalized = layout.Normalize();
        string temporaryPath = Path.Combine(directory, $".{Path.GetFileName(FilePath)}.{Guid.NewGuid():N}.tmp");
        try
        {
            await using (FileStream output = new(temporaryPath, FileMode.CreateNew, FileAccess.Write, FileShare.None))
            {
                await JsonSerializer.SerializeAsync(output, new PersistedLayoutV1(normalized), SerializationOptions,
                    cancellationToken: cancellationToken).ConfigureAwait(false);
                await output.FlushAsync(cancellationToken).ConfigureAwait(false);
            }
            File.Move(temporaryPath, FilePath, overwrite: true);
        }
        finally
        {
            if (File.Exists(temporaryPath))
            {
                File.Delete(temporaryPath);
            }
        }
    }

    private static DockShellLayoutState Parse(JsonElement root)
    {
        if (root.ValueKind != JsonValueKind.Object)
        {
            return DockShellLayoutState.Default;
        }

        if (!root.TryGetProperty(SchemaVersionProperty, out JsonElement versionValue))
        {
            return HasRecognizedV0Shape(root) ? ParseV0AndMigrate(root) : DockShellLayoutState.Default;
        }
        if (versionValue.ValueKind != JsonValueKind.Number || !versionValue.TryGetInt32(out int schemaVersion))
        {
            return DockShellLayoutState.Default;
        }

        return schemaVersion switch
        {
            0 when HasRecognizedV0Shape(root) => ParseV0AndMigrate(root),
            DockShellLayoutState.CurrentSchemaVersion => ParseV1(root),
            _ => DockShellLayoutState.Default,
        };
    }

    private static bool HasRecognizedV0Shape(JsonElement root)
    {
        return root.TryGetProperty("leftDockWidth", out _) &&
               root.TryGetProperty("rightDockWidth", out _) &&
               root.TryGetProperty("bottomDockHeight", out _) &&
               root.TryGetProperty("activeWorkspace", out _) &&
               root.TryGetProperty("leftTab", out _) &&
               root.TryGetProperty("centerTab", out _) &&
               root.TryGetProperty("rightTab", out _) &&
               root.TryGetProperty("bottomTab", out _);
    }

    private static DockShellLayoutState ParseV0AndMigrate(JsonElement root)
    {
        DockShellLayoutState defaults = DockShellLayoutState.Default;
        return new DockShellLayoutState(
            LeftDockWidth: ReadDouble(root, "leftDockWidth", defaults.LeftDockWidth),
            RightDockWidth: ReadDouble(root, "rightDockWidth", defaults.RightDockWidth),
            BottomDockHeight: ReadDouble(root, "bottomDockHeight", defaults.BottomDockHeight),
            ActiveWorkspace: ReadWorkspace(root, "activeWorkspace", defaults.ActiveWorkspace),
            LeftTab: ReadString(root, "leftTab", defaults.LeftTab),
            CenterTab: ReadString(root, "centerTab", defaults.CenterTab),
            RightTab: ReadString(root, "rightTab", defaults.RightTab),
            BottomTab: ReadString(root, "bottomTab", defaults.BottomTab),
            IsLeftDockVisible: defaults.IsLeftDockVisible,
            IsRightDockVisible: defaults.IsRightDockVisible,
            IsBottomDockVisible: defaults.IsBottomDockVisible).Normalize();
    }

    private static DockShellLayoutState ParseV1(JsonElement root)
    {
        DockShellLayoutState defaults = DockShellLayoutState.Default;
        return new DockShellLayoutState(
            LeftDockWidth: ReadDouble(root, "leftDockWidth", defaults.LeftDockWidth),
            RightDockWidth: ReadDouble(root, "rightDockWidth", defaults.RightDockWidth),
            BottomDockHeight: ReadDouble(root, "bottomDockHeight", defaults.BottomDockHeight),
            ActiveWorkspace: ReadWorkspace(root, "activeWorkspace", defaults.ActiveWorkspace),
            LeftTab: ReadString(root, "leftTab", defaults.LeftTab),
            CenterTab: ReadString(root, "centerTab", defaults.CenterTab),
            RightTab: ReadString(root, "rightTab", defaults.RightTab),
            BottomTab: ReadString(root, "bottomTab", defaults.BottomTab),
            IsLeftDockVisible: ReadBoolean(root, "isLeftDockVisible", defaults.IsLeftDockVisible),
            IsRightDockVisible: ReadBoolean(root, "isRightDockVisible", defaults.IsRightDockVisible),
            IsBottomDockVisible: ReadBoolean(root, "isBottomDockVisible", defaults.IsBottomDockVisible)).Normalize();
    }

    private static double ReadDouble(JsonElement root, string name, double fallback)
    {
        return root.TryGetProperty(name, out JsonElement value) && value.ValueKind == JsonValueKind.Number &&
               value.TryGetDouble(out double parsed)
            ? parsed
            : fallback;
    }

    private static bool ReadBoolean(JsonElement root, string name, bool fallback)
    {
        return root.TryGetProperty(name, out JsonElement value) &&
               (value.ValueKind == JsonValueKind.True || value.ValueKind == JsonValueKind.False)
            ? value.GetBoolean()
            : fallback;
    }

    private static string ReadString(JsonElement root, string name, string fallback)
    {
        return root.TryGetProperty(name, out JsonElement value) && value.ValueKind == JsonValueKind.String
            ? value.GetString() ?? fallback
            : fallback;
    }

    private static DockShellWorkspace ReadWorkspace(JsonElement root, string name, DockShellWorkspace fallback)
    {
        string value = ReadString(root, name, fallback.ToString());
        return Enum.TryParse(value, ignoreCase: false, out DockShellWorkspace workspace) ? workspace : fallback;
    }

    private sealed record PersistedLayoutV1
    {
        public PersistedLayoutV1(DockShellLayoutState layout)
        {
            SchemaVersion = DockShellLayoutState.CurrentSchemaVersion;
            LeftDockWidth = layout.LeftDockWidth;
            RightDockWidth = layout.RightDockWidth;
            BottomDockHeight = layout.BottomDockHeight;
            ActiveWorkspace = layout.ActiveWorkspace.ToString();
            LeftTab = layout.LeftTab;
            CenterTab = layout.CenterTab;
            RightTab = layout.RightTab;
            BottomTab = layout.BottomTab;
            IsLeftDockVisible = layout.IsLeftDockVisible;
            IsRightDockVisible = layout.IsRightDockVisible;
            IsBottomDockVisible = layout.IsBottomDockVisible;
        }

        public int SchemaVersion { get; }
        public double LeftDockWidth { get; }
        public double RightDockWidth { get; }
        public double BottomDockHeight { get; }
        public string ActiveWorkspace { get; }
        public string LeftTab { get; }
        public string CenterTab { get; }
        public string RightTab { get; }
        public string BottomTab { get; }
        public bool IsLeftDockVisible { get; }
        public bool IsRightDockVisible { get; }
        public bool IsBottomDockVisible { get; }
    }
}
