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
    std::string_view category;
    std::string_view iconId;
    std::uint32_t displayOrder;
};

[[nodiscard]] std::array<BuiltInNodeDefinitionUVE, 20U> MakeBuiltInDefinitionsUVE() {
    return {
        BuiltInNodeDefinitionUVE{
            "flow.sequence", "Sequence",
            {ScriptPinDescriptorUVE{"In", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Execution},
             ScriptPinDescriptorUVE{"Then", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Execution},
             ScriptPinDescriptorUVE{"Then2", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Execution}},
            "Flow", "node.flow", 100U},
        BuiltInNodeDefinitionUVE{
            "flow.branch", "Branch",
            {ScriptPinDescriptorUVE{"In", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Execution},
             ScriptPinDescriptorUVE{"Condition", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Boolean},
             ScriptPinDescriptorUVE{"True", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Execution},
             ScriptPinDescriptorUVE{"False", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Execution}},
            "Flow", "node.flow", 101U},
        BuiltInNodeDefinitionUVE{
            "math.float.add", "Add Float",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Math", "node.math.float", 300U},
        BuiltInNodeDefinitionUVE{
            "math.float.subtract", "Subtract Float",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Math", "node.math.float", 301U},
        BuiltInNodeDefinitionUVE{
            "math.float.multiply", "Multiply Float",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Math", "node.math.float", 302U},
        BuiltInNodeDefinitionUVE{
            "math.float.divide", "Divide Float",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Math", "node.math.float", 303U},
        BuiltInNodeDefinitionUVE{
            "math.vector3.make", "Make Vector3",
            {ScriptPinDescriptorUVE{"X", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Y", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Z", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Vector", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector3}},
            "Math", "node.math.vector3", 400U},
        BuiltInNodeDefinitionUVE{
            "math.vector3.add", "Add Vector3",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector3}},
            "Math", "node.math.vector3", 401U},
        BuiltInNodeDefinitionUVE{
            "math.vector3.subtract", "Subtract Vector3",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector3}},
            "Math", "node.math.vector3", 402U},
        BuiltInNodeDefinitionUVE{
            "math.vector3.multiply", "Multiply Vector3",
            {ScriptPinDescriptorUVE{"Vector", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Scale", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector3}},
            "Math", "node.math.vector3", 403U},
        BuiltInNodeDefinitionUVE{
            "math.vector3.dot", "Dot Vector3",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Math", "node.math.vector3", 404U},
        BuiltInNodeDefinitionUVE{
            "math.vector3.cross", "Cross Vector3",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector3}},
            "Math", "node.math.vector3", 405U},
        BuiltInNodeDefinitionUVE{
            "math.vector3.length", "Length Vector3",
            {ScriptPinDescriptorUVE{"Vector", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Length", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Math", "node.math.vector3", 406U},
        BuiltInNodeDefinitionUVE{
            "math.vector3.normalize", "Normalize Vector3",
            {ScriptPinDescriptorUVE{"Vector", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector3}},
            "Math", "node.math.vector3", 407U},
        BuiltInNodeDefinitionUVE{
            "logic.boolean.not", "Not Boolean",
            {ScriptPinDescriptorUVE{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Boolean},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
            "Logic", "node.logic.boolean", 500U},
        BuiltInNodeDefinitionUVE{
            "logic.boolean.and", "And Boolean",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Boolean},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Boolean},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
            "Logic", "node.logic.boolean", 501U},
        BuiltInNodeDefinitionUVE{
            "logic.boolean.or", "Or Boolean",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Boolean},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Boolean},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
            "Logic", "node.logic.boolean", 502U},
        BuiltInNodeDefinitionUVE{
            "logic.boolean.xor", "Xor Boolean",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Boolean},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Boolean},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
            "Logic", "node.logic.boolean", 503U},
        BuiltInNodeDefinitionUVE{
            "query.entity.has_component", "Has Component",
            {ScriptPinDescriptorUVE{"Entity", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Entity},
             ScriptPinDescriptorUVE{"Component", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Component},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
            "Entity Query", "node.entity.query", 600U},
        BuiltInNodeDefinitionUVE{
            "query.entity.get_component", "Get Component",
            {ScriptPinDescriptorUVE{"Entity", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Entity},
             ScriptPinDescriptorUVE{"Component", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Component},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Component}},
            "Entity Query", "node.entity.query", 601U},
    };
}

} // namespace

bool RegisterBuiltInScriptNodesUVE(ScriptNodeRegistryUVE& registry) {
    std::array<BuiltInNodeDefinitionUVE, 20U> definitions = MakeBuiltInDefinitionsUVE();
    for (const BuiltInNodeDefinitionUVE& definition : definitions) {
        if (registry.FindNodeTypeUVE(definition.typeId) != nullptr) {
            return false;
        }
    }
    for (BuiltInNodeDefinitionUVE& definition : definitions) {
        if (!registry.RegisterNodeTypeUVE(ScriptNodeTypeDescriptorUVE{
                std::string{definition.typeId}, std::string{definition.displayName}, std::move(definition.pins),
                std::string{definition.category}, std::string{definition.iconId}, definition.displayOrder,
                kScriptNodePresentationFlagNoneUVE})) {
            return false;
        }
    }
    return true;
}

} // namespace UVE::Scripting
