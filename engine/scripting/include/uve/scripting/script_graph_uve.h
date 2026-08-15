// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace UVE::Scripting {

enum class ScriptPinDirectionUVE : std::uint8_t {
    Input = 0,
    Output = 1,
};

enum class ScriptValueTypeUVE : std::uint8_t {
    Execution = 0,
    Boolean = 1,
    Number = 2,
    Vector2 = 3,
    Vector3 = 4,
    Entity = 5,
    Asset = 6,
};

enum class ScriptValidationCodeUVE : std::uint8_t {
    EmptyNodeType = 0,
    DuplicateNodeType,
    DuplicatePinName,
    EmptyPinName,
    EmptyNodeTypeReference,
    DuplicateNodeId,
    EmptyLinkEndpoint,
    UnknownNodeType,
    UnknownPin,
    WrongPinDirection,
    IncompatiblePinTypes,
    DuplicateLink,
    SelfLink,
};

struct ScriptPinDescriptorUVE final {
    std::string name;
    ScriptPinDirectionUVE direction = ScriptPinDirectionUVE::Input;
    ScriptValueTypeUVE type = ScriptValueTypeUVE::Number;
};

struct ScriptNodeTypeDescriptorUVE final {
    std::string typeId;
    std::string displayName;
    std::vector<ScriptPinDescriptorUVE> pins;
};

struct ScriptNodeUVE final {
    std::uint32_t id = 0U;
    std::string typeId;
};

struct ScriptPinEndpointUVE final {
    std::uint32_t nodeId = 0U;
    std::string pinName;

    [[nodiscard]] bool operator==(const ScriptPinEndpointUVE&) const = default;
};

struct ScriptLinkUVE final {
    ScriptPinEndpointUVE output;
    ScriptPinEndpointUVE input;

    [[nodiscard]] bool operator==(const ScriptLinkUVE&) const = default;
};

struct ScriptValidationDiagnosticUVE final {
    ScriptValidationCodeUVE code = ScriptValidationCodeUVE::EmptyNodeType;
    std::uint32_t nodeId = 0U;
    std::string pinName;
    std::string message;

    [[nodiscard]] bool operator==(const ScriptValidationDiagnosticUVE&) const = default;
};

class ScriptNodeRegistryUVE final {
public:
    ScriptNodeRegistryUVE() = default;
    ScriptNodeRegistryUVE(const ScriptNodeRegistryUVE&) = delete;
    ScriptNodeRegistryUVE& operator=(const ScriptNodeRegistryUVE&) = delete;

    [[nodiscard]] bool RegisterNodeTypeUVE(ScriptNodeTypeDescriptorUVE descriptor);
    [[nodiscard]] const ScriptNodeTypeDescriptorUVE* FindNodeTypeUVE(std::string_view typeId) const noexcept;
    [[nodiscard]] std::vector<std::string> GetNodeTypeIdsUVE() const;
    [[nodiscard]] std::size_t GetNodeTypeCountUVE() const noexcept;

private:
    std::unordered_map<std::string, ScriptNodeTypeDescriptorUVE> m_nodeTypes;
};

class ScriptGraphUVE final {
public:
    ScriptGraphUVE() = default;

    [[nodiscard]] bool AddNodeUVE(ScriptNodeUVE node);
    [[nodiscard]] bool AddLinkUVE(ScriptLinkUVE link);
    [[nodiscard]] bool RemoveNodeUVE(std::uint32_t nodeId);
    [[nodiscard]] bool RemoveLinkUVE(const ScriptLinkUVE& link);
    [[nodiscard]] const std::vector<ScriptNodeUVE>& GetNodesUVE() const noexcept;
    [[nodiscard]] const std::vector<ScriptLinkUVE>& GetLinksUVE() const noexcept;
    [[nodiscard]] std::vector<ScriptValidationDiagnosticUVE> ValidateUVE(
        const ScriptNodeRegistryUVE& registry) const;

private:
    std::vector<ScriptNodeUVE> m_nodes;
    std::vector<ScriptLinkUVE> m_links;
};

[[nodiscard]] bool AreScriptPinTypesCompatibleUVE(ScriptValueTypeUVE output,
                                                    ScriptValueTypeUVE input) noexcept;

} // namespace UVE::Scripting
