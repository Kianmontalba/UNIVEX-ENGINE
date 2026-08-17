// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
using Avalonia;
using UniVex.EditorHost;

namespace UniVex.EditorHost.Tests;

public sealed class VisualScriptCanvasControlTests
{
    [Fact]
    public void ApplySnapshot_UsesCopiedBridgeRevisionAndProvidesDeterministicGeometry()
    {
        VisualScriptCanvasControl control = CreateControl();
        BridgeVisualScriptNode node = new(
            7U, "test.node", "Test Node", new BridgeVisualScriptPoint(0F, 0F), true,
            new[] { new BridgeVisualScriptPin("In", 0, 1), new BridgeVisualScriptPin("Out", 1, 1) });
        BridgeVisualScriptCanvasSnapshot snapshot = Snapshot(node);

        control.ApplySnapshot(snapshot, 42UL);

        Assert.Equal(42UL, control.BridgeRevision);
        Rect rect = control.GetNodeScreenRect(7U);
        Assert.True(rect.Contains(new Point(400D, 300D)));
        Assert.Equal(7U, control.HitTestNode(new Point(400D, 300D)));
        Point output = control.GetPinScreenPoint(7U, "Out", outputSide: true);
        Point input = control.GetPinScreenPoint(7U, "In", outputSide: false);
        Assert.Equal(rect.Right, output.X);
        Assert.Equal(rect.Left, input.X);
        Assert.True(output.Y > input.Y);
    }

    [Fact]
    public void NodePresentation_MapsKnownCategoriesToStableLiquidGlassStyles()
    {
        BridgeVisualScriptNode eventNode = new(
            1U, "test.event", "Event", new BridgeVisualScriptPoint(0F, 0F), false,
            Array.Empty<BridgeVisualScriptPin>(), Category: "Event");
        BridgeVisualScriptNode unknownNode = eventNode with { Category = "FutureCategory" };

        VisualScriptNodePresentation eventStyle = VisualScriptCanvasControl.DescribeNodePresentation(eventNode);
        VisualScriptNodePresentation fallbackStyle = VisualScriptCanvasControl.DescribeNodePresentation(unknownNode);

        Assert.Equal("#78E5D5", eventStyle.AccentHex);
        Assert.Equal("E", eventStyle.IconGlyph);
        Assert.False(eventStyle.UsesFallback);
        Assert.Equal("#1B222C", fallbackStyle.FillHex);
        Assert.Equal("?", fallbackStyle.IconGlyph);
        Assert.True(fallbackStyle.UsesFallback);
    }

    [Fact]
    public void NodePresentation_IsCaseInsensitiveAndDoesNotMutateCopiedNodeDto()
    {
        BridgeVisualScriptNode node = new(
            2U, "test.math", "Math", new BridgeVisualScriptPoint(0F, 0F), false,
            new[] { new BridgeVisualScriptPin("Value", 0, 3) }, Category: "mAtH");

        VisualScriptNodePresentation style = VisualScriptCanvasControl.DescribeNodePresentation(node);

        Assert.Equal("#30291A", style.FillHex);
        Assert.Equal("#F4C56B", style.PinHex);
        Assert.Equal("mAtH", node.Category);
    }

    [Fact]
    public void PaletteFilter_SearchesCopiedDescriptorTextAndSortsDeterministically()
    {
        BridgeVisualScriptPaletteEntry math = new(
            "math.add", "Add", "Math", "node.math", 20U, 0U, Array.Empty<BridgeVisualScriptPin>());
        BridgeVisualScriptPaletteEntry eventNode = new(
            "event.begin", "Begin", "Event", "node.event", 10U, 0U, Array.Empty<BridgeVisualScriptPin>());

        IReadOnlyList<BridgeVisualScriptPaletteEntry> result = VisualScriptCanvasControl.FilterPalette(
            new[] { math, eventNode }, "event");

        Assert.Single(result);
        Assert.Equal("event.begin", result[0].TypeId);
        Assert.Equal("Event", result[0].Category);
        Assert.Empty(VisualScriptCanvasControl.FilterPalette(new[] { math, eventNode }, "missing"));
    }

