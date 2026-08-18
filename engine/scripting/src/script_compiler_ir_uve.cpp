// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/scripting/script_compiler_ir_uve.h"

#include <algorithm>
#include <utility>

namespace UVE::Scripting {
namespace {

bool LessLinkUVE(const ScriptLinkUVE& lhs, const ScriptLinkUVE& rhs) noexcept {
    if (lhs.output.nodeId != rhs.output.nodeId) {
        return lhs.output.nodeId < rhs.output.nodeId;
    }
    if (lhs.output.pinName != rhs.output.pinName) {
        return lhs.output.pinName < rhs.output.pinName;
    }
    if (lhs.input.nodeId != rhs.input.nodeId) {
        return lhs.input.nodeId < rhs.input.nodeId;
    }
    return lhs.input.pinName < rhs.input.pinName;
}

} // namespace

ScriptIrCompileResultUVE CompileScriptGraphToIrUVE(const ScriptGraphUVE& graph,
                                                   const ScriptNodeRegistryUVE& registry) {
    ScriptIrCompileResultUVE result;
    result.diagnostics = graph.ValidateUVE(registry);
    if (!result.diagnostics.empty()) {
        return result;
    }
    for (const ScriptNodeUVE& node : graph.GetNodesUVE()) {
        if (node.typeId == "flow.sequence" || node.typeId == "flow.branch") {
            result.diagnostics.push_back({ScriptValidationCodeUVE::UnsupportedRuntimeNode, node.id, {},
                                          "Flow node runtime execution is deferred to the ConditionalJump increment."});
        }
    }
    if (!result.diagnostics.empty()) {
        return result;
    }

    ScriptIrProgramUVE program;
    std::vector<ScriptNodeUVE> nodes = graph.GetNodesUVE();
    std::sort(nodes.begin(), nodes.end(), [](const ScriptNodeUVE& lhs, const ScriptNodeUVE& rhs) {
        return lhs.id < rhs.id;
    });
    program.instructions.reserve(nodes.size() + graph.GetLinksUVE().size());
    program.sourceNodeIds.reserve(nodes.size() + graph.GetLinksUVE().size());

    for (const ScriptNodeUVE& node : nodes) {
        program.instructions.push_back(ScriptIrInstructionUVE{
            ScriptIrInstructionKindUVE::ExecuteNode,
            node.id,
            0U,
            node.typeId,
            {},
            {},
        });
        program.sourceNodeIds.push_back(node.id);
    }

    std::vector<ScriptLinkUVE> links = graph.GetLinksUVE();
    std::sort(links.begin(), links.end(), LessLinkUVE);
    for (const ScriptLinkUVE& link : links) {
        program.instructions.push_back(ScriptIrInstructionUVE{
            ScriptIrInstructionKindUVE::TransferValue,
            link.output.nodeId,
            link.input.nodeId,
            {},
            link.output.pinName,
            link.input.pinName,
        });
        program.sourceNodeIds.push_back(link.output.nodeId);
    }

    result.program = std::move(program);
    return result;
}

} // namespace UVE::Scripting
