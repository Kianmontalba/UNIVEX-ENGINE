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

const ScriptLinkUVE* FindExecutionLinkUVE(const std::vector<ScriptLinkUVE>& links,
                                          const std::uint32_t sourceNodeId,
                                          const std::string& outputPinName) noexcept {
    const auto iterator = std::find_if(links.begin(), links.end(), [&](const ScriptLinkUVE& link) {
        return link.output.nodeId == sourceNodeId && link.output.pinName == outputPinName;
    });
    return iterator == links.end() ? nullptr : &*iterator;
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
    std::vector<ScriptLinkUVE> stagedConditionLinks;
    // Every validated non-execution link is a data dependency. Schedule all of them
    // together instead of maintaining a typed one-hop allow-list. The graph validator
    // already guarantees one upstream link per input and compatible endpoints.
    std::vector<ScriptLinkUVE> dataLinks;
    dataLinks.reserve(links.size());
    for (const ScriptLinkUVE& link : links) {
        if (!IsExecutionLinkUVE(link, nodes, registry)) {
            dataLinks.push_back(link);
        }
    }

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

    const auto findNodeIndex = [&](const std::uint32_t nodeId) -> std::optional<std::size_t> {
        for (std::size_t index = 0U; index < nodes.size(); ++index) {
            if (nodes[index].id == nodeId) {
                return index;
            }
        }
        return std::nullopt;
    };
    const auto isExecutionActionNodeUVE = [&](const ScriptNodeUVE& node) noexcept {
        const ScriptNodeTypeDescriptorUVE* descriptor = registry.FindNodeTypeUVE(node.typeId);
        return descriptor != nullptr && descriptor->executionRequired;
    };

    std::vector<std::vector<std::size_t>> dataDependents(nodes.size());
    std::vector<std::size_t> dataIndegrees(nodes.size(), 0U);
    for (const ScriptLinkUVE& link : dataLinks) {
        const std::optional<std::size_t> sourceIndex = findNodeIndex(link.output.nodeId);
        const std::optional<std::size_t> consumerIndex = findNodeIndex(link.input.nodeId);
        if (!sourceIndex.has_value() || !consumerIndex.has_value()) {
            result.diagnostics.push_back({ScriptValidationCodeUVE::UnsupportedRuntimeNode,
                                          link.input.nodeId, link.input.pinName,
                                          "Data dependency references a node outside the validated graph."});
            return result;
        }
        dataDependents[*sourceIndex].push_back(*consumerIndex);
        ++dataIndegrees[*consumerIndex];
    }

    std::vector<std::size_t> readyNodeIndices;
    readyNodeIndices.reserve(nodes.size());
    for (std::size_t index = 0U; index < nodes.size(); ++index) {
        if (dataIndegrees[index] == 0U) {
            readyNodeIndices.push_back(index);
        }
    }

    std::vector<ScriptNodeUVE> instructionNodes;
    instructionNodes.reserve(nodes.size());
    while (!readyNodeIndices.empty()) {
        const auto readyIterator = std::min_element(
            readyNodeIndices.begin(), readyNodeIndices.end(), [&](const std::size_t lhs, const std::size_t rhs) {
                const ScriptNodeUVE& left = nodes[lhs];
                const ScriptNodeUVE& right = nodes[rhs];
                const bool leftIsControl = isExecutionActionNodeUVE(left);
                const bool rightIsControl = isExecutionActionNodeUVE(right);
                if (leftIsControl != rightIsControl) {
                    return !leftIsControl;
                }
                return left.id < right.id;
            });
        const std::size_t nodeIndex = *readyIterator;
        readyNodeIndices.erase(readyIterator);
        instructionNodes.push_back(nodes[nodeIndex]);
        for (const std::size_t dependentIndex : dataDependents[nodeIndex]) {
            --dataIndegrees[dependentIndex];
            if (dataIndegrees[dependentIndex] == 0U) {
                readyNodeIndices.push_back(dependentIndex);
            }
        }
    }

    if (instructionNodes.size() != nodes.size()) {
        for (std::size_t index = 0U; index < nodes.size(); ++index) {
            if (dataIndegrees[index] != 0U) {
                result.diagnostics.push_back({ScriptValidationCodeUVE::UnsupportedRuntimeNode,
                                              nodes[index].id, {},
                                              "Data dependency cycle prevents deterministic graph scheduling."});
                break;
            }
        }
        return result;
    }

    // The complete data-link set is emitted after each producer node. Keeping it in
    // the existing staging channel preserves the established IR/bytecode TransferValue
    // representation and all downstream VM/debugger behavior without an API change.
    const auto isSupportedBranchConditionProducerUVE = [](const std::string& typeId) noexcept {
        return typeId == "logic.boolean.not" || typeId == "logic.boolean.and" ||
               typeId == "logic.boolean.or" || typeId == "logic.boolean.xor" ||
               typeId == "logic.boolean.equal" || typeId == "logic.boolean.not_equal" ||
               typeId == "logic.boolean.greater" || typeId == "logic.boolean.less" ||
               typeId == "logic.boolean.greater_equal" || typeId == "logic.boolean.less_equal" ||
               typeId == "query.entity.has_component" || typeId == "variable.get_boolean" ||
               typeId == "convert.number_to_boolean";
    };
    for (const ScriptLinkUVE& link : dataLinks) {
        const ScriptNodeUVE* consumerNode = FindNodeUVE(nodes, link.input.nodeId);
        if (consumerNode == nullptr || consumerNode->typeId != "flow.branch" || link.input.pinName != "Condition") {
            continue;
        }
        const ScriptNodeUVE* sourceNode = FindNodeUVE(nodes, link.output.nodeId);
        if (sourceNode == nullptr || !isSupportedBranchConditionProducerUVE(sourceNode->typeId)) {
            result.diagnostics.push_back({ScriptValidationCodeUVE::UnsupportedRuntimeNode, link.input.nodeId,
                                          link.input.pinName,
                                          "flow.branch data-condition scheduling supports only built-in Boolean producer nodes."});
        }
    }
    if (!result.diagnostics.empty()) {
        return result;
    }

    stagedConditionLinks = std::move(dataLinks);

    const auto flowExtraEntryCountUVE = [](const ScriptNodeUVE& node) noexcept -> std::size_t {
        if (node.typeId == "flow.do_once") {
            return 1U;
        }
        if (node.typeId == "flow.gate") {
            return 2U;
        }
        return 0U;
    };
    const std::size_t flowExtraInstructionCount =
        static_cast<std::size_t>(std::count_if(
            instructionNodes.begin(), instructionNodes.end(), [&](const ScriptNodeUVE& node) {
                return flowExtraEntryCountUVE(node) != 0U;
            })) +
        static_cast<std::size_t>(std::count_if(
            instructionNodes.begin(), instructionNodes.end(),
            [&](const ScriptNodeUVE& node) { return flowExtraEntryCountUVE(node) == 2U; }));
    const std::size_t stagedInstructionCount = instructionNodes.size() + flowExtraInstructionCount +
                                               stagedConditionLinks.size();
    if (stagedInstructionCount > ScriptIrProgramUVE::kMaximumInstructionsUVE) {
        result.diagnostics.push_back({ScriptValidationCodeUVE::InstructionCountExceeded, 0U, {},
                                      "Compiled IR instruction count exceeds the maximum of 256."});
        return result;
    }
    const auto isStagedLinkUVE = [&](const ScriptLinkUVE& link) {
        return std::find_if(stagedConditionLinks.begin(), stagedConditionLinks.end(),
                            [&](const ScriptLinkUVE& stagedLink) {
                                return stagedLink.output.nodeId == link.output.nodeId &&
                                       stagedLink.output.pinName == link.output.pinName &&
                                       stagedLink.input.nodeId == link.input.nodeId &&
                                       stagedLink.input.pinName == link.input.pinName;
                            }) != stagedConditionLinks.end();
    };
    const auto findStagedInstructionIndex = [&](const std::uint32_t nodeId) -> std::optional<std::size_t> {
        const std::optional<std::size_t> baseIndex = FindNodeInstructionIndexUVE(instructionNodes, nodeId);
        if (!baseIndex.has_value()) {
            return std::nullopt;
        }
        std::size_t stagedBefore = 0U;
        const auto countStagedBefore = [&](const std::vector<ScriptLinkUVE>& stagedLinks) {
            for (const ScriptLinkUVE& stagedLink : stagedLinks) {
                const std::optional<std::size_t> sourceIndex =
                    FindNodeInstructionIndexUVE(instructionNodes, stagedLink.output.nodeId);
                if (sourceIndex.has_value() && *sourceIndex < *baseIndex) {
                    ++stagedBefore;
                }
            }
        };
        countStagedBefore(stagedConditionLinks);
        std::size_t flowEntriesBefore = 0U;
        for (std::size_t index = 0U; index < *baseIndex; ++index) {
            flowEntriesBefore += flowExtraEntryCountUVE(instructionNodes[index]);
        }
        return *baseIndex + stagedBefore + flowEntriesBefore;
    };
    const auto findFlowInstructionIndex = [&](const std::uint32_t nodeId,
                                              const std::string& inputPinName) -> std::optional<std::size_t> {
        const std::optional<std::size_t> primaryIndex = findStagedInstructionIndex(nodeId);
        const ScriptNodeUVE* targetNode = FindNodeUVE(instructionNodes, nodeId);
        if (!primaryIndex.has_value() || targetNode == nullptr) {
            return std::nullopt;
        }
        if (targetNode->typeId == "flow.do_once" && inputPinName == "Reset") {
            return *primaryIndex + 1U;
        }
        if (targetNode->typeId == "flow.gate" && inputPinName == "Open") {
            return *primaryIndex + 1U;
        }
        if (targetNode->typeId == "flow.gate" && inputPinName == "Close") {
            return *primaryIndex + 2U;
        }
        return primaryIndex;
    };
    const auto resolveExecutionTarget = [&](const std::uint32_t sourceNodeId,
                                            const char* outputPin) -> std::uint32_t {
        const ScriptLinkUVE* executionLink = FindExecutionLinkUVE(links, sourceNodeId, outputPin);
        if (executionLink == nullptr) {
            return static_cast<std::uint32_t>(stagedInstructionCount);
        }
        const std::optional<std::size_t> targetIndex =
            findFlowInstructionIndex(executionLink->input.nodeId, executionLink->input.pinName);
        return targetIndex.has_value() ? static_cast<std::uint32_t>(*targetIndex)
                                       : static_cast<std::uint32_t>(stagedInstructionCount);
    };

    ScriptIrProgramUVE program;
    program.instructions.reserve(stagedInstructionCount + links.size());
    program.sourceNodeIds.reserve(stagedInstructionCount + links.size());

    const auto appendFlowControlEntry = [&](const ScriptNodeUVE& node, const char* sourcePinName,
                                             const std::uint32_t trueTarget,
                                             const std::uint32_t falseTarget,
                                             const std::uint32_t defaultTarget) {
        program.instructions.push_back(ScriptIrInstructionUVE{
            ScriptIrInstructionKindUVE::FlowControlDispatch,
            node.id,
            0U,
            node.typeId,
            sourcePinName,
            {},
            trueTarget,
            falseTarget,
            0U,
            0U,
            false,
            defaultTarget,
        });
        program.sourceNodeIds.push_back(node.id);
    };
    for (const ScriptNodeUVE& node : instructionNodes) {
        if (node.typeId == "flow.sequence") {
            program.instructions.push_back(ScriptIrInstructionUVE{
                ScriptIrInstructionKindUVE::SequenceDispatch,
                node.id,
                0U,
                node.typeId,
                "Then",
                "Then2",
                0U,
                0U,
                resolveExecutionTarget(node.id, "Then"),
                resolveExecutionTarget(node.id, "Then2"),
            });
            program.sourceNodeIds.push_back(node.id);
        } else if (node.typeId == "flow.branch") {
            program.instructions.push_back(ScriptIrInstructionUVE{
                ScriptIrInstructionKindUVE::ConditionalJump,
                node.id,
                0U,
                node.typeId,
                "Condition",
                {},
                resolveExecutionTarget(node.id, "True"),
                resolveExecutionTarget(node.id, "False"),
                0U,
                0U,
            });
            program.sourceNodeIds.push_back(node.id);
        } else if (node.typeId == "flow.return") {
            appendFlowControlEntry(node, "In", static_cast<std::uint32_t>(stagedInstructionCount),
                                   static_cast<std::uint32_t>(stagedInstructionCount),
                                   static_cast<std::uint32_t>(stagedInstructionCount));
        } else if (node.typeId == "flow.do_once") {
            appendFlowControlEntry(node, "In", resolveExecutionTarget(node.id, "Then"),
                                   static_cast<std::uint32_t>(stagedInstructionCount),
                                   resolveExecutionTarget(node.id, "Default"));
            appendFlowControlEntry(node, "Reset", static_cast<std::uint32_t>(stagedInstructionCount),
                                   static_cast<std::uint32_t>(stagedInstructionCount),
                                   static_cast<std::uint32_t>(stagedInstructionCount));
        } else if (node.typeId == "flow.gate") {
            appendFlowControlEntry(node, "In", resolveExecutionTarget(node.id, "Exit"),
                                   resolveExecutionTarget(node.id, "Default"),
                                   resolveExecutionTarget(node.id, "Default"));
            appendFlowControlEntry(node, "Open", static_cast<std::uint32_t>(stagedInstructionCount),
                                   static_cast<std::uint32_t>(stagedInstructionCount),
                                   static_cast<std::uint32_t>(stagedInstructionCount));
            appendFlowControlEntry(node, "Close", static_cast<std::uint32_t>(stagedInstructionCount),
                                   static_cast<std::uint32_t>(stagedInstructionCount),
                                   static_cast<std::uint32_t>(stagedInstructionCount));
        } else if (node.typeId == "flow.switch") {
            appendFlowControlEntry(node, "In", resolveExecutionTarget(node.id, "Case0"),
                                   resolveExecutionTarget(node.id, "Case1"),
                                   resolveExecutionTarget(node.id, "Default"));
        } else if (node.typeId == "flow.event") {
            appendFlowControlEntry(node, "Event", resolveExecutionTarget(node.id, "Then"),
                                   static_cast<std::uint32_t>(stagedInstructionCount),
                                   static_cast<std::uint32_t>(stagedInstructionCount));
        } else if (node.typeId == "flow.loop" || node.typeId == "flow.for_loop") {
            appendFlowControlEntry(node, "In", resolveExecutionTarget(node.id, "Body"),
                                   resolveExecutionTarget(node.id, "Completed"),
                                   resolveExecutionTarget(node.id, "Completed"));
        } else if (node.typeId == "flow.while_loop") {
            appendFlowControlEntry(node, "In", resolveExecutionTarget(node.id, "Body"),
                                   resolveExecutionTarget(node.id, "Completed"),
                                   resolveExecutionTarget(node.id, "Completed"));
        } else if (node.typeId == "flow.delay") {
            appendFlowControlEntry(node, "In", resolveExecutionTarget(node.id, "Then"),
                                   static_cast<std::uint32_t>(stagedInstructionCount),
                                   resolveExecutionTarget(node.id, "Then"));
        } else if (isExecutionActionNodeUVE(node)) {
            appendFlowControlEntry(node, "In", resolveExecutionTarget(node.id, "Then"),
                                   static_cast<std::uint32_t>(stagedInstructionCount),
                                   static_cast<std::uint32_t>(stagedInstructionCount));
        } else {
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
            const auto emitStagedLinks = [&](const std::vector<ScriptLinkUVE>& stagedLinks) {
                for (const ScriptLinkUVE& stagedLink : stagedLinks) {
                    if (stagedLink.output.nodeId != node.id) {
                        continue;
                    }
                    program.instructions.push_back(ScriptIrInstructionUVE{
                        ScriptIrInstructionKindUVE::TransferValue,
                        stagedLink.output.nodeId,
                        stagedLink.input.nodeId,
                        {},
                        stagedLink.output.pinName,
                        stagedLink.input.pinName,
                        0U,
                        0U,
                        0U,
                        0U,
                        true,
                    });
                    program.sourceNodeIds.push_back(stagedLink.output.nodeId);
                }
            };
            emitStagedLinks(stagedConditionLinks);
    }

    for (const ScriptLinkUVE& link : links) {
        if (isStagedLinkUVE(link)) {
            continue;
        }
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
