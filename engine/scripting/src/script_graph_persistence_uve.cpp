#include "uve/scripting/script_graph_persistence_uve.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <limits>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace UVE::Scripting {
namespace {

using JsonUVE = nlohmann::ordered_json;

void AddDiagnosticUVE(std::vector<ScriptPersistenceDiagnosticUVE>& diagnostics,
                     const ScriptPersistenceDiagnosticCodeUVE code,
                     std::string message) {
    diagnostics.push_back({code, std::move(message)});
}

bool HasOnlyKeysUVE(const JsonUVE& object, const std::initializer_list<std::string_view> keys) {
    for (const auto& [key, value] : object.items()) {
        static_cast<void>(value);
        if (std::find(keys.begin(), keys.end(), key) == keys.end()) {
            return false;
        }
    }
    return true;
}

bool IsNonEmptyBoundedStringUVE(const JsonUVE& object, const char* key,
                                const ScriptGraphPersistenceLimitsUVE limits) {
    return object.contains(key) && object.at(key).is_string() && !object.at(key).get_ref<const std::string&>().empty() &&
           object.at(key).get_ref<const std::string&>().size() <= limits.maximumStringBytes;
}

bool IsUnsigned32UVE(const JsonUVE& value) noexcept {
    return value.is_number_unsigned() && value.get<std::uint64_t>() <= std::numeric_limits<std::uint32_t>::max();
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

bool IsValidLayoutUVE(const ScriptGraphSchemaUVE& schema,
                     const ScriptGraphPersistenceLimitsUVE limits) {
    if (schema.layout.size() > limits.maximumLayoutEntries) {
        return false;
    }
    std::unordered_set<std::uint32_t> nodeIds;
    nodeIds.reserve(schema.graph.GetNodesUVE().size());
    for (const ScriptNodeUVE& node : schema.graph.GetNodesUVE()) {
        if (node.id == 0U || !nodeIds.insert(node.id).second) {
            return false;
        }
    }
    std::unordered_set<std::uint32_t> layoutIds;
    layoutIds.reserve(schema.layout.size());
    for (const ScriptGraphLayoutEntryUVE& entry : schema.layout) {
        if (entry.nodeId == 0U || !std::isfinite(entry.x) || !std::isfinite(entry.y) ||
            !layoutIds.insert(entry.nodeId).second) {
            return false;
        }
    }
    return true;
}

bool IsValidMetadataUVE(const ScriptGraphSchemaUVE& schema,
                       const ScriptGraphPersistenceLimitsUVE limits) {
    if (schema.metadata.size() > limits.maximumMetadataEntries) {
        return false;
    }
    for (const auto& [key, value] : schema.metadata) {
        if (key.empty() || key.size() > limits.maximumStringBytes || value.size() > limits.maximumStringBytes) {
            return false;
        }
    }
    return true;
}

JsonUVE EncodeSchemaRootUVE(const ScriptGraphSchemaUVE& schema) {
    JsonUVE root;
    root["schemaVersion"] = schema.schemaVersion;
    root["nodes"] = JsonUVE::array();
    std::vector<ScriptNodeUVE> nodes = schema.graph.GetNodesUVE();
    std::sort(nodes.begin(), nodes.end(), [](const ScriptNodeUVE& left, const ScriptNodeUVE& right) {
        return left.id < right.id;
    });
    for (const ScriptNodeUVE& node : nodes) {
        root["nodes"].push_back({{"id", node.id}, {"typeId", node.typeId}});
    }

    root["links"] = JsonUVE::array();
    std::vector<ScriptLinkUVE> links = schema.graph.GetLinksUVE();
    std::sort(links.begin(), links.end(), [](const ScriptLinkUVE& left, const ScriptLinkUVE& right) {
        if (left.output.nodeId != right.output.nodeId) {
            return left.output.nodeId < right.output.nodeId;
        }
        if (left.output.pinName != right.output.pinName) {
            return left.output.pinName < right.output.pinName;
        }
        if (left.input.nodeId != right.input.nodeId) {
            return left.input.nodeId < right.input.nodeId;
        }
        return left.input.pinName < right.input.pinName;
    });
    for (const ScriptLinkUVE& link : links) {
        root["links"].push_back({
            {"output", {{"nodeId", link.output.nodeId}, {"pinName", link.output.pinName}}},
            {"input", {{"nodeId", link.input.nodeId}, {"pinName", link.input.pinName}}},
        });
    }

    root["layout"] = JsonUVE::array();
    std::vector<ScriptGraphLayoutEntryUVE> layout = schema.layout;
    std::sort(layout.begin(), layout.end(), [](const ScriptGraphLayoutEntryUVE& left,
                                               const ScriptGraphLayoutEntryUVE& right) {
        return left.nodeId < right.nodeId;
    });
    for (const ScriptGraphLayoutEntryUVE& entry : layout) {
        root["layout"].push_back({{"nodeId", entry.nodeId}, {"x", entry.x}, {"y", entry.y}});
    }

    root["metadata"] = JsonUVE::object();
    for (const auto& [key, value] : schema.metadata) {
        root["metadata"][key] = value;
    }
    return root;
}

} // namespace

std::string EncodeScriptGraphSchemaUVE(const ScriptGraphSchemaUVE& schema,
                                       std::vector<ScriptPersistenceDiagnosticUVE>& diagnostics,
                                       const ScriptGraphPersistenceLimitsUVE limits) {
    if (schema.schemaVersion != kScriptGraphSchemaVersionUVE) {
        AddDiagnosticUVE(diagnostics, ScriptPersistenceDiagnosticCodeUVE::UnsupportedVersion,
                         "Only visual script graph schema version 1 can be encoded.");
        return {};
    }
    if (schema.graph.GetNodesUVE().size() > limits.maximumNodes ||
        schema.graph.GetLinksUVE().size() > limits.maximumLinks) {
        AddDiagnosticUVE(diagnostics, ScriptPersistenceDiagnosticCodeUVE::LimitExceeded,
                         "Graph exceeds persistence node or link limits.");
        return {};
    }
    for (const ScriptNodeUVE& node : schema.graph.GetNodesUVE()) {
        if (node.id == 0U || node.typeId.empty() || node.typeId.size() > limits.maximumStringBytes) {
            AddDiagnosticUVE(diagnostics, ScriptPersistenceDiagnosticCodeUVE::InvalidField,
                             "Graph contains an invalid node identifier or typeId.");
            return {};
        }
    }
    for (const ScriptLinkUVE& link : schema.graph.GetLinksUVE()) {
        if (link.output.nodeId == 0U || link.input.nodeId == 0U || link.output.pinName.empty() ||
            link.input.pinName.empty() || link.output.pinName.size() > limits.maximumStringBytes ||
            link.input.pinName.size() > limits.maximumStringBytes) {
            AddDiagnosticUVE(diagnostics, ScriptPersistenceDiagnosticCodeUVE::InvalidField,
                             "Graph contains an invalid link endpoint.");
            return {};
        }
    }
    if (!IsValidLayoutUVE(schema, limits) || !IsValidMetadataUVE(schema, limits)) {
        AddDiagnosticUVE(diagnostics, ScriptPersistenceDiagnosticCodeUVE::InvalidField,
                         "Graph layout or metadata contains invalid, duplicate, or out-of-bounds values.");
        return {};
    }

    const std::string encoded = EncodeSchemaRootUVE(schema).dump();
    if (encoded.size() > limits.maximumTextBytes) {
        AddDiagnosticUVE(diagnostics, ScriptPersistenceDiagnosticCodeUVE::LimitExceeded,
                         "Encoded graph exceeds text-size limit.");
        return {};
    }
    return encoded;
}

ScriptGraphSchemaDecodeResultUVE DecodeScriptGraphSchemaUVE(
    const std::string& text, const ScriptGraphPersistenceLimitsUVE limits) {
    ScriptGraphSchemaDecodeResultUVE result;
    if (text.size() > limits.maximumTextBytes) {
        AddDiagnosticUVE(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::LimitExceeded,
                         "Graph text exceeds persistence limit.");
        return result;
    }

    try {
        const JsonUVE root = JsonUVE::parse(text);
        if (!root.is_object()) {
            AddDiagnosticUVE(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::InvalidField,
                             "Graph schema root must be an object.");
            return result;
        }
        if (!HasOnlyKeysUVE(root, {"schemaVersion", "nodes", "links", "layout", "metadata"})) {
            AddDiagnosticUVE(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::UnknownField,
                             "Graph schema contains an unknown root field.");
            return result;
        }
        if (!root.contains("schemaVersion") || !root.at("schemaVersion").is_number_unsigned()) {
            AddDiagnosticUVE(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::MissingField,
                             "Graph schemaVersion is required.");
            return result;
        }
        const std::uint32_t version = root.at("schemaVersion").get<std::uint32_t>();
        if (version != kScriptGraphSchemaVersionUVE) {
            AddDiagnosticUVE(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::UnsupportedVersion,
                             "Unsupported graph schema version; no migration is registered for it.");
            return result;
        }
        if (!root.contains("nodes") || !root.at("nodes").is_array() || !root.contains("links") ||
            !root.at("links").is_array() || !root.contains("layout") || !root.at("layout").is_array() ||
            !root.contains("metadata") || !root.at("metadata").is_object()) {
            AddDiagnosticUVE(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::MissingField,
                             "Graph schema requires nodes, links, layout, and metadata.");
            return result;
        }
        if (root.at("nodes").size() > limits.maximumNodes || root.at("links").size() > limits.maximumLinks ||
            root.at("layout").size() > limits.maximumLayoutEntries ||
            root.at("metadata").size() > limits.maximumMetadataEntries) {
            AddDiagnosticUVE(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::LimitExceeded,
                             "Graph schema exceeds a bounded array or metadata limit.");
            return result;
        }

        ScriptGraphSchemaUVE schema;
        schema.schemaVersion = version;
        for (const JsonUVE& nodeJson : root.at("nodes")) {
            if (!nodeJson.is_object() || !HasOnlyKeysUVE(nodeJson, {"id", "typeId"}) ||
                !nodeJson.contains("id") || !IsUnsigned32UVE(nodeJson.at("id")) ||
                !IsNonEmptyBoundedStringUVE(nodeJson, "typeId", limits)) {
                AddDiagnosticUVE(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::InvalidField,
                                 "Graph node requires only an unsigned id and bounded non-empty typeId.");
                return result;
            }
            if (!schema.graph.AddNodeUVE({nodeJson.at("id").get<std::uint32_t>(),
                                          nodeJson.at("typeId").get<std::string>()})) {
                AddDiagnosticUVE(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::DuplicateEntry,
                                 "Graph contains a duplicate or invalid node.");
                return result;
            }
        }
        for (const JsonUVE& linkJson : root.at("links")) {
            if (!linkJson.is_object() || !HasOnlyKeysUVE(linkJson, {"output", "input"}) ||
                !linkJson.contains("output") || !linkJson.contains("input") ||
                !linkJson.at("output").is_object() || !linkJson.at("input").is_object()) {
                AddDiagnosticUVE(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::InvalidField,
                                 "Graph link requires only output and input objects.");
                return result;
            }
            const JsonUVE& output = linkJson.at("output");
            const JsonUVE& input = linkJson.at("input");
            if (!HasOnlyKeysUVE(output, {"nodeId", "pinName"}) || !HasOnlyKeysUVE(input, {"nodeId", "pinName"}) ||
                !IsUnsigned32UVE(output.value("nodeId", JsonUVE{})) ||
                !IsUnsigned32UVE(input.value("nodeId", JsonUVE{})) ||
                !IsNonEmptyBoundedStringUVE(output, "pinName", limits) ||
                !IsNonEmptyBoundedStringUVE(input, "pinName", limits)) {
                AddDiagnosticUVE(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::InvalidField,
                                 "Graph link endpoints require unsigned nodeId and bounded pinName.");
                return result;
            }
            if (!schema.graph.AddLinkUVE({
                    {output.at("nodeId").get<std::uint32_t>(), output.at("pinName").get<std::string>()},
                    {input.at("nodeId").get<std::uint32_t>(), input.at("pinName").get<std::string>()},
                })) {
                AddDiagnosticUVE(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::DuplicateEntry,
                                 "Graph contains a duplicate or invalid link.");
                return result;
            }
        }
        std::unordered_set<std::uint32_t> layoutIds;
        layoutIds.reserve(root.at("layout").size());
        for (const JsonUVE& entryJson : root.at("layout")) {
            if (!entryJson.is_object() || !HasOnlyKeysUVE(entryJson, {"nodeId", "x", "y"}) ||
                !IsUnsigned32UVE(entryJson.value("nodeId", JsonUVE{}))) {
                AddDiagnosticUVE(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::InvalidField,
                                 "Graph layout entry requires only an unsigned nodeId, x, and y.");
                return result;
            }
            ScriptGraphLayoutEntryUVE entry{entryJson.at("nodeId").get<std::uint32_t>(), 0.0F, 0.0F};
            if (!ReadFiniteFloatUVE(entryJson, "x", entry.x) || !ReadFiniteFloatUVE(entryJson, "y", entry.y) ||
                entry.nodeId == 0U || !layoutIds.insert(entry.nodeId).second) {
                AddDiagnosticUVE(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::DuplicateEntry,
                                 "Graph layout contains duplicate or invalid node positions.");
                return result;
            }
            schema.layout.push_back(entry);
        }
        for (const auto& [key, value] : root.at("metadata").items()) {
            if (key.empty() || key.size() > limits.maximumStringBytes || !value.is_string() ||
                value.get_ref<const std::string&>().size() > limits.maximumStringBytes) {
                AddDiagnosticUVE(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::InvalidField,
                                 "Graph metadata keys and values must be bounded strings.");
                return result;
            }
            schema.metadata.emplace(key, value.get<std::string>());
        }
        result.schema = std::move(schema);
    } catch (const nlohmann::json::exception&) {
        AddDiagnosticUVE(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::InvalidJson,
                         "Graph text is not valid JSON or contains an invalid JSON value.");
    }
    return result;
}

std::string EncodeScriptGraphUVE(const ScriptGraphUVE& graph,
                                 std::vector<ScriptPersistenceDiagnosticUVE>& diagnostics,
                                 const ScriptGraphPersistenceLimitsUVE limits) {
    ScriptGraphSchemaUVE schema;
    schema.graph = graph;
    return EncodeScriptGraphSchemaUVE(schema, diagnostics, limits);
}

ScriptGraphDecodeResultUVE DecodeScriptGraphUVE(const std::string& text,
                                                const ScriptGraphPersistenceLimitsUVE limits) {
    const ScriptGraphSchemaDecodeResultUVE decoded = DecodeScriptGraphSchemaUVE(text, limits);
    ScriptGraphDecodeResultUVE result{std::nullopt, decoded.diagnostics};
    if (decoded.schema.has_value()) {
        result.graph = decoded.schema->graph;
    }
    return result;
}

std::string EncodeScriptGraphWorkspaceUVE(
    const ScriptGraphWorkspaceSchemaUVE& workspace,
    std::vector<ScriptPersistenceDiagnosticUVE>& diagnostics,
    const ScriptGraphWorkspacePersistenceLimitsUVE limits) {
    if (workspace.schemaVersion != kScriptGraphWorkspaceSchemaVersionUVE) {
        AddDiagnosticUVE(diagnostics, ScriptPersistenceDiagnosticCodeUVE::UnsupportedVersion,
                         "Only visual script workspace schema version 1 can be encoded.");
        return {};
    }
    if (workspace.branches.empty() || workspace.branches.size() > limits.maximumBranches) {
        AddDiagnosticUVE(diagnostics, ScriptPersistenceDiagnosticCodeUVE::LimitExceeded,
                         "Script workspace must contain between one and the configured maximum branches.");
        return {};
    }
    JsonUVE root;
    root["schemaVersion"] = workspace.schemaVersion;
    root["branches"] = JsonUVE::array();
    std::unordered_set<std::string> names;
    for (const ScriptGraphWorkspaceBranchUVE& branch : workspace.branches) {
        if (branch.name.empty() || branch.name.size() > limits.maximumBranchNameBytes ||
            !names.insert(branch.name).second) {
            AddDiagnosticUVE(diagnostics, ScriptPersistenceDiagnosticCodeUVE::DuplicateEntry,
                             "Script workspace contains an empty, oversized, or duplicate branch name.");
            return {};
        }
        std::vector<ScriptPersistenceDiagnosticUVE> graphDiagnostics;
        const std::string graphText = EncodeScriptGraphSchemaUVE(branch.schema, graphDiagnostics, limits.graphLimits);
        if (!graphDiagnostics.empty()) {
            diagnostics.insert(diagnostics.end(), graphDiagnostics.begin(), graphDiagnostics.end());
            return {};
        }
        root["branches"].push_back({{"name", branch.name},
                                      {"view", {{"panX", branch.view.panX}, {"panY", branch.view.panY},
                                                  {"zoom", branch.view.zoom}}},
                                      {"graph", JsonUVE::parse(graphText)}});
    }
    const std::string encoded = root.dump();
    if (encoded.size() > limits.maximumTextBytes) {
        AddDiagnosticUVE(diagnostics, ScriptPersistenceDiagnosticCodeUVE::LimitExceeded,
                         "Encoded script workspace exceeds text-size limit.");
        return {};
    }
    return encoded;
}

ScriptGraphWorkspaceDecodeResultUVE DecodeScriptGraphWorkspaceUVE(
    const std::string& text,
    const ScriptGraphWorkspacePersistenceLimitsUVE limits) {
    ScriptGraphWorkspaceDecodeResultUVE result;
    if (text.size() > limits.maximumTextBytes) {
        AddDiagnosticUVE(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::LimitExceeded,
                         "Script workspace text exceeds persistence limit.");
        return result;
    }
    try {
        const JsonUVE root = JsonUVE::parse(text);
        if (!root.is_object() || !root.contains("schemaVersion") ||
            !root.at("schemaVersion").is_number_unsigned() ||
            root.at("schemaVersion").get<std::uint32_t>() != kScriptGraphWorkspaceSchemaVersionUVE) {
            AddDiagnosticUVE(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::UnsupportedVersion,
                             "Unsupported or missing script workspace schema version.");
            return result;
        }
        if (!root.contains("branches") || !root.at("branches").is_array() ||
            root.at("branches").empty() || root.at("branches").size() > limits.maximumBranches) {
            AddDiagnosticUVE(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::LimitExceeded,
                             "Script workspace branches are missing or exceed the configured limit.");
            return result;
        }
        ScriptGraphWorkspaceSchemaUVE workspace{};
        workspace.schemaVersion = kScriptGraphWorkspaceSchemaVersionUVE;
        std::unordered_set<std::string> names;
        for (const JsonUVE& branchJson : root.at("branches")) {
            if (!branchJson.is_object() || !branchJson.contains("name") ||
                !branchJson.at("name").is_string() || !branchJson.contains("view") ||
                !branchJson.at("view").is_object() || !branchJson.contains("graph") ||
                !branchJson.at("graph").is_object()) {
                AddDiagnosticUVE(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::MissingField,
                                 "Each script workspace branch requires a name, view, and graph object.");
                return result;
            }
            const std::string& name = branchJson.at("name").get_ref<const std::string&>();
            if (name.empty() || name.size() > limits.maximumBranchNameBytes || !names.insert(name).second) {
                AddDiagnosticUVE(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::DuplicateEntry,
                                 "Script workspace contains an empty, oversized, or duplicate branch name.");
                return result;
            }
            ScriptGraphWorkspaceViewUVE view{};
            const JsonUVE& viewJson = branchJson.at("view");
            if (!ReadFiniteFloatUVE(viewJson, "panX", view.panX) ||
                !ReadFiniteFloatUVE(viewJson, "panY", view.panY) ||
                !ReadFiniteFloatUVE(viewJson, "zoom", view.zoom) ||
                !std::isfinite(view.zoom) || view.zoom < 0.1F || view.zoom > 8.0F) {
                AddDiagnosticUVE(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::InvalidField,
                                 "Script workspace branch view contains invalid pan or zoom values.");
                return result;
            }
            const ScriptGraphSchemaDecodeResultUVE graphResult =
                DecodeScriptGraphSchemaUVE(branchJson.at("graph").dump(), limits.graphLimits);
            if (!graphResult.IsSuccessUVE()) {
                result.diagnostics.insert(result.diagnostics.end(), graphResult.diagnostics.begin(),
                                          graphResult.diagnostics.end());
                return result;
            }
            workspace.branches.push_back(ScriptGraphWorkspaceBranchUVE{name, *graphResult.schema, view});
        }
        result.workspace = std::move(workspace);
    } catch (const nlohmann::json::exception&) {
        AddDiagnosticUVE(result.diagnostics, ScriptPersistenceDiagnosticCodeUVE::InvalidJson,
                         "Script workspace text is not valid JSON.");
    }
    return result;
}

} // namespace UVE::Scripting
