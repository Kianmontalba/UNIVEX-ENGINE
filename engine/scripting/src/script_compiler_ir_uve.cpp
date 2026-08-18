#include "uve/scripting/script_compiler_ir_uve.h"

#include <algorithm>
#include <optional>
#include <utility>
#include <vector>

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

const ScriptNodeUVE* FindNodeUVE(const std::vector<ScriptNodeUVE>& nodes, const std::uint32_t nodeId) noexcept {
    const auto iterator = std::find_if(nodes.begin(), nodes.end(), [nodeId](const ScriptNodeUVE& node) {
        return node.id == nodeId;
    });
    return iterator == nodes.end() ? nullptr : &*iterator;
}

std::optional<std::size_t> FindNodeInstructionIndexUVE(const std::vector<ScriptNodeUVE>& nodes,
                                                       const std::uint32_t nodeId) noexcept {
    for (std::size_t index = 0U; index < nodes.size(); ++index) {
        if (nodes[index].id == nodeId) {
            return index;
        }
    }
    return std::nullopt;
}

std::optional<std::uint32_t> FindExecutionTargetUVE(const std::vector<ScriptLinkUVE>& links,
                                                    const std::uint32_t sourceNodeId,
                                                    const std::string& outputPinName) {
    const auto iterator = std::find_if(links.begin(), links.end(), [&](const ScriptLinkUVE& link) {
        return link.output.nodeId == sourceNodeId && link.output.pinName == outputPinName;
    });
    return iterator == links.end() ? std::nullopt : std::optional<std::uint32_t>(iterator->input.nodeId);
}

const ScriptPinDescriptorUVE* FindPinUVE(const ScriptNodeTypeDescriptorUVE& descriptor,
                                        const std::string& pinName) noexcept {
    const auto iterator = std::find_if(descriptor.pins.begin(), descriptor.pins.end(), [&](const ScriptPinDescriptorUVE& pin) {
        return pin.name == pinName;
    });
    return iterator == descriptor.pins.end() ? nullptr : &*iterator;
}

bool IsExecutionLinkUVE(const ScriptLinkUVE& link, const std::vector<ScriptNodeUVE>& nodes,
                        const ScriptNodeRegistryUVE& registry) {
    const ScriptNodeUVE* outputNode = FindNodeUVE(nodes, link.output.nodeId);
    const ScriptNodeUVE* inputNode = FindNodeUVE(nodes, link.input.nodeId);
    if (outputNode == nullptr || inputNode == nullptr) {
        return false;
    }
    const ScriptNodeTypeDescriptorUVE* outputDescriptor = registry.FindNodeTypeUVE(outputNode->typeId);
    const ScriptNodeTypeDescriptorUVE* inputDescriptor = registry.FindNodeTypeUVE(inputNode->typeId);
    const ScriptPinDescriptorUVE* outputPin = outputDescriptor == nullptr
        ? nullptr
        : FindPinUVE(*outputDescriptor, link.output.pinName);
    const ScriptPinDescriptorUVE* inputPin = inputDescriptor == nullptr
        ? nullptr
        : FindPinUVE(*inputDescriptor, link.input.pinName);
    return (outputPin != nullptr && outputPin->role == ScriptPinRoleUVE::Execution) ||
           (inputPin != nullptr && inputPin->role == ScriptPinRoleUVE::Execution);
}

} // namespace

