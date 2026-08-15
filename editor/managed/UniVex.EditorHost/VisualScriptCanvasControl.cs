// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
using System.Globalization;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Media;
using UniVex.EditorHost;

namespace UniVex.EditorHost;

public sealed record VisualScriptNodePresentation(
    string FillHex,
    string HeaderHex,
    string AccentHex,
    string PinHex,
    string IconGlyph,
    bool UsesFallback);

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

    private static readonly IBrush BackgroundBrush = new SolidColorBrush(Color.Parse("#0D1219"));
    private static readonly IBrush GridBrush = new SolidColorBrush(Color.Parse("#24303D"));
    private static readonly IBrush GridMajorBrush = new SolidColorBrush(Color.Parse("#314052"));
    private static readonly IBrush NodeBrush = new SolidColorBrush(Color.Parse("#17222D"));
    private static readonly IBrush SelectedNodeBrush = new SolidColorBrush(Color.Parse("#1D3445"));
    private static readonly IBrush LinkBrush = new SolidColorBrush(Color.Parse("#78B7D8"));
    private static readonly IBrush SelectedBrush = new SolidColorBrush(Color.Parse("#F5C66A"));
    private static readonly IBrush ForegroundBrush = new SolidColorBrush(Color.Parse("#EFF7FF"));
    private static readonly IBrush MutedBrush = new SolidColorBrush(Color.Parse("#9AAEC0"));
    private static readonly IBrush GlassHighlightBrush = new SolidColorBrush(Color.Parse("#5D7B91"));
    private static readonly Pen GridPen = new(GridBrush, 1D);
    private static readonly Pen GridMajorPen = new(GridMajorBrush, 1D);
    private static readonly Pen LinkPen = new(LinkBrush, 2D);
    private static readonly Pen SelectedLinkPen = new(SelectedBrush, 3D);
    private static readonly Pen NodePen = new(new SolidColorBrush(Color.Parse("#496274")), 1D);
    private static readonly Pen SelectedPen = new(SelectedBrush, 2D);
    private static readonly Pen GlassHighlightPen = new(GlassHighlightBrush, 1D);

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

    public static VisualScriptNodePresentation DescribeNodePresentation(BridgeVisualScriptNode node)
    {
        ArgumentNullException.ThrowIfNull(node);
        string category = node.Category.Trim();
        return category.ToUpperInvariant() switch
        {
            "EVENT" => new("#182B35", "#315C63", "#78E5D5", "#7EDBD1", "E", false),
            "FLOW" => new("#211E35", "#4B466E", "#BBA9FF", "#B8A8FF", "F", false),
            "DATA" => new("#172A38", "#2D586E", "#8BD3FF", "#86D0F7", "D", false),
            "MATH" => new("#30291A", "#5B4D2F", "#FFD081", "#F4C56B", "∑", false),
            "UTILITY" => new("#202A31", "#3D4D59", "#B5C7D8", "#B5C7D8", "U", false),
            _ => new("#1B222C", "#2D3440", "#8C9AAF", "#9AAEC0", "?", true),
        };
    }

    public static IReadOnlyList<BridgeVisualScriptPaletteEntry> FilterPalette(
        IEnumerable<BridgeVisualScriptPaletteEntry> entries, string? query)
    {
        ArgumentNullException.ThrowIfNull(entries);
        string filter = query?.Trim() ?? string.Empty;
        return entries
            .Where(entry => filter.Length == 0 ||
                            entry.TypeId.Contains(filter, StringComparison.OrdinalIgnoreCase) ||
                            entry.DisplayName.Contains(filter, StringComparison.OrdinalIgnoreCase) ||
                            entry.Category.Contains(filter, StringComparison.OrdinalIgnoreCase))
            .OrderBy(entry => entry.DisplayOrder)
            .ThenBy(entry => entry.Category, StringComparer.OrdinalIgnoreCase)
            .ThenBy(entry => entry.TypeId, StringComparer.Ordinal)
            .ToArray();
    }

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

    public void RequestPaletteInsertion(BridgeVisualScriptPaletteEntry entry) {
        ArgumentNullException.ThrowIfNull(entry);
        Emit(new BridgeCommand(bridgeRevision, "addVisualScriptNodeType",
            VisualScriptNodeTypeId: entry.TypeId,
            VisualScriptPosition: new BridgeVisualScriptPoint(0F, 0F)));
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
        double minorSpacing = Math.Clamp(24D * zoom, 8D, 72D);
        double majorSpacing = minorSpacing * 4D;
        for (double x = Mod(canvas.View.Pan.X * zoom, minorSpacing); x < Bounds.Width; x += minorSpacing)
        {
            context.DrawLine(GridPen, new Point(x, 0D), new Point(x, Bounds.Height));
        }
        for (double y = Mod(canvas.View.Pan.Y * zoom, minorSpacing); y < Bounds.Height; y += minorSpacing)
        {
            context.DrawLine(GridPen, new Point(0D, y), new Point(Bounds.Width, y));
        }
        for (double x = Mod(canvas.View.Pan.X * zoom, majorSpacing); x < Bounds.Width; x += majorSpacing)
        {
            context.DrawLine(GridMajorPen, new Point(x, 0D), new Point(x, Bounds.Height));
        }
        for (double y = Mod(canvas.View.Pan.Y * zoom, majorSpacing); y < Bounds.Height; y += majorSpacing)
        {
            context.DrawLine(GridMajorPen, new Point(0D, y), new Point(Bounds.Width, y));
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
            double curve = Math.Max(32D * canvas.View.Zoom, Math.Abs(input.X - output.X) * 0.42D);
            StreamGeometry geometry = new();
            using (StreamGeometryContext geometryContext = geometry.Open())
            {
                geometryContext.BeginFigure(output, isFilled: false);
                geometryContext.CubicBezierTo(
                    new Point(output.X + curve, output.Y),
                    new Point(input.X - curve, input.Y),
                    input);
            }
            bool selected = outputNode.IsSelected || inputNode.IsSelected ||
                            canvas.SelectedNodeIds.Contains(outputNode.Id) || canvas.SelectedNodeIds.Contains(inputNode.Id);
            context.DrawGeometry(null, selected ? SelectedLinkPen : LinkPen, geometry);
        }
    }

    private void DrawNodes(DrawingContext context)
    {
        foreach (BridgeVisualScriptNode node in canvas.Nodes.OrderBy(item => item.DisplayOrder).ThenBy(item => item.Id))
        {
            Rect rect = NodeRect(node);
            bool selected = node.IsSelected || canvas.SelectedNodeIds.Contains(node.Id);
            VisualScriptNodePresentation presentation = DescribeNodePresentation(node);
            IBrush fill = new SolidColorBrush(Color.Parse(selected ? "#213B4D" : presentation.FillHex));
            IBrush headerBrush = new SolidColorBrush(Color.Parse(presentation.HeaderHex));
            IBrush accentBrush = new SolidColorBrush(Color.Parse(presentation.AccentHex));
            context.DrawRectangle(fill, selected ? SelectedPen : NodePen, rect, 8D, 8D);
            Rect header = new(rect.X, rect.Y, rect.Width, Math.Min(HeaderHeight * canvas.View.Zoom, rect.Height));
            context.DrawRectangle(headerBrush, null, header, 8D, 8D);
            context.DrawLine(GlassHighlightPen, new Point(rect.X + 8D, rect.Y + 1D),
                new Point(rect.Right - 8D, rect.Y + 1D));
            context.DrawEllipse(accentBrush, null,
                new Point(rect.X + 15D * canvas.View.Zoom, rect.Y + 14D * canvas.View.Zoom),
                7D * canvas.View.Zoom, 7D * canvas.View.Zoom);
            DrawText(context, presentation.IconGlyph,
                new Point(rect.X + 11D * canvas.View.Zoom, rect.Y + 6D * canvas.View.Zoom),
                9D * canvas.View.Zoom, fill);
            string title = node.DisplayName.Length == 0 ? node.TypeId : node.DisplayName;
            string category = node.Category.Length == 0 ? "Uncategorized" : node.Category;
            DrawText(context, title,
                new Point(rect.X + 28D * canvas.View.Zoom, rect.Y + 3D * canvas.View.Zoom),
                11D * canvas.View.Zoom, ForegroundBrush);
            DrawText(context, presentation.UsesFallback ? "fallback" : category.ToLowerInvariant(),
                new Point(rect.X + 28D * canvas.View.Zoom, rect.Y + 15D * canvas.View.Zoom),
                8D * canvas.View.Zoom, MutedBrush);
            for (int index = 0; index < node.Pins.Count; ++index)
            {
                BridgeVisualScriptPin pin = node.Pins[index];
                bool output = pin.Direction != 0;
                double y = rect.Y + (HeaderHeight + NodePadding + index * PinRowHeight) * canvas.View.Zoom;
                double x = output ? rect.Right - NodePadding * canvas.View.Zoom : rect.X + NodePadding * canvas.View.Zoom;
                IBrush pinBrush = PinBrush(pin, presentation.PinHex);
                context.DrawEllipse(pinBrush, null, new Point(x, y), 4D * canvas.View.Zoom, 4D * canvas.View.Zoom);
                double textX = output ? rect.X + NodePadding * canvas.View.Zoom : rect.X + 16D * canvas.View.Zoom;
                DrawText(context, pin.Name, new Point(textX, y - 6D * canvas.View.Zoom),
                    10D * canvas.View.Zoom, ForegroundBrush);
                if (!string.IsNullOrWhiteSpace(pin.DefaultValue))
                {
                    DrawText(context, pin.DefaultValue,
                        new Point(rect.X + rect.Width / 2D, y - 6D * canvas.View.Zoom),
                        8D * canvas.View.Zoom, MutedBrush);
                }
            }
        }
    }

    private static IBrush PinBrush(BridgeVisualScriptPin pin, string fallbackHex) => pin.Type switch
    {
        1 => new SolidColorBrush(Color.Parse("#8BD3FF")),
        2 => new SolidColorBrush(Color.Parse("#BBA9FF")),
        3 => new SolidColorBrush(Color.Parse("#FFD081")),
        4 => new SolidColorBrush(Color.Parse("#78E5D5")),
        _ => new SolidColorBrush(Color.Parse(fallbackHex)),
    };

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
