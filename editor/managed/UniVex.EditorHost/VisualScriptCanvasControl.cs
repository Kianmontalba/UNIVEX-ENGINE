// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
using System.Globalization;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Media;
using UniVex.EditorHost;

namespace UniVex.EditorHost;

public sealed class VisualScriptCanvasCommandEventArgs : EventArgs
{
    public VisualScriptCanvasCommandEventArgs(BridgeCommand command)
    {
        Command = command;
    }

    public BridgeCommand Command { get; }
}

/// <summary>
/// Managed presentation-only visual-scripting canvas. It renders copied native DTOs and emits
/// named value-only commands; it never retains native graph, ECS, renderer, or filesystem objects.
/// </summary>
public sealed class VisualScriptCanvasControl : Control
{
    private const double NodeWidth = 184D;
    private const double HeaderHeight = 28D;
    private const double PinRowHeight = 17D;
    private const double NodePadding = 8D;
    private const double MinimumZoom = 0.1D;
    private const double MaximumZoom = 8D;

    private static readonly IBrush BackgroundBrush = new SolidColorBrush(Color.Parse("#11151B"));
    private static readonly IBrush GridBrush = new SolidColorBrush(Color.Parse("#222A35"));
    private static readonly IBrush NodeBrush = new SolidColorBrush(Color.Parse("#263241"));
    private static readonly IBrush SelectedNodeBrush = new SolidColorBrush(Color.Parse("#304A64"));
    private static readonly IBrush NodeHeaderBrush = new SolidColorBrush(Color.Parse("#34465B"));
    private static readonly IBrush LinkBrush = new SolidColorBrush(Color.Parse("#8EB6D8"));
    private static readonly IBrush SelectedBrush = new SolidColorBrush(Color.Parse("#F0B84B"));
    private static readonly IBrush ForegroundBrush = new SolidColorBrush(Color.Parse("#E6EDF5"));
    private static readonly IBrush MutedBrush = new SolidColorBrush(Color.Parse("#8C9AAF"));
    private static readonly Pen GridPen = new(GridBrush, 1D);
    private static readonly Pen LinkPen = new(LinkBrush, 2D);
    private static readonly Pen NodePen = new(new SolidColorBrush(Color.Parse("#536477")), 1D);
    private static readonly Pen SelectedPen = new(SelectedBrush, 2D);

    private BridgeVisualScriptCanvasSnapshot canvas = EmptyCanvas();
    private ulong bridgeRevision;
    private Point pointerDown;
    private Point lastPointer;
    private uint? pressedNodeId;
    private bool pointerCaptured;
    private bool movedNode;
    private bool panning;
    private BridgeVisualScriptPoint panAtPointerDown = new(0F, 0F);

    public event EventHandler<VisualScriptCanvasCommandEventArgs>? CommandRequested;

    public BridgeVisualScriptCanvasSnapshot CanvasSnapshot => canvas;

    public ulong BridgeRevision => bridgeRevision;

    public Rect GetNodeScreenRect(uint nodeId)
    {
        BridgeVisualScriptNode? node = canvas.Nodes.FirstOrDefault(item => item.Id == nodeId);
        return node is null ? new Rect() : NodeRect(node);
    }

    public Point GetPinScreenPoint(uint nodeId, string pinName, bool outputSide)
    {
        BridgeVisualScriptNode? node = canvas.Nodes.FirstOrDefault(item => item.Id == nodeId);
        return node is null ? new Point(double.NaN, double.NaN) : PinAnchor(node, pinName, outputSide);
    }

    public void ApplySnapshot(BridgeVisualScriptCanvasSnapshot next, ulong nextBridgeRevision)
    {
        ArgumentNullException.ThrowIfNull(next);
        canvas = next;
        bridgeRevision = nextBridgeRevision;
        InvalidateVisual();
    }

