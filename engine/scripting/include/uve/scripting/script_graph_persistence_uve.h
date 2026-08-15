// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/scripting/script_graph_uve.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace UVE::Scripting {

enum class ScriptPersistenceDiagnosticCodeUVE : std::uint8_t {
    InvalidJson = 0,
    UnsupportedVersion,
    MissingField,
    InvalidField,
    LimitExceeded,
    DuplicateEntry,
};

struct ScriptPersistenceDiagnosticUVE final {
    ScriptPersistenceDiagnosticCodeUVE code = ScriptPersistenceDiagnosticCodeUVE::InvalidJson;
    std::string message;
};

struct ScriptGraphPersistenceLimitsUVE final {
    std::size_t maximumNodes = 4096U;
    std::size_t maximumLinks = 8192U;
    std::size_t maximumTextBytes = 1U << 20U;
};

struct ScriptGraphDecodeResultUVE final {
    std::optional<ScriptGraphUVE> graph;
    std::vector<ScriptPersistenceDiagnosticUVE> diagnostics;

    [[nodiscard]] bool IsSuccessUVE() const noexcept {
        return graph.has_value() && diagnostics.empty();
    }
};

[[nodiscard]] std::string EncodeScriptGraphUVE(
    const ScriptGraphUVE& graph,
    std::vector<ScriptPersistenceDiagnosticUVE>& diagnostics,
    ScriptGraphPersistenceLimitsUVE limits = {});

[[nodiscard]] ScriptGraphDecodeResultUVE DecodeScriptGraphUVE(
    const std::string& text,
    ScriptGraphPersistenceLimitsUVE limits = {});

} // namespace UVE::Scripting
