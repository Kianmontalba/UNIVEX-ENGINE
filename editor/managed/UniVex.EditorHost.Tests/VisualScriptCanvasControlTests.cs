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

    private static BridgeVisualScriptCanvasSnapshot Snapshot(BridgeVisualScriptNode? node = null) =>
        new(3UL, 2UL, new BridgeVisualScriptView(new BridgeVisualScriptPoint(0F, 0F), 1F),
            false, false, false, false,
            node is null ? Array.Empty<BridgeVisualScriptNode>() : new[] { node },
            Array.Empty<BridgeVisualScriptLink>(),
            node is null ? Array.Empty<uint>() : new[] { node.Id },
            new[] { "test.node" }, Array.Empty<BridgeVisualScriptDiagnostic>());
}
