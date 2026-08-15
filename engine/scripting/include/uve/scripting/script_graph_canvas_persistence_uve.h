// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/scripting/script_graph_canvas_uve.h"
#include "uve/scripting/script_graph_persistence_uve.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace UVE::Scripting {

struct ScriptGraphCanvasLayoutPersistenceLimitsUVE final {
    std::size_t maximumEntries = kMaximumScriptGraphCanvasEntriesUVE;
    std::size_t maximumTextBytes = 1U << 16U;
};

struct ScriptGraphCanvasLayoutDecodeResultUVE final {
    std::optional<ScriptGraphCanvasLayoutSnapshotUVE> layout;
    std::vector<ScriptPersistenceDiagnosticUVE> diagnostics;

    [[nodiscard]] bool IsSuccessUVE() const noexcept {
        return layout.has_value() && diagnostics.empty();
    }
};

[[nodiscard]] std::string EncodeScriptGraphCanvasLayoutUVE(
    const ScriptGraphCanvasLayoutSnapshotUVE& layout,
    std::vector<ScriptPersistenceDiagnosticUVE>& diagnostics,
    ScriptGraphCanvasLayoutPersistenceLimitsUVE limits = {});

[[nodiscard]] ScriptGraphCanvasLayoutDecodeResultUVE DecodeScriptGraphCanvasLayoutUVE(
    const std::string& text,
    ScriptGraphCanvasLayoutPersistenceLimitsUVE limits = {});

} // namespace UVE::Scripting
