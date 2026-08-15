// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/scripting/script_graph_uve.h"

#include <algorithm>
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

void AddDiagnosticUVE(std::vector<ScriptValidationDiagnosticUVE>& diagnostics,
                      ScriptValidationCodeUVE code,
                      std::uint32_t nodeId,
                      std::string pinName,
                      std::string message) {
    diagnostics.push_back(ScriptValidationDiagnosticUVE{code, nodeId, std::move(pinName), std::move(message)});
}

} // namespace

bool ScriptNodeRegistryUVE::RegisterNodeTypeUVE(ScriptNodeTypeDescriptorUVE descriptor) {
    if (descriptor.typeId.empty() || descriptor.displayName.empty() ||
        m_nodeTypes.contains(descriptor.typeId)) {
        return false;
    }

    std::unordered_set<std::string> pinNames;
    pinNames.reserve(descriptor.pins.size());
    for (const ScriptPinDescriptorUVE& pin : descriptor.pins) {
        if (pin.name.empty() || !pinNames.insert(pin.name).second) {
            return false;
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

std::size_t ScriptNodeRegistryUVE::GetNodeTypeCountUVE() const noexcept {
    return m_nodeTypes.size();
}

bool ScriptGraphUVE::AddNodeUVE(ScriptNodeUVE node) {
    if (node.typeId.empty() || FindNodeUVE(m_nodes, node.id) != nullptr) {
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

const std::vector<ScriptNodeUVE>& ScriptGraphUVE::GetNodesUVE() const noexcept {
    return m_nodes;
}

const std::vector<ScriptLinkUVE>& ScriptGraphUVE::GetLinksUVE() const noexcept {
    return m_links;
}

std::vector<ScriptValidationDiagnosticUVE> ScriptGraphUVE::ValidateUVE(
    const ScriptNodeRegistryUVE& registry) const {
    std::vector<ScriptValidationDiagnosticUVE> diagnostics;
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

    for (const ScriptLinkUVE& link : m_links) {
        if (link.output.nodeId == link.input.nodeId) {
            AddDiagnosticUVE(diagnostics, ScriptValidationCodeUVE::SelfLink, link.output.nodeId,
                             link.output.pinName, "A node cannot link to itself.");
        }
        const ScriptNodeUVE* outputNode = FindNodeUVE(m_nodes, link.output.nodeId);
        const ScriptNodeUVE* inputNode = FindNodeUVE(m_nodes, link.input.nodeId);
        if (outputNode == nullptr || inputNode == nullptr) {
            AddDiagnosticUVE(diagnostics, ScriptValidationCodeUVE::EmptyLinkEndpoint,
                             outputNode == nullptr ? link.output.nodeId : link.input.nodeId, {},
                             "Link references a node that is not present in the graph.");
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
                             link.output.pinName, "Output pin is not registered on the node.");
            continue;
        }
        if (inputPin == nullptr) {
            AddDiagnosticUVE(diagnostics, ScriptValidationCodeUVE::UnknownPin, link.input.nodeId,
                             link.input.pinName, "Input pin is not registered on the node.");
            continue;
        }
        if (outputPin->direction != ScriptPinDirectionUVE::Output) {
            AddDiagnosticUVE(diagnostics, ScriptValidationCodeUVE::WrongPinDirection,
                             link.output.nodeId, link.output.pinName, "Link source pin must be an output.");
        }
        if (inputPin->direction != ScriptPinDirectionUVE::Input) {
            AddDiagnosticUVE(diagnostics, ScriptValidationCodeUVE::WrongPinDirection,
                             link.input.nodeId, link.input.pinName, "Link destination pin must be an input.");
        }
        if (outputPin->direction == ScriptPinDirectionUVE::Output &&
            inputPin->direction == ScriptPinDirectionUVE::Input &&
            !AreScriptPinTypesCompatibleUVE(outputPin->type, inputPin->type)) {
            AddDiagnosticUVE(diagnostics, ScriptValidationCodeUVE::IncompatiblePinTypes,
                             link.input.nodeId, link.input.pinName, "Linked pin types are incompatible.");
        }
    }
    return diagnostics;
}

bool AreScriptPinTypesCompatibleUVE(const ScriptValueTypeUVE output,
                                    const ScriptValueTypeUVE input) noexcept {
    return output == input;
}

} // namespace UVE::Scripting
