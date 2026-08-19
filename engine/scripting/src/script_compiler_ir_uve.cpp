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
    const auto conversionHasDataDependency = [&](const ScriptNodeUVE& sourceNode) {
        if (sourceNode.typeId.rfind("convert.", 0U) != 0U) {
            return false;
        }
        return std::find_if(links.begin(), links.end(), [&](const ScriptLinkUVE& dependency) {
            return dependency.input.nodeId == sourceNode.id &&
                   !IsExecutionLinkUVE(dependency, nodes, registry);
        }) != links.end();
    };
    const auto isInputBooleanProducer = [](const ScriptNodeUVE& sourceNode, const ScriptLinkUVE& link) {
        return (sourceNode.typeId == "input.key_pressed" || sourceNode.typeId == "input.key_released" ||
                sourceNode.typeId == "input.key_down" || sourceNode.typeId == "input.mouse_button" ||
                sourceNode.typeId == "input.gamepad_button" || sourceNode.typeId == "input.get_action") &&
               link.output.pinName == "Result";
    };
    const auto isInputNumberProducer = [](const ScriptNodeUVE& sourceNode, const ScriptLinkUVE& link) {
        return sourceNode.typeId == "input.get_axis" && link.output.pinName == "Result";
    };
    const auto isInputVector2Producer = [](const ScriptNodeUVE& sourceNode, const ScriptLinkUVE& link) {
        return sourceNode.typeId == "input.mouse_position" && link.output.pinName == "Position";
    };

    std::size_t sequenceNodeCount = 0U;
    std::size_t branchNodeCount = 0U;
    std::optional<std::uint32_t> branchNodeId;
    std::optional<std::uint32_t> branchConditionSourceNodeId;
    std::optional<ScriptLinkUVE> branchConditionLink;
    std::optional<ScriptLinkUVE> branchConditionDependencyLink;
    std::vector<ScriptLinkUVE> stagedConditionLinks;
    std::vector<ScriptLinkUVE> stagedComponentLinks;
    std::vector<ScriptLinkUVE> stagedEntityLinks;
    std::vector<ScriptLinkUVE> stagedBooleanLinks;
    std::vector<ScriptLinkUVE> stagedNumberLinks;
    std::vector<ScriptLinkUVE> stagedComparisonNumberLinks;
    std::vector<ScriptLinkUVE> stagedVector2ScaleLinks;
    std::vector<ScriptLinkUVE> stagedVector2Links;
    std::vector<ScriptLinkUVE> stagedVector3ScaleLinks;
    std::vector<ScriptLinkUVE> stagedVector3Links;
    std::vector<ScriptLinkUVE> stagedRotationLinks;
    std::vector<ScriptLinkUVE> stagedTransformLinks;
    std::vector<ScriptLinkUVE> stagedConversionLinks;
    std::vector<ScriptLinkUVE> stagedCollectionLinks;
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
                     sourceNode->typeId == "logic.boolean.equal" || sourceNode->typeId == "logic.boolean.not_equal" ||
                     sourceNode->typeId == "logic.boolean.greater" || sourceNode->typeId == "logic.boolean.less" ||
                     sourceNode->typeId == "logic.boolean.greater_equal" || sourceNode->typeId == "logic.boolean.less_equal" ||
                     sourceNode->typeId == "query.entity.has_component" || sourceNode->typeId == "variable.get_boolean" ||
                     sourceNode->typeId == "convert.number_to_boolean");
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
            consumerNode->typeId != "query.entity.has_component" || link.input.pinName != "Component") {
            continue;
        }
        const bool approvedComponentProducer =
            sourceNode->typeId == "query.entity.get_component" && link.output.pinName == "Result";
        const bool hasProducerDependency = std::find_if(
            links.begin(), links.end(), [&](const ScriptLinkUVE& dependency) {
                return dependency.input.nodeId == sourceNode->id &&
                       !IsExecutionLinkUVE(dependency, nodes, registry);
            }) != links.end();
        if (!approvedComponentProducer || hasProducerDependency || !stagedComponentLinks.empty()) {
            result.diagnostics.push_back({ScriptValidationCodeUVE::UnsupportedRuntimeNode, link.input.nodeId,
                                          link.input.pinName,
                                          "Entity query Component staging supports one direct query.entity.get_component.Result token; additional consumers and composed/deeper query dependencies remain deferred."});
            continue;
        }
        stagedComponentLinks.push_back(link);
    }
    for (const ScriptLinkUVE& link : links) {
        if (IsExecutionLinkUVE(link, nodes, registry)) {
            continue;
        }
        const ScriptNodeUVE* sourceNode = FindNodeUVE(nodes, link.output.nodeId);
        const ScriptNodeUVE* consumerNode = FindNodeUVE(nodes, link.input.nodeId);
        const bool isEntityConsumer = consumerNode != nullptr &&
            (consumerNode->typeId == "query.entity.has_component" || consumerNode->typeId == "query.entity.get_component" ||
             consumerNode->typeId == "entity.destroy" || consumerNode->typeId == "entity.add_component" ||
             consumerNode->typeId == "entity.remove_component" || consumerNode->typeId == "camera.set_position" ||
             consumerNode->typeId == "camera.set_rotation" || consumerNode->typeId == "camera.look_at" ||
             consumerNode->typeId == "camera.set_fov" || consumerNode->typeId == "camera.shake" ||
             consumerNode->typeId == "camera.set_active" || consumerNode->typeId == "animation.play" ||
             consumerNode->typeId == "animation.stop" || consumerNode->typeId == "animation.pause" ||
             consumerNode->typeId == "animation.blend" || consumerNode->typeId == "animation.blend_space" ||
             consumerNode->typeId == "animation.set_speed" || consumerNode->typeId == "animation.set_weight" ||
             consumerNode->typeId == "animation.montage" || consumerNode->typeId == "animation.get_current_animation" ||
             consumerNode->typeId == "animation.is_playing" || consumerNode->typeId == "motion.query.build" ||
             consumerNode->typeId == "motion.query.search" || consumerNode->typeId == "motion.query.get_best_match" ||
             consumerNode->typeId == "motion.query.set_trajectory" || consumerNode->typeId == "motion.query.set_pose" ||
             consumerNode->typeId == "motion.query.set_velocity" || consumerNode->typeId == "motion.query.set_facing" ||
             consumerNode->typeId == "motion.query.set_yaw" || consumerNode->typeId == "motion.query.transition" ||
             consumerNode->typeId == "motion.query.motion_warp" || consumerNode->typeId == "physics.raycast" ||
             consumerNode->typeId == "physics.sphere_cast" || consumerNode->typeId == "physics.box_cast" ||
             consumerNode->typeId == "physics.capsule_cast" || consumerNode->typeId == "physics.apply_force" ||
             consumerNode->typeId == "physics.apply_impulse" || consumerNode->typeId == "physics.set_velocity" ||
             consumerNode->typeId == "physics.get_velocity" || consumerNode->typeId == "physics.enable_gravity" ||
             consumerNode->typeId == "physics.is_colliding");
        const bool approvedEntityProducer = sourceNode != nullptr &&
            (sourceNode->typeId == "entity.spawn" || sourceNode->typeId == "camera.get_camera") &&
            link.output.pinName == "Result";
        if (sourceNode == nullptr || !isEntityConsumer || !approvedEntityProducer) {
            continue;
        }
        const ScriptNodeTypeDescriptorUVE* sourceDescriptor = registry.FindNodeTypeUVE(sourceNode->typeId);
        const ScriptNodeTypeDescriptorUVE* consumerDescriptor = registry.FindNodeTypeUVE(consumerNode->typeId);
        const ScriptPinDescriptorUVE* sourcePin = sourceDescriptor == nullptr ? nullptr : FindPinUVE(*sourceDescriptor, link.output.pinName);
        const ScriptPinDescriptorUVE* consumerPin = consumerDescriptor == nullptr ? nullptr : FindPinUVE(*consumerDescriptor, link.input.pinName);
        const bool validEntityType = sourcePin != nullptr && consumerPin != nullptr &&
            sourcePin->direction == ScriptPinDirectionUVE::Output && consumerPin->direction == ScriptPinDirectionUVE::Input &&
            sourcePin->role == ScriptPinRoleUVE::Data && consumerPin->role == ScriptPinRoleUVE::Data &&
            sourcePin->type == ScriptValueTypeUVE::Entity && consumerPin->type == ScriptValueTypeUVE::Entity;
        const bool hasProducerDependency = std::find_if(
            links.begin(), links.end(), [&](const ScriptLinkUVE& dependency) {
                return dependency.input.nodeId == sourceNode->id &&
                       !IsExecutionLinkUVE(dependency, nodes, registry);
            }) != links.end();
        if (!validEntityType || !approvedEntityProducer || hasProducerDependency || !stagedEntityLinks.empty()) {
            result.diagnostics.push_back({ScriptValidationCodeUVE::UnsupportedRuntimeNode, link.input.nodeId,
                                          link.input.pinName,
                                          "Entity staging supports one direct entity.spawn or camera.get_camera Result producer; composed dependencies and additional Entity consumers remain deferred."});
            continue;
        }
        stagedEntityLinks.push_back(link);
    }
    for (const ScriptLinkUVE& link : links) {
        if (IsExecutionLinkUVE(link, nodes, registry)) {
            continue;
        }
        const ScriptNodeUVE* sourceNode = FindNodeUVE(nodes, link.output.nodeId);
        const ScriptNodeUVE* consumerNode = FindNodeUVE(nodes, link.input.nodeId);
        const bool isNumberComparisonConsumer =
            consumerNode != nullptr &&
            (consumerNode->typeId == "logic.boolean.equal" || consumerNode->typeId == "logic.boolean.not_equal" ||
             consumerNode->typeId == "logic.boolean.greater" || consumerNode->typeId == "logic.boolean.less" ||
             consumerNode->typeId == "logic.boolean.greater_equal" || consumerNode->typeId == "logic.boolean.less_equal");
        if (sourceNode == nullptr || consumerNode == nullptr ||
            consumerNode->typeId.rfind("logic.boolean.", 0U) != 0U || isNumberComparisonConsumer) {
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
            (sourceNode->typeId.rfind("logic.boolean.", 0U) == 0U ||
             sourceNode->typeId == "query.entity.has_component" ||
             sourceNode->typeId == "variable.get_boolean" ||
             sourceNode->typeId == "convert.number_to_boolean" ||
             isInputBooleanProducer(*sourceNode, link)) &&
            link.output.pinName == "Result";
        const bool validBooleanInput =
            (consumerNode->typeId == "logic.boolean.not" && link.input.pinName == "Value") ||
            (consumerNode->typeId != "logic.boolean.not" &&
             (link.input.pinName == "A" || link.input.pinName == "B"));
        if (!approvedBooleanProducer || conversionHasDataDependency(*sourceNode) || !validBooleanInput ||
            !stagedBooleanLinks.empty()) {
            result.diagnostics.push_back({ScriptValidationCodeUVE::UnsupportedRuntimeNode, link.input.nodeId,
                                          link.input.pinName,
                                          "Boolean data-link staging supports one direct logic.boolean.*.Result or query.entity.has_component.Result producer; additional consumers and composed/deeper Boolean dependencies remain deferred."});
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
            (consumerNode->typeId.rfind("math.float.", 0U) != 0U && consumerNode->typeId != "flow.switch" &&
             consumerNode->typeId != "flow.loop" && consumerNode->typeId != "flow.for_loop" && consumerNode->typeId != "flow.delay")) {
            continue;
        }
        const bool approvedNumberProducer =
            (sourceNode->typeId == "engine.get_time" && link.output.pinName == "Value") ||
            (sourceNode->typeId == "math.vector3.dot" && link.output.pinName == "Result") ||
            (sourceNode->typeId == "math.vector3.length" && link.output.pinName == "Length") ||
            (sourceNode->typeId == "math.vector2.dot" && link.output.pinName == "Result") ||
            (sourceNode->typeId == "math.vector2.distance" && link.output.pinName == "Distance") ||
            (sourceNode->typeId == "math.vector3.distance" && link.output.pinName == "Distance") ||
            (sourceNode->typeId == "math.vector2.length" && link.output.pinName == "Length") ||
            (sourceNode->typeId.rfind("math.float.", 0U) == 0U && link.output.pinName == "Result") ||
            (sourceNode->typeId == "math.rotation.degrees" && link.output.pinName == "Degrees") ||
            (sourceNode->typeId == "math.rotation.radians" && link.output.pinName == "Radians") ||
            (sourceNode->typeId == "variable.get_number" && link.output.pinName == "Result") ||
            (sourceNode->typeId == "convert.boolean_to_number" && link.output.pinName == "Result") ||
            isInputNumberProducer(*sourceNode, link);
        const bool validDirectNumberLink = approvedNumberProducer &&
                                           ((consumerNode->typeId == "flow.switch" && link.input.pinName == "Value") ||
                                            ((consumerNode->typeId == "flow.loop" || consumerNode->typeId == "flow.for_loop") &&
                                             link.input.pinName == "Count") ||
                                            (consumerNode->typeId == "flow.delay" && link.input.pinName == "Frames") ||
                                            (consumerNode->typeId != "flow.switch" && consumerNode->typeId != "flow.loop" &&
                                             consumerNode->typeId != "flow.for_loop" && consumerNode->typeId != "flow.delay" &&
                                             (link.input.pinName == "A" || link.input.pinName == "B")));
        if (!validDirectNumberLink || conversionHasDataDependency(*sourceNode) || !stagedNumberLinks.empty()) {
            result.diagnostics.push_back({ScriptValidationCodeUVE::UnsupportedRuntimeNode, link.input.nodeId,
                                          link.input.pinName,
                                          "Float data-link staging supports one direct built-in Number or variable.get_number.Result producer; additional composed and deeper Number dependencies remain deferred."});
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
            (consumerNode->typeId != "logic.boolean.equal" && consumerNode->typeId != "logic.boolean.not_equal" &&
             consumerNode->typeId != "logic.boolean.greater" && consumerNode->typeId != "logic.boolean.less" &&
             consumerNode->typeId != "logic.boolean.greater_equal" && consumerNode->typeId != "logic.boolean.less_equal")) {
            continue;
        }
        const bool approvedNumberProducer =
            (sourceNode->typeId == "engine.get_time" && link.output.pinName == "Value") ||
            (sourceNode->typeId == "math.vector3.dot" && link.output.pinName == "Result") ||
            (sourceNode->typeId == "math.vector3.length" && link.output.pinName == "Length") ||
            (sourceNode->typeId == "math.vector2.dot" && link.output.pinName == "Result") ||
            (sourceNode->typeId == "math.vector2.distance" && link.output.pinName == "Distance") ||
            (sourceNode->typeId == "math.vector3.distance" && link.output.pinName == "Distance") ||
            (sourceNode->typeId == "math.vector2.length" && link.output.pinName == "Length") ||
            (sourceNode->typeId.rfind("math.float.", 0U) == 0U && link.output.pinName == "Result") ||
            (sourceNode->typeId == "math.rotation.degrees" && link.output.pinName == "Degrees") ||
            (sourceNode->typeId == "math.rotation.radians" && link.output.pinName == "Radians") ||
            (sourceNode->typeId == "variable.get_number" && link.output.pinName == "Result") ||
            (sourceNode->typeId == "convert.boolean_to_number" && link.output.pinName == "Result") ||
            isInputNumberProducer(*sourceNode, link);
        const bool validComparisonInput = link.input.pinName == "A" || link.input.pinName == "B";
        const bool sameComparisonConsumer = std::any_of(
            stagedComparisonNumberLinks.begin(), stagedComparisonNumberLinks.end(),
            [&](const ScriptLinkUVE& stagedLink) { return stagedLink.input.nodeId == link.input.nodeId; });
        const bool duplicateComparisonInput = std::any_of(
            stagedComparisonNumberLinks.begin(), stagedComparisonNumberLinks.end(),
            [&](const ScriptLinkUVE& stagedLink) {
                return stagedLink.input.nodeId == link.input.nodeId &&
                       stagedLink.input.pinName == link.input.pinName;
            });
        if (!approvedNumberProducer || conversionHasDataDependency(*sourceNode) || !validComparisonInput || duplicateComparisonInput ||
            stagedComparisonNumberLinks.size() >= 2U ||
            (!stagedComparisonNumberLinks.empty() && !sameComparisonConsumer)) {
            result.diagnostics.push_back({ScriptValidationCodeUVE::UnsupportedRuntimeNode, link.input.nodeId,
                                          link.input.pinName,
                                          "Boolean comparison staging supports up to two direct Number or variable.get_number producers on one comparison A/B pair; composed dependencies and multiple comparison consumers remain deferred."});
            continue;
        }
        stagedComparisonNumberLinks.push_back(link);
    }
    for (const ScriptLinkUVE& link : links) {
        if (IsExecutionLinkUVE(link, nodes, registry)) {
            continue;
        }
        const ScriptNodeUVE* sourceNode = FindNodeUVE(nodes, link.output.nodeId);
        const ScriptNodeUVE* consumerNode = FindNodeUVE(nodes, link.input.nodeId);
        if (sourceNode == nullptr || consumerNode == nullptr ||
            consumerNode->typeId != "math.vector2.multiply" || link.input.pinName != "Scale") {
            continue;
        }
        const bool approvedNumberProducer =
            (sourceNode->typeId == "engine.get_time" && link.output.pinName == "Value") ||
            (sourceNode->typeId == "math.vector3.dot" && link.output.pinName == "Result") ||
            (sourceNode->typeId == "math.vector3.length" && link.output.pinName == "Length") ||
            (sourceNode->typeId == "math.vector2.dot" && link.output.pinName == "Result") ||
            (sourceNode->typeId == "math.vector2.distance" && link.output.pinName == "Distance") ||
            (sourceNode->typeId == "math.vector3.distance" && link.output.pinName == "Distance") ||
            (sourceNode->typeId == "math.vector2.length" && link.output.pinName == "Length") ||
            (sourceNode->typeId.rfind("math.float.", 0U) == 0U && link.output.pinName == "Result") ||
            (sourceNode->typeId == "math.rotation.degrees" && link.output.pinName == "Degrees") ||
            (sourceNode->typeId == "math.rotation.radians" && link.output.pinName == "Radians") ||
            (sourceNode->typeId == "variable.get_number" && link.output.pinName == "Result") ||
            (sourceNode->typeId == "convert.boolean_to_number" && link.output.pinName == "Result") ||
            isInputNumberProducer(*sourceNode, link);
        const bool hasProducerDependency = std::find_if(
            links.begin(), links.end(), [&](const ScriptLinkUVE& dependency) {
                return dependency.input.nodeId == sourceNode->id &&
                       !IsExecutionLinkUVE(dependency, nodes, registry);
            }) != links.end();
        if (!approvedNumberProducer || conversionHasDataDependency(*sourceNode) || hasProducerDependency || !stagedVector2ScaleLinks.empty()) {
            result.diagnostics.push_back({ScriptValidationCodeUVE::UnsupportedRuntimeNode, link.input.nodeId,
                                          link.input.pinName,
                                          "Vector2 Scale staging supports one direct Number producer; additional Scale consumers and composed/deeper dependencies remain deferred."});
            continue;
        }
        stagedVector2ScaleLinks.push_back(link);
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
            (sourceNode->typeId.rfind("math.float.", 0U) == 0U && link.output.pinName == "Result") ||
            (sourceNode->typeId == "math.rotation.degrees" && link.output.pinName == "Degrees") ||
            (sourceNode->typeId == "math.rotation.radians" && link.output.pinName == "Radians") ||
            (sourceNode->typeId == "variable.get_number" && link.output.pinName == "Result") ||
            (sourceNode->typeId == "convert.boolean_to_number" && link.output.pinName == "Result") ||
            isInputNumberProducer(*sourceNode, link);
        const bool hasProducerDependency = std::find_if(
            links.begin(), links.end(), [&](const ScriptLinkUVE& dependency) {
                return dependency.input.nodeId == sourceNode->id &&
                       !IsExecutionLinkUVE(dependency, nodes, registry);
            }) != links.end();
        if (!approvedNumberProducer || conversionHasDataDependency(*sourceNode) || hasProducerDependency || !stagedVector3ScaleLinks.empty()) {
            result.diagnostics.push_back({ScriptValidationCodeUVE::UnsupportedRuntimeNode, link.input.nodeId,
                                          link.input.pinName,
                                          "Vector3 Scale staging supports one direct Number or variable.get_number.Result producer; additional Scale consumers and composed/deeper dependencies remain deferred."});
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
            consumerNode->typeId.rfind("math.vector2.", 0U) != 0U ||
            (consumerNode->typeId == "math.vector2.multiply" && link.input.pinName == "Scale")) {
            continue;
        }
        const bool approvedProducer =
            sourceNode->typeId == "math.vector2.make" || sourceNode->typeId == "math.vector2.add" ||
            sourceNode->typeId == "math.vector2.subtract" || sourceNode->typeId == "math.vector2.multiply" ||
            sourceNode->typeId == "math.vector2.normalize" || sourceNode->typeId == "math.vector2.direction" ||
            sourceNode->typeId == "math.vector2.lerp" || sourceNode->typeId == "convert.vector3_to_vector2" ||
            isInputVector2Producer(*sourceNode, link);
        const bool approvedOutput = link.output.pinName == "Vector" || link.output.pinName == "Result" ||
                                     link.output.pinName == "Position";
        const bool approvedInput = link.input.pinName == "A" || link.input.pinName == "B" ||
                                   link.input.pinName == "Vector";
        if (!approvedProducer || conversionHasDataDependency(*sourceNode) || !approvedOutput || !approvedInput ||
            !stagedVector2Links.empty()) {
            result.diagnostics.push_back({ScriptValidationCodeUVE::UnsupportedRuntimeNode, link.input.nodeId,
                                          link.input.pinName,
                                          "Vector2 data-link staging supports one direct Vector2 producer; composed and deeper vector dependencies remain deferred."});
            continue;
        }
        stagedVector2Links.push_back(link);
    }
    for (const ScriptLinkUVE& link : links) {
        if (IsExecutionLinkUVE(link, nodes, registry)) {
            continue;
        }
        const ScriptNodeUVE* sourceNode = FindNodeUVE(nodes, link.output.nodeId);
        const ScriptNodeUVE* consumerNode = FindNodeUVE(nodes, link.input.nodeId);
        if (sourceNode == nullptr || consumerNode == nullptr ||
            (consumerNode->typeId.rfind("math.vector3.", 0U) != 0U &&
             consumerNode->typeId != "math.transform.set_position" && consumerNode->typeId != "math.transform.translate") ||
            (consumerNode->typeId == "math.vector3.multiply" && link.input.pinName == "Scale")) {
            continue;
        }
        const bool approvedProducer =
            sourceNode->typeId == "math.vector3.make" || sourceNode->typeId == "math.vector3.add" ||
            sourceNode->typeId == "math.vector3.subtract" || sourceNode->typeId == "math.vector3.multiply" ||
            sourceNode->typeId == "math.vector3.cross" || sourceNode->typeId == "math.vector3.normalize" ||
            sourceNode->typeId == "math.vector3.direction" || sourceNode->typeId == "math.vector3.lerp" ||
            sourceNode->typeId == "math.rotation.rotate" ||
            (sourceNode->typeId == "math.transform.get_position" && link.output.pinName == "Position") ||
            (sourceNode->typeId == "math.transform.get_scale" && link.output.pinName == "Scale") ||
            (sourceNode->typeId == "math.transform.transform_point" && link.output.pinName == "Result") ||
            sourceNode->typeId == "variable.get_vector3" || sourceNode->typeId == "convert.vector2_to_vector3";
        const bool approvedOutput = link.output.pinName == "Vector" || link.output.pinName == "Result" ||
                                     (sourceNode->typeId == "math.transform.get_position" && link.output.pinName == "Position") ||
                                     (sourceNode->typeId == "math.transform.get_scale" && link.output.pinName == "Scale");
        const bool approvedInput = link.input.pinName == "A" || link.input.pinName == "B" ||
                                   link.input.pinName == "Vector" || link.input.pinName == "Position" ||
                                   link.input.pinName == "Translation";
        if (!approvedProducer || conversionHasDataDependency(*sourceNode) || !approvedOutput || !approvedInput ||
            !stagedVector3Links.empty()) {
            result.diagnostics.push_back({ScriptValidationCodeUVE::UnsupportedRuntimeNode, link.input.nodeId,
                                          link.input.pinName,
                                          "Vector3 data-link staging supports one direct built-in or variable.get_vector3.Result producer; composed and deeper vector dependencies remain deferred."});
            continue;
        }
        stagedVector3Links.push_back(link);
    }
    for (const ScriptLinkUVE& link : links) {
        if (IsExecutionLinkUVE(link, nodes, registry)) continue;
        const ScriptNodeUVE* sourceNode = FindNodeUVE(nodes, link.output.nodeId);
        const ScriptNodeUVE* consumerNode = FindNodeUVE(nodes, link.input.nodeId);
        if (sourceNode == nullptr || consumerNode == nullptr ||
            (consumerNode->typeId.rfind("math.rotation.", 0U) != 0U &&
             consumerNode->typeId != "math.transform.set_rotation" &&
             consumerNode->typeId != "math.transform.rotate")) {
            continue;
        }
        const ScriptNodeTypeDescriptorUVE* sourceDescriptor = registry.FindNodeTypeUVE(sourceNode->typeId);
        const ScriptNodeTypeDescriptorUVE* consumerDescriptor = registry.FindNodeTypeUVE(consumerNode->typeId);
        const ScriptPinDescriptorUVE* sourcePin = sourceDescriptor == nullptr ? nullptr : FindPinUVE(*sourceDescriptor, link.output.pinName);
        const ScriptPinDescriptorUVE* consumerPin = consumerDescriptor == nullptr ? nullptr : FindPinUVE(*consumerDescriptor, link.input.pinName);
        const bool validRotationType = sourcePin != nullptr && consumerPin != nullptr &&
            sourcePin->direction == ScriptPinDirectionUVE::Output && consumerPin->direction == ScriptPinDirectionUVE::Input &&
            sourcePin->role == ScriptPinRoleUVE::Data && consumerPin->role == ScriptPinRoleUVE::Data &&
            sourcePin->type == ScriptValueTypeUVE::Rotation && consumerPin->type == ScriptValueTypeUVE::Rotation;
        const bool approvedProducer =
            ((sourceNode->typeId == "math.rotation.make" || sourceNode->typeId == "math.rotation.euler" ||
              sourceNode->typeId == "math.rotation.quaternion" || sourceNode->typeId == "math.rotation.look_at") &&
             link.output.pinName == "Rotation") ||
            (sourceNode->typeId == "math.rotation.slerp" && link.output.pinName == "Result") ||
            (sourceNode->typeId == "math.transform.get_rotation" && link.output.pinName == "Rotation");
        const bool duplicateInput = std::any_of(stagedRotationLinks.begin(), stagedRotationLinks.end(),
            [&](const ScriptLinkUVE& staged) { return staged.input.nodeId == link.input.nodeId && staged.input.pinName == link.input.pinName; });
        const bool sameConsumer = stagedRotationLinks.empty() ||
            std::all_of(stagedRotationLinks.begin(), stagedRotationLinks.end(),
                [&](const ScriptLinkUVE& staged) { return staged.input.nodeId == link.input.nodeId; });
        if (!validRotationType || !approvedProducer || duplicateInput || stagedRotationLinks.size() >= 2U || !sameConsumer) {
            result.diagnostics.push_back({ScriptValidationCodeUVE::UnsupportedRuntimeNode, link.input.nodeId, link.input.pinName,
                                          "Rotation data-link staging supports up to two direct Rotation producers on one consumer; composed and additional consumers remain deferred."});
            continue;
        }
        stagedRotationLinks.push_back(link);
    }
    for (const ScriptLinkUVE& link : links) {
        if (IsExecutionLinkUVE(link, nodes, registry)) {
            continue;
        }
        const ScriptNodeUVE* sourceNode = FindNodeUVE(nodes, link.output.nodeId);
        const ScriptNodeUVE* consumerNode = FindNodeUVE(nodes, link.input.nodeId);
        if (sourceNode == nullptr || consumerNode == nullptr || link.input.pinName != "Transform") {
            continue;
        }
        const ScriptNodeTypeDescriptorUVE* sourceDescriptor = registry.FindNodeTypeUVE(sourceNode->typeId);
        const ScriptNodeTypeDescriptorUVE* consumerDescriptor = registry.FindNodeTypeUVE(consumerNode->typeId);
        const ScriptPinDescriptorUVE* sourcePin = sourceDescriptor == nullptr ? nullptr : FindPinUVE(*sourceDescriptor, link.output.pinName);
        const ScriptPinDescriptorUVE* consumerPin = consumerDescriptor == nullptr ? nullptr : FindPinUVE(*consumerDescriptor, link.input.pinName);
        const bool validTransformType = sourcePin != nullptr && consumerPin != nullptr &&
            sourcePin->direction == ScriptPinDirectionUVE::Output && consumerPin->direction == ScriptPinDirectionUVE::Input &&
            sourcePin->role == ScriptPinRoleUVE::Data && consumerPin->role == ScriptPinRoleUVE::Data &&
            sourcePin->type == ScriptValueTypeUVE::Transform && consumerPin->type == ScriptValueTypeUVE::Transform;
        const bool approvedProducer =
            (sourceNode->typeId == "math.transform.make" && link.output.pinName == "Transform") ||
            ((sourceNode->typeId == "math.transform.set_position" || sourceNode->typeId == "math.transform.set_rotation" ||
              sourceNode->typeId == "math.transform.set_scale" || sourceNode->typeId == "math.transform.translate" ||
              sourceNode->typeId == "math.transform.rotate") && link.output.pinName == "Result");
        const bool duplicateInput = std::any_of(stagedTransformLinks.begin(), stagedTransformLinks.end(),
            [&](const ScriptLinkUVE& staged) { return staged.input.nodeId == link.input.nodeId && staged.input.pinName == link.input.pinName; });
        if (!validTransformType || !approvedProducer || duplicateInput || !stagedTransformLinks.empty()) {
            result.diagnostics.push_back({ScriptValidationCodeUVE::UnsupportedRuntimeNode, link.input.nodeId, link.input.pinName,
                                          "Transform data-link staging supports one direct Transform producer; composed and additional Transform consumers remain deferred."});
            continue;
        }
        stagedTransformLinks.push_back(link);
    }
    for (const ScriptLinkUVE& link : links) {
        if (IsExecutionLinkUVE(link, nodes, registry)) {
            continue;
        }
        const ScriptNodeUVE* sourceNode = FindNodeUVE(nodes, link.output.nodeId);
        const ScriptNodeUVE* consumerNode = FindNodeUVE(nodes, link.input.nodeId);
        if (sourceNode == nullptr || consumerNode == nullptr || consumerNode->typeId.rfind("convert.", 0U) != 0U) {
            continue;
        }
        const ScriptNodeTypeDescriptorUVE* sourceDescriptor = registry.FindNodeTypeUVE(sourceNode->typeId);
        const ScriptNodeTypeDescriptorUVE* consumerDescriptor = registry.FindNodeTypeUVE(consumerNode->typeId);
        const ScriptPinDescriptorUVE* outputPin = sourceDescriptor == nullptr
            ? nullptr
            : FindPinUVE(*sourceDescriptor, link.output.pinName);
        const ScriptPinDescriptorUVE* inputPin = consumerDescriptor == nullptr
            ? nullptr
            : FindPinUVE(*consumerDescriptor, link.input.pinName);
        const bool validTypedSource = outputPin != nullptr && inputPin != nullptr &&
                                      outputPin->direction == ScriptPinDirectionUVE::Output &&
                                      inputPin->direction == ScriptPinDirectionUVE::Input &&
                                      outputPin->role == ScriptPinRoleUVE::Data &&
                                      inputPin->role == ScriptPinRoleUVE::Data &&
                                      outputPin->type == inputPin->type;
        const bool validConversionInput =
            (consumerNode->typeId == "convert.number_to_boolean" && link.input.pinName == "Value" &&
             inputPin != nullptr && inputPin->type == ScriptValueTypeUVE::Number) ||
            (consumerNode->typeId == "convert.boolean_to_number" && link.input.pinName == "Value" &&
             inputPin != nullptr && inputPin->type == ScriptValueTypeUVE::Boolean) ||
            (consumerNode->typeId == "convert.vector2_to_vector3" &&
             ((link.input.pinName == "Vector" && inputPin != nullptr && inputPin->type == ScriptValueTypeUVE::Vector2) ||
              (link.input.pinName == "Z" && inputPin != nullptr && inputPin->type == ScriptValueTypeUVE::Number))) ||
            (consumerNode->typeId == "convert.vector3_to_vector2" && link.input.pinName == "Vector" &&
             inputPin != nullptr && inputPin->type == ScriptValueTypeUVE::Vector3);
        const bool hasProducerDependency = std::find_if(
            links.begin(), links.end(), [&](const ScriptLinkUVE& dependency) {
                return dependency.input.nodeId == sourceNode->id &&
                       !IsExecutionLinkUVE(dependency, nodes, registry);
            }) != links.end();
        const bool duplicateConversionInput = std::any_of(
            stagedConversionLinks.begin(), stagedConversionLinks.end(), [&](const ScriptLinkUVE& stagedLink) {
                return stagedLink.input.nodeId == link.input.nodeId && stagedLink.input.pinName == link.input.pinName;
            });
        const bool sameConversionNode = stagedConversionLinks.empty() ||
                                        stagedConversionLinks.front().input.nodeId == link.input.nodeId;
        if (!validTypedSource || !validConversionInput || hasProducerDependency || duplicateConversionInput ||
            !sameConversionNode || stagedConversionLinks.size() >= 2U) {
            result.diagnostics.push_back({ScriptValidationCodeUVE::UnsupportedRuntimeNode, link.input.nodeId,
                                          link.input.pinName,
                                          "Conversion staging supports at most two direct typed inputs on one conversion node; composed conversion dependencies and additional consumers remain deferred."});
            continue;
        }
        stagedConversionLinks.push_back(link);
    }
    for (const ScriptLinkUVE& link : links) {
        if (IsExecutionLinkUVE(link, nodes, registry)) {
            continue;
        }
        const ScriptNodeUVE* sourceNode = FindNodeUVE(nodes, link.output.nodeId);
        const ScriptNodeUVE* consumerNode = FindNodeUVE(nodes, link.input.nodeId);
        const auto isCollectionPair = [](const std::string& sourceType, const std::string& consumerType,
                                         const char* suffix) {
            return sourceType == std::string{"variable.get_"} + suffix &&
                   consumerType == std::string{"variable.set_"} + suffix;
        };
        const auto isCollectionGet = [](const std::string& typeId) {
            return typeId == "variable.get_array" || typeId == "variable.get_map" ||
                   typeId == "variable.get_set" || typeId == "variable.get_struct";
        };
        const auto isCollectionSet = [](const std::string& typeId) {
            return typeId == "variable.set_array" || typeId == "variable.set_map" ||
                   typeId == "variable.set_set" || typeId == "variable.set_struct";
        };
        if (sourceNode == nullptr || consumerNode == nullptr ||
            (!isCollectionGet(sourceNode->typeId) && !isCollectionSet(consumerNode->typeId))) {
            continue;
        }
        const bool validPair = sourceNode != nullptr && consumerNode != nullptr &&
                               link.output.pinName == "Result" && link.input.pinName == "Value" &&
                               (isCollectionPair(sourceNode->typeId, consumerNode->typeId, "array") ||
                                isCollectionPair(sourceNode->typeId, consumerNode->typeId, "map") ||
                                isCollectionPair(sourceNode->typeId, consumerNode->typeId, "set") ||
                                isCollectionPair(sourceNode->typeId, consumerNode->typeId, "struct"));
        const bool hasProducerDependency = sourceNode != nullptr &&
            std::find_if(links.begin(), links.end(), [&](const ScriptLinkUVE& dependency) {
                return dependency.input.nodeId == sourceNode->id &&
                       !IsExecutionLinkUVE(dependency, nodes, registry);
            }) != links.end();
        if (!validPair || hasProducerDependency || !stagedCollectionLinks.empty()) {
            result.diagnostics.push_back({ScriptValidationCodeUVE::UnsupportedRuntimeNode, link.input.nodeId,
                                          link.input.pinName,
                                          "Collection staging supports one direct variable.get_array/map/set/struct.Result producer before a matching variable.set_* Value input; composed dependencies and additional consumers remain deferred."});
            continue;
        }
        stagedCollectionLinks.push_back(link);
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
        for (const ScriptLinkUVE& stagedLink : stagedLinks) {
            const auto sourceIterator = std::find_if(instructionNodes.begin(), instructionNodes.end(),
                                                     [&](const ScriptNodeUVE& node) {
                                                         return node.id == stagedLink.output.nodeId;
                                                     });
            const auto consumerIterator = std::find_if(instructionNodes.begin(), instructionNodes.end(),
                                                       [&](const ScriptNodeUVE& node) {
                                                           return node.id == stagedLink.input.nodeId;
                                                       });
            if (sourceIterator != instructionNodes.end() && consumerIterator != instructionNodes.end() &&
                sourceIterator != consumerIterator && sourceIterator > consumerIterator) {
                const ScriptNodeUVE sourceNode = *sourceIterator;
                instructionNodes.erase(sourceIterator);
                const auto updatedConsumerIterator = std::find_if(instructionNodes.begin(), instructionNodes.end(),
                                                                  [&](const ScriptNodeUVE& node) {
                                                                      return node.id == stagedLink.input.nodeId;
                                                                  });
                instructionNodes.insert(updatedConsumerIterator, sourceNode);
            }
        }
    };
    moveStagedProducerBeforeConsumer(stagedComponentLinks);
    moveStagedProducerBeforeConsumer(stagedEntityLinks);
    moveStagedProducerBeforeConsumer(stagedBooleanLinks);
    moveStagedProducerBeforeConsumer(stagedNumberLinks);
    moveStagedProducerBeforeConsumer(stagedComparisonNumberLinks);
    moveStagedProducerBeforeConsumer(stagedVector2ScaleLinks);
    moveStagedProducerBeforeConsumer(stagedVector2Links);
    moveStagedProducerBeforeConsumer(stagedVector3ScaleLinks);
    moveStagedProducerBeforeConsumer(stagedVector3Links);
    moveStagedProducerBeforeConsumer(stagedRotationLinks);
    moveStagedProducerBeforeConsumer(stagedTransformLinks);
    moveStagedProducerBeforeConsumer(stagedCollectionLinks);
    moveStagedProducerBeforeConsumer(stagedConversionLinks);

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
                                                stagedConditionLinks.size() + stagedComponentLinks.size() +
                                                stagedEntityLinks.size() + stagedBooleanLinks.size() + stagedNumberLinks.size() +
                                                stagedComparisonNumberLinks.size() + stagedVector2ScaleLinks.size() +
                                                stagedVector2Links.size() + stagedVector3ScaleLinks.size() +
                                                stagedVector3Links.size() + stagedRotationLinks.size() + stagedTransformLinks.size() +
                                                stagedCollectionLinks.size() + stagedConversionLinks.size();
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
        countStagedBefore(stagedComponentLinks);
        countStagedBefore(stagedEntityLinks);
        countStagedBefore(stagedBooleanLinks);
        countStagedBefore(stagedNumberLinks);
        countStagedBefore(stagedComparisonNumberLinks);
        countStagedBefore(stagedVector2ScaleLinks);
        countStagedBefore(stagedVector2Links);
        countStagedBefore(stagedVector3ScaleLinks);
        countStagedBefore(stagedVector3Links);
        countStagedBefore(stagedRotationLinks);
        countStagedBefore(stagedTransformLinks);
        countStagedBefore(stagedCollectionLinks);
        countStagedBefore(stagedConversionLinks);
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
            emitStagedLinks(stagedComponentLinks);
            emitStagedLinks(stagedEntityLinks);
            emitStagedLinks(stagedBooleanLinks);
            emitStagedLinks(stagedNumberLinks);
            emitStagedLinks(stagedComparisonNumberLinks);
            emitStagedLinks(stagedVector2ScaleLinks);
            emitStagedLinks(stagedVector2Links);
            emitStagedLinks(stagedVector3ScaleLinks);
            emitStagedLinks(stagedVector3Links);
            emitStagedLinks(stagedRotationLinks);
            emitStagedLinks(stagedTransformLinks);
            emitStagedLinks(stagedCollectionLinks);
            emitStagedLinks(stagedConversionLinks);
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
        if (isStagedLink(stagedConditionLinks) || isStagedLink(stagedComponentLinks) ||
            isStagedLink(stagedEntityLinks) || isStagedLink(stagedBooleanLinks) || isStagedLink(stagedNumberLinks) ||
            isStagedLink(stagedComparisonNumberLinks) || isStagedLink(stagedVector2ScaleLinks) ||
            isStagedLink(stagedVector2Links) || isStagedLink(stagedVector3ScaleLinks) ||
            isStagedLink(stagedVector3Links) || isStagedLink(stagedRotationLinks) ||
            isStagedLink(stagedTransformLinks) || isStagedLink(stagedCollectionLinks) ||
            isStagedLink(stagedConversionLinks)) {
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
