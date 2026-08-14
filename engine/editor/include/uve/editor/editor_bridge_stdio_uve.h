// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstddef>
#include <iosfwd>

#include "uve/editor/editor_bridge_uve.h"

namespace UVE::Editor {

/// EditorBridgeStdioServerUVE exposes one EditorBridgeUVE through bounded length-prefixed UTF-8
/// JSON-RPC frames on caller-owned streams. It is main-thread only and is used exclusively by the
/// headless bridge-server process mode; native ImGui never shares this server's EditorUVE instance.
class EditorBridgeStdioServerUVE final {
public:
    /// Frames larger than this are rejected before allocating a JSON body.
    static constexpr std::size_t kMaximumFrameBytesUVE = 1024U * 1024U;

    explicit EditorBridgeStdioServerUVE(EditorBridgeUVE& bridge) noexcept;

    /// Serves frames until input reaches EOF. A malformed request returns one stable error frame and
    /// leaves the authoritative editor state unchanged. The caller owns lifecycle and stream closure.
    [[nodiscard]] int ServeUVE(std::istream& input, std::ostream& output, std::ostream& diagnostics);

private:
    EditorBridgeUVE* m_bridge = nullptr;
};

} // namespace UVE::Editor