    public override void Render(DrawingContext context)
    {
        base.Render(context);
        context.DrawRectangle(BackgroundBrush, null, new Rect(Bounds.Size));
        DrawGrid(context);
        DrawLinks(context);
        DrawNodes(context);
        DrawFooter(context);
    }

    protected override void OnPointerPressed(PointerPressedEventArgs e)
    {
        base.OnPointerPressed(e);
        PointerPoint point = e.GetCurrentPoint(this);
        if (!point.Properties.IsLeftButtonPressed && !point.Properties.IsMiddleButtonPressed)
        {
            return;
        }

        pointerDown = point.Position;
        lastPointer = pointerDown;
        e.Pointer.Capture(this);
        pointerCaptured = true;

        if (point.Properties.IsMiddleButtonPressed)
        {
            panning = true;
            panAtPointerDown = canvas.View.Pan;
            e.Handled = true;
            return;
        }

        panning = false;
        movedNode = false;
        pressedNodeId = HitTestNode(pointerDown);
        if (pressedNodeId.HasValue)
        {
            EmitSelection(new[] { pressedNodeId.Value });
        }
        else
        {
            EmitSelection(Array.Empty<uint>());
        }
        e.Handled = true;
    }

    protected override void OnPointerMoved(PointerEventArgs e)
    {
        base.OnPointerMoved(e);
        if (!pointerCaptured)
        {
            return;
        }

        Point position = e.GetPosition(this);
        if (panning)
        {
            lastPointer = position;
            InvalidateVisual();
            e.Handled = true;
            return;
        }
        Vector displacement = position - pointerDown;
        if (pressedNodeId.HasValue && displacement.Length > 3D)
        {
            movedNode = true;
            lastPointer = position;
            InvalidateVisual();
            e.Handled = true;
        }
    }

    protected override void OnPointerReleased(PointerReleasedEventArgs e)
    {
        base.OnPointerReleased(e);
        if (!pointerCaptured)
        {
            return;
        }

        Point position = e.GetPosition(this);
        if (panning)
        {
            Vector delta = position - pointerDown;
            double zoom = Math.Max(canvas.View.Zoom, 0.1F);
            BridgeVisualScriptPoint nextPan = new(
                panAtPointerDown.X + (float)(delta.X / zoom),
                panAtPointerDown.Y + (float)(delta.Y / zoom));
            EmitView(new BridgeVisualScriptView(nextPan, canvas.View.Zoom));
        }
        else if (pressedNodeId.HasValue && movedNode)
        {
            BridgeVisualScriptNode? node = canvas.Nodes.FirstOrDefault(item => item.Id == pressedNodeId.Value);
            if (node is not null)
            {
                Vector delta = position - pointerDown;
                double zoom = Math.Max(canvas.View.Zoom, 0.1F);
                EmitMove(pressedNodeId.Value, new BridgeVisualScriptPoint(
                    node.Position.X + (float)(delta.X / zoom),
                    node.Position.Y + (float)(delta.Y / zoom)));
            }
        }

        e.Pointer.Capture(null);
        pointerCaptured = false;
        pressedNodeId = null;
        movedNode = false;
        panning = false;
        lastPointer = position;
        e.Handled = true;
    }

    protected override void OnPointerWheelChanged(PointerWheelEventArgs e)
    {
        base.OnPointerWheelChanged(e);
        double nextZoom = Math.Clamp(canvas.View.Zoom * Math.Pow(1.1D, e.Delta.Y), MinimumZoom, MaximumZoom);
        EmitView(new BridgeVisualScriptView(canvas.View.Pan, (float)nextZoom));
        e.Handled = true;
    }

    public void RequestUndo() => EmitSimple("undoVisualScript");

    public void RequestRedo() => EmitSimple("redoVisualScript");

