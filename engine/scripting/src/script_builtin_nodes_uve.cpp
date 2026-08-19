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

[[nodiscard]] std::array<BuiltInNodeDefinitionUVE, 130U> MakeBuiltInDefinitionsUVE() {
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
            "flow.return", "Return",
            {ScriptPinDescriptorUVE{"In", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Execution}},
            "Flow", "node.flow", 102U},
        BuiltInNodeDefinitionUVE{
            "flow.do_once", "Do Once",
            {ScriptPinDescriptorUVE{"In", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Execution},
             ScriptPinDescriptorUVE{"Reset", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Execution},
             ScriptPinDescriptorUVE{"Then", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Execution},
             ScriptPinDescriptorUVE{"Default", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Execution}},
            "Flow", "node.flow", 103U},
        BuiltInNodeDefinitionUVE{
            "flow.gate", "Gate",
            {ScriptPinDescriptorUVE{"In", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Execution},
             ScriptPinDescriptorUVE{"Open", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Execution},
             ScriptPinDescriptorUVE{"Close", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Execution},
             ScriptPinDescriptorUVE{"Exit", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Execution},
             ScriptPinDescriptorUVE{"Default", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Execution}},
            "Flow", "node.flow", 104U},
        BuiltInNodeDefinitionUVE{
            "flow.switch", "Switch",
            {ScriptPinDescriptorUVE{"In", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Execution},
             ScriptPinDescriptorUVE{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Case0", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Execution},
             ScriptPinDescriptorUVE{"Case1", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Execution},
             ScriptPinDescriptorUVE{"Default", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Execution}},
            "Flow", "node.flow", 105U},
        BuiltInNodeDefinitionUVE{
            "flow.event", "Event",
            {ScriptPinDescriptorUVE{"Then", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Execution}},
            "Flow", "node.flow", 106U},
        BuiltInNodeDefinitionUVE{
            "flow.loop", "Loop",
            {ScriptPinDescriptorUVE{"In", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Execution},
             ScriptPinDescriptorUVE{"Count", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Body", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Execution},
             ScriptPinDescriptorUVE{"Completed", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Execution}},
            "Flow", "node.flow", 107U},
        BuiltInNodeDefinitionUVE{
            "flow.for_loop", "For Loop",
            {ScriptPinDescriptorUVE{"In", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Execution},
             ScriptPinDescriptorUVE{"Count", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Body", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Execution},
             ScriptPinDescriptorUVE{"Completed", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Execution},
             ScriptPinDescriptorUVE{"Index", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Flow", "node.flow", 108U},
        BuiltInNodeDefinitionUVE{
            "flow.while_loop", "While Loop",
            {ScriptPinDescriptorUVE{"In", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Execution},
             ScriptPinDescriptorUVE{"Condition", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Boolean},
             ScriptPinDescriptorUVE{"Body", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Execution},
             ScriptPinDescriptorUVE{"Completed", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Execution}},
            "Flow", "node.flow", 109U},
        BuiltInNodeDefinitionUVE{
            "flow.delay", "Delay",
            {ScriptPinDescriptorUVE{"In", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Execution},
             ScriptPinDescriptorUVE{"Frames", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Then", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Execution}},
            "Flow", "node.flow", 110U},
        BuiltInNodeDefinitionUVE{
            "convert.number_to_boolean", "Number to Boolean",
            {ScriptPinDescriptorUVE{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
            "Conversion", "node.conversion", 200U},
        BuiltInNodeDefinitionUVE{
            "convert.boolean_to_number", "Boolean to Number",
            {ScriptPinDescriptorUVE{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Boolean},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Conversion", "node.conversion", 201U},
        BuiltInNodeDefinitionUVE{
            "convert.vector2_to_vector3", "Vector2 to Vector3",
            {ScriptPinDescriptorUVE{"Vector", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector2},
             ScriptPinDescriptorUVE{"Z", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number,
                                    ScriptPinRoleUVE::Data, std::string{"0"}},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector3}},
            "Conversion", "node.conversion", 202U},
        BuiltInNodeDefinitionUVE{
            "convert.vector3_to_vector2", "Vector3 to Vector2",
            {ScriptPinDescriptorUVE{"Vector", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector2}},
            "Conversion", "node.conversion", 203U},
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
            "math.float.lerp", "Lerp Float",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Alpha", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Math", "node.math.float", 310U},
        BuiltInNodeDefinitionUVE{
            "math.float.remap", "Remap Float",
            {ScriptPinDescriptorUVE{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"FromMin", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"FromMax", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"ToMin", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"ToMax", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Math", "node.math.float", 311U},
        BuiltInNodeDefinitionUVE{
            "math.float.sin", "Sin Float",
            {ScriptPinDescriptorUVE{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Math", "node.math.float", 312U},
        BuiltInNodeDefinitionUVE{
            "math.float.cos", "Cos Float",
            {ScriptPinDescriptorUVE{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Math", "node.math.float", 313U},
        BuiltInNodeDefinitionUVE{
            "math.float.tan", "Tan Float",
            {ScriptPinDescriptorUVE{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Math", "node.math.float", 314U},
        BuiltInNodeDefinitionUVE{
            "math.float.sqrt", "Sqrt Float",
            {ScriptPinDescriptorUVE{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Math", "node.math.float", 315U},
        BuiltInNodeDefinitionUVE{
            "math.float.random", "Random Float",
            {ScriptPinDescriptorUVE{"Seed", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Math", "node.math.float", 316U},
        BuiltInNodeDefinitionUVE{
            "math.float.random_range", "Random Range Float",
            {ScriptPinDescriptorUVE{"Seed", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Min", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Max", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Math", "node.math.float", 317U},
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
            "math.vector2.dot", "Dot Vector2",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector2},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector2},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Math", "node.math.vector2", 356U},
        BuiltInNodeDefinitionUVE{
            "math.vector2.distance", "Distance Vector2",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector2},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector2},
             ScriptPinDescriptorUVE{"Distance", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Math", "node.math.vector2", 357U},
        BuiltInNodeDefinitionUVE{
            "math.vector2.direction", "Direction Vector2",
            {ScriptPinDescriptorUVE{"From", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector2},
             ScriptPinDescriptorUVE{"To", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector2},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector2}},
            "Math", "node.math.vector2", 358U},
        BuiltInNodeDefinitionUVE{
            "math.vector2.lerp", "Lerp Vector2",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector2},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector2},
             ScriptPinDescriptorUVE{"Alpha", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector2}},
            "Math", "node.math.vector2", 359U},
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
            "math.vector3.distance", "Distance Vector3",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Distance", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Math", "node.math.vector3", 408U},
        BuiltInNodeDefinitionUVE{
            "math.vector3.direction", "Direction Vector3",
            {ScriptPinDescriptorUVE{"From", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"To", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector3}},
            "Math", "node.math.vector3", 409U},
        BuiltInNodeDefinitionUVE{
            "math.vector3.lerp", "Lerp Vector3",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Alpha", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector3}},
            "Math", "node.math.vector3", 410U},
        BuiltInNodeDefinitionUVE{
            "math.rotation.make", "Make Rotation",
            {ScriptPinDescriptorUVE{"Axis", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Radians", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Rotation", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Rotation}},
            "Rotation", "node.math.rotation", 450U},
        BuiltInNodeDefinitionUVE{
            "math.rotation.break", "Break Rotation",
            {ScriptPinDescriptorUVE{"Rotation", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Rotation},
             ScriptPinDescriptorUVE{"Axis", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Radians", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Rotation", "node.math.rotation", 451U},
        BuiltInNodeDefinitionUVE{
            "math.rotation.degrees", "Rotation to Degrees",
            {ScriptPinDescriptorUVE{"Radians", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Degrees", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Rotation", "node.math.rotation", 452U},
        BuiltInNodeDefinitionUVE{
            "math.rotation.radians", "Degrees to Rotation Radians",
            {ScriptPinDescriptorUVE{"Degrees", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Radians", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Rotation", "node.math.rotation", 453U},
        BuiltInNodeDefinitionUVE{
            "math.rotation.euler", "Make Euler Rotation",
            {ScriptPinDescriptorUVE{"Radians", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Rotation", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Rotation}},
            "Rotation", "node.math.rotation", 454U},
        BuiltInNodeDefinitionUVE{
            "math.rotation.quaternion", "Make Quaternion Rotation",
            {ScriptPinDescriptorUVE{"X", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Y", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Z", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"W", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Rotation", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Rotation}},
            "Rotation", "node.math.rotation", 455U},
        BuiltInNodeDefinitionUVE{
            "math.rotation.look_at", "Look At Rotation",
            {ScriptPinDescriptorUVE{"Direction", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Up", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Rotation", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Rotation}},
            "Rotation", "node.math.rotation", 456U},
        BuiltInNodeDefinitionUVE{
            "math.rotation.slerp", "Slerp Rotation",
            {ScriptPinDescriptorUVE{"A", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Rotation},
             ScriptPinDescriptorUVE{"B", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Rotation},
             ScriptPinDescriptorUVE{"Alpha", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Rotation}},
            "Rotation", "node.math.rotation", 457U},
        BuiltInNodeDefinitionUVE{
            "math.rotation.rotate", "Rotate Vector by Rotation",
            {ScriptPinDescriptorUVE{"Rotation", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Rotation},
             ScriptPinDescriptorUVE{"Vector", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector3}},
            "Rotation", "node.math.rotation", 458U},
        BuiltInNodeDefinitionUVE{
            "math.transform.make", "Make Transform",
            {ScriptPinDescriptorUVE{"Position", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Rotation", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Rotation},
             ScriptPinDescriptorUVE{"Scale", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Transform", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Transform}},
            "Transform", "node.math.transform", 600U},
        BuiltInNodeDefinitionUVE{
            "math.transform.break", "Break Transform",
            {ScriptPinDescriptorUVE{"Transform", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Transform},
             ScriptPinDescriptorUVE{"Position", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Rotation", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Rotation},
             ScriptPinDescriptorUVE{"Scale", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector3}},
            "Transform", "node.math.transform", 601U},
        BuiltInNodeDefinitionUVE{
            "math.transform.get_position", "Get Transform Position",
            {ScriptPinDescriptorUVE{"Transform", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Transform},
             ScriptPinDescriptorUVE{"Position", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector3}},
            "Transform", "node.math.transform", 602U},
        BuiltInNodeDefinitionUVE{
            "math.transform.set_position", "Set Transform Position",
            {ScriptPinDescriptorUVE{"Transform", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Transform},
             ScriptPinDescriptorUVE{"Position", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Transform}},
            "Transform", "node.math.transform", 603U},
        BuiltInNodeDefinitionUVE{
            "math.transform.get_rotation", "Get Transform Rotation",
            {ScriptPinDescriptorUVE{"Transform", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Transform},
             ScriptPinDescriptorUVE{"Rotation", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Rotation}},
            "Transform", "node.math.transform", 604U},
        BuiltInNodeDefinitionUVE{
            "math.transform.set_rotation", "Set Transform Rotation",
            {ScriptPinDescriptorUVE{"Transform", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Transform},
             ScriptPinDescriptorUVE{"Rotation", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Rotation},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Transform}},
            "Transform", "node.math.transform", 605U},
        BuiltInNodeDefinitionUVE{
            "math.transform.get_scale", "Get Transform Scale",
            {ScriptPinDescriptorUVE{"Transform", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Transform},
             ScriptPinDescriptorUVE{"Scale", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector3}},
            "Transform", "node.math.transform", 606U},
        BuiltInNodeDefinitionUVE{
            "math.transform.set_scale", "Set Transform Scale",
            {ScriptPinDescriptorUVE{"Transform", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Transform},
             ScriptPinDescriptorUVE{"Scale", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Transform}},
            "Transform", "node.math.transform", 607U},
        BuiltInNodeDefinitionUVE{
            "math.transform.translate", "Translate Transform",
            {ScriptPinDescriptorUVE{"Transform", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Transform},
             ScriptPinDescriptorUVE{"Translation", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Transform}},
            "Transform", "node.math.transform", 608U},
        BuiltInNodeDefinitionUVE{
            "math.transform.rotate", "Rotate Transform",
            {ScriptPinDescriptorUVE{"Transform", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Transform},
             ScriptPinDescriptorUVE{"Rotation", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Rotation},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Transform}},
            "Transform", "node.math.transform", 609U},
        BuiltInNodeDefinitionUVE{
            "math.transform.transform_point", "Transform Point",
            {ScriptPinDescriptorUVE{"Transform", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Transform},
             ScriptPinDescriptorUVE{"Point", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector3}},
            "Transform", "node.math.transform", 610U},
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
            "Entity Query", "node.entity.query", 620U},
        BuiltInNodeDefinitionUVE{
            "query.entity.get_component", "Get Component",
            {ScriptPinDescriptorUVE{"Entity", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Entity},
             ScriptPinDescriptorUVE{"Component", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Component},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Component}},
            "Entity Query", "node.entity.query", 621U},
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
        BuiltInNodeDefinitionUVE{
            "variable.make_array", "Make Array Variable",
            {ScriptPinDescriptorUVE{"Slot", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number, ScriptPinRoleUVE::Data, "0"},
             ScriptPinDescriptorUVE{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Array},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Array}},
            "Variable", "node.variable", 809U},
        BuiltInNodeDefinitionUVE{
            "variable.get_array", "Get Array Variable",
            {ScriptPinDescriptorUVE{"Slot", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number, ScriptPinRoleUVE::Data, "0"},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Array}},
            "Variable", "node.variable", 810U},
        BuiltInNodeDefinitionUVE{
            "variable.set_array", "Set Array Variable",
            {ScriptPinDescriptorUVE{"Slot", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number, ScriptPinRoleUVE::Data, "0"},
             ScriptPinDescriptorUVE{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Array},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Array}},
            "Variable", "node.variable", 811U},
        BuiltInNodeDefinitionUVE{
            "variable.make_map", "Make Map Variable",
            {ScriptPinDescriptorUVE{"Slot", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number, ScriptPinRoleUVE::Data, "0"},
             ScriptPinDescriptorUVE{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Map},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Map}},
            "Variable", "node.variable", 812U},
        BuiltInNodeDefinitionUVE{
            "variable.get_map", "Get Map Variable",
            {ScriptPinDescriptorUVE{"Slot", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number, ScriptPinRoleUVE::Data, "0"},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Map}},
            "Variable", "node.variable", 813U},
        BuiltInNodeDefinitionUVE{
            "variable.set_map", "Set Map Variable",
            {ScriptPinDescriptorUVE{"Slot", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number, ScriptPinRoleUVE::Data, "0"},
             ScriptPinDescriptorUVE{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Map},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Map}},
            "Variable", "node.variable", 814U},
        BuiltInNodeDefinitionUVE{
            "variable.make_set", "Make Set Variable",
            {ScriptPinDescriptorUVE{"Slot", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number, ScriptPinRoleUVE::Data, "0"},
             ScriptPinDescriptorUVE{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Set},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Set}},
            "Variable", "node.variable", 815U},
        BuiltInNodeDefinitionUVE{
            "variable.get_set", "Get Set Variable",
            {ScriptPinDescriptorUVE{"Slot", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number, ScriptPinRoleUVE::Data, "0"},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Set}},
            "Variable", "node.variable", 816U},
        BuiltInNodeDefinitionUVE{
            "variable.set_set", "Set Set Variable",
            {ScriptPinDescriptorUVE{"Slot", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number, ScriptPinRoleUVE::Data, "0"},
             ScriptPinDescriptorUVE{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Set},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Set}},
            "Variable", "node.variable", 817U},
        BuiltInNodeDefinitionUVE{
            "variable.make_struct", "Make Struct Variable",
            {ScriptPinDescriptorUVE{"Slot", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number, ScriptPinRoleUVE::Data, "0"},
             ScriptPinDescriptorUVE{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Struct},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Struct}},
            "Variable", "node.variable", 818U},
        BuiltInNodeDefinitionUVE{
            "variable.get_struct", "Get Struct Variable",
            {ScriptPinDescriptorUVE{"Slot", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number, ScriptPinRoleUVE::Data, "0"},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Struct}},
            "Variable", "node.variable", 819U},
        BuiltInNodeDefinitionUVE{
            "variable.set_struct", "Set Struct Variable",
            {ScriptPinDescriptorUVE{"Slot", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number, ScriptPinRoleUVE::Data, "0"},
             ScriptPinDescriptorUVE{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Struct},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Struct}},
            "Variable", "node.variable", 820U},
        BuiltInNodeDefinitionUVE{
            "entity.spawn", "Spawn Entity",
            {ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Entity}},
            "Entity", "node.entity", 900U},
        BuiltInNodeDefinitionUVE{
            "entity.destroy", "Destroy Entity",
            {ScriptPinDescriptorUVE{"Entity", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Entity},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
            "Entity", "node.entity", 901U},
        BuiltInNodeDefinitionUVE{
            "entity.find", "Find Entity",
            {ScriptPinDescriptorUVE{"Component", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Component},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Entity}},
            "Entity", "node.entity", 902U},
        BuiltInNodeDefinitionUVE{
            "entity.get_entity", "Get Entity",
            {ScriptPinDescriptorUVE{"Handle", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Entity}},
            "Entity", "node.entity", 903U},
        BuiltInNodeDefinitionUVE{
            "entity.add_component", "Add Component",
            {ScriptPinDescriptorUVE{"Entity", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Entity},
             ScriptPinDescriptorUVE{"Component", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Component},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
            "Entity", "node.entity", 904U},
        BuiltInNodeDefinitionUVE{
            "entity.remove_component", "Remove Component",
            {ScriptPinDescriptorUVE{"Entity", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Entity},
             ScriptPinDescriptorUVE{"Component", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Component},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
            "Entity", "node.entity", 905U},
        BuiltInNodeDefinitionUVE{
            "input.key_pressed", "Key Pressed",
            {ScriptPinDescriptorUVE{"Key", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
            "Input", "node.input", 1000U},
        BuiltInNodeDefinitionUVE{
            "input.key_released", "Key Released",
            {ScriptPinDescriptorUVE{"Key", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
            "Input", "node.input", 1001U},
        BuiltInNodeDefinitionUVE{
            "input.key_down", "Key Down",
            {ScriptPinDescriptorUVE{"Key", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
            "Input", "node.input", 1002U},
        BuiltInNodeDefinitionUVE{
            "input.mouse_position", "Mouse Position",
            {ScriptPinDescriptorUVE{"Position", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Vector2}},
            "Input", "node.input", 1003U},
        BuiltInNodeDefinitionUVE{
            "input.mouse_button", "Mouse Button",
            {ScriptPinDescriptorUVE{"Button", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
            "Input", "node.input", 1004U},
        BuiltInNodeDefinitionUVE{
            "input.gamepad_button", "Gamepad Button",
            {ScriptPinDescriptorUVE{"Gamepad", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Button", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
            "Input", "node.input", 1005U},
        BuiltInNodeDefinitionUVE{
            "input.get_axis", "Get Axis",
            {ScriptPinDescriptorUVE{"Gamepad", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Axis", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}},
            "Input", "node.input", 1006U},
        BuiltInNodeDefinitionUVE{
            "input.get_action", "Get Action",
            {ScriptPinDescriptorUVE{"Action", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
            "Input", "node.input", 1007U},
        BuiltInNodeDefinitionUVE{
            "camera.get_camera", "Get Camera",
            {ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Entity}},
            "Camera", "node.camera", 1010U},
        BuiltInNodeDefinitionUVE{
            "camera.set_position", "Set Camera Position",
            {ScriptPinDescriptorUVE{"Camera", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Entity},
             ScriptPinDescriptorUVE{"Position", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
            "Camera", "node.camera", 1011U},
        BuiltInNodeDefinitionUVE{
            "camera.set_rotation", "Set Camera Rotation",
            {ScriptPinDescriptorUVE{"Camera", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Entity},
             ScriptPinDescriptorUVE{"Rotation", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Rotation},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
            "Camera", "node.camera", 1012U},
        BuiltInNodeDefinitionUVE{
            "camera.look_at", "Camera Look At",
            {ScriptPinDescriptorUVE{"Camera", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Entity},
             ScriptPinDescriptorUVE{"Target", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Vector3},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
            "Camera", "node.camera", 1013U},
        BuiltInNodeDefinitionUVE{
            "camera.set_fov", "Set Camera FOV",
            {ScriptPinDescriptorUVE{"Camera", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Entity},
             ScriptPinDescriptorUVE{"FOV", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
            "Camera", "node.camera", 1014U},
        BuiltInNodeDefinitionUVE{
            "camera.shake", "Camera Shake",
            {ScriptPinDescriptorUVE{"Camera", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Entity},
             ScriptPinDescriptorUVE{"Amplitude", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Duration", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
            "Camera", "node.camera", 1015U},
        BuiltInNodeDefinitionUVE{
            "camera.set_active", "Set Camera Active",
            {ScriptPinDescriptorUVE{"Camera", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Entity},
             ScriptPinDescriptorUVE{"Active", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Boolean},
             ScriptPinDescriptorUVE{"Result", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
            "Camera", "node.camera", 1016U},
    };
}

} // namespace

bool RegisterBuiltInScriptNodesUVE(ScriptNodeRegistryUVE& registry) {
    std::array<BuiltInNodeDefinitionUVE, 130U> definitions = MakeBuiltInDefinitionsUVE();
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