    [Fact]
    public void RequestPaletteInsertion_UsesNamedTypeOnlyCommandWithNativeRevision()
    {
        VisualScriptCanvasControl control = CreateControl();
        control.ApplySnapshot(Snapshot(), 23UL);
        BridgeVisualScriptPaletteEntry entry = new(
            "math.add", "Add", "Math", "node.math", 1U, 0U, Array.Empty<BridgeVisualScriptPin>());
        List<BridgeCommand> commands = new();
        control.CommandRequested += (_, args) => commands.Add(args.Command);

        control.RequestPaletteInsertion(entry);

        Assert.Single(commands);
        Assert.Equal("addVisualScriptNodeType", commands[0].Kind);
        Assert.Equal(23UL, commands[0].ExpectedRevision);
        Assert.Equal("math.add", commands[0].VisualScriptNodeTypeId);
    }

    [Fact]
    public void RequestPaletteInsertion_PreservesCanvasCursorPosition()
    {
        VisualScriptCanvasControl control = CreateControl();
        control.ApplySnapshot(Snapshot(), 24UL);
        BridgeVisualScriptPaletteEntry entry = new(
            "math.add", "Add", "Math", "node.math", 1U, 0U, Array.Empty<BridgeVisualScriptPin>());
        List<BridgeCommand> commands = new();
        control.CommandRequested += (_, args) => commands.Add(args.Command);

        control.RequestPaletteInsertion(entry, new BridgeVisualScriptPoint(120F, -48F));

        Assert.Single(commands);
        Assert.Equal("math.add", commands[0].VisualScriptNodeTypeId);
        Assert.Equal(new BridgeVisualScriptPoint(120F, -48F), commands[0].VisualScriptPosition);
    }

    [Fact]
    public void ScreenToCanvasPoint_AccountsForPanAndZoom()
    {
        VisualScriptCanvasControl control = CreateControl();
        BridgeVisualScriptCanvasSnapshot snapshot = Snapshot() with
        {
            View = new BridgeVisualScriptView(new BridgeVisualScriptPoint(10F, -20F), 2F),
        };
        control.ApplySnapshot(snapshot, 25UL);

        BridgeVisualScriptPoint canvasPoint = control.ScreenToCanvasPoint(new Point(400D, 300D));

        Assert.Equal(-10F, canvasPoint.X);
        Assert.Equal(20F, canvasPoint.Y);
    }

    [Fact]
    public void LinkAuthoring_EmitsNamedCommandOnlyForCompatibleInputTarget()
    {
        VisualScriptCanvasControl control = CreateControl();
        control.ApplySnapshot(LinkSnapshot(), 31UL);
        List<BridgeCommand> commands = new();
        control.CommandRequested += (_, args) => commands.Add(args.Command);
        Point output = control.GetPinScreenPoint(1U, "Out", outputSide: true);

        Assert.True(control.BeginLinkAuthoring(1U, "Out", output));
        Assert.Equal(new uint[] { 2U }, control.GetCompatibleInputNodeIds());
        Assert.True(control.CompleteLinkAuthoring(2U, "In"));
        Assert.Single(commands);
        Assert.Equal("addVisualScriptLink", commands[0].Kind);
        Assert.Equal(31UL, commands[0].ExpectedRevision);
        BridgeVisualScriptLink link = commands[0].VisualScriptLink ?? throw new Xunit.Sdk.XunitException("Link payload was not emitted.");
        Assert.Equal(new BridgeVisualScriptEndpoint(1U, "Out"), link.Output);
        Assert.Equal(new BridgeVisualScriptEndpoint(2U, "In"), link.Input);
        Assert.Contains("native validation", control.LinkAuthoringFeedback, StringComparison.OrdinalIgnoreCase);
    }

    [Fact]
    public void LinkAuthoring_RejectsIncompatibleInputBeforeEmittingMutation()
    {
        VisualScriptCanvasControl control = CreateControl();
        control.ApplySnapshot(LinkSnapshot(), 32UL);
        List<BridgeCommand> commands = new();
        control.CommandRequested += (_, args) => commands.Add(args.Command);

        Assert.True(control.BeginLinkAuthoring(1U, "Out", control.GetPinScreenPoint(1U, "Out", true)));
        Assert.False(control.CompleteLinkAuthoring(2U, "Bad"));
        Assert.Empty(commands);
        Assert.Contains("incompatible", control.LinkAuthoringFeedback, StringComparison.OrdinalIgnoreCase);
    }

