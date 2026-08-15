// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/scripting/script_graph_persistence_uve.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <utility>

namespace UVE::Scripting {
namespace {
using JsonUVE = nlohmann::ordered_json;
constexpr std::uint32_t kGraphSchemaVersionUVE = 1U;

void AddDiagnostic(std::vector<ScriptPersistenceDiagnosticUVE>& diagnostics,
                   const ScriptPersistenceDiagnosticCodeUVE code,
                   std::string message) {
    diagnostics.push_back({code, std::move(message)});
}

bool HasRequiredString(const JsonUVE& object, const char* key) {
    return object.contains(key) && object.at(key).is_string() && !object.at(key).get<std::string>().empty();
}

} // namespace

std::string EncodeScriptGraphUVE(const ScriptGraphUVE& graph,
                                 std::vector<ScriptPersistenceDiagnosticUVE>& diagnostics,
                                 const ScriptGraphPersistenceLimitsUVE limits) {
    if (graph.GetNodesUVE().size() > limits.maximumNodes || graph.GetLinksUVE().size() > limits.maximumLinks) {
        AddDiagnostic(diagnostics, ScriptPersistenceDiagnosticCodeUVE::LimitExceeded,
                      "Graph exceeds persistence limits.");
        return {};
    }
    JsonUVE root;
    root["schemaVersion"] = kGraphSchemaVersionUVE;
    root["nodes"] = JsonUVE::array();
    for (const ScriptNodeUVE& node : graph.GetNodesUVE()) {
        root["nodes"].push_back({{"id", node.id}, {"typeId", node.typeId}});
    }
    root["links"] = JsonUVE::array();
    for (const ScriptLinkUVE& link : graph.GetLinksUVE()) {
        root["links"].push_back({
            {"output", {{"nodeId", link.output.nodeId}, {"pinName", link.output.pinName}}},
            {"input", {{"nodeId", link.input.nodeId}, {"pinName", link.input.pinName}}},
        });
    }
    const std::string encoded = root.dump();
    if (encoded.size() > limits.maximumTextBytes) {
        AddDiagnostic(diagnostics, ScriptPersistenceDiagnosticCodeUVE::LimitExceeded,
                      "Encoded graph exceeds text-size limit.");
        return {};
    }
    return encoded;
}

ScriptGraphDecodeResultUVE DecodeScriptGraphUVE(const std::string& text,
                                                const ScriptGraphPersistenceLimitsUVE limits) {
    ScriptGraphDecodeResultUVE result;
    if (text.size() > limits.maximumTextBytes) {
        AddDiagnostic(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::LimitExceeded,
                      "Graph text exceeds persistence limit.");
        return result;
    }
    try {
        const JsonUVE root = JsonUVE::parse(text);
        if (!root.is_object() || !root.contains("schemaVersion") || !root.at("schemaVersion").is_number_unsigned()) {
            AddDiagnostic(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::MissingField,
                          "Graph schemaVersion is required.");
            return result;
        }
        if (root.at("schemaVersion").get<std::uint32_t>() != kGraphSchemaVersionUVE) {
            AddDiagnostic(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::UnsupportedVersion,
                          "Unsupported graph schema version.");
            return result;
        }
        if (!root.contains("nodes") || !root.at("nodes").is_array() || !root.contains("links") ||
            !root.at("links").is_array()) {
            AddDiagnostic(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::MissingField,
                          "Graph nodes and links arrays are required.");
            return result;
        }
        if (root.at("nodes").size() > limits.maximumNodes || root.at("links").size() > limits.maximumLinks) {
            AddDiagnostic(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::LimitExceeded,
                          "Graph exceeds persistence limits.");
            return result;
        }
        ScriptGraphUVE graph;
        for (const JsonUVE& nodeJson : root.at("nodes")) {
            if (!nodeJson.is_object() || !nodeJson.contains("id") || !nodeJson.at("id").is_number_unsigned() ||
                !HasRequiredString(nodeJson, "typeId")) {
                AddDiagnostic(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::InvalidField,
                              "Graph node requires unsigned id and non-empty typeId.");
                return result;
            }
            if (!graph.AddNodeUVE({nodeJson.at("id").get<std::uint32_t>(), nodeJson.at("typeId").get<std::string>()})) {
                AddDiagnostic(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::DuplicateEntry,
                              "Graph contains a duplicate or invalid node.");
                return result;
            }
        }
        for (const JsonUVE& linkJson : root.at("links")) {
            if (!linkJson.is_object() || !linkJson.contains("output") || !linkJson.contains("input") ||
                !linkJson.at("output").is_object() || !linkJson.at("input").is_object()) {
                AddDiagnostic(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::InvalidField,
                              "Graph link requires output and input objects.");
                return result;
            }
            const JsonUVE& output = linkJson.at("output");
            const JsonUVE& input = linkJson.at("input");
            if (!output.contains("nodeId") || !output.at("nodeId").is_number_unsigned() ||
                !output.contains("pinName") || !output.at("pinName").is_string() ||
                !input.contains("nodeId") || !input.at("nodeId").is_number_unsigned() ||
                !input.contains("pinName") || !input.at("pinName").is_string()) {
                AddDiagnostic(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::InvalidField,
                              "Graph link endpoints require unsigned nodeId and string pinName.");
                return result;
            }
            if (!graph.AddLinkUVE({
                    {output.at("nodeId").get<std::uint32_t>(), output.at("pinName").get<std::string>()},
                    {input.at("nodeId").get<std::uint32_t>(), input.at("pinName").get<std::string>()},
                })) {
                AddDiagnostic(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::DuplicateEntry,
                              "Graph contains a duplicate or invalid link.");
                return result;
            }
        }
        result.graph = std::move(graph);
    } catch (const nlohmann::json::exception&) {
        AddDiagnostic(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::InvalidJson,
                      "Graph text is not valid JSON.");
    }
    return result;
}

} // namespace UVE::Scripting
