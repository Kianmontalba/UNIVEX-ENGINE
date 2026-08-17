// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/scripting/script_builtin_nodes_uve.h"

#include <array>
#include <string_view>
#include <utility>

namespace UVE::Scripting {
namespace {

struct BuiltInNodeDefinitionUVE final {
    std::string_view typeId;
    std::string_view displayName;
    std::vector<ScriptPinDescriptorUVE> pins;
    std::uint32_t displayOrder;
};

[[nodiscard]] std::array<BuiltInNodeDefinitionUVE, 8U> MakeVector3DefinitionsUVE() {
    return {
        BuiltInNodeDefinitionUVE{
            "math.vector3.make", "Make Vector3",
            {ScriptPinDescriptorUVE{"X", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Y", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Z", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Vector", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector3}},
            400U},
        BuiltInNodeDefinitionUVE{
            "math.vector3.add", "Add Vector3",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector3}},
            401U},
        BuiltInNodeDefinitionUVE{
            "math.vector3.subtract", "Subtract Vector3",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector3}},
            402U},
        BuiltInNodeDefinitionUVE{
            "math.vector3.multiply", "Multiply Vector3",
            {ScriptPinDescriptorUVE{"Vector", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Scale", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector3}},
            403U},
        BuiltInNodeDefinitionUVE{
            "math.vector3.dot", "Dot Vector3",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            404U},
        BuiltInNodeDefinitionUVE{
            "math.vector3.cross", "Cross Vector3",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector3}},
            405U},
        BuiltInNodeDefinitionUVE{
            "math.vector3.length", "Length Vector3",
            {ScriptPinDescriptorUVE{"Vector", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Length", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            406U},
        BuiltInNodeDefinitionUVE{
            "math.vector3.normalize", "Normalize Vector3",
            {ScriptPinDescriptorUVE{"Vector", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector3}},
            407U},
    };
}

} // namespace

bool RegisterBuiltInScriptNodesUVE(ScriptNodeRegistryUVE& registry) {
    std::array<BuiltInNodeDefinitionUVE, 8U> definitions = MakeVector3DefinitionsUVE();
    for (const BuiltInNodeDefinitionUVE& definition : definitions) {
        if (registry.FindNodeTypeUVE(definition.typeId) != nullptr) {
            return false;
        }
    }
    for (BuiltInNodeDefinitionUVE& definition : definitions) {
        if (!registry.RegisterNodeTypeUVE(ScriptNodeTypeDescriptorUVE{
                std::string{definition.typeId}, std::string{definition.displayName}, std::move(definition.pins),
                "Math", "node.math.vector3", definition.displayOrder,
                kScriptNodePresentationFlagNoneUVE})) {
            return false;
        }
    }
    return true;
}

} // namespace UVE::Scripting