    private static BridgeVisualScriptCanvasSnapshot EmptyCanvas() => new(
        0UL, 0UL, new BridgeVisualScriptView(new BridgeVisualScriptPoint(0F, 0F), 1F),
        false, false, false, false, false, false, false,
        Array.Empty<BridgeVisualScriptNode>(), Array.Empty<BridgeVisualScriptLink>(),
        Array.Empty<uint>(), Array.Empty<string>(), Array.Empty<BridgeVisualScriptDiagnostic>());

    private void DrawGrid(DrawingContext context)
    {
        double zoom = Math.Max(canvas.View.Zoom, 0.1F);
        double spacing = Math.Clamp(32D * zoom, 8D, 96D);
        for (double x = Mod(canvas.View.Pan.X * zoom, spacing); x < Bounds.Width; x += spacing)
        {
            context.DrawLine(GridPen, new Point(x, 0D), new Point(x, Bounds.Height));
        }
        for (double y = Mod(canvas.View.Pan.Y * zoom, spacing); y < Bounds.Height; y += spacing)
        {
            context.DrawLine(GridPen, new Point(0D, y), new Point(Bounds.Width, y));
        }
    }

    private void DrawLinks(DrawingContext context)
    {
        foreach (BridgeVisualScriptLink link in canvas.Links)
        {
            BridgeVisualScriptNode? outputNode = canvas.Nodes.FirstOrDefault(node => node.Id == link.Output.NodeId);
            BridgeVisualScriptNode? inputNode = canvas.Nodes.FirstOrDefault(node => node.Id == link.Input.NodeId);
            if (outputNode is null || inputNode is null)
            {
                continue;
            }
            Point output = PinAnchor(outputNode, link.Output.PinName, outputSide: true);
            Point input = PinAnchor(inputNode, link.Input.PinName, outputSide: false);
            Point midpoint = new((output.X + input.X) / 2D, (output.Y + input.Y) / 2D);
            context.DrawLine(LinkPen, output, new Point(midpoint.X, output.Y));
            context.DrawLine(LinkPen, new Point(midpoint.X, output.Y), new Point(midpoint.X, input.Y));
            context.DrawLine(LinkPen, new Point(midpoint.X, input.Y), input);
        }
    }

    private void DrawNodes(DrawingContext context)
    {
        foreach (BridgeVisualScriptNode node in canvas.Nodes)
        {
            Rect rect = NodeRect(node);
            bool selected = node.IsSelected || canvas.SelectedNodeIds.Contains(node.Id);
            context.DrawRectangle(selected ? SelectedNodeBrush : NodeBrush, selected ? SelectedPen : NodePen, rect, 4D, 4D);
            Rect header = new(rect.X, rect.Y, rect.Width, Math.Min(HeaderHeight * canvas.View.Zoom, rect.Height));
            context.DrawRectangle(NodeHeaderBrush, null, header, 4D, 4D);
            DrawText(context, node.DisplayName.Length == 0 ? node.TypeId : node.DisplayName,
                new Point(rect.X + NodePadding * canvas.View.Zoom, rect.Y + 6D * canvas.View.Zoom), 12D * canvas.View.Zoom, ForegroundBrush);
            for (int index = 0; index < node.Pins.Count; ++index)
            {
                BridgeVisualScriptPin pin = node.Pins[index];
                bool output = pin.Direction != 0;
                double y = rect.Y + (HeaderHeight + NodePadding + index * PinRowHeight) * canvas.View.Zoom;
                double x = output ? rect.Right - NodePadding * canvas.View.Zoom : rect.X + NodePadding * canvas.View.Zoom;
                context.DrawEllipse(output ? SelectedBrush : LinkBrush, null, new Point(x, y), 3D * canvas.View.Zoom, 3D * canvas.View.Zoom);
                double textX = output ? rect.X + NodePadding * canvas.View.Zoom : rect.X + 15D * canvas.View.Zoom;
                DrawText(context, pin.Name, new Point(textX, y - 6D * canvas.View.Zoom), 10D * canvas.View.Zoom, MutedBrush);
            }
        }
    }

