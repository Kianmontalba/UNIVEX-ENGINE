// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/scripting/script_graph_canvas_persistence_uve.h"

#include <nlohmann/json.hpp>

#include <cmath>
#include <cstdint>
#include <utility>
#include <unordered_set>

namespace UVE::Scripting {
namespace {
using JsonUVE = nlohmann::ordered_json;
constexpr std::uint32_t kCanvasLayoutSchemaVersionUVE = 1U;

void AddDiagnostic(std::vector<ScriptPersistenceDiagnosticUVE>& diagnostics,
                   const ScriptPersistenceDiagnosticCodeUVE code,
                   std::string message) {
    diagnostics.push_back({code, std::move(message)});
}

bool IsFinitePointUVE(const ScriptGraphCanvasPointUVE point) noexcept {
    return std::isfinite(point.x) && std::isfinite(point.y);
}

bool IsValidViewUVE(const ScriptGraphCanvasViewUVE view) noexcept {
    return IsFinitePointUVE(view.pan) && std::isfinite(view.zoom) &&
           view.zoom >= kMinimumScriptGraphCanvasZoomUVE &&
           view.zoom <= kMaximumScriptGraphCanvasZoomUVE;
}

bool ValidateLayoutUVE(const ScriptGraphCanvasLayoutSnapshotUVE& layout,
                       const ScriptGraphCanvasLayoutPersistenceLimitsUVE limits) {
    if (layout.entries.size() > limits.maximumEntries || !IsValidViewUVE(layout.view)) {
        return false;
    }
    std::unordered_set<std::uint32_t> nodeIds;
    nodeIds.reserve(layout.entries.size());
    for (const ScriptGraphCanvasLayoutEntryUVE& entry : layout.entries) {
        if (entry.nodeId == 0U || !IsFinitePointUVE(entry.position) || !nodeIds.insert(entry.nodeId).second) {
            return false;
        }
    }
    return true;
}

bool IsNumberUVE(const JsonUVE& value) noexcept {
    return value.is_number_float() || value.is_number_integer() || value.is_number_unsigned();
}

bool ReadFiniteFloatUVE(const JsonUVE& object, const char* key, float& output) {
    if (!object.contains(key) || !IsNumberUVE(object.at(key))) {
        return false;
    }
    output = object.at(key).get<float>();
    return std::isfinite(output);
}

} // namespace

std::string EncodeScriptGraphCanvasLayoutUVE(
    const ScriptGraphCanvasLayoutSnapshotUVE& layout,
    std::vector<ScriptPersistenceDiagnosticUVE>& diagnostics,
    const ScriptGraphCanvasLayoutPersistenceLimitsUVE limits) {
    if (!ValidateLayoutUVE(layout, limits)) {
        AddDiagnostic(diagnostics, ScriptPersistenceDiagnosticCodeUVE::InvalidField,
                      "Canvas layout contains invalid, duplicate, or out-of-bounds values.");
        return {};
    }

    JsonUVE root;
    root["schemaVersion"] = kCanvasLayoutSchemaVersionUVE;
    root["view"] = {{"pan", {{"x", layout.view.pan.x}, {"y", layout.view.pan.y}}},
                     {"zoom", layout.view.zoom}};
    root["entries"] = JsonUVE::array();
    for (const ScriptGraphCanvasLayoutEntryUVE& entry : layout.entries) {
        root["entries"].push_back({{"nodeId", entry.nodeId},
                                   {"x", entry.position.x},
                                   {"y", entry.position.y}});
    }

    const std::string encoded = root.dump();
    if (encoded.size() > limits.maximumTextBytes) {
        AddDiagnostic(diagnostics, ScriptPersistenceDiagnosticCodeUVE::LimitExceeded,
                      "Encoded canvas layout exceeds text-size limit.");
        return {};
    }
    return encoded;
}

ScriptGraphCanvasLayoutDecodeResultUVE DecodeScriptGraphCanvasLayoutUVE(
    const std::string& text,
    const ScriptGraphCanvasLayoutPersistenceLimitsUVE limits) {
    ScriptGraphCanvasLayoutDecodeResultUVE result;
    if (text.size() > limits.maximumTextBytes) {
        AddDiagnostic(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::LimitExceeded,
                      "Canvas layout text exceeds persistence limit.");
        return result;
    }

    try {
        const JsonUVE root = JsonUVE::parse(text);
        if (!root.is_object() || !root.contains("schemaVersion") ||
            !root.at("schemaVersion").is_number_unsigned()) {
            AddDiagnostic(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::MissingField,
                          "Canvas layout schemaVersion is required.");
            return result;
        }
        if (root.at("schemaVersion").get<std::uint32_t>() != kCanvasLayoutSchemaVersionUVE) {
            AddDiagnostic(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::UnsupportedVersion,
                          "Unsupported canvas layout schema version.");
            return result;
        }
        if (!root.contains("view") || !root.at("view").is_object() ||
            !root.contains("entries") || !root.at("entries").is_array()) {
            AddDiagnostic(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::MissingField,
                          "Canvas layout view and entries are required.");
            return result;
        }
        const JsonUVE& view = root.at("view");
        const JsonUVE& pan = view.contains("pan") ? view.at("pan") : JsonUVE{};
        if (!pan.is_object()) {
            AddDiagnostic(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::InvalidField,
                          "Canvas layout pan must be an object.");
            return result;
        }
        ScriptGraphCanvasLayoutSnapshotUVE layout{};
        if (!ReadFiniteFloatUVE(pan, "x", layout.view.pan.x) ||
            !ReadFiniteFloatUVE(pan, "y", layout.view.pan.y) ||
            !ReadFiniteFloatUVE(view, "zoom", layout.view.zoom) ||
            !IsValidViewUVE(layout.view)) {
            AddDiagnostic(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::InvalidField,
                          "Canvas layout view must contain finite pan values and bounded zoom.");
            return result;
        }
        if (root.at("entries").size() > limits.maximumEntries) {
            AddDiagnostic(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::LimitExceeded,
                          "Canvas layout exceeds entry limit.");
            return result;
        }

        std::unordered_set<std::uint32_t> nodeIds;
        nodeIds.reserve(root.at("entries").size());
        for (const JsonUVE& entryJson : root.at("entries")) {
            if (!entryJson.is_object() || !entryJson.contains("nodeId") ||
                !entryJson.at("nodeId").is_number_unsigned()) {
                AddDiagnostic(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::InvalidField,
                              "Canvas layout entry requires an unsigned nodeId.");
                return result;
            }
            ScriptGraphCanvasLayoutEntryUVE entry{};
            entry.nodeId = entryJson.at("nodeId").get<std::uint32_t>();
            if (!ReadFiniteFloatUVE(entryJson, "x", entry.position.x) ||
                !ReadFiniteFloatUVE(entryJson, "y", entry.position.y)) {
                AddDiagnostic(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::InvalidField,
                              "Canvas layout entry requires finite x and y values.");
                return result;
            }
            if (entry.nodeId == 0U || !nodeIds.insert(entry.nodeId).second) {
                AddDiagnostic(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::DuplicateEntry,
                              "Canvas layout contains a duplicate or invalid nodeId.");
                return result;
            }
            layout.entries.push_back(entry);
        }
        result.layout = std::move(layout);
    } catch (const nlohmann::json::exception&) {
        AddDiagnostic(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::InvalidJson,
                      "Canvas layout text is not valid JSON.");
    }
    return result;
}

} // namespace UVE::Scripting