ScriptIrCompileResultUVE CompileScriptGraphToIrUVE(const ScriptGraphUVE& graph,
                                                   const ScriptNodeRegistryUVE& registry) {
    ScriptIrCompileResultUVE result;
    result.diagnostics = graph.ValidateUVE(registry);
    if (!result.diagnostics.empty()) {
        return result;
    }

    std::vector<ScriptNodeUVE> nodes = graph.GetNodesUVE();
    std::sort(nodes.begin(), nodes.end(), [](const ScriptNodeUVE& lhs, const ScriptNodeUVE& rhs) {
        return lhs.id < rhs.id;
    });
    std::vector<ScriptLinkUVE> links = graph.GetLinksUVE();
    std::sort(links.begin(), links.end(), LessLinkUVE);

    std::size_t sequenceNodeCount = 0U;
    std::size_t branchNodeCount = 0U;
    for (const ScriptNodeUVE& node : nodes) {
        if (node.typeId == "flow.sequence") {
            ++sequenceNodeCount;
            if (sequenceNodeCount > 1U) {
                result.diagnostics.push_back({ScriptValidationCodeUVE::UnsupportedRuntimeNode, node.id, {},
                                              "Only one flow.sequence direct-dispatch node is supported per compiled graph."});
            }
            for (const char* outputPin : {"Then", "Then2"}) {
                const std::optional<std::uint32_t> targetNodeId =
                    FindExecutionTargetUVE(links, node.id, outputPin);
                if (targetNodeId.has_value()) {
                    const ScriptNodeUVE* targetNode = FindNodeUVE(nodes, *targetNodeId);
                    if (targetNode == nullptr || targetNode->typeId == "flow.sequence" ||
                        targetNode->typeId == "flow.branch") {
                        result.diagnostics.push_back({ScriptValidationCodeUVE::UnsupportedRuntimeNode, node.id,
                                                      outputPin,
                                                      "flow.sequence direct dispatch supports only non-flow target nodes."});
                    }
                }
            }
        } else if (node.typeId == "flow.branch") {
            ++branchNodeCount;
            if (branchNodeCount > 1U) {
                result.diagnostics.push_back({ScriptValidationCodeUVE::UnsupportedRuntimeNode, node.id, {},
                                              "Only one flow.branch direct-dispatch node is supported per compiled graph."});
            }
            for (const ScriptLinkUVE& link : links) {
                if (link.input.nodeId == node.id && link.input.pinName == "Condition" &&
                    !IsExecutionLinkUVE(link, nodes, registry)) {
                    result.diagnostics.push_back({ScriptValidationCodeUVE::UnsupportedRuntimeNode, node.id,
                                                  "Condition",
                                                  "flow.branch requires a caller-bound Boolean Condition; graph data-condition staging remains deferred."});
                }
            }
            for (const char* outputPin : {"True", "False"}) {
                const std::optional<std::uint32_t> targetNodeId =
                    FindExecutionTargetUVE(links, node.id, outputPin);
                if (targetNodeId.has_value()) {
                    const ScriptNodeUVE* targetNode = FindNodeUVE(nodes, *targetNodeId);
                    if (targetNode == nullptr || targetNode->typeId == "flow.sequence" ||
                        targetNode->typeId == "flow.branch") {
                        result.diagnostics.push_back({ScriptValidationCodeUVE::UnsupportedRuntimeNode, node.id,
                                                      outputPin,
                                                      "flow.branch direct dispatch supports only non-flow target nodes."});
                    }
                }
            }
        }
    }
    if (!result.diagnostics.empty()) {
        return result;
    }

    ScriptIrProgramUVE program;
    program.instructions.reserve(nodes.size() + links.size());
    program.sourceNodeIds.reserve(nodes.size() + links.size());

    for (const ScriptNodeUVE& node : nodes) {
        if (node.typeId == "flow.sequence") {
            const auto resolveTarget = [&](const char* pinName) -> std::uint32_t {
                const std::optional<std::uint32_t> targetNodeId = FindExecutionTargetUVE(links, node.id, pinName);
                if (!targetNodeId.has_value()) {
                    return static_cast<std::uint32_t>(nodes.size());
                }
                const std::optional<std::size_t> targetIndex =
                    FindNodeInstructionIndexUVE(nodes, *targetNodeId);
                return targetIndex.has_value() ? static_cast<std::uint32_t>(*targetIndex)
                                               : static_cast<std::uint32_t>(nodes.size());
            };
            program.instructions.push_back(ScriptIrInstructionUVE{
                ScriptIrInstructionKindUVE::SequenceDispatch,
                node.id,
                0U,
                node.typeId,
                "Then",
                "Then2",
                0U,
                0U,
                resolveTarget("Then"),
                resolveTarget("Then2"),
            });
        } else if (node.typeId == "flow.branch") {
            const auto resolveTarget = [&](const char* pinName) -> std::uint32_t {
                const std::optional<std::uint32_t> targetNodeId = FindExecutionTargetUVE(links, node.id, pinName);
                if (!targetNodeId.has_value()) {
                    return static_cast<std::uint32_t>(nodes.size());
                }
                const std::optional<std::size_t> targetIndex =
                    FindNodeInstructionIndexUVE(nodes, *targetNodeId);
                return targetIndex.has_value() ? static_cast<std::uint32_t>(*targetIndex)
                                               : static_cast<std::uint32_t>(nodes.size());
            };
            program.instructions.push_back(ScriptIrInstructionUVE{
                ScriptIrInstructionKindUVE::ConditionalJump,
                node.id,
                0U,
                node.typeId,
                "Condition",
                {},
                resolveTarget("True"),
                resolveTarget("False"),
                0U,
                0U,
            });
        } else {
            program.instructions.push_back(ScriptIrInstructionUVE{
                ScriptIrInstructionKindUVE::ExecuteNode,
                node.id,
                0U,
                node.typeId,
                {},
                {},
            });
        }
        program.sourceNodeIds.push_back(node.id);
    }

    for (const ScriptLinkUVE& link : links) {
        if (IsExecutionLinkUVE(link, nodes, registry)) {
            continue;
        }
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