    private void DrawFooter(DrawingContext context)
    {
        string text = $"{canvas.Nodes.Count} node(s) · {canvas.Links.Count} link(s) · zoom {canvas.View.Zoom:0.00} · " +
                      $"palette {canvas.PaletteNodeTypeIds.Count} · diagnostics {canvas.Diagnostics.Count} · " +
                      $"dirty {canvas.Dirty} · undo {canvas.CanUndo} · redo {canvas.CanRedo}";
        DrawText(context, text, new Point(10D, Math.Max(10D, Bounds.Height - 22D)), 11D, MutedBrush);
        if (canvas.NodesTruncated || canvas.LinksTruncated || canvas.PaletteTruncated || canvas.DiagnosticsTruncated)
        {
            DrawText(context, "Native snapshot truncated by bounded bridge policy.",
                new Point(10D, Math.Max(10D, Bounds.Height - 40D)), 11D, SelectedBrush);
        }
    }

    private Rect NodeRect(BridgeVisualScriptNode node)
    {
        double zoom = Math.Max(canvas.View.Zoom, 0.1F);
        double height = HeaderHeight + NodePadding * 2D + Math.Max(1, node.Pins.Count) * PinRowHeight;
        Point topLeft = WorldToScreen(node.Position, new Size(NodeWidth * zoom, height * zoom));
        return new Rect(topLeft, new Size(NodeWidth * zoom, height * zoom));
    }

    private Point PinAnchor(BridgeVisualScriptNode node, string pinName, bool outputSide)
    {
        Rect rect = NodeRect(node);
        int index = Math.Max(0, node.Pins.ToList().FindIndex(pin => pin.Name == pinName));
        double y = rect.Y + (HeaderHeight + NodePadding + index * PinRowHeight) * canvas.View.Zoom;
        return new Point(outputSide ? rect.Right : rect.Left, y);
    }

    public uint? HitTestNode(Point point)
    {
        for (int index = canvas.Nodes.Count - 1; index >= 0; --index)
        {
            BridgeVisualScriptNode node = canvas.Nodes[index];
            if (NodeRect(node).Contains(point))
            {
                return node.Id;
            }
        }
        return null;
    }

    private Point WorldToScreen(BridgeVisualScriptPoint point, Size size) => new(
        (point.X + canvas.View.Pan.X) * canvas.View.Zoom + Bounds.Width / 2D - size.Width / 2D,
        (point.Y + canvas.View.Pan.Y) * canvas.View.Zoom + Bounds.Height / 2D - size.Height / 2D);

    private void EmitSelection(IReadOnlyList<uint> selection) => Emit(new BridgeCommand(
        bridgeRevision, "setVisualScriptSelection", VisualScriptSelection: selection));

    private void EmitMove(uint nodeId, BridgeVisualScriptPoint position) => Emit(new BridgeCommand(
        bridgeRevision, "moveVisualScriptNode", VisualScriptNodeId: nodeId, VisualScriptPosition: position));

    private void EmitView(BridgeVisualScriptView view) => Emit(new BridgeCommand(
        bridgeRevision, "setVisualScriptView", VisualScriptView: view));

    private void EmitSimple(string kind) => Emit(new BridgeCommand(bridgeRevision, kind));

    private void Emit(BridgeCommand command) =>
        CommandRequested?.Invoke(this, new VisualScriptCanvasCommandEventArgs(command));

    private static void DrawText(DrawingContext context, string text, Point origin, double size, IBrush brush)
    {
        FormattedText formatted = new(text, CultureInfo.InvariantCulture, FlowDirection.LeftToRight,
            new Typeface("Inter"), Math.Max(8D, size), brush);
        context.DrawText(formatted, origin);
    }

    private static double Mod(double value, double modulus)
    {
        double result = value % modulus;
        return result < 0D ? result + modulus : result;
    }
}
