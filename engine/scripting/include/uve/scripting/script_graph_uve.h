// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
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
    Component = 7,
    Rotation = 8,
    Transform = 9,
    Array = 10,
    Map = 11,
    Set = 12,
    Struct = 13,
};

enum class ScriptPinRoleUVE : std::uint8_t {
    Execution = 0,
    Data = 1,
};

inline constexpr std::uint32_t kScriptNodePresentationFlagNoneUVE = 0U;
inline constexpr std::uint32_t kScriptNodePresentationFlagCompactUVE = 1U << 0U;
inline constexpr std::uint32_t kScriptNodePresentationFlagCollapsibleUVE = 1U << 1U;

enum class ScriptDiagnosticSeverityUVE : std::uint8_t {
    Info = 0,
    Warning = 1,
    Error = 2,
};

// Values are serialized through the editor bridge; append new codes without renumbering existing entries.
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
    ExecutionLinkCardinality,
    UnsupportedRuntimeNode,
    DataLinkCardinality,
    NodeCountExceeded,
    LinkCountExceeded,
};

struct ScriptPinDescriptorUVE final {
    std::string name;
    ScriptPinDirectionUVE direction = ScriptPinDirectionUVE::Input;
    ScriptValueTypeUVE type = ScriptValueTypeUVE::Number;
    ScriptPinRoleUVE role = ScriptPinRoleUVE::Data;
    std::optional<std::string> defaultValue;

    ScriptPinDescriptorUVE() = default;
    ScriptPinDescriptorUVE(std::string pinName, const ScriptPinDirectionUVE pinDirection,
                          const ScriptValueTypeUVE pinType, const ScriptPinRoleUVE pinRole = ScriptPinRoleUVE::Data,
                          std::optional<std::string> pinDefaultValue = std::nullopt)
        : name(std::move(pinName)), direction(pinDirection), type(pinType), role(pinRole),
          defaultValue(std::move(pinDefaultValue)) {}
};

struct ScriptNodeTypeDescriptorUVE final {
    std::string typeId;
    std::string displayName;
    std::vector<ScriptPinDescriptorUVE> pins;
    std::string category = "Uncategorized";
    std::string iconId = "node.default";
    std::uint32_t displayOrder = 0U;
    std::uint32_t presentationFlags = kScriptNodePresentationFlagNoneUVE;

    ScriptNodeTypeDescriptorUVE() = default;
    ScriptNodeTypeDescriptorUVE(std::string nodeTypeId, std::string nodeDisplayName,
                               std::vector<ScriptPinDescriptorUVE> nodePins,
                               std::string nodeCategory = "Uncategorized",
                               std::string nodeIconId = "node.default",
                               const std::uint32_t nodeDisplayOrder = 0U,
                               const std::uint32_t nodePresentationFlags = kScriptNodePresentationFlagNoneUVE)
        : typeId(std::move(nodeTypeId)), displayName(std::move(nodeDisplayName)), pins(std::move(nodePins)),
          category(std::move(nodeCategory)), iconId(std::move(nodeIconId)), displayOrder(nodeDisplayOrder),
          presentationFlags(nodePresentationFlags) {}
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

inline constexpr std::size_t kMaximumScriptGraphNodesUVE = 64U;
inline constexpr std::size_t kMaximumScriptGraphLinksUVE = 256U;
inline constexpr std::size_t kMaximumScriptDiagnosticMessageBytesUVE = 512U;
inline constexpr std::size_t kMaximumScriptDiagnosticSourceContextBytesUVE = 256U;

struct ScriptValidationDiagnosticUVE final {
    ScriptValidationCodeUVE code = ScriptValidationCodeUVE::EmptyNodeType;
    ScriptDiagnosticSeverityUVE severity = ScriptDiagnosticSeverityUVE::Error;
    std::uint32_t nodeId = 0U;
    std::string pinName;
    std::string message;
    std::string sourceContext;
    std::optional<ScriptPinEndpointUVE> relatedEndpoint;

    ScriptValidationDiagnosticUVE() = default;
    ScriptValidationDiagnosticUVE(ScriptValidationCodeUVE diagnosticCode, const std::uint32_t diagnosticNodeId,
                                  std::string diagnosticPinName, std::string diagnosticMessage,
                                  std::optional<ScriptPinEndpointUVE> diagnosticRelatedEndpoint = std::nullopt,
                                  ScriptDiagnosticSeverityUVE diagnosticSeverity = ScriptDiagnosticSeverityUVE::Error,
                                  std::string diagnosticSourceContext = {})
        : code(diagnosticCode), severity(diagnosticSeverity), nodeId(diagnosticNodeId),
          pinName(std::move(diagnosticPinName)), message(std::move(diagnosticMessage)),
          sourceContext(std::move(diagnosticSourceContext)), relatedEndpoint(std::move(diagnosticRelatedEndpoint)) {}

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
    [[nodiscard]] std::vector<ScriptNodeTypeDescriptorUVE> GetNodeTypeDescriptorsUVE() const;
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
