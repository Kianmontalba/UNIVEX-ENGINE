#pragma once

#include "uve/scripting/script_graph_uve.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace UVE::Scripting {

inline constexpr std::uint32_t kScriptGraphSchemaVersionUVE = 1U;

enum class ScriptPersistenceDiagnosticCodeUVE : std::uint8_t {
    InvalidJson = 0,
    UnsupportedVersion,
    MissingField,
    InvalidField,
    LimitExceeded,
    DuplicateEntry,
    UnknownField,
};

struct ScriptPersistenceDiagnosticUVE final {
    ScriptPersistenceDiagnosticCodeUVE code = ScriptPersistenceDiagnosticCodeUVE::InvalidJson;
    std::string message;
};

struct ScriptGraphPersistenceLimitsUVE final {
    std::size_t maximumNodes = 4096U;
    std::size_t maximumLinks = 8192U;
    std::size_t maximumLayoutEntries = 4096U;
    std::size_t maximumMetadataEntries = 128U;
    std::size_t maximumStringBytes = 4096U;
    std::size_t maximumTextBytes = 1U << 20U;
};

struct ScriptGraphLayoutEntryUVE final {
    std::uint32_t nodeId = 0U;
    float x = 0.0F;
    float y = 0.0F;

    [[nodiscard]] bool operator==(const ScriptGraphLayoutEntryUVE&) const noexcept = default;
};

struct ScriptGraphSchemaUVE final {
    std::uint32_t schemaVersion = kScriptGraphSchemaVersionUVE;
    ScriptGraphUVE graph;
    std::vector<ScriptGraphLayoutEntryUVE> layout;
    std::map<std::string, std::string> metadata;

    [[nodiscard]] bool operator==(const ScriptGraphSchemaUVE& other) const {
        if (schemaVersion != other.schemaVersion || graph.GetNodesUVE().size() != other.graph.GetNodesUVE().size() ||
            graph.GetLinksUVE() != other.graph.GetLinksUVE() || layout != other.layout || metadata != other.metadata) {
            return false;
        }
        for (std::size_t index = 0U; index < graph.GetNodesUVE().size(); ++index) {
            const ScriptNodeUVE& left = graph.GetNodesUVE()[index];
            const ScriptNodeUVE& right = other.graph.GetNodesUVE()[index];
            if (left.id != right.id || left.typeId != right.typeId) {
                return false;
            }
        }
        return true;
    }
};

struct ScriptGraphSchemaDecodeResultUVE final {
    std::optional<ScriptGraphSchemaUVE> schema;
    std::vector<ScriptPersistenceDiagnosticUVE> diagnostics;

    [[nodiscard]] bool IsSuccessUVE() const noexcept {
        return schema.has_value() && diagnostics.empty();
    }
};

inline constexpr std::uint32_t kScriptGraphWorkspaceSchemaVersionUVE = 1U;

struct ScriptGraphWorkspaceViewUVE final {
    float panX = 0.0F;
    float panY = 0.0F;
    float zoom = 1.0F;

    [[nodiscard]] bool operator==(const ScriptGraphWorkspaceViewUVE&) const noexcept = default;
};

struct ScriptGraphWorkspaceBranchUVE final {
    std::string name;
    ScriptGraphSchemaUVE schema;
    ScriptGraphWorkspaceViewUVE view{};

    [[nodiscard]] bool operator==(const ScriptGraphWorkspaceBranchUVE&) const = default;
};

struct ScriptGraphWorkspaceSchemaUVE final {
    std::uint32_t schemaVersion = kScriptGraphWorkspaceSchemaVersionUVE;
    std::vector<ScriptGraphWorkspaceBranchUVE> branches;

    [[nodiscard]] bool operator==(const ScriptGraphWorkspaceSchemaUVE&) const = default;
};

struct ScriptGraphWorkspacePersistenceLimitsUVE final {
    std::size_t maximumBranches = 64U;
    std::size_t maximumBranchNameBytes = 96U;
    ScriptGraphPersistenceLimitsUVE graphLimits{};
    std::size_t maximumTextBytes = 8U << 20U;
};

struct ScriptGraphWorkspaceDecodeResultUVE final {
    std::optional<ScriptGraphWorkspaceSchemaUVE> workspace;
    std::vector<ScriptPersistenceDiagnosticUVE> diagnostics;

    [[nodiscard]] bool IsSuccessUVE() const noexcept {
        return workspace.has_value() && diagnostics.empty();
    }
};

[[nodiscard]] std::string EncodeScriptGraphWorkspaceUVE(
    const ScriptGraphWorkspaceSchemaUVE& workspace,
    std::vector<ScriptPersistenceDiagnosticUVE>& diagnostics,
    ScriptGraphWorkspacePersistenceLimitsUVE limits = {});

[[nodiscard]] ScriptGraphWorkspaceDecodeResultUVE DecodeScriptGraphWorkspaceUVE(
    const std::string& text,
    ScriptGraphWorkspacePersistenceLimitsUVE limits = {});

/// Compatibility result retained for graph-only callers from the initial persistence seam.
struct ScriptGraphDecodeResultUVE final {
    std::optional<ScriptGraphUVE> graph;
    std::vector<ScriptPersistenceDiagnosticUVE> diagnostics;

    [[nodiscard]] bool IsSuccessUVE() const noexcept {
        return graph.has_value() && diagnostics.empty();
    }
};

[[nodiscard]] std::string EncodeScriptGraphSchemaUVE(
    const ScriptGraphSchemaUVE& schema,
    std::vector<ScriptPersistenceDiagnosticUVE>& diagnostics,
    ScriptGraphPersistenceLimitsUVE limits = {});

[[nodiscard]] ScriptGraphSchemaDecodeResultUVE DecodeScriptGraphSchemaUVE(
    const std::string& text,
    ScriptGraphPersistenceLimitsUVE limits = {});

[[nodiscard]] std::string EncodeScriptGraphUVE(
    const ScriptGraphUVE& graph,
    std::vector<ScriptPersistenceDiagnosticUVE>& diagnostics,
    ScriptGraphPersistenceLimitsUVE limits = {});

[[nodiscard]] ScriptGraphDecodeResultUVE DecodeScriptGraphUVE(
    const std::string& text,
    ScriptGraphPersistenceLimitsUVE limits = {});

} // namespace UVE::Scripting
