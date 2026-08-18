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
    std::optional<std::uint32_t> branchNodeId;
    std::optional<std::uint32_t> branchConditionSourceNodeId;
    std::optional<ScriptLinkUVE> branchConditionLink;
    std::optional<ScriptLinkUVE> branchConditionDependencyLink;
    std::vector<ScriptLinkUVE> stagedConditionLinks;
    std::vector<ScriptLinkUVE> stagedBooleanLinks;
    std::vector<ScriptLinkUVE> stagedNumberLinks;
    std::vector<ScriptLinkUVE> stagedVector3ScaleLinks;
    std::vector<ScriptLinkUVE> stagedVector3Links;
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
            branchNodeId = node.id;
            if (branchNodeCount > 1U) {
                result.diagnostics.push_back({ScriptValidationCodeUVE::UnsupportedRuntimeNode, node.id, {},
                                              "Only one flow.branch direct-dispatch node is supported per compiled graph."});
            }
            for (const ScriptLinkUVE& link : links) {
                if (link.input.nodeId != node.id || link.input.pinName != "Condition" ||
                    IsExecutionLinkUVE(link, nodes, registry)) {
                    continue;
                }
                branchConditionLink = link;
                branchConditionSourceNodeId = link.output.nodeId;
                const ScriptNodeUVE* sourceNode = FindNodeUVE(nodes, link.output.nodeId);
                const bool supportedProducer =
                    sourceNode != nullptr &&
                    (sourceNode->typeId == "logic.boolean.not" || sourceNode->typeId == "logic.boolean.and" ||
                     sourceNode->typeId == "logic.boolean.or" || sourceNode->typeId == "logic.boolean.xor" ||
                     sourceNode->typeId == "query.entity.has_component");
                if (!supportedProducer) {
                    result.diagnostics.push_back({ScriptValidationCodeUVE::UnsupportedRuntimeNode, node.id,
                                                  "Condition",
                                                  "flow.branch data-condition staging supports only built-in Boolean producer nodes."});
                }
                for (const ScriptLinkUVE& dependency : links) {
                    if (dependency.input.nodeId != link.output.nodeId ||
                        IsExecutionLinkUVE(dependency, nodes, registry)) {
                        continue;
                    }
                    const ScriptNodeUVE* dependencyNode = FindNodeUVE(nodes, dependency.output.nodeId);
                    const bool supportedDependency =
                        dependencyNode != nullptr &&
                        (dependencyNode->typeId == "logic.boolean.not" ||
                         dependencyNode->typeId == "logic.boolean.and" ||
                         dependencyNode->typeId == "logic.boolean.or" ||
                         dependencyNode->typeId == "logic.boolean.xor");
                    if (!supportedDependency || branchConditionDependencyLink.has_value()) {
                        result.diagnostics.push_back({ScriptValidationCodeUVE::UnsupportedRuntimeNode,
                                                      dependency.input.nodeId, dependency.input.pinName,
                                                      "flow.branch condition scheduling supports at most one built-in Boolean dependency hop."});
                        continue;
                    }
                    branchConditionDependencyLink = dependency;
                    stagedConditionLinks.push_back(dependency);
                    for (const ScriptLinkUVE& deeperDependency : links) {
                        if (deeperDependency.input.nodeId == dependency.output.nodeId &&
                            !IsExecutionLinkUVE(deeperDependency, nodes, registry)) {
                            result.diagnostics.push_back({ScriptValidationCodeUVE::UnsupportedRuntimeNode,
                                                          deeperDependency.input.nodeId, deeperDependency.input.pinName,
                                                          "flow.branch condition scheduling defers dependencies deeper than one producer hop."});
                        }
                    }
                }
                if (branchConditionLink.has_value()) {
                    stagedConditionLinks.push_back(*branchConditionLink);
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
    for (const ScriptLinkUVE& link : links) {
        if (IsExecutionLinkUVE(link, nodes, registry)) {
            continue;
        }
        const ScriptNodeUVE* sourceNode = FindNodeUVE(nodes, link.output.nodeId);
        const ScriptNodeUVE* consumerNode = FindNodeUVE(nodes, link.input.nodeId);
        if (sourceNode == nullptr || consumerNode == nullptr ||
            consumerNode->typeId.rfind("logic.boolean.", 0U) != 0U) {
            continue;
        }
        if (branchConditionDependencyLink.has_value() &&
            branchConditionDependencyLink->output.nodeId == link.output.nodeId &&
            branchConditionDependencyLink->output.pinName == link.output.pinName &&
            branchConditionDependencyLink->input.nodeId == link.input.nodeId &&
            branchConditionDependencyLink->input.pinName == link.input.pinName) {
            continue;
        }
        const bool approvedBooleanProducer =
            sourceNode->typeId.rfind("logic.boolean.", 0U) == 0U && link.output.pinName == "Result";
        const bool validBooleanInput =
            (consumerNode->typeId == "logic.boolean.not" && link.input.pinName == "Value") ||
            (consumerNode->typeId != "logic.boolean.not" &&
             (link.input.pinName == "A" || link.input.pinName == "B"));
        if (!approvedBooleanProducer || !validBooleanInput || !stagedBooleanLinks.empty()) {
            result.diagnostics.push_back({ScriptValidationCodeUVE::UnsupportedRuntimeNode, link.input.nodeId,
                                          link.input.pinName,
                                          "Boolean data-link staging supports one direct built-in logic.boolean.*.Result producer; additional consumers and composed/deeper Boolean dependencies remain deferred."});
            continue;
        }
        stagedBooleanLinks.push_back(link);
    }
    for (const ScriptLinkUVE& link : links) {
        if (IsExecutionLinkUVE(link, nodes, registry)) {
            continue;
        }
        const ScriptNodeUVE* sourceNode = FindNodeUVE(nodes, link.output.nodeId);
        const ScriptNodeUVE* consumerNode = FindNodeUVE(nodes, link.input.nodeId);
        if (sourceNode == nullptr || consumerNode == nullptr ||
            consumerNode->typeId.rfind("math.float.", 0U) != 0U) {
            continue;
        }
        const bool approvedNumberProducer =
            (sourceNode->typeId == "engine.get_time" && link.output.pinName == "Value") ||
            (sourceNode->typeId == "math.vector3.dot" && link.output.pinName == "Result") ||
            (sourceNode->typeId == "math.vector3.length" && link.output.pinName == "Length") ||
            (sourceNode->typeId.rfind("math.float.", 0U) == 0U && link.output.pinName == "Result");
        const bool validDirectNumberLink = approvedNumberProducer &&
                                           (link.input.pinName == "A" || link.input.pinName == "B");
        if (!validDirectNumberLink || !stagedNumberLinks.empty()) {
            result.diagnostics.push_back({ScriptValidationCodeUVE::UnsupportedRuntimeNode, link.input.nodeId,
                                          link.input.pinName,
                                          "Float data-link staging supports one direct built-in Number producer (engine.get_time, Vector3 dot/length, or Float Result); additional composed and deeper Number dependencies remain deferred."});
            continue;
        }
        stagedNumberLinks.push_back(link);
    }
    for (const ScriptLinkUVE& link : links) {
        if (IsExecutionLinkUVE(link, nodes, registry)) {
            continue;
        }
        const ScriptNodeUVE* sourceNode = FindNodeUVE(nodes, link.output.nodeId);
        const ScriptNodeUVE* consumerNode = FindNodeUVE(nodes, link.input.nodeId);
        if (sourceNode == nullptr || consumerNode == nullptr ||
            consumerNode->typeId != "math.vector3.multiply" || link.input.pinName != "Scale") {
            continue;
        }
        const bool approvedNumberProducer =
            (sourceNode->typeId == "engine.get_time" && link.output.pinName == "Value") ||
            (sourceNode->typeId.rfind("math.float.", 0U) == 0U && link.output.pinName == "Result");
        const bool hasProducerDependency = std::find_if(
            links.begin(), links.end(), [&](const ScriptLinkUVE& dependency) {
                return dependency.input.nodeId == sourceNode->id &&
                       !IsExecutionLinkUVE(dependency, nodes, registry);
            }) != links.end();
        if (!approvedNumberProducer || hasProducerDependency || !stagedVector3ScaleLinks.empty()) {
            result.diagnostics.push_back({ScriptValidationCodeUVE::UnsupportedRuntimeNode, link.input.nodeId,
                                          link.input.pinName,
                                          "Vector3 Scale staging supports one direct engine.get_time.Value or math.float.*.Result Number producer; additional Scale consumers and composed/deeper dependencies remain deferred."});
            continue;
        }
        stagedVector3ScaleLinks.push_back(link);
    }
    for (const ScriptLinkUVE& link : links) {
        if (IsExecutionLinkUVE(link, nodes, registry)) {
            continue;
        }
        const ScriptNodeUVE* sourceNode = FindNodeUVE(nodes, link.output.nodeId);
        const ScriptNodeUVE* consumerNode = FindNodeUVE(nodes, link.input.nodeId);
        if (sourceNode == nullptr || consumerNode == nullptr ||
            consumerNode->typeId.rfind("math.vector3.", 0U) != 0U ||
            (consumerNode->typeId == "math.vector3.multiply" && link.input.pinName == "Scale")) {
            continue;
        }
        const bool approvedProducer =
            sourceNode->typeId == "math.vector3.make" || sourceNode->typeId == "math.vector3.add" ||
            sourceNode->typeId == "math.vector3.subtract" || sourceNode->typeId == "math.vector3.multiply" ||
            sourceNode->typeId == "math.vector3.cross" || sourceNode->typeId == "math.vector3.normalize";
        const bool approvedOutput = link.output.pinName == "Vector" || link.output.pinName == "Result";
        const bool approvedInput = link.input.pinName == "A" || link.input.pinName == "B" ||
                                   link.input.pinName == "Vector";
        if (!approvedProducer || !approvedOutput || !approvedInput || !stagedVector3Links.empty()) {
            result.diagnostics.push_back({ScriptValidationCodeUVE::UnsupportedRuntimeNode, link.input.nodeId,
                                          link.input.pinName,
                                          "Vector3 data-link staging supports only one direct built-in Vector3 producer; composed and deeper vector dependencies remain deferred."});
            continue;
        }
        stagedVector3Links.push_back(link);
    }
    if (!result.diagnostics.empty()) {
        return result;
    }

    std::vector<ScriptNodeUVE> instructionNodes = nodes;
    if (branchNodeId.has_value() && branchConditionSourceNodeId.has_value()) {
        std::vector<std::uint32_t> conditionOrder;
        if (branchConditionDependencyLink.has_value()) {
            conditionOrder.push_back(branchConditionDependencyLink->output.nodeId);
        }
        conditionOrder.push_back(*branchConditionSourceNodeId);
        std::vector<ScriptNodeUVE> conditionNodes;
        for (const std::uint32_t conditionNodeId : conditionOrder) {
            const auto iterator = std::find_if(instructionNodes.begin(), instructionNodes.end(),
                                               [&](const ScriptNodeUVE& node) {
                                                   return node.id == conditionNodeId;
                                               });
            if (iterator != instructionNodes.end()) {
                conditionNodes.push_back(*iterator);
                instructionNodes.erase(iterator);
            }
        }
        const auto branchIterator = std::find_if(instructionNodes.begin(), instructionNodes.end(),
                                                 [&](const ScriptNodeUVE& node) {
                                                     return node.id == *branchNodeId;
                                                 });
        instructionNodes.insert(branchIterator, conditionNodes.begin(), conditionNodes.end());
    }
    const auto moveStagedProducerBeforeConsumer = [&](const std::vector<ScriptLinkUVE>& stagedLinks) {
        if (stagedLinks.empty()) {
            return;
        }
        const ScriptLinkUVE& stagedLink = stagedLinks.front();
        const auto sourceIterator = std::find_if(instructionNodes.begin(), instructionNodes.end(),
                                                 [&](const ScriptNodeUVE& node) {
                                                     return node.id == stagedLink.output.nodeId;
                                                 });
        const auto consumerIterator = std::find_if(instructionNodes.begin(), instructionNodes.end(),
                                                   [&](const ScriptNodeUVE& node) {
                                                       return node.id == stagedLink.input.nodeId;
                                                   });
        if (sourceIterator != instructionNodes.end() && consumerIterator != instructionNodes.end() &&
            sourceIterator != consumerIterator) {
            const ScriptNodeUVE sourceNode = *sourceIterator;
            instructionNodes.erase(sourceIterator);
            const auto updatedConsumerIterator = std::find_if(instructionNodes.begin(), instructionNodes.end(),
                                                              [&](const ScriptNodeUVE& node) {
                                                                  return node.id == stagedLink.input.nodeId;
                                                              });
            instructionNodes.insert(updatedConsumerIterator, sourceNode);
        }
    };
    moveStagedProducerBeforeConsumer(stagedBooleanLinks);
    moveStagedProducerBeforeConsumer(stagedNumberLinks);
    moveStagedProducerBeforeConsumer(stagedVector3ScaleLinks);
    moveStagedProducerBeforeConsumer(stagedVector3Links);

    const std::size_t stagedInstructionCount = instructionNodes.size() + stagedConditionLinks.size() +
                                                stagedBooleanLinks.size() + stagedNumberLinks.size() +
                                                stagedVector3ScaleLinks.size() + stagedVector3Links.size();
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
        countStagedBefore(stagedBooleanLinks);
        countStagedBefore(stagedNumberLinks);
        countStagedBefore(stagedVector3ScaleLinks);
        countStagedBefore(stagedVector3Links);
        return *baseIndex + stagedBefore;
    };

    ScriptIrProgramUVE program;
    program.instructions.reserve(stagedInstructionCount + links.size());
    program.sourceNodeIds.reserve(stagedInstructionCount + links.size());

    for (const ScriptNodeUVE& node : instructionNodes) {
        if (node.typeId == "flow.sequence") {
            const auto resolveTarget = [&](const char* pinName) -> std::uint32_t {
                const std::optional<std::uint32_t> targetNodeId = FindExecutionTargetUVE(links, node.id, pinName);
                if (!targetNodeId.has_value()) {
                    return static_cast<std::uint32_t>(stagedInstructionCount);
                }
                const std::optional<std::size_t> targetIndex = findStagedInstructionIndex(*targetNodeId);
                return targetIndex.has_value() ? static_cast<std::uint32_t>(*targetIndex)
                                               : static_cast<std::uint32_t>(stagedInstructionCount);
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
                    return static_cast<std::uint32_t>(stagedInstructionCount);
                }
                const std::optional<std::size_t> targetIndex = findStagedInstructionIndex(*targetNodeId);
                return targetIndex.has_value() ? static_cast<std::uint32_t>(*targetIndex)
                                               : static_cast<std::uint32_t>(stagedInstructionCount);
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
                    });
                    program.sourceNodeIds.push_back(stagedLink.output.nodeId);
                }
            };
            emitStagedLinks(stagedConditionLinks);
            emitStagedLinks(stagedBooleanLinks);
            emitStagedLinks(stagedNumberLinks);
            emitStagedLinks(stagedVector3ScaleLinks);
            emitStagedLinks(stagedVector3Links);
    }

    for (const ScriptLinkUVE& link : links) {
        const auto isStagedLink = [&](const std::vector<ScriptLinkUVE>& stagedLinks) {
            return std::find_if(stagedLinks.begin(), stagedLinks.end(),
                                [&](const ScriptLinkUVE& stagedLink) {
                                    return stagedLink.output.nodeId == link.output.nodeId &&
                                           stagedLink.output.pinName == link.output.pinName &&
                                           stagedLink.input.nodeId == link.input.nodeId &&
                                           stagedLink.input.pinName == link.input.pinName;
                                }) != stagedLinks.end();
        };
                if (isStagedLink(stagedConditionLinks) || isStagedLink(stagedBooleanLinks) ||
            isStagedLink(stagedNumberLinks) || isStagedLink(stagedVector3ScaleLinks) ||
            isStagedLink(stagedVector3Links)) {
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
