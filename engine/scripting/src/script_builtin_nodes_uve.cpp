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

[[nodiscard]] std::array<BuiltInNodeDefinitionUVE, 49U> MakeBuiltInDefinitionsUVE() {
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
            "math.float.modulo", "Modulo Float",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Math", "node.math.float", 304U},
        BuiltInNodeDefinitionUVE{
            "math.float.abs", "Abs Float",
            {ScriptPinDescriptorUVE{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Math", "node.math.float", 305U},
        BuiltInNodeDefinitionUVE{
            "math.float.min", "Min Float",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Math", "node.math.float", 306U},
        BuiltInNodeDefinitionUVE{
            "math.float.max", "Max Float",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Math", "node.math.float", 307U},
        BuiltInNodeDefinitionUVE{
            "math.float.clamp", "Clamp Float",
            {ScriptPinDescriptorUVE{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Min", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Max", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Math", "node.math.float", 308U},
        BuiltInNodeDefinitionUVE{
            "math.float.power", "Power Float",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Math", "node.math.float", 309U},
        BuiltInNodeDefinitionUVE{
            "math.vector2.make", "Make Vector2",
            {ScriptPinDescriptorUVE{"X", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Y", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Vector", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector2}},
            "Math", "node.math.vector2", 350U},
        BuiltInNodeDefinitionUVE{
            "math.vector2.add", "Add Vector2",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector2},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector2},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector2}},
            "Math", "node.math.vector2", 351U},
        BuiltInNodeDefinitionUVE{
            "math.vector2.subtract", "Subtract Vector2",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector2},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector2},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector2}},
            "Math", "node.math.vector2", 352U},
        BuiltInNodeDefinitionUVE{
            "math.vector2.multiply", "Multiply Vector2",
            {ScriptPinDescriptorUVE{"Vector", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector2},
             ScriptPinDescriptorUVE{"Scale", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector2}},
            "Math", "node.math.vector2", 353U},
        BuiltInNodeDefinitionUVE{
            "math.vector2.length", "Length Vector2",
            {ScriptPinDescriptorUVE{"Vector", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector2},
             ScriptPinDescriptorUVE{"Length", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Math", "node.math.vector2", 354U},
        BuiltInNodeDefinitionUVE{
            "math.vector2.normalize", "Normalize Vector2",
            {ScriptPinDescriptorUVE{"Vector", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector2},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector2}},
            "Math", "node.math.vector2", 355U},
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
            "logic.boolean.equal", "Equal Number",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
            "Logic", "node.logic.boolean", 504U},
        BuiltInNodeDefinitionUVE{
            "logic.boolean.not_equal", "Not Equal Number",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
            "Logic", "node.logic.boolean", 505U},
        BuiltInNodeDefinitionUVE{
            "logic.boolean.greater", "Greater Number",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
            "Logic", "node.logic.boolean", 506U},
        BuiltInNodeDefinitionUVE{
            "logic.boolean.less", "Less Number",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
            "Logic", "node.logic.boolean", 507U},
        BuiltInNodeDefinitionUVE{
            "logic.boolean.greater_equal", "Greater Equal Number",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
            "Logic", "node.logic.boolean", 508U},
        BuiltInNodeDefinitionUVE{
            "logic.boolean.less_equal", "Less Equal Number",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
            "Logic", "node.logic.boolean", 509U},
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
        BuiltInNodeDefinitionUVE{
            "engine.log", "Log Number",
            {ScriptPinDescriptorUVE{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number}},
            "Engine", "node.engine", 700U},
        BuiltInNodeDefinitionUVE{
            "engine.get_time", "Get Time",
            {ScriptPinDescriptorUVE{"Value", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Engine", "node.engine", 701U},
        BuiltInNodeDefinitionUVE{
            "variable.make_number", "Make Number Variable",
            {ScriptPinDescriptorUVE{"Slot", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number, ScriptPinRoleUVE::Data, "0"},
             ScriptPinDescriptorUVE{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number, ScriptPinRoleUVE::Data, "0"},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Variable", "node.variable", 800U},
        BuiltInNodeDefinitionUVE{
            "variable.get_number", "Get Number Variable",
            {ScriptPinDescriptorUVE{"Slot", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number, ScriptPinRoleUVE::Data, "0"},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Variable", "node.variable", 801U},
        BuiltInNodeDefinitionUVE{
            "variable.set_number", "Set Number Variable",
            {ScriptPinDescriptorUVE{"Slot", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number, ScriptPinRoleUVE::Data, "0"},
             ScriptPinDescriptorUVE{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Variable", "node.variable", 802U},
        BuiltInNodeDefinitionUVE{
            "variable.make_boolean", "Make Boolean Variable",
            {ScriptPinDescriptorUVE{"Slot", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number, ScriptPinRoleUVE::Data, "0"},
             ScriptPinDescriptorUVE{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Boolean},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
            "Variable", "node.variable", 803U},
        BuiltInNodeDefinitionUVE{
            "variable.get_boolean", "Get Boolean Variable",
            {ScriptPinDescriptorUVE{"Slot", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number, ScriptPinRoleUVE::Data, "0"},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
            "Variable", "node.variable", 804U},
        BuiltInNodeDefinitionUVE{
            "variable.set_boolean", "Set Boolean Variable",
            {ScriptPinDescriptorUVE{"Slot", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number, ScriptPinRoleUVE::Data, "0"},
             ScriptPinDescriptorUVE{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Boolean},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
            "Variable", "node.variable", 805U},
        BuiltInNodeDefinitionUVE{
            "variable.make_vector3", "Make Vector3 Variable",
            {ScriptPinDescriptorUVE{"Slot", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number, ScriptPinRoleUVE::Data, "0"},
             ScriptPinDescriptorUVE{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector3}},
            "Variable", "node.variable", 806U},
        BuiltInNodeDefinitionUVE{
            "variable.get_vector3", "Get Vector3 Variable",
            {ScriptPinDescriptorUVE{"Slot", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number, ScriptPinRoleUVE::Data, "0"},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector3}},
            "Variable", "node.variable", 807U},
        BuiltInNodeDefinitionUVE{
            "variable.set_vector3", "Set Vector3 Variable",
            {ScriptPinDescriptorUVE{"Slot", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number, ScriptPinRoleUVE::Data, "0"},
             ScriptPinDescriptorUVE{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector3}},
            "Variable", "node.variable", 808U},
    };
}

} // namespace

bool RegisterBuiltInScriptNodesUVE(ScriptNodeRegistryUVE& registry) {
    std::array<BuiltInNodeDefinitionUVE, 49U> definitions = MakeBuiltInDefinitionsUVE();
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