    [Fact]
    public void RequestUndoAndRedo_EmitNamedCommandsWithCurrentBridgeRevision()
    {
        VisualScriptCanvasControl control = CreateControl();
        control.ApplySnapshot(Snapshot(), 19UL);
        List<BridgeCommand> commands = new();
        control.CommandRequested += (_, args) => commands.Add(args.Command);

        control.RequestUndo();
        control.RequestRedo();

        Assert.Collection(commands,
            undo =>
            {
                Assert.Equal("undoVisualScript", undo.Kind);
                Assert.Equal(19UL, undo.ExpectedRevision);
            },
            redo =>
            {
                Assert.Equal("redoVisualScript", redo.Kind);
                Assert.Equal(19UL, redo.ExpectedRevision);
            });
    }

    [Fact]
    public void ScriptRuntimeInstanceEntry_DisplayTextPresentsCopiedExecutionFacts()
    {
        BridgeScriptRuntimeInstanceEntry entry = new(7U, 3U, 9UL, 4U, 12, 2, true);

        Assert.Equal("Entity 7:3 · generation 9 · program v4 · 12 instruction(s) · 2 state value(s) · enabled",
            entry.DisplayText);
        Assert.Contains("disabled", new BridgeScriptRuntimeInstanceEntry(8U, 1U, 10UL, 5U, 3, 0, false).DisplayText);
    }

    [Fact]
    public void ScriptRuntimeInstanceEntry_MatchesLocalFilterWithoutNativeMutation()
    {
        BridgeScriptRuntimeInstanceEntry entry = new(7U, 3U, 9UL, 4U, 12, 2, true);

        Assert.True(entry.MatchesFilter("ENTITY 7:3"));
        Assert.True(entry.MatchesFilter("enabled"));
        Assert.True(entry.MatchesFilter("  "));
        Assert.False(entry.MatchesFilter("entity 99"));
    }

    [Fact]
    public void EmptySnapshot_HitTestingDoesNotInventManagedNodes()
    {
        VisualScriptCanvasControl control = CreateControl();
        control.ApplySnapshot(Snapshot(), 1UL);

        Assert.Equal(new Rect(), control.GetNodeScreenRect(99U));
        Assert.Null(control.HitTestNode(new Point(400D, 300D)));
        Assert.True(double.IsNaN(control.GetPinScreenPoint(99U, "missing", outputSide: true).X));
    }

    private static VisualScriptCanvasControl CreateControl()
    {
        VisualScriptCanvasControl control = new();
        control.Measure(new Size(800D, 600D));
        control.Arrange(new Rect(0D, 0D, 800D, 600D));
        return control;
    }

    private static BridgeVisualScriptCanvasSnapshot LinkSnapshot() =>
        new(4UL, 3UL, new BridgeVisualScriptView(new BridgeVisualScriptPoint(0F, 0F), 1F),
            false, false, false, false, false, false, false,
            new[]
            {
                new BridgeVisualScriptNode(1U, "test.source", "Source", new BridgeVisualScriptPoint(-120F, 0F), false,
                    new[] { new BridgeVisualScriptPin("Out", 1, 2) }),
                new BridgeVisualScriptNode(2U, "test.sink", "Sink", new BridgeVisualScriptPoint(120F, 0F), false,
                    new[] { new BridgeVisualScriptPin("In", 0, 2), new BridgeVisualScriptPin("Bad", 0, 3) }),
            },
            Array.Empty<BridgeVisualScriptLink>(), Array.Empty<uint>(), new[] { "test.source", "test.sink" },
            Array.Empty<BridgeVisualScriptDiagnostic>());

    private static BridgeVisualScriptCanvasSnapshot Snapshot(BridgeVisualScriptNode? node = null) =>
        new(3UL, 2UL, new BridgeVisualScriptView(new BridgeVisualScriptPoint(0F, 0F), 1F),
            false, false, false, false, false, false, false,
            node is null ? Array.Empty<BridgeVisualScriptNode>() : new[] { node },
            Array.Empty<BridgeVisualScriptLink>(),
            node is null ? Array.Empty<uint>() : new[] { node.Id },
            new[] { "test.node" }, Array.Empty<BridgeVisualScriptDiagnostic>());
}
