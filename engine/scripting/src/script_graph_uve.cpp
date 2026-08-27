// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/scripting/script_graph_uve.h"

#include <algorithm>
#include <set>
#include <string>
#include <unordered_set>

namespace UVE::Scripting {
namespace {

const ScriptPinDescriptorUVE* FindPinUVE(const ScriptNodeTypeDescriptorUVE& descriptor,
                                         std::string_view pinName) noexcept {
    const auto iterator = std::find_if(descriptor.pins.cbegin(), descriptor.pins.cend(),
                                       [pinName](const ScriptPinDescriptorUVE& pin) {
                                           return pin.name == pinName;
                                       });
    return iterator == descriptor.pins.cend() ? nullptr : &(*iterator);
}

const ScriptNodeUVE* FindNodeUVE(const std::vector<ScriptNodeUVE>& nodes,
                                 std::uint32_t nodeId) noexcept {
    const auto iterator = std::find_if(nodes.cbegin(), nodes.cend(), [nodeId](const ScriptNodeUVE& node) {
        return node.id == nodeId;
    });
    return iterator == nodes.cend() ? nullptr : &(*iterator);
}

std::string BoundDiagnosticTextUVE(std::string value, const std::size_t maximumBytes) {
    if (value.size() > maximumBytes) {
        value.resize(maximumBytes);
    }
    return value;
}

void AddDiagnosticUVE(std::vector<ScriptValidationDiagnosticUVE>& diagnostics,
                      ScriptValidationCodeUVE code,
                      std::uint32_t nodeId,
                      std::string pinName,
                      std::string message,
                      std::optional<ScriptPinEndpointUVE> relatedEndpoint = std::nullopt) {
    std::string sourceContext = nodeId == 0U ? "Graph" : "Node #" + std::to_string(nodeId);
    if (!pinName.empty()) {
        sourceContext += " / pin " + pinName;
    }
    diagnostics.push_back(ScriptValidationDiagnosticUVE{
        code,
        nodeId,
        std::move(pinName),
        BoundDiagnosticTextUVE(std::move(message), kMaximumScriptDiagnosticMessageBytesUVE),
        std::move(relatedEndpoint),
        ScriptDiagnosticSeverityUVE::Error,
        BoundDiagnosticTextUVE(std::move(sourceContext), kMaximumScriptDiagnosticSourceContextBytesUVE)});
}

} // namespace

bool ScriptNodeRegistryUVE::RegisterNodeTypeUVE(ScriptNodeTypeDescriptorUVE descriptor) {
    if (descriptor.typeId.empty() || descriptor.displayName.empty() || descriptor.category.empty() ||
        descriptor.iconId.empty() || m_nodeTypes.contains(descriptor.typeId)) {
        return false;
    }

    std::unordered_set<std::string> pinNames;
    pinNames.reserve(descriptor.pins.size());
    for (ScriptPinDescriptorUVE& pin : descriptor.pins) {
        if (pin.name.empty() || !pinNames.insert(pin.name).second) {
            return false;
        }
        if (pin.type == ScriptValueTypeUVE::Execution) {
            pin.role = ScriptPinRoleUVE::Execution;
            if (pin.defaultValue.has_value()) {
                return false;
            }
        }
    }

    const std::string typeId = descriptor.typeId;
    m_nodeTypes.emplace(typeId, std::move(descriptor));
    return true;
}

const ScriptNodeTypeDescriptorUVE* ScriptNodeRegistryUVE::FindNodeTypeUVE(
    std::string_view typeId) const noexcept {
    const auto iterator = m_nodeTypes.find(std::string(typeId));
    return iterator == m_nodeTypes.cend() ? nullptr : &iterator->second;
}

std::vector<std::string> ScriptNodeRegistryUVE::GetNodeTypeIdsUVE() const {
    std::vector<std::string> typeIds;
    typeIds.reserve(m_nodeTypes.size());
    for (const auto& [typeId, descriptor] : m_nodeTypes) {
        static_cast<void>(descriptor);
        typeIds.push_back(typeId);
    }
    std::sort(typeIds.begin(), typeIds.end());
    return typeIds;
}

std::vector<ScriptNodeTypeDescriptorUVE> ScriptNodeRegistryUVE::GetNodeTypeDescriptorsUVE() const {
    std::vector<ScriptNodeTypeDescriptorUVE> descriptors;
    descriptors.reserve(m_nodeTypes.size());
    for (const auto& [typeId, descriptor] : m_nodeTypes) {
        static_cast<void>(typeId);
        descriptors.push_back(descriptor);
    }
    std::sort(descriptors.begin(), descriptors.end(), [](const ScriptNodeTypeDescriptorUVE& left,
                                                         const ScriptNodeTypeDescriptorUVE& right) {
        if (left.displayOrder != right.displayOrder) {
            return left.displayOrder < right.displayOrder;
        }
        if (left.category != right.category) {
            return left.category < right.category;
        }
        return left.typeId < right.typeId;
    });
    return descriptors;
}

std::size_t ScriptNodeRegistryUVE::GetNodeTypeCountUVE() const noexcept {
    return m_nodeTypes.size();
}

bool ScriptGraphUVE::AddNodeUVE(ScriptNodeUVE node) {
    if (node.id == 0U || node.typeId.empty() || FindNodeUVE(m_nodes, node.id) != nullptr) {
        return false;
    }
    m_nodes.push_back(std::move(node));
    return true;
}

bool ScriptGraphUVE::AddLinkUVE(ScriptLinkUVE link) {
    if (link.output.nodeId == 0U || link.input.nodeId == 0U || link.output.pinName.empty() ||
        link.input.pinName.empty()) {
        return false;
    }
    const auto duplicate = std::find_if(m_links.cbegin(), m_links.cend(),
                                         [&link](const ScriptLinkUVE& rhs) {
                                         return link.output.nodeId == rhs.output.nodeId &&
                                                link.output.pinName == rhs.output.pinName &&
                                                link.input.nodeId == rhs.input.nodeId &&
                                                link.input.pinName == rhs.input.pinName;
                                         });
    if (duplicate != m_links.cend()) {
        return false;
    }
    m_links.push_back(std::move(link));
    return true;
}

bool ScriptGraphUVE::RemoveNodeUVE(const std::uint32_t nodeId) {
    const auto node = std::find_if(m_nodes.begin(), m_nodes.end(), [nodeId](const ScriptNodeUVE& candidate) {
        return candidate.id == nodeId;
    });
    if (node == m_nodes.end()) {
        return false;
    }
    m_nodes.erase(node);
    m_links.erase(std::remove_if(m_links.begin(), m_links.end(), [nodeId](const ScriptLinkUVE& link) {
        return link.output.nodeId == nodeId || link.input.nodeId == nodeId;
    }), m_links.end());
    return true;
}

bool ScriptGraphUVE::RemoveLinkUVE(const ScriptLinkUVE& link) {
    const auto existing = std::find_if(m_links.begin(), m_links.end(), [&link](const ScriptLinkUVE& candidate) {
        return candidate.output.nodeId == link.output.nodeId && candidate.output.pinName == link.output.pinName &&
               candidate.input.nodeId == link.input.nodeId && candidate.input.pinName == link.input.pinName;
    });
    if (existing == m_links.end()) {
        return false;
    }
    m_links.erase(existing);
    return true;
}

const std::vector<ScriptNodeUVE>& ScriptGraphUVE::GetNodesUVE() const noexcept {
    return m_nodes;
}

const std::vector<ScriptLinkUVE>& ScriptGraphUVE::GetLinksUVE() const noexcept {
    return m_links;
}

std::vector<ScriptValidationDiagnosticUVE> ScriptGraphUVE::ValidateUVE(
    const ScriptNodeRegistryUVE& registry) const {
    std::vector<ScriptValidationDiagnosticUVE> diagnostics;
    if (m_nodes.size() > kMaximumScriptGraphNodesUVE) {
        AddDiagnosticUVE(diagnostics, ScriptValidationCodeUVE::NodeCountExceeded, 0U, {},
                         "Graph node count exceeds the maximum of " +
                             std::to_string(kMaximumScriptGraphNodesUVE) + ".");
        return diagnostics;
    }
    if (m_links.size() > kMaximumScriptGraphLinksUVE) {
        AddDiagnosticUVE(diagnostics, ScriptValidationCodeUVE::LinkCountExceeded, 0U, {},
                         "Graph link count exceeds the maximum of " +
                             std::to_string(kMaximumScriptGraphLinksUVE) + ".");
        return diagnostics;
    }
    for (const ScriptNodeUVE& node : m_nodes) {
        if (node.typeId.empty()) {
            AddDiagnosticUVE(diagnostics, ScriptValidationCodeUVE::EmptyNodeType, node.id, {},
                             "Node type identifier is empty.");
            continue;
        }
        if (registry.FindNodeTypeUVE(node.typeId) == nullptr) {
            AddDiagnosticUVE(diagnostics, ScriptValidationCodeUVE::UnknownNodeType, node.id, {},
                             "Node type is not registered: " + node.typeId);
        }
    }

    std::set<std::pair<std::uint32_t, std::string>> linkedExecutionOutputs;
    std::set<std::pair<std::uint32_t, std::string>> linkedExecutionInputs;
    std::set<std::pair<std::uint32_t, std::string>> linkedDataInputs;
    for (const ScriptLinkUVE& link : m_links) {
        if (link.output.nodeId == link.input.nodeId) {
            AddDiagnosticUVE(diagnostics, ScriptValidationCodeUVE::SelfLink, link.output.nodeId,
                             link.output.pinName, "A node cannot link to itself.", link.input);
        }
        const ScriptNodeUVE* outputNode = FindNodeUVE(m_nodes, link.output.nodeId);
        const ScriptNodeUVE* inputNode = FindNodeUVE(m_nodes, link.input.nodeId);
        if (outputNode == nullptr || inputNode == nullptr) {
            const bool outputMissing = outputNode == nullptr;
            AddDiagnosticUVE(diagnostics, ScriptValidationCodeUVE::EmptyLinkEndpoint,
                             outputMissing ? link.output.nodeId : link.input.nodeId,
                             outputMissing ? link.output.pinName : link.input.pinName,
                             "Link references a node that is not present in the graph.",
                             outputMissing ? std::optional<ScriptPinEndpointUVE>{link.input}
                                           : std::optional<ScriptPinEndpointUVE>{link.output});
            continue;
        }
        const ScriptNodeTypeDescriptorUVE* outputType = registry.FindNodeTypeUVE(outputNode->typeId);
        const ScriptNodeTypeDescriptorUVE* inputType = registry.FindNodeTypeUVE(inputNode->typeId);
        if (outputType == nullptr || inputType == nullptr) {
            continue;
        }
        const ScriptPinDescriptorUVE* outputPin = FindPinUVE(*outputType, link.output.pinName);
        const ScriptPinDescriptorUVE* inputPin = FindPinUVE(*inputType, link.input.pinName);
        if (outputPin == nullptr) {
            AddDiagnosticUVE(diagnostics, ScriptValidationCodeUVE::UnknownPin, link.output.nodeId,
                             link.output.pinName, "Output pin is not registered on the node.", link.input);
            continue;
        }
        if (inputPin == nullptr) {
            AddDiagnosticUVE(diagnostics, ScriptValidationCodeUVE::UnknownPin, link.input.nodeId,
                             link.input.pinName, "Input pin is not registered on the node.", link.output);
            continue;
        }
        if (outputPin->direction != ScriptPinDirectionUVE::Output) {
            AddDiagnosticUVE(diagnostics, ScriptValidationCodeUVE::WrongPinDirection,
                             link.output.nodeId, link.output.pinName, "Link source pin must be an output.", link.input);
        }
        if (inputPin->direction != ScriptPinDirectionUVE::Input) {
            AddDiagnosticUVE(diagnostics, ScriptValidationCodeUVE::WrongPinDirection,
                             link.input.nodeId, link.input.pinName, "Link destination pin must be an input.", link.output);
        }
        if (outputPin->direction == ScriptPinDirectionUVE::Output &&
            inputPin->direction == ScriptPinDirectionUVE::Input &&
            !AreScriptPinTypesCompatibleUVE(outputPin->type, inputPin->type)) {
            AddDiagnosticUVE(diagnostics, ScriptValidationCodeUVE::IncompatiblePinTypes,
                             link.input.nodeId, link.input.pinName, "Linked pin types are incompatible.", link.output);
        }
        if (outputPin->direction == ScriptPinDirectionUVE::Output &&
            inputPin->direction == ScriptPinDirectionUVE::Input &&
            outputPin->role == ScriptPinRoleUVE::Execution &&
            inputPin->role == ScriptPinRoleUVE::Execution) {
            if (!linkedExecutionOutputs.emplace(link.output.nodeId, link.output.pinName).second) {
                AddDiagnosticUVE(diagnostics, ScriptValidationCodeUVE::ExecutionLinkCardinality,
                                 link.output.nodeId, link.output.pinName,
                                 "An execution output may have only one downstream link.", link.input);
            }
            if (!linkedExecutionInputs.emplace(link.input.nodeId, link.input.pinName).second) {
                AddDiagnosticUVE(diagnostics, ScriptValidationCodeUVE::ExecutionLinkCardinality,
                                 link.input.nodeId, link.input.pinName,
                                 "An execution input may have only one upstream link.", link.output);
            }
        } else if (outputPin->direction == ScriptPinDirectionUVE::Output &&
                   inputPin->direction == ScriptPinDirectionUVE::Input &&
                   outputPin->role != ScriptPinRoleUVE::Execution &&
                   inputPin->role != ScriptPinRoleUVE::Execution &&
                   !linkedDataInputs.emplace(link.input.nodeId, link.input.pinName).second) {
            AddDiagnosticUVE(diagnostics, ScriptValidationCodeUVE::DataLinkCardinality,
                             link.input.nodeId, link.input.pinName,
                             "A data input may have only one upstream link.", link.output);
        }
    }
    return diagnostics;
}

bool AreScriptPinTypesCompatibleUVE(const ScriptValueTypeUVE output,
                                    const ScriptValueTypeUVE input) noexcept {
    return output == input;
}

bool IsScriptExecutionActionDescriptorUVE(const ScriptNodeTypeDescriptorUVE& descriptor) noexcept {
    if (descriptor.typeId.rfind("flow.", 0U) == 0U) {
        return false;
    }
    return std::any_of(descriptor.pins.cbegin(), descriptor.pins.cend(), [](const ScriptPinDescriptorUVE& pin) {
        return pin.direction == ScriptPinDirectionUVE::Input && pin.role == ScriptPinRoleUVE::Execution;
    });
}

} // namespace UVE::Scripting
