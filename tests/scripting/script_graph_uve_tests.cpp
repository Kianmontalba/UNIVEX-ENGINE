// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/scripting/script_builtin_nodes_uve.h"
#include "uve/scripting/script_bytecode_uve.h"
#include "uve/scripting/script_runtime_uve.h"
#include "uve/scripting/script_vm_uve.h"
#include "uve/scripting/script_entity_query_adapter_uve.h"
#include "uve/scripting/script_compiler_ir_uve.h"
#include "uve/scripting/script_debugger_uve.h"
#include "uve/scripting/script_graph_editor_backend_uve.h"
#include "uve/scripting/script_graph_canvas_uve.h"
#include "uve/scripting/script_graph_canvas_persistence_uve.h"
#include "uve/scripting/script_graph_persistence_uve.h"
#include "uve/scripting/script_graph_uve.h"
#include "uve/scripting/script_graph_runtime_binding_uve.h"
#include "uve/scripting/script_component_runtime_ownership_uve.h"
#include "uve/scripting/script_hot_reload_uve.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

#include "uve/events/event_system_uve.h"
#include "uve/memory/memory_manager_uve.h"
#include "uve/scene/components/name_component_uve.h"
#include "uve/scene/entity_manager_uve.h"

namespace UVE::Scripting {
namespace {

ScriptNodeTypeDescriptorUVE MakeSourceNodeUVE() {
    return ScriptNodeTypeDescriptorUVE{
        "test.source",
        "Test Source",
        {{"Out", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number},
         {"Exec", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Execution}},
    };
}

ScriptNodeTypeDescriptorUVE MakeSinkNodeUVE() {
    return ScriptNodeTypeDescriptorUVE{
        "test.sink",
        "Test Sink",
        {{"In", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
         {"Exec", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Execution}},
    };
}

ScriptNodeTypeDescriptorUVE MakeBooleanSourceNodeUVE() {
    return ScriptNodeTypeDescriptorUVE{
        "test.boolean-source",
        "Boolean Source",
        {{"Out", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Boolean}},
    };
}

struct EngineLogCaptureUVE final {
    std::size_t callCount = 0U;
    float lastValue = 0.0F;
    bool accept = true;
};

bool CaptureEngineLogUVE(void* userData, const float value) noexcept {
    auto* capture = static_cast<EngineLogCaptureUVE*>(userData);
    if (capture == nullptr) {
        return false;
    }
    ++capture->callCount;
    capture->lastValue = value;
    return capture->accept;
}

void RegisterTestNodesUVE(ScriptNodeRegistryUVE& registry) {
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(MakeSourceNodeUVE()));
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(MakeSinkNodeUVE()));
}

} // namespace

TEST(ScriptNodeRegistryUVETest, RegisterNodeTypeUVE_RejectsDuplicateAndMalformedDescriptors) {
    ScriptNodeRegistryUVE registry;
    EXPECT_FALSE(registry.RegisterNodeTypeUVE({"", "Empty", {}}));
    EXPECT_FALSE(registry.RegisterNodeTypeUVE({"test.empty-display", "", {}}));
    EXPECT_FALSE(registry.RegisterNodeTypeUVE({"test.empty-pin", "Invalid", {{"", ScriptPinDirectionUVE::Input,
                                                                                ScriptValueTypeUVE::Number}}}));
    EXPECT_FALSE(registry.RegisterNodeTypeUVE({"test.duplicate-pin", "Invalid",
                                                {{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number},
                                                 {"Value", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}}}));
    EXPECT_TRUE(registry.RegisterNodeTypeUVE(MakeSourceNodeUVE()));
    EXPECT_FALSE(registry.RegisterNodeTypeUVE(MakeSourceNodeUVE()));
    EXPECT_EQ(registry.GetNodeTypeCountUVE(), 1U);
}

TEST(ScriptNodeRegistryUVETest, BuiltInVector3Catalog_RegistersDeterministicDescriptorContracts) {
    ScriptNodeRegistryUVE registry;

    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    EXPECT_FALSE(RegisterBuiltInScriptNodesUVE(registry));
    EXPECT_EQ(registry.GetNodeTypeCountUVE(), 21U);

    const std::vector<ScriptNodeTypeDescriptorUVE> descriptors = registry.GetNodeTypeDescriptorsUVE();
    ASSERT_EQ(descriptors.size(), 21U);
    const std::vector<std::string> expectedIds{
        "flow.sequence", "flow.branch",
        "math.float.add", "math.float.subtract", "math.float.multiply", "math.float.divide",
        "math.vector3.make", "math.vector3.add", "math.vector3.subtract", "math.vector3.multiply",
        "math.vector3.dot", "math.vector3.cross", "math.vector3.length", "math.vector3.normalize",
        "logic.boolean.not", "logic.boolean.and", "logic.boolean.or", "logic.boolean.xor",
        "query.entity.has_component", "query.entity.get_component", "engine.log"};
    ASSERT_EQ(expectedIds.size(), descriptors.size());
    for (std::size_t index = 0U; index < expectedIds.size(); ++index) {
        EXPECT_EQ(descriptors[index].typeId, expectedIds[index]);
    }
    for (std::size_t index = 0U; index < 2U; ++index) {
        EXPECT_EQ(descriptors[index].category, "Flow");
        EXPECT_EQ(descriptors[index].iconId, "node.flow");
    }
    for (std::size_t index = 2U; index < 6U; ++index) {
        EXPECT_EQ(descriptors[index].category, "Math");
        EXPECT_EQ(descriptors[index].iconId, "node.math.float");
    }
    for (std::size_t index = 6U; index < 14U; ++index) {
        EXPECT_EQ(descriptors[index].category, "Math");
        EXPECT_EQ(descriptors[index].iconId, "node.math.vector3");
    }
    for (std::size_t index = 14U; index < 18U; ++index) {
        EXPECT_EQ(descriptors[index].category, "Logic");
        EXPECT_EQ(descriptors[index].iconId, "node.logic.boolean");
    }
    for (std::size_t index = 18U; index < 20U; ++index) {
        EXPECT_EQ(descriptors[index].category, "Entity Query");
        EXPECT_EQ(descriptors[index].iconId, "node.entity.query");
    }
    EXPECT_EQ(descriptors[20U].category, "Engine");
    EXPECT_EQ(descriptors[20U].iconId, "node.engine");

    const ScriptNodeTypeDescriptorUVE* sequence = registry.FindNodeTypeUVE("flow.sequence");
    ASSERT_NE(sequence, nullptr);
    ASSERT_EQ(sequence->pins.size(), 3U);
    EXPECT_EQ(sequence->pins[0].role, ScriptPinRoleUVE::Execution);
    EXPECT_EQ(sequence->pins[1].role, ScriptPinRoleUVE::Execution);
    EXPECT_EQ(sequence->pins[2].role, ScriptPinRoleUVE::Execution);
    EXPECT_EQ(sequence->pins[1].name, "Then");
    EXPECT_EQ(sequence->pins[2].name, "Then2");

    const ScriptNodeTypeDescriptorUVE* branch = registry.FindNodeTypeUVE("flow.branch");
    ASSERT_NE(branch, nullptr);
    ASSERT_EQ(branch->pins.size(), 4U);
    EXPECT_EQ(branch->pins[0].role, ScriptPinRoleUVE::Execution);
    EXPECT_EQ(branch->pins[1].type, ScriptValueTypeUVE::Boolean);
    EXPECT_EQ(branch->pins[2].role, ScriptPinRoleUVE::Execution);
    EXPECT_EQ(branch->pins[3].role, ScriptPinRoleUVE::Execution);

    const ScriptNodeTypeDescriptorUVE* hasComponent = registry.FindNodeTypeUVE("query.entity.has_component");
    ASSERT_NE(hasComponent, nullptr);
    ASSERT_EQ(hasComponent->pins.size(), 3U);
    EXPECT_EQ(hasComponent->pins[0].type, ScriptValueTypeUVE::Entity);
    EXPECT_EQ(hasComponent->pins[1].type, ScriptValueTypeUVE::Component);
    EXPECT_EQ(hasComponent->pins[2].type, ScriptValueTypeUVE::Boolean);

    const ScriptNodeTypeDescriptorUVE* getComponent = registry.FindNodeTypeUVE("query.entity.get_component");
    ASSERT_NE(getComponent, nullptr);
    ASSERT_EQ(getComponent->pins.size(), 3U);
    EXPECT_EQ(getComponent->pins[0].type, ScriptValueTypeUVE::Entity);
    EXPECT_EQ(getComponent->pins[1].type, ScriptValueTypeUVE::Component);
    EXPECT_EQ(getComponent->pins[2].type, ScriptValueTypeUVE::Component);

    const ScriptNodeTypeDescriptorUVE* engineLog = registry.FindNodeTypeUVE("engine.log");
    ASSERT_NE(engineLog, nullptr);
    ASSERT_EQ(engineLog->pins.size(), 1U);
    EXPECT_EQ(engineLog->category, "Engine");
    EXPECT_EQ(engineLog->iconId, "node.engine");
    EXPECT_EQ(engineLog->pins[0].name, "Value");
    EXPECT_EQ(engineLog->pins[0].type, ScriptValueTypeUVE::Number);

    const ScriptNodeTypeDescriptorUVE* make = registry.FindNodeTypeUVE("math.vector3.make");
    ASSERT_NE(make, nullptr);
    ASSERT_EQ(make->pins.size(), 4U);
    EXPECT_EQ(make->pins[0].type, ScriptValueTypeUVE::Number);
    EXPECT_EQ(make->pins[3].direction, ScriptPinDirectionUVE::Output);
    EXPECT_EQ(make->pins[3].type, ScriptValueTypeUVE::Vector3);

    const ScriptNodeTypeDescriptorUVE* dot = registry.FindNodeTypeUVE("math.vector3.dot");
    ASSERT_NE(dot, nullptr);
    ASSERT_EQ(dot->pins.size(), 3U);
    EXPECT_EQ(dot->pins[0].type, ScriptValueTypeUVE::Vector3);
    EXPECT_EQ(dot->pins[2].type, ScriptValueTypeUVE::Number);

    const ScriptNodeTypeDescriptorUVE* multiply = registry.FindNodeTypeUVE("math.vector3.multiply");
    ASSERT_NE(multiply, nullptr);
    ASSERT_EQ(multiply->pins.size(), 3U);
    EXPECT_EQ(multiply->pins[0].type, ScriptValueTypeUVE::Vector3);
    EXPECT_EQ(multiply->pins[1].type, ScriptValueTypeUVE::Number);
    EXPECT_EQ(multiply->pins[2].type, ScriptValueTypeUVE::Vector3);
}

TEST(ScriptGraphUVETest, ValidateUVE_EnforcesExecutionLinkCardinality) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "flow.sequence"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "flow.sequence"}));
    ASSERT_TRUE(graph.AddNodeUVE({3U, "flow.branch"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{1U, "Then"}, {3U, "In"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{1U, "Then"}, {2U, "In"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{2U, "Then"}, {3U, "In"}}));

    const std::vector<ScriptValidationDiagnosticUVE> diagnostics = graph.ValidateUVE(registry);
    ASSERT_EQ(diagnostics.size(), 2U);
    EXPECT_EQ(diagnostics[0].code, ScriptValidationCodeUVE::ExecutionLinkCardinality);
    EXPECT_EQ(diagnostics[0].nodeId, 1U);
    EXPECT_EQ(diagnostics[1].code, ScriptValidationCodeUVE::ExecutionLinkCardinality);
    EXPECT_EQ(diagnostics[1].nodeId, 3U);
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_PreservesEngineLogBindingNode) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({70U, "engine.log"}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.program->instructions.size(), 1U);
    EXPECT_EQ(result.program->instructions.front().kind, ScriptIrInstructionKindUVE::ExecuteNode);
    EXPECT_EQ(result.program->instructions.front().sourceNodeId, 70U);
    EXPECT_EQ(result.program->instructions.front().nodeTypeId, "engine.log");
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_LowersFlowSequenceDirectDispatch) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "flow.sequence"}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.program->instructions.size(), 1U);
    EXPECT_EQ(result.program->instructions.front().kind, ScriptIrInstructionKindUVE::SequenceDispatch);
    EXPECT_EQ(result.program->instructions.front().firstTargetInstructionIndex, 1U);
    EXPECT_EQ(result.program->instructions.front().secondTargetInstructionIndex, 1U);
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_LowersDirectSequenceExecutionLinks) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(MakeSinkNodeUVE()));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "flow.sequence"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "test.sink"}));
    ASSERT_TRUE(graph.AddNodeUVE({3U, "test.sink"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{1U, "Then"}, {2U, "Exec"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{1U, "Then2"}, {3U, "Exec"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.program->instructions.size(), 3U);
    EXPECT_EQ(result.program->instructions[0].kind, ScriptIrInstructionKindUVE::SequenceDispatch);
    EXPECT_EQ(result.program->instructions[0].firstTargetInstructionIndex, 1U);
    EXPECT_EQ(result.program->instructions[0].secondTargetInstructionIndex, 2U);
    EXPECT_EQ(result.program->instructions[1].kind, ScriptIrInstructionKindUVE::ExecuteNode);
    EXPECT_EQ(result.program->instructions[2].kind, ScriptIrInstructionKindUVE::ExecuteNode);
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_LowersFlowBranchToConditionalJump) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(MakeSinkNodeUVE()));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "flow.branch"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "test.sink"}));
    ASSERT_TRUE(graph.AddNodeUVE({3U, "test.sink"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{1U, "True"}, {2U, "Exec"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{1U, "False"}, {3U, "Exec"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.program->instructions.size(), 3U);
    const ScriptIrInstructionUVE& branch = result.program->instructions.front();
    EXPECT_EQ(branch.kind, ScriptIrInstructionKindUVE::ConditionalJump);
    EXPECT_EQ(branch.sourceNodeId, 1U);
    EXPECT_EQ(branch.nodeTypeId, "flow.branch");
    EXPECT_EQ(branch.sourcePinName, "Condition");
    EXPECT_TRUE(branch.targetPinName.empty());
    EXPECT_EQ(branch.trueTargetInstructionIndex, 1U);
    EXPECT_EQ(branch.falseTargetInstructionIndex, 2U);
    EXPECT_EQ(result.program->instructions[1].kind, ScriptIrInstructionKindUVE::ExecuteNode);
    EXPECT_EQ(result.program->instructions[2].kind, ScriptIrInstructionKindUVE::ExecuteNode);
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_StagesBooleanConditionBeforeFlowBranch) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(MakeSinkNodeUVE()));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "flow.branch"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "logic.boolean.not"}));
    ASSERT_TRUE(graph.AddNodeUVE({3U, "test.sink"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{2U, "Result"}, {1U, "Condition"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{1U, "True"}, {3U, "Exec"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.program->instructions.size(), 4U);
    EXPECT_EQ(result.program->instructions[0].kind, ScriptIrInstructionKindUVE::ExecuteNode);
    EXPECT_EQ(result.program->instructions[0].sourceNodeId, 2U);
    EXPECT_EQ(result.program->instructions[1].kind, ScriptIrInstructionKindUVE::TransferValue);
    EXPECT_EQ(result.program->instructions[1].sourceNodeId, 2U);
    EXPECT_EQ(result.program->instructions[1].targetNodeId, 1U);
    EXPECT_EQ(result.program->instructions[1].sourcePinName, "Result");
    EXPECT_EQ(result.program->instructions[1].targetPinName, "Condition");
    EXPECT_EQ(result.program->instructions[2].kind, ScriptIrInstructionKindUVE::ConditionalJump);
    EXPECT_EQ(result.program->instructions[2].sourceNodeId, 1U);
    EXPECT_EQ(result.program->instructions[2].trueTargetInstructionIndex, 3U);
    EXPECT_EQ(result.program->instructions[2].falseTargetInstructionIndex, 4U);
    EXPECT_EQ(result.program->instructions[3].kind, ScriptIrInstructionKindUVE::ExecuteNode);
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_RejectsFlowBranchTooDeepConditionDependency) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "flow.branch"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "logic.boolean.not"}));
    ASSERT_TRUE(graph.AddNodeUVE({3U, "logic.boolean.not"}));
    ASSERT_TRUE(graph.AddNodeUVE({4U, "logic.boolean.not"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{4U, "Result"}, {3U, "Value"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{3U, "Result"}, {2U, "Value"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{2U, "Result"}, {1U, "Condition"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    EXPECT_FALSE(result.IsSuccessUVE());
    ASSERT_EQ(result.diagnostics.size(), 1U);
    EXPECT_EQ(result.diagnostics.front().code, ScriptValidationCodeUVE::UnsupportedRuntimeNode);
    EXPECT_EQ(result.diagnostics.front().nodeId, 3U);
    EXPECT_EQ(result.diagnostics.front().pinName, "Value");
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_RejectsFlowBranchConditionCycle) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "flow.branch"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "logic.boolean.not"}));
    ASSERT_TRUE(graph.AddNodeUVE({3U, "logic.boolean.not"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{3U, "Result"}, {2U, "Value"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{2U, "Result"}, {3U, "Value"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{2U, "Result"}, {1U, "Condition"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    EXPECT_FALSE(result.IsSuccessUVE());
    ASSERT_EQ(result.diagnostics.size(), 1U);
    EXPECT_EQ(result.diagnostics.front().code, ScriptValidationCodeUVE::UnsupportedRuntimeNode);
    EXPECT_EQ(result.diagnostics.front().nodeId, 3U);
    EXPECT_EQ(result.diagnostics.front().pinName, "Value");
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_RejectsNonBuiltinConditionProducer) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(MakeBooleanSourceNodeUVE()));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "flow.branch"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "test.boolean-source"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{2U, "Out"}, {1U, "Condition"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    EXPECT_FALSE(result.IsSuccessUVE());
    ASSERT_EQ(result.diagnostics.size(), 1U);
    EXPECT_EQ(result.diagnostics.front().code, ScriptValidationCodeUVE::UnsupportedRuntimeNode);
    EXPECT_EQ(result.diagnostics.front().nodeId, 1U);
    EXPECT_EQ(result.diagnostics.front().pinName, "Condition");
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_RunsCompiledStagedBooleanConditionDependency) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(MakeSinkNodeUVE()));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "flow.branch"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "logic.boolean.not"}));
    ASSERT_TRUE(graph.AddNodeUVE({3U, "logic.boolean.not"}));
    ASSERT_TRUE(graph.AddNodeUVE({4U, "test.sink"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{3U, "Result"}, {2U, "Value"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{2U, "Result"}, {1U, "Condition"}}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{1U, "True"}, {4U, "Exec"}}));

    const ScriptIrCompileResultUVE compiled = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(compiled.IsSuccessUVE());
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    const std::optional<ScriptBytecodeProgramUVE> bytecode =
        LowerIrToBytecodeUVE(*compiled.program, diagnostics);
    ASSERT_TRUE(bytecode.has_value());
    ASSERT_TRUE(diagnostics.empty());
    ASSERT_EQ(bytecode->instructions.size(), 6U);

    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(3U, "Value", true));
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(*bytecode, context);
    EXPECT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 6U);
    EXPECT_FALSE(std::get<bool>(*context.FindOutputUVE(3U, "Result")));
    EXPECT_TRUE(std::get<bool>(*context.FindOutputUVE(2U, "Result")));
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_EngineLogUsesCallerOwnedBinding) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 70U, 0U, "engine.log", {}, {}});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(70U, "Value", 42.5F));
    EngineLogCaptureUVE capture;
    const ScriptEngineCallBindingsUVE bindings{CaptureEngineLogUVE, &capture};
    ScriptVmExecutionOptionsUVE options;
    options.engineCallBindings = &bindings;

    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context, options);
    EXPECT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(capture.callCount, 1U);
    EXPECT_FLOAT_EQ(capture.lastValue, 42.5F);
    ASSERT_EQ(result.trace.size(), 2U);
    EXPECT_EQ(result.trace.front().nodeTypeId, "engine.log");
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_EngineLogRejectsUnboundOrRejectedCall) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 71U, 0U, "engine.log", {}, {}});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(71U, "Value", 1.0F));

    const ScriptVmExecutionResultUVE unbound = ExecuteScriptBytecodeUVE(program, context);
    EXPECT_EQ(unbound.status, ScriptVmStatusUVE::NodeExecutionFailed);
    ASSERT_EQ(unbound.diagnostics.size(), 1U);

    EngineLogCaptureUVE capture;
    capture.accept = false;
    const ScriptEngineCallBindingsUVE bindings{CaptureEngineLogUVE, &capture};
    ScriptVmExecutionOptionsUVE options;
    options.engineCallBindings = &bindings;
    const ScriptVmExecutionResultUVE rejected = ExecuteScriptBytecodeUVE(program, context, options);
    EXPECT_EQ(rejected.status, ScriptVmStatusUVE::NodeExecutionFailed);
    EXPECT_EQ(capture.callCount, 1U);
}

TEST(ScriptNodeRegistryUVETest, FindNodeTypeUVE_ReturnsCopiedStableDescriptorView) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(MakeSourceNodeUVE()));
    const ScriptNodeTypeDescriptorUVE* descriptor = registry.FindNodeTypeUVE("test.source");
    ASSERT_NE(descriptor, nullptr);
    EXPECT_EQ(descriptor->displayName, "Test Source");
    EXPECT_EQ(descriptor->pins.size(), 2U);
    EXPECT_EQ(registry.FindNodeTypeUVE("missing"), nullptr);
}

TEST(ScriptNodeRegistryUVETest, DescriptorV2_PreservesPresentationMetadataAndOrdersDescriptorsDeterministically) {
    ScriptNodeRegistryUVE registry;
    ScriptNodeTypeDescriptorUVE late{
        "test.late", "Late", {{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number,
                                  ScriptPinRoleUVE::Data, std::string("0.016")}},
        "FLOW", "node.branch", 20U, kScriptNodePresentationFlagCollapsibleUVE};
    ScriptNodeTypeDescriptorUVE early{
        "test.early", "Early", {{"Exec", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Execution}},
        "EVENT", "node.event", 10U, kScriptNodePresentationFlagCompactUVE};
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(late));
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(early));

    const ScriptNodeTypeDescriptorUVE* found = registry.FindNodeTypeUVE("test.late");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->category, "FLOW");
    EXPECT_EQ(found->iconId, "node.branch");
    EXPECT_EQ(found->displayOrder, 20U);
    EXPECT_EQ(found->presentationFlags, kScriptNodePresentationFlagCollapsibleUVE);
    ASSERT_EQ(found->pins.size(), 1U);
    EXPECT_EQ(found->pins[0].defaultValue, std::optional<std::string>("0.016"));

    const std::vector<ScriptNodeTypeDescriptorUVE> ordered = registry.GetNodeTypeDescriptorsUVE();
    ASSERT_EQ(ordered.size(), 2U);
    EXPECT_EQ(ordered[0].typeId, "test.early");
    EXPECT_EQ(ordered[1].typeId, "test.late");
    EXPECT_EQ(ordered[0].pins[0].role, ScriptPinRoleUVE::Execution);
}

TEST(ScriptNodeRegistryUVETest, DescriptorV2_RejectsExecutionDefaultValues) {
    ScriptNodeRegistryUVE registry;
    EXPECT_FALSE(registry.RegisterNodeTypeUVE({
        "test.invalid-exec-default", "Invalid", {{"Exec", ScriptPinDirectionUVE::Output,
                                                     ScriptValueTypeUVE::Execution, ScriptPinRoleUVE::Execution,
                                                     std::string("true")}}}));
}

TEST(ScriptGraphUVETest, AddNodeUVE_RejectsEmptyTypeAndDuplicateIdsWithoutMutation) {
    ScriptGraphUVE graph;
    EXPECT_FALSE(graph.AddNodeUVE({1U, ""}));
    EXPECT_TRUE(graph.AddNodeUVE({1U, "test.source"}));
    EXPECT_FALSE(graph.AddNodeUVE({1U, "test.sink"}));
    EXPECT_EQ(graph.GetNodesUVE().size(), 1U);
}

TEST(ScriptGraphUVETest, AddLinkUVE_RejectsEmptyEndpointsAndDuplicateLinks) {
    ScriptGraphUVE graph;
    EXPECT_FALSE(graph.AddLinkUVE({{0U, "Out"}, {2U, "In"}}));
    EXPECT_FALSE(graph.AddLinkUVE({{1U, ""}, {2U, "In"}}));
    const ScriptLinkUVE link{{1U, "Out"}, {2U, "In"}};
    EXPECT_TRUE(graph.AddLinkUVE(link));
    EXPECT_FALSE(graph.AddLinkUVE(link));
    EXPECT_EQ(graph.GetLinksUVE().size(), 1U);
}

TEST(ScriptGraphUVETest, ValidateUVE_ValidTypedOutputToInputHasNoDiagnostics) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "test.source"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "test.sink"}));
    ASSERT_TRUE(graph.AddLinkUVE({{1U, "Out"}, {2U, "In"}}));
    EXPECT_TRUE(graph.ValidateUVE(registry).empty());
}

TEST(ScriptGraphUVETest, ValidateUVE_ReportsUnknownNodeTypeAndUnknownPins) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "missing.node"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "test.sink"}));
    ASSERT_TRUE(graph.AddLinkUVE({{1U, "Out"}, {2U, "Missing"}}));
    const auto diagnostics = graph.ValidateUVE(registry);
    ASSERT_EQ(diagnostics.size(), 1U);
    EXPECT_EQ(diagnostics[0].code, ScriptValidationCodeUVE::UnknownNodeType);
}

TEST(ScriptGraphUVETest, ValidateUVE_ReportsWrongDirectionsAndIncompatibleTypes) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ASSERT_TRUE(registry.RegisterNodeTypeUVE({"test.boolean-sink", "Boolean Sink",
                                              {{"In", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Boolean}}}));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "test.source"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "test.boolean-sink"}));
    ASSERT_TRUE(graph.AddNodeUVE({3U, "test.boolean-sink"}));
    ASSERT_TRUE(graph.AddLinkUVE({{2U, "In"}, {3U, "In"}}));
    ASSERT_TRUE(graph.AddLinkUVE({{1U, "Out"}, {2U, "In"}}));
    const auto diagnostics = graph.ValidateUVE(registry);
    ASSERT_EQ(diagnostics.size(), 2U);
    EXPECT_EQ(diagnostics[0].code, ScriptValidationCodeUVE::WrongPinDirection);
    EXPECT_EQ(diagnostics[0].sourceContext, "Node #2 / pin In");
    ASSERT_TRUE(diagnostics[0].relatedEndpoint.has_value());
    EXPECT_EQ(*diagnostics[0].relatedEndpoint, (ScriptPinEndpointUVE{3U, "In"}));
    EXPECT_EQ(diagnostics[1].code, ScriptValidationCodeUVE::IncompatiblePinTypes);
    EXPECT_EQ(diagnostics[1].sourceContext, "Node #2 / pin In");
    ASSERT_TRUE(diagnostics[1].relatedEndpoint.has_value());
    EXPECT_EQ(*diagnostics[1].relatedEndpoint, (ScriptPinEndpointUVE{1U, "Out"}));
}

TEST(ScriptGraphUVETest, ValidateUVE_ReportsSelfLinkAndMissingNodeDeterministically) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "test.source"}));
    ASSERT_TRUE(graph.AddLinkUVE({{1U, "Out"}, {1U, "Exec"}}));
    ASSERT_TRUE(graph.AddLinkUVE({{1U, "Out"}, {9U, "In"}}));
    const auto diagnostics = graph.ValidateUVE(registry);
    ASSERT_EQ(diagnostics.size(), 3U);
    EXPECT_EQ(diagnostics[0].code, ScriptValidationCodeUVE::SelfLink);
    ASSERT_TRUE(diagnostics[0].relatedEndpoint.has_value());
    EXPECT_EQ(*diagnostics[0].relatedEndpoint, (ScriptPinEndpointUVE{1U, "Exec"}));
    EXPECT_EQ(diagnostics[1].code, ScriptValidationCodeUVE::WrongPinDirection);
    ASSERT_TRUE(diagnostics[1].relatedEndpoint.has_value());
    EXPECT_EQ(*diagnostics[1].relatedEndpoint, (ScriptPinEndpointUVE{1U, "Out"}));
    EXPECT_EQ(diagnostics[2].code, ScriptValidationCodeUVE::EmptyLinkEndpoint);
    ASSERT_TRUE(diagnostics[2].relatedEndpoint.has_value());
    EXPECT_EQ(*diagnostics[2].relatedEndpoint, (ScriptPinEndpointUVE{1U, "Out"}));
}

TEST(ScriptPinCompatibilityUVETest, AreScriptPinTypesCompatibleUVE_RequiresExactTypes) {
    EXPECT_TRUE(AreScriptPinTypesCompatibleUVE(ScriptValueTypeUVE::Execution, ScriptValueTypeUVE::Execution));
    EXPECT_TRUE(AreScriptPinTypesCompatibleUVE(ScriptValueTypeUVE::Vector3, ScriptValueTypeUVE::Vector3));
    EXPECT_FALSE(AreScriptPinTypesCompatibleUVE(ScriptValueTypeUVE::Number, ScriptValueTypeUVE::Boolean));
    EXPECT_FALSE(AreScriptPinTypesCompatibleUVE(ScriptValueTypeUVE::Entity, ScriptValueTypeUVE::Asset));
}

} // namespace UVE::Scripting

namespace UVE::Scripting {

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_SortsNodesAndLinksDeterministically) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({20U, "test.sink"}));
    ASSERT_TRUE(graph.AddNodeUVE({10U, "test.source"}));
    ASSERT_TRUE(graph.AddLinkUVE({{10U, "Out"}, {20U, "In"}}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_TRUE(result.program.has_value());
    EXPECT_EQ(result.program->version, ScriptIrProgramUVE::kCurrentVersionUVE);
    ASSERT_EQ(result.program->instructions.size(), 3U);
    EXPECT_EQ(result.program->instructions[0].kind, ScriptIrInstructionKindUVE::ExecuteNode);
    EXPECT_EQ(result.program->instructions[0].sourceNodeId, 10U);
    EXPECT_EQ(result.program->instructions[1].sourceNodeId, 20U);
    EXPECT_EQ(result.program->instructions[2].kind, ScriptIrInstructionKindUVE::TransferValue);
    EXPECT_EQ(result.program->instructions[2].sourceNodeId, 10U);
    EXPECT_EQ(result.program->instructions[2].targetNodeId, 20U);
    EXPECT_EQ(result.program->sourceNodeIds, (std::vector<std::uint32_t>{10U, 20U, 10U}));
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_BoundsDiagnosticPresentationFields) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphUVE graph;
    const std::string longTypeId(900U, 'x');
    ASSERT_TRUE(graph.AddNodeUVE({1U, longTypeId}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_FALSE(result.IsSuccessUVE());
    ASSERT_EQ(result.diagnostics.size(), 1U);
    EXPECT_EQ(result.diagnostics[0].severity, ScriptDiagnosticSeverityUVE::Error);
    EXPECT_LE(result.diagnostics[0].message.size(), kMaximumScriptDiagnosticMessageBytesUVE);
    EXPECT_LE(result.diagnostics[0].sourceContext.size(), kMaximumScriptDiagnosticSourceContextBytesUVE);
}

TEST(ScriptCompilerIRUVETest, CompileScriptGraphToIrUVE_RejectsInvalidGraphWithoutPartialProgram) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "missing.node"}));

    const ScriptIrCompileResultUVE result = CompileScriptGraphToIrUVE(graph, registry);
    EXPECT_FALSE(result.IsSuccessUVE());
    EXPECT_FALSE(result.program.has_value());
    ASSERT_EQ(result.diagnostics.size(), 1U);
    EXPECT_EQ(result.diagnostics[0].code, ScriptValidationCodeUVE::UnknownNodeType);
    EXPECT_EQ(result.diagnostics[0].severity, ScriptDiagnosticSeverityUVE::Error);
    EXPECT_EQ(result.diagnostics[0].sourceContext, "Node #1");
    EXPECT_EQ(result.diagnostics[0].message, "Node type is not registered: missing.node");
}

} // namespace UVE::Scripting


namespace UVE::Scripting {

TEST(ScriptGraphRuntimeBindingUVETest, BindUVE_CompilesLowersAndAttachesValidatedGraph) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({10U, "test.source"}));
    ASSERT_TRUE(graph.AddNodeUVE({20U, "test.sink"}));
    ASSERT_TRUE(graph.AddLinkUVE({{10U, "Out"}, {20U, "In"}}));
    ScriptRuntimeUVE runtime;

    const ScriptGraphRuntimeBindingResultUVE result =
        ScriptGraphRuntimeBindingUVE::BindUVE(graph, registry, runtime, {7U, 3U});

    ASSERT_TRUE(result.IsAcceptedUVE());
    EXPECT_EQ(result.runtimeCode, ScriptRuntimeAttachCodeUVE::Accepted);
    EXPECT_TRUE(result.compileDiagnostics.empty());
    EXPECT_TRUE(result.bytecodeDiagnostics.empty());
    EXPECT_TRUE(runtime.HasInstanceUVE({7U, 3U}));
    const auto snapshots = runtime.GetSnapshotUVE();
    ASSERT_EQ(snapshots.size(), 1U);
    EXPECT_EQ(snapshots[0].instructionCount, 3U);
}

TEST(ScriptGraphRuntimeBindingUVETest, BindUVE_RejectsBeforeRuntimeMutationWhenGraphCompileFails) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "missing.node"}));
    ScriptRuntimeUVE runtime;

    const ScriptGraphRuntimeBindingResultUVE result =
        ScriptGraphRuntimeBindingUVE::BindUVE(graph, registry, runtime, {8U, 1U});

    EXPECT_EQ(result.code, ScriptGraphRuntimeBindingCodeUVE::CompileRejected);
    ASSERT_EQ(result.compileDiagnostics.size(), 1U);
    EXPECT_EQ(result.compileDiagnostics[0].code, ScriptValidationCodeUVE::UnknownNodeType);
    EXPECT_EQ(runtime.GetInstanceCountUVE(), 0U);
}

TEST(ScriptGraphRuntimeBindingUVETest, BindUVE_RejectsDuplicateEntityWithoutOverwritingRuntimeState) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "test.source"}));
    ScriptRuntimeUVE runtime;
    ASSERT_TRUE(runtime.AttachUVE({9U, 2U}, ScriptBytecodeProgramUVE{}));

    const ScriptGraphRuntimeBindingResultUVE result =
        ScriptGraphRuntimeBindingUVE::BindUVE(graph, registry, runtime, {9U, 2U});

    EXPECT_EQ(result.code, ScriptGraphRuntimeBindingCodeUVE::RuntimeRejected);
    EXPECT_EQ(result.runtimeCode, ScriptRuntimeAttachCodeUVE::DuplicateInstance);
    EXPECT_EQ(runtime.GetInstanceCountUVE(), 1U);
}

TEST(ScriptGraphRuntimeBindingUVETest, BindUVE_RejectsInvalidEntityWithoutCompilationOrRuntimeMutation) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "test.source"}));
    ScriptRuntimeUVE runtime;

    const ScriptGraphRuntimeBindingResultUVE result =
        ScriptGraphRuntimeBindingUVE::BindUVE(graph, registry, runtime, Scene::kInvalidEntityUVE);

    EXPECT_EQ(result.code, ScriptGraphRuntimeBindingCodeUVE::InvalidEntity);
    EXPECT_TRUE(result.compileDiagnostics.empty());
    EXPECT_EQ(runtime.GetInstanceCountUVE(), 0U);
}

TEST(ScriptComponentRuntimeOwnershipUVETest, ReconcileUVE_AttachesValidatedPathThroughGraphBinding) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "test.source"}));
    ScriptRuntimeUVE runtime;

    const ScriptComponentRuntimeOwnershipResultUVE result =
        ScriptComponentRuntimeOwnershipUVE::ReconcileUVE(
            Scene::ScriptComponentUVE{"scripts/player.uvescript"}, graph, registry, runtime, {8U, 1U});

    EXPECT_EQ(result.code, ScriptComponentRuntimeOwnershipCodeUVE::Attached);
    EXPECT_TRUE(result.IsAcceptedUVE());
    EXPECT_EQ(runtime.GetInstanceCountUVE(), 1U);
}

TEST(ScriptComponentRuntimeOwnershipUVETest, ReconcileUVE_EmptyPathDetachesIdempotently) {
    ScriptRuntimeUVE runtime;
    ASSERT_TRUE(runtime.AttachUVE({9U, 1U}, ScriptBytecodeProgramUVE{}));
    ScriptNodeRegistryUVE registry;
    ScriptGraphUVE graph;

    const ScriptComponentRuntimeOwnershipResultUVE detached =
        ScriptComponentRuntimeOwnershipUVE::ReconcileUVE(
            Scene::ScriptComponentUVE{}, graph, registry, runtime, {9U, 1U});
    EXPECT_EQ(detached.code, ScriptComponentRuntimeOwnershipCodeUVE::Detached);
    EXPECT_EQ(runtime.GetInstanceCountUVE(), 0U);

    const ScriptComponentRuntimeOwnershipResultUVE alreadyDetached =
        ScriptComponentRuntimeOwnershipUVE::ReconcileUVE(
            Scene::ScriptComponentUVE{}, graph, registry, runtime, {9U, 1U});
    EXPECT_EQ(alreadyDetached.code, ScriptComponentRuntimeOwnershipCodeUVE::Detached);
    EXPECT_EQ(runtime.GetInstanceCountUVE(), 0U);
}

TEST(ScriptComponentRuntimeOwnershipUVETest, ReconcileUVE_RejectsInvalidPathAndGraphWithoutRuntimeMutation) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphUVE invalidGraph;
    ASSERT_TRUE(invalidGraph.AddNodeUVE({1U, "unknown.node"}));
    ScriptRuntimeUVE runtime;
    const Scene::EntityUVE entity{10U, 1U};

    const ScriptComponentRuntimeOwnershipResultUVE invalidPath =
        ScriptComponentRuntimeOwnershipUVE::ReconcileUVE(
            Scene::ScriptComponentUVE{"../outside.uvescript"}, invalidGraph, registry, runtime, entity);
    EXPECT_EQ(invalidPath.code, ScriptComponentRuntimeOwnershipCodeUVE::InvalidComponent);
    EXPECT_EQ(runtime.GetInstanceCountUVE(), 0U);

    const ScriptComponentRuntimeOwnershipResultUVE rejectedGraph =
        ScriptComponentRuntimeOwnershipUVE::ReconcileUVE(
            Scene::ScriptComponentUVE{"scripts/player.uvescript"}, invalidGraph, registry, runtime, entity);
    EXPECT_EQ(rejectedGraph.code, ScriptComponentRuntimeOwnershipCodeUVE::GraphRejected);
    EXPECT_EQ(runtime.GetInstanceCountUVE(), 0U);
}

TEST(ScriptComponentRuntimeOwnershipUVETest, ReconcileUVE_RejectsReplacementWhileRuntimeIsActive) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "test.source"}));
    ScriptRuntimeUVE runtime;
    const Scene::EntityUVE entity{11U, 1U};
    ASSERT_TRUE(runtime.AttachUVE(entity, ScriptBytecodeProgramUVE{}));

    const ScriptComponentRuntimeOwnershipResultUVE result =
        ScriptComponentRuntimeOwnershipUVE::ReconcileUVE(
            Scene::ScriptComponentUVE{"scripts/replacement.uvescript"}, graph, registry, runtime, entity);

    EXPECT_EQ(result.code, ScriptComponentRuntimeOwnershipCodeUVE::DuplicateRuntime);
    EXPECT_EQ(runtime.GetInstanceCountUVE(), 1U);
}

TEST(ScriptBytecodeUVETest, EncodeDecodeScriptBytecodeUVE_RoundTripsVersionedProgram) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 4U, 0U, "test.source", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::TransferValue, 4U, 9U, {}, "Out", "In"});
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    const std::vector<std::uint8_t> bytes = EncodeScriptBytecodeUVE(program, diagnostics);
    ASSERT_TRUE(diagnostics.empty());
    const ScriptBytecodeDecodeResultUVE decoded = DecodeScriptBytecodeUVE(bytes);
    ASSERT_TRUE(decoded.IsSuccessUVE());
    ASSERT_EQ(decoded.program->instructions.size(), 2U);
    EXPECT_EQ(decoded.program->version, ScriptBytecodeProgramUVE::kCurrentVersionUVE);
    EXPECT_EQ(decoded.program->instructions[0].nodeTypeId, "test.source");
    EXPECT_EQ(decoded.program->instructions[1].sourcePinName, "Out");
}

TEST(ScriptBytecodeUVETest, LegacyV1DataOnlyBytecode_DecodesAndExecutes) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 4U, 0U, "test.source", {}, {}});
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    std::vector<std::uint8_t> bytes = EncodeScriptBytecodeUVE(program, diagnostics);
    ASSERT_TRUE(diagnostics.empty());
    bytes[4U] = static_cast<std::uint8_t>(ScriptBytecodeProgramUVE::kLegacyVersionUVE);
    bytes[5U] = 0U;
    bytes[6U] = 0U;
    bytes[7U] = 0U;

    const ScriptBytecodeDecodeResultUVE decoded = DecodeScriptBytecodeUVE(bytes);
    ASSERT_TRUE(decoded.IsSuccessUVE());
    EXPECT_EQ(decoded.program->version, ScriptBytecodeProgramUVE::kLegacyVersionUVE);
    const ScriptVmExecutionResultUVE execution = ExecuteScriptBytecodeUVE(*decoded.program);
    EXPECT_TRUE(execution.IsSuccessUVE());
    EXPECT_EQ(execution.instructionsExecuted, 1U);
}

TEST(ScriptBytecodeUVETest, ConditionalJumpV2_RoundTripsTargetsAndRejectsLegacyEncoding) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ConditionalJump, 7U, 0U, "flow.branch",
                                    "Condition", {}, 1U, 0U});
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    const std::vector<std::uint8_t> bytes = EncodeScriptBytecodeUVE(program, diagnostics);
    ASSERT_TRUE(diagnostics.empty());
    const ScriptBytecodeDecodeResultUVE decoded = DecodeScriptBytecodeUVE(bytes);
    ASSERT_TRUE(decoded.IsSuccessUVE());
    ASSERT_EQ(decoded.program->instructions.size(), 1U);
    EXPECT_EQ(decoded.program->instructions.front().kind, ScriptIrInstructionKindUVE::ConditionalJump);
    EXPECT_EQ(decoded.program->instructions.front().trueTargetInstructionIndex, 1U);
    EXPECT_EQ(decoded.program->instructions.front().falseTargetInstructionIndex, 0U);

    std::vector<std::uint8_t> legacyBytes = bytes;
    legacyBytes[4U] = static_cast<std::uint8_t>(ScriptBytecodeProgramUVE::kLegacyVersionUVE);
    legacyBytes[5U] = 0U;
    legacyBytes[6U] = 0U;
    legacyBytes[7U] = 0U;
    const ScriptBytecodeDecodeResultUVE legacy = DecodeScriptBytecodeUVE(legacyBytes);
    EXPECT_FALSE(legacy.IsSuccessUVE());
    ASSERT_EQ(legacy.diagnostics.size(), 1U);
    EXPECT_EQ(legacy.diagnostics.front().code, ScriptBytecodeDiagnosticCodeUVE::InvalidInstruction);
}

TEST(ScriptBytecodeUVETest, SequenceDispatchV3_RoundTripsOrderedTargets) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::SequenceDispatch, 7U, 0U, "flow.sequence",
                                    "Then", "Then2", 0U, 0U, 1U, 0U});
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    const std::vector<std::uint8_t> bytes = EncodeScriptBytecodeUVE(program, diagnostics);
    ASSERT_TRUE(diagnostics.empty());
    const ScriptBytecodeDecodeResultUVE decoded = DecodeScriptBytecodeUVE(bytes);
    ASSERT_TRUE(decoded.IsSuccessUVE());
    ASSERT_EQ(decoded.program->instructions.size(), 1U);
    EXPECT_EQ(decoded.program->version, ScriptBytecodeProgramUVE::kCurrentVersionUVE);
    EXPECT_EQ(decoded.program->instructions.front().kind, ScriptIrInstructionKindUVE::SequenceDispatch);
    EXPECT_EQ(decoded.program->instructions.front().firstTargetInstructionIndex, 1U);
    EXPECT_EQ(decoded.program->instructions.front().secondTargetInstructionIndex, 0U);
}

TEST(ScriptBytecodeUVETest, EncodeScriptBytecodeUVE_RejectsOutOfRangeConditionalJumpTarget) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ConditionalJump, 1U, 0U, "flow.branch",
                                    "Condition", {}, 2U, 0U});
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    EXPECT_TRUE(EncodeScriptBytecodeUVE(program, diagnostics).empty());
    ASSERT_EQ(diagnostics.size(), 1U);
    EXPECT_EQ(diagnostics.front().code, ScriptBytecodeDiagnosticCodeUVE::InvalidInstruction);
}

TEST(ScriptBytecodeUVETest, DecodeScriptBytecodeUVE_RejectsCorruptHeadersAndTruncation) {
    const ScriptBytecodeDecodeResultUVE badMagic = DecodeScriptBytecodeUVE({0U, 1U, 2U, 3U});
    ASSERT_FALSE(badMagic.IsSuccessUVE());
    ASSERT_EQ(badMagic.diagnostics.size(), 1U);
    EXPECT_EQ(badMagic.diagnostics[0].code, ScriptBytecodeDiagnosticCodeUVE::InvalidMagic);
    const ScriptBytecodeDecodeResultUVE truncated = DecodeScriptBytecodeUVE({'U', 'V', 'E', 'S', 1U, 0U, 0U, 0U});
    ASSERT_FALSE(truncated.IsSuccessUVE());
    EXPECT_EQ(truncated.diagnostics[0].code, ScriptBytecodeDiagnosticCodeUVE::Truncated);
}

TEST(ScriptBytecodeUVETest, EncodeScriptBytecodeUVE_RejectsInstructionLimit) {
    ScriptBytecodeProgramUVE program;
    program.instructions.resize(ScriptBytecodeProgramUVE::kMaximumInstructionsUVE + 1U);
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    EXPECT_TRUE(EncodeScriptBytecodeUVE(program, diagnostics).empty());
    ASSERT_EQ(diagnostics.size(), 1U);
    EXPECT_EQ(diagnostics[0].code, ScriptBytecodeDiagnosticCodeUVE::InstructionLimitExceeded);
}

} // namespace UVE::Scripting


namespace UVE::Scripting {

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_CompletesValidProgramWithinBudget) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 1U, 0U, "test.source", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::TransferValue, 1U, 2U, {}, "Out", "In"});
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, {2U});
    EXPECT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 2U);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_ConditionalJumpSelectsTrueAndFalseTargets) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ConditionalJump, 1U, 0U, "flow.branch",
                                    "Condition", {}, 1U, 2U});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 20U, 0U, "math.float.add", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 30U, 0U, "math.float.subtract", {}, {}});

    ScriptVmExecutionContextUVE trueContext;
    ASSERT_TRUE(trueContext.SetInputUVE(1U, "Condition", true));
    ASSERT_TRUE(trueContext.SetInputUVE(20U, "A", 5.0F));
    ASSERT_TRUE(trueContext.SetInputUVE(20U, "B", 2.0F));
    ASSERT_TRUE(trueContext.SetInputUVE(30U, "A", 5.0F));
    ASSERT_TRUE(trueContext.SetInputUVE(30U, "B", 2.0F));
    const ScriptVmExecutionResultUVE trueResult = ExecuteScriptBytecodeUVE(program, trueContext);
    ASSERT_TRUE(trueResult.IsSuccessUVE());
    ASSERT_EQ(trueResult.trace.size(), 4U);
    EXPECT_EQ(trueResult.trace[0].message, "ConditionalJump evaluated true.");
    EXPECT_EQ(trueResult.trace[1].sourceNodeId, 20U);
    EXPECT_EQ(trueResult.trace[2].sourceNodeId, 30U);
    EXPECT_EQ(trueResult.trace[3].kind, ScriptVmTraceEventKindUVE::Completed);

    ScriptVmExecutionContextUVE falseContext = trueContext;
    ASSERT_TRUE(falseContext.SetInputUVE(1U, "Condition", false));
    const ScriptVmExecutionResultUVE falseResult = ExecuteScriptBytecodeUVE(program, falseContext);
    ASSERT_TRUE(falseResult.IsSuccessUVE());
    ASSERT_EQ(falseResult.trace.size(), 3U);
    EXPECT_EQ(falseResult.trace[0].message, "ConditionalJump evaluated false.");
    EXPECT_EQ(falseResult.trace[1].sourceNodeId, 30U);
    EXPECT_EQ(falseResult.trace[2].kind, ScriptVmTraceEventKindUVE::Completed);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_SequenceDispatchExecutesOrderedDirectTargets) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::SequenceDispatch, 1U, 0U, "flow.sequence",
                                    "Then", "Then2", 0U, 0U, 1U, 2U});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 20U, 0U, "math.float.add", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 30U, 0U, "math.float.subtract", {}, {}});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(20U, "A", 5.0F));
    ASSERT_TRUE(context.SetInputUVE(20U, "B", 2.0F));
    ASSERT_TRUE(context.SetInputUVE(30U, "A", 5.0F));
    ASSERT_TRUE(context.SetInputUVE(30U, "B", 2.0F));

    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context);
    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 3U);
    ASSERT_EQ(result.trace.size(), 4U);
    EXPECT_EQ(result.trace[0].message, "SequenceDispatch selected ordered execution targets.");
    EXPECT_EQ(result.trace[1].sourceNodeId, 20U);
    EXPECT_EQ(result.trace[2].sourceNodeId, 30U);
    EXPECT_EQ(result.trace[3].kind, ScriptVmTraceEventKindUVE::Completed);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_SequenceDispatchSkipsMissingFirstOutput) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::SequenceDispatch, 1U, 0U, "flow.sequence",
                                    "Then", "Then2", 0U, 0U, 2U, 1U});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 30U, 0U, "math.float.add", {}, {}});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(30U, "A", 5.0F));
    ASSERT_TRUE(context.SetInputUVE(30U, "B", 2.0F));

    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context);
    ASSERT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 2U);
    ASSERT_EQ(result.trace.size(), 3U);
    EXPECT_EQ(result.trace[1].sourceNodeId, 30U);
    EXPECT_EQ(result.trace[2].kind, ScriptVmTraceEventKindUVE::Completed);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_SequenceDispatchSelfLoopStopsAtBudget) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::SequenceDispatch, 1U, 0U, "flow.sequence",
                                    "Then", "Then2", 0U, 0U, 0U, 0U});
    ScriptVmExecutionContextUVE context;
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context, {2U});
    EXPECT_EQ(result.status, ScriptVmStatusUVE::InstructionBudgetExceeded);
    EXPECT_EQ(result.instructionsExecuted, 2U);
    ASSERT_EQ(result.trace.size(), 3U);
    EXPECT_EQ(result.trace.back().kind, ScriptVmTraceEventKindUVE::Failed);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_ConditionalJumpRequiresBooleanCondition) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ConditionalJump, 1U, 0U, "flow.branch",
                                    "Condition", {}, 1U, 1U});
    ScriptVmExecutionContextUVE context;
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context);
    EXPECT_EQ(result.status, ScriptVmStatusUVE::NodeExecutionFailed);
    ASSERT_EQ(result.trace.size(), 1U);
    EXPECT_EQ(result.trace.front().kind, ScriptVmTraceEventKindUVE::Failed);
    EXPECT_EQ(result.trace.front().message, "ConditionalJump requires a Boolean condition input.");
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_ConditionalJumpSelfLoopStopsAtBudget) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ConditionalJump, 1U, 0U, "flow.branch",
                                    "Condition", {}, 0U, 0U});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(1U, "Condition", true));
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context, {3U});
    EXPECT_EQ(result.status, ScriptVmStatusUVE::InstructionBudgetExceeded);
    EXPECT_EQ(result.instructionsExecuted, 3U);
    ASSERT_EQ(result.trace.size(), 4U);
    EXPECT_EQ(result.trace.back().kind, ScriptVmTraceEventKindUVE::Failed);
    EXPECT_TRUE(result.trace.back().message.find("Instruction budget") != std::string::npos);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_ConditionalJumpRejectsInvalidTarget) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ConditionalJump, 1U, 0U, "flow.branch",
                                    "Condition", {}, 2U, 0U});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(1U, "Condition", true));
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context);
    EXPECT_EQ(result.status, ScriptVmStatusUVE::NodeExecutionFailed);
    ASSERT_EQ(result.trace.size(), 1U);
    EXPECT_EQ(result.trace.front().kind, ScriptVmTraceEventKindUVE::Failed);
    EXPECT_TRUE(result.trace.front().message.find("outside") != std::string::npos);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_CapturesNodeAndCompletionTraceInOrder) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back(
        {ScriptIrInstructionKindUVE::ExecuteNode, 1U, 0U, "math.float.add", {}, {}});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(1U, "A", 2.0F));
    ASSERT_TRUE(context.SetInputUVE(1U, "B", 3.0F));

    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.trace.size(), 2U);
    EXPECT_EQ(result.trace[0].kind, ScriptVmTraceEventKindUVE::NodeExecuted);
    EXPECT_EQ(result.trace[0].instructionIndex, 0U);
    EXPECT_EQ(result.trace[0].sourceNodeId, 1U);
    EXPECT_EQ(result.trace[0].nodeTypeId, "math.float.add");
    EXPECT_EQ(result.trace[1].kind, ScriptVmTraceEventKindUVE::Completed);
    EXPECT_FALSE(result.traceTruncated);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_CapturesTypedValueTransferTrace) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back(
        {ScriptIrInstructionKindUVE::TransferValue, 4U, 9U, {}, "Result", "A"});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetOutputUVE(4U, "Result", ScriptVector3ValueUVE{{1.0F, 2.0F, 3.0F}}));

    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context);
    ASSERT_TRUE(result.IsSuccessUVE());
    ASSERT_EQ(result.trace.size(), 2U);
    EXPECT_EQ(result.trace[0].kind, ScriptVmTraceEventKindUVE::ValueTransferred);
    EXPECT_EQ(result.trace[0].sourceNodeId, 4U);
    EXPECT_EQ(result.trace[0].targetNodeId, 9U);
    EXPECT_EQ(result.trace[1].kind, ScriptVmTraceEventKindUVE::Completed);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_CapturesFailureTraceWithDiagnosticMessage) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back(
        {ScriptIrInstructionKindUVE::ExecuteNode, 12U, 0U, "math.vector3.normalize", {}, {}});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(12U, "Vector", ScriptVector3ValueUVE{{0.0F, 0.0F, 0.0F}}));

    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context);
    ASSERT_FALSE(result.IsSuccessUVE());
    ASSERT_EQ(result.trace.size(), 1U);
    EXPECT_EQ(result.trace.front().kind, ScriptVmTraceEventKindUVE::Failed);
    EXPECT_EQ(result.trace.front().instructionIndex, 0U);
    ASSERT_EQ(result.diagnostics.size(), 1U);
    EXPECT_EQ(result.trace.front().message, result.diagnostics.front().message);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_BoundsTraceEventCount) {
    ScriptBytecodeProgramUVE program;
    program.instructions.resize(ScriptVmExecutionResultUVE::kMaximumTraceEventsUVE + 3U);

    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program);
    EXPECT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.trace.size(), ScriptVmExecutionResultUVE::kMaximumTraceEventsUVE);
    EXPECT_TRUE(result.traceTruncated);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_DispatchesAllBuiltInVector3Nodes) {
    const auto makeProgram = [](const std::uint32_t nodeId, const char* nodeTypeId) {
        ScriptBytecodeProgramUVE program;
        program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, nodeId, 0U, nodeTypeId, {}, {}});
        return program;
    };

    ScriptVmExecutionContextUVE makeContext;
    ASSERT_TRUE(makeContext.SetInputUVE(1U, "X", 1.0F));
    ASSERT_TRUE(makeContext.SetInputUVE(1U, "Y", -2.0F));
    ASSERT_TRUE(makeContext.SetInputUVE(1U, "Z", 3.0F));
    EXPECT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(1U, "math.vector3.make"), makeContext).IsSuccessUVE());
    const auto makeOutput = makeContext.FindOutputUVE(1U, "Vector");
    ASSERT_TRUE(makeOutput.has_value());
    ASSERT_TRUE(std::holds_alternative<ScriptVector3ValueUVE>(*makeOutput));
    EXPECT_EQ(std::get<ScriptVector3ValueUVE>(*makeOutput), (ScriptVector3ValueUVE{{1.0F, -2.0F, 3.0F}}));

    ScriptVmExecutionContextUVE addContext;
    ASSERT_TRUE(addContext.SetInputUVE(2U, "A", ScriptVector3ValueUVE{{1.0F, 2.0F, 3.0F}}));
    ASSERT_TRUE(addContext.SetInputUVE(2U, "B", ScriptVector3ValueUVE{{4.0F, 5.0F, 6.0F}}));
    EXPECT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(2U, "math.vector3.add"), addContext).IsSuccessUVE());
    EXPECT_EQ(std::get<ScriptVector3ValueUVE>(*addContext.FindOutputUVE(2U, "Result")),
              (ScriptVector3ValueUVE{{5.0F, 7.0F, 9.0F}}));

    ScriptVmExecutionContextUVE subtractContext;
    ASSERT_TRUE(subtractContext.SetInputUVE(3U, "A", ScriptVector3ValueUVE{{4.0F, 5.0F, 6.0F}}));
    ASSERT_TRUE(subtractContext.SetInputUVE(3U, "B", ScriptVector3ValueUVE{{1.0F, 2.0F, 3.0F}}));
    EXPECT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(3U, "math.vector3.subtract"), subtractContext).IsSuccessUVE());
    EXPECT_EQ(std::get<ScriptVector3ValueUVE>(*subtractContext.FindOutputUVE(3U, "Result")),
              (ScriptVector3ValueUVE{{3.0F, 3.0F, 3.0F}}));

    ScriptVmExecutionContextUVE multiplyContext;
    ASSERT_TRUE(multiplyContext.SetInputUVE(4U, "Vector", ScriptVector3ValueUVE{{1.0F, -2.0F, 3.0F}}));
    ASSERT_TRUE(multiplyContext.SetInputUVE(4U, "Scale", 2.0F));
    EXPECT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(4U, "math.vector3.multiply"), multiplyContext).IsSuccessUVE());
    EXPECT_EQ(std::get<ScriptVector3ValueUVE>(*multiplyContext.FindOutputUVE(4U, "Result")),
              (ScriptVector3ValueUVE{{2.0F, -4.0F, 6.0F}}));

    ScriptVmExecutionContextUVE dotContext;
    ASSERT_TRUE(dotContext.SetInputUVE(5U, "A", ScriptVector3ValueUVE{{1.0F, 2.0F, 3.0F}}));
    ASSERT_TRUE(dotContext.SetInputUVE(5U, "B", ScriptVector3ValueUVE{{4.0F, 5.0F, 6.0F}}));
    EXPECT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(5U, "math.vector3.dot"), dotContext).IsSuccessUVE());
    EXPECT_FLOAT_EQ(std::get<float>(*dotContext.FindOutputUVE(5U, "Result")), 32.0F);

    ScriptVmExecutionContextUVE crossContext;
    ASSERT_TRUE(crossContext.SetInputUVE(6U, "A", ScriptVector3ValueUVE{{1.0F, 0.0F, 0.0F}}));
    ASSERT_TRUE(crossContext.SetInputUVE(6U, "B", ScriptVector3ValueUVE{{0.0F, 1.0F, 0.0F}}));
    EXPECT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(6U, "math.vector3.cross"), crossContext).IsSuccessUVE());
    EXPECT_EQ(std::get<ScriptVector3ValueUVE>(*crossContext.FindOutputUVE(6U, "Result")),
              (ScriptVector3ValueUVE{{0.0F, 0.0F, 1.0F}}));

    ScriptVmExecutionContextUVE lengthContext;
    ASSERT_TRUE(lengthContext.SetInputUVE(7U, "Vector", ScriptVector3ValueUVE{{3.0F, 4.0F, 0.0F}}));
    EXPECT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(7U, "math.vector3.length"), lengthContext).IsSuccessUVE());
    EXPECT_FLOAT_EQ(std::get<float>(*lengthContext.FindOutputUVE(7U, "Length")), 5.0F);

    ScriptVmExecutionContextUVE normalizeContext;
    ASSERT_TRUE(normalizeContext.SetInputUVE(8U, "Vector", ScriptVector3ValueUVE{{0.0F, 3.0F, 4.0F}}));
    EXPECT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(8U, "math.vector3.normalize"), normalizeContext).IsSuccessUVE());
    EXPECT_EQ(std::get<ScriptVector3ValueUVE>(*normalizeContext.FindOutputUVE(8U, "Result")),
              (ScriptVector3ValueUVE{{0.0F, 0.6F, 0.8F}}));
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_DispatchesFloatAndBooleanNodes) {
    const auto makeProgram = [](const std::uint32_t nodeId, const char* nodeTypeId) {
        ScriptBytecodeProgramUVE program;
        program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, nodeId, 0U, nodeTypeId, {}, {}});
        return program;
    };

    const auto runFloat = [&](const char* nodeTypeId, float lhs, float rhs) {
        ScriptVmExecutionContextUVE context;
        EXPECT_TRUE(context.SetInputUVE(10U, "A", lhs));
        EXPECT_TRUE(context.SetInputUVE(10U, "B", rhs));
        EXPECT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(10U, nodeTypeId), context).IsSuccessUVE());
        const auto output = context.FindOutputUVE(10U, "Result");
        EXPECT_TRUE(output.has_value());
        if (!output.has_value() || !std::holds_alternative<float>(*output)) {
            return 0.0F;
        }
        return std::get<float>(*output);
    };
    EXPECT_FLOAT_EQ(runFloat("math.float.add", 2.0F, 3.0F), 5.0F);
    EXPECT_FLOAT_EQ(runFloat("math.float.subtract", 2.0F, 3.0F), -1.0F);
    EXPECT_FLOAT_EQ(runFloat("math.float.multiply", 2.0F, 3.0F), 6.0F);
    EXPECT_FLOAT_EQ(runFloat("math.float.divide", 6.0F, 3.0F), 2.0F);

    ScriptVmExecutionContextUVE divideByZeroContext;
    ASSERT_TRUE(divideByZeroContext.SetInputUVE(11U, "A", 1.0F));
    ASSERT_TRUE(divideByZeroContext.SetInputUVE(11U, "B", 0.0F));
    const ScriptVmExecutionResultUVE divideByZero =
        ExecuteScriptBytecodeUVE(makeProgram(11U, "math.float.divide"), divideByZeroContext);
    EXPECT_EQ(divideByZero.status, ScriptVmStatusUVE::NodeExecutionFailed);

    const auto runBoolean = [&](const char* nodeTypeId, bool lhs, bool rhs) {
        ScriptVmExecutionContextUVE context;
        EXPECT_TRUE(context.SetInputUVE(20U, "A", lhs));
        EXPECT_TRUE(context.SetInputUVE(20U, "B", rhs));
        EXPECT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(20U, nodeTypeId), context).IsSuccessUVE());
        const auto output = context.FindOutputUVE(20U, "Result");
        EXPECT_TRUE(output.has_value());
        if (!output.has_value() || !std::holds_alternative<bool>(*output)) {
            return false;
        }
        return std::get<bool>(*output);
    };
    EXPECT_FALSE(runBoolean("logic.boolean.and", true, false));
    EXPECT_TRUE(runBoolean("logic.boolean.or", true, false));
    EXPECT_TRUE(runBoolean("logic.boolean.xor", true, false));

    ScriptVmExecutionContextUVE notContext;
    ASSERT_TRUE(notContext.SetInputUVE(21U, "Value", false));
    ASSERT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(21U, "logic.boolean.not"), notContext).IsSuccessUVE());
    EXPECT_TRUE(std::get<bool>(*notContext.FindOutputUVE(21U, "Result")));
}

TEST(ScriptEntityQueryAdapterUVETest, PopulateComponentFactsUVE_StagesEcsPresenceInBindingOrder) {
    Memory::MemoryManagerUVE memoryManager;
    Events::EventSystemUVE eventSystem;
    Scene::EntityManagerUVE entityManager{memoryManager.GetDefaultAllocatorUVE(), eventSystem};
    const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<Scene::NameComponentUVE>(entity, Scene::NameComponentUVE{"Hero"});

    struct MissingComponentUVE final {};
    const std::vector<ScriptEntityComponentTypeBindingUVE> bindings{
        {"NameComponentUVE", std::type_index(typeid(Scene::NameComponentUVE))},
        {"MissingComponentUVE", std::type_index(typeid(MissingComponentUVE))},
    };
    ScriptVmExecutionContextUVE context;
    const ScriptEntityQueryAdapterResultUVE populated =
        ScriptEntityQueryAdapterUVE::PopulateComponentFactsUVE(entityManager, entity, bindings, context);
    ASSERT_TRUE(populated.IsAppliedUVE());
    EXPECT_EQ(populated.factsWritten, 2U);
    const auto present = context.FindComponentFactUVE(entity, "NameComponentUVE");
    ASSERT_TRUE(present.has_value());
    EXPECT_TRUE(present->present);
    const auto absent = context.FindComponentFactUVE(entity, "MissingComponentUVE");
    ASSERT_TRUE(absent.has_value());
    EXPECT_FALSE(absent->present);

    ASSERT_TRUE(context.SetInputUVE(50U, "Entity", ScriptEntityValueUVE{entity}));
    ASSERT_TRUE(context.SetInputUVE(
        50U, "Component", ScriptComponentValueUVE{Scene::kInvalidEntityUVE, "NameComponentUVE", false}));
    ScriptBytecodeProgramUVE hasProgram;
    hasProgram.instructions.push_back(
        {ScriptIrInstructionKindUVE::ExecuteNode, 50U, 0U, "query.entity.has_component", {}, {}});
    ASSERT_TRUE(ExecuteScriptBytecodeUVE(hasProgram, context).IsSuccessUVE());
    EXPECT_TRUE(std::get<bool>(*context.FindOutputUVE(50U, "Result")));

    ASSERT_TRUE(context.SetInputUVE(51U, "Entity", ScriptEntityValueUVE{entity}));
    ASSERT_TRUE(context.SetInputUVE(
        51U, "Component", ScriptComponentValueUVE{Scene::kInvalidEntityUVE, "MissingComponentUVE", false}));
    ScriptBytecodeProgramUVE getProgram;
    getProgram.instructions.push_back(
        {ScriptIrInstructionKindUVE::ExecuteNode, 51U, 0U, "query.entity.get_component", {}, {}});
    ASSERT_TRUE(ExecuteScriptBytecodeUVE(getProgram, context).IsSuccessUVE());
    const auto copied = context.FindOutputUVE(51U, "Result");
    ASSERT_TRUE(copied.has_value());
    ASSERT_TRUE(std::holds_alternative<ScriptComponentValueUVE>(*copied));
    EXPECT_FALSE(std::get<ScriptComponentValueUVE>(*copied).present);

    const std::size_t factCountBeforeInvalid = context.componentFacts.size();
    const auto rejected = ScriptEntityQueryAdapterUVE::PopulateComponentFactsUVE(
        entityManager, Scene::kInvalidEntityUVE, bindings, context);
    EXPECT_EQ(rejected.code, ScriptEntityQueryAdapterCodeUVE::InvalidEntity);
    EXPECT_EQ(context.componentFacts.size(), factCountBeforeInvalid);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_DispatchesEntityQueryNodesFromCopiedFacts) {
    const auto makeProgram = [](const std::uint32_t nodeId, const char* nodeTypeId) {
        ScriptBytecodeProgramUVE program;
        program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, nodeId, 0U, nodeTypeId, {}, {}});
        return program;
    };
    const Scene::EntityUVE entity{42U, 3U};
    const ScriptComponentValueUVE componentToken{Scene::kInvalidEntityUVE, "MeshComponentUVE", false};
    ScriptVmExecutionContextUVE hasContext;
    ASSERT_TRUE(hasContext.SetInputUVE(40U, "Entity", ScriptEntityValueUVE{entity}));
    ASSERT_TRUE(hasContext.SetInputUVE(40U, "Component", componentToken));
    ASSERT_TRUE(hasContext.SetComponentFactUVE(entity, "MeshComponentUVE", true));
    ASSERT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(40U, "query.entity.has_component"), hasContext).IsSuccessUVE());
    EXPECT_EQ(std::get<bool>(*hasContext.FindOutputUVE(40U, "Result")), true);

    ScriptVmExecutionContextUVE getContext;
    ASSERT_TRUE(getContext.SetInputUVE(41U, "Entity", ScriptEntityValueUVE{entity}));
    ASSERT_TRUE(getContext.SetInputUVE(41U, "Component", componentToken));
    ASSERT_TRUE(getContext.SetComponentFactUVE(entity, "MeshComponentUVE", false));
    ASSERT_TRUE(ExecuteScriptBytecodeUVE(makeProgram(41U, "query.entity.get_component"), getContext).IsSuccessUVE());
    const auto componentOutput = getContext.FindOutputUVE(41U, "Result");
    ASSERT_TRUE(componentOutput.has_value());
    ASSERT_TRUE(std::holds_alternative<ScriptComponentValueUVE>(*componentOutput));
    EXPECT_FALSE(std::get<ScriptComponentValueUVE>(*componentOutput).present);

    ScriptVmExecutionContextUVE missingFactContext;
    ASSERT_TRUE(missingFactContext.SetInputUVE(42U, "Entity", ScriptEntityValueUVE{entity}));
    ASSERT_TRUE(missingFactContext.SetInputUVE(42U, "Component", componentToken));
    const ScriptVmExecutionResultUVE missingFact =
        ExecuteScriptBytecodeUVE(makeProgram(42U, "query.entity.has_component"), missingFactContext);
    EXPECT_EQ(missingFact.status, ScriptVmStatusUVE::NodeExecutionFailed);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_ResolvesCompilerStyleNodeThenTransferOrdering) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back(
        {ScriptIrInstructionKindUVE::ExecuteNode, 20U, 0U, "math.vector3.make", {}, {}});
    program.instructions.push_back(
        {ScriptIrInstructionKindUVE::ExecuteNode, 30U, 0U, "math.vector3.add", {}, {}});
    program.instructions.push_back(
        {ScriptIrInstructionKindUVE::TransferValue, 20U, 30U, {}, "Vector", "A"});

    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(20U, "X", 1.0F));
    ASSERT_TRUE(context.SetInputUVE(20U, "Y", 2.0F));
    ASSERT_TRUE(context.SetInputUVE(20U, "Z", 3.0F));
    ASSERT_TRUE(context.SetInputUVE(30U, "B", ScriptVector3ValueUVE{{4.0F, 5.0F, 6.0F}}));

    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, context);
    EXPECT_TRUE(result.IsSuccessUVE());
    EXPECT_EQ(result.instructionsExecuted, 3U);
    EXPECT_EQ(std::get<ScriptVector3ValueUVE>(*context.FindOutputUVE(30U, "Result")),
              (ScriptVector3ValueUVE{{5.0F, 7.0F, 9.0F}}));
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_TransfersTypedValuesAndRejectsNodeFailure) {
    ScriptBytecodeProgramUVE transferProgram;
    transferProgram.instructions.push_back(
        {ScriptIrInstructionKindUVE::TransferValue, 10U, 11U, {}, "Result", "Vector"});
    ScriptVmExecutionContextUVE transferContext;
    ASSERT_TRUE(transferContext.SetOutputUVE(10U, "Result", ScriptVector3ValueUVE{{7.0F, 8.0F, 9.0F}}));
    const ScriptVmExecutionResultUVE transferred = ExecuteScriptBytecodeUVE(transferProgram, transferContext);
    EXPECT_TRUE(transferred.IsSuccessUVE());
    EXPECT_EQ(std::get<ScriptVector3ValueUVE>(*transferContext.FindInputUVE(11U, "Vector")),
              (ScriptVector3ValueUVE{{7.0F, 8.0F, 9.0F}}));

    ScriptBytecodeProgramUVE failingProgram;
    failingProgram.instructions.push_back(
        {ScriptIrInstructionKindUVE::ExecuteNode, 12U, 0U, "math.vector3.normalize", {}, {}});
    ScriptVmExecutionContextUVE failingContext;
    ASSERT_TRUE(failingContext.SetInputUVE(12U, "Vector", ScriptVector3ValueUVE{{0.0F, 0.0F, 0.0F}}));
    const ScriptVmExecutionResultUVE failed = ExecuteScriptBytecodeUVE(failingProgram, failingContext);
    EXPECT_FALSE(failed.IsSuccessUVE());
    EXPECT_EQ(failed.status, ScriptVmStatusUVE::NodeExecutionFailed);
    ASSERT_EQ(failed.diagnostics.size(), 1U);
    EXPECT_EQ(failed.diagnostics[0].instructionIndex, 0U);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_StopsAtInstructionBudget) {
    ScriptBytecodeProgramUVE program;
    program.instructions.resize(3U);
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program, {2U});
    EXPECT_FALSE(result.IsSuccessUVE());
    EXPECT_EQ(result.status, ScriptVmStatusUVE::InstructionBudgetExceeded);
    EXPECT_EQ(result.instructionsExecuted, 2U);
    ASSERT_EQ(result.diagnostics.size(), 1U);
    EXPECT_EQ(result.diagnostics[0].instructionIndex, 2U);
}

TEST(ScriptVmUVETest, ExecuteScriptBytecodeUVE_RejectsUnsupportedVersion) {
    ScriptBytecodeProgramUVE program;
    program.version = 99U;
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(program);
    EXPECT_FALSE(result.IsSuccessUVE());
    EXPECT_EQ(result.status, ScriptVmStatusUVE::InvalidInstruction);
    EXPECT_EQ(result.instructionsExecuted, 0U);
}

} // namespace UVE::Scripting


namespace UVE::Scripting {

TEST(ScriptRuntimeUVETest, AttachUVE_RejectsInvalidDuplicateAndAcceptsGenerationalIdentity) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE program;
    EXPECT_FALSE(runtime.AttachUVE(Scene::kInvalidEntityUVE, program));
    const Scene::EntityUVE first{3U, 1U};
    const Scene::EntityUVE replacement{3U, 2U};
    EXPECT_TRUE(runtime.AttachUVE(first, program));
    EXPECT_FALSE(runtime.AttachUVE(first, program));
    EXPECT_TRUE(runtime.AttachUVE(replacement, program));
    EXPECT_EQ(runtime.GetInstanceCountUVE(), 2U);
}

TEST(ScriptRuntimeUVETest, AttachDetailedUVEReturnsStructuredDiagnosticsForValidationAndCapacity) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE valid;

    const ScriptRuntimeAttachResultUVE invalidEntity = runtime.AttachDetailedUVE(Scene::kInvalidEntityUVE, valid);
    EXPECT_EQ(invalidEntity.code, ScriptRuntimeAttachCodeUVE::InvalidEntity);
    EXPECT_FALSE(invalidEntity.IsAcceptedUVE());
    EXPECT_FALSE(invalidEntity.message.empty());

    ScriptBytecodeProgramUVE invalidProgram;
    invalidProgram.version = 99U;
    const ScriptRuntimeAttachResultUVE invalid = runtime.AttachDetailedUVE({1U, 1U}, invalidProgram);
    EXPECT_EQ(invalid.code, ScriptRuntimeAttachCodeUVE::InvalidProgram);
    EXPECT_FALSE(invalid.IsAcceptedUVE());
    EXPECT_FALSE(invalid.diagnostics.empty());
    EXPECT_FALSE(invalid.message.empty());

    const Scene::EntityUVE entity{1U, 1U};
    const ScriptRuntimeAttachResultUVE accepted = runtime.AttachDetailedUVE(entity, valid);
    EXPECT_EQ(accepted.code, ScriptRuntimeAttachCodeUVE::Accepted);
    EXPECT_TRUE(accepted.IsAcceptedUVE());
    EXPECT_FALSE(accepted.message.empty());
    EXPECT_TRUE(runtime.AttachUVE({2U, 1U}, valid));

    const ScriptRuntimeAttachResultUVE duplicate = runtime.AttachDetailedUVE(entity, valid);
    EXPECT_EQ(duplicate.code, ScriptRuntimeAttachCodeUVE::DuplicateInstance);
    EXPECT_FALSE(duplicate.IsAcceptedUVE());
    EXPECT_FALSE(duplicate.message.empty());

    ScriptRuntimeUVE capacityRuntime;
    for (std::size_t index = 0U; index < ScriptRuntimeUVE::kMaximumInstancesUVE; ++index) {
        ASSERT_TRUE(capacityRuntime.AttachUVE(
            {static_cast<std::uint32_t>(1000U + index), 1U}, valid));
    }
    const ScriptRuntimeAttachResultUVE capacity = capacityRuntime.AttachDetailedUVE({9999U, 1U}, valid);
    EXPECT_EQ(capacity.code, ScriptRuntimeAttachCodeUVE::CapacityExceeded);
    EXPECT_FALSE(capacity.IsAcceptedUVE());
    EXPECT_FALSE(capacity.message.empty());
}

TEST(ScriptRuntimeUVETest, TickDetailedUVE_UsesBorrowedEngineLogBinding) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 70U, 0U, "engine.log", {}, {}});
    const Scene::EntityUVE entity{70U, 1U};
    ASSERT_TRUE(runtime.AttachUVE(entity, program));
    std::optional<ScriptRuntimeStateUVE> state = runtime.GetStateUVE(entity);
    ASSERT_TRUE(state.has_value());
    ASSERT_TRUE(state->executionContext.SetInputUVE(70U, "Value", 9.25F));
    ASSERT_TRUE(runtime.SetStateUVE(entity, *state));

    EngineLogCaptureUVE capture;
    const ScriptEngineCallBindingsUVE bindings{CaptureEngineLogUVE, &capture};
    ScriptVmExecutionOptionsUVE options;
    options.engineCallBindings = &bindings;
    const ScriptRuntimeTickBatchResultUVE tick = runtime.TickDetailedUVE(options);
    ASSERT_TRUE(tick.IsSuccessUVE());
    ASSERT_EQ(tick.results.size(), 1U);
    EXPECT_EQ(tick.results.front().execution.status, ScriptVmStatusUVE::Completed);
    EXPECT_EQ(capture.callCount, 1U);
    EXPECT_FLOAT_EQ(capture.lastValue, 9.25F);
}

TEST(ScriptRuntimeUVETest, TickUVE_IsDeterministicAndSkipsDisabledInstances) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE program;
    program.instructions.resize(2U);
    ASSERT_TRUE(runtime.AttachUVE({9U, 1U}, program));
    ASSERT_TRUE(runtime.AttachUVE({2U, 4U}, program));
    ASSERT_TRUE(runtime.AttachUVE({5U, 1U}, program));
    ASSERT_TRUE(runtime.SetEnabledUVE({5U, 1U}, false));
    const auto results = runtime.TickUVE({2U});
    ASSERT_EQ(results.size(), 2U);
    EXPECT_EQ(results[0].entity, (Scene::EntityUVE{2U, 4U}));
    EXPECT_EQ(results[1].entity, (Scene::EntityUVE{9U, 1U}));
    EXPECT_TRUE(results[0].execution.IsSuccessUVE());
    EXPECT_EQ(results[0].execution.instructionsExecuted, 2U);
}

TEST(ScriptRuntimeUVETest, TickDetailedUVE_SummarizesEnabledCompletedAndDisabledInstances) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE program;
    program.instructions.resize(2U);
    ASSERT_TRUE(runtime.AttachUVE({9U, 1U}, program));
    ASSERT_TRUE(runtime.AttachUVE({2U, 4U}, program));
    ASSERT_TRUE(runtime.SetEnabledUVE({2U, 4U}, false));

    const ScriptRuntimeTickBatchResultUVE detailed = runtime.TickDetailedUVE({4U});
    ASSERT_EQ(detailed.results.size(), 1U);
    EXPECT_EQ(detailed.results[0].entity, (Scene::EntityUVE{9U, 1U}));
    EXPECT_EQ(detailed.summary.enabledInstanceCount, 1U);
    EXPECT_EQ(detailed.summary.completedCount, 1U);
    EXPECT_EQ(detailed.summary.instructionBudgetExceededCount, 0U);
    EXPECT_EQ(detailed.summary.invalidInstructionCount, 0U);
    EXPECT_EQ(detailed.summary.diagnosticCount, 0U);
    EXPECT_TRUE(detailed.IsSuccessUVE());

    const auto legacyResults = runtime.TickUVE({4U});
    ASSERT_EQ(legacyResults.size(), detailed.results.size());
    EXPECT_EQ(legacyResults[0].entity, detailed.results[0].entity);
    EXPECT_EQ(legacyResults[0].execution.instructionsExecuted,
              detailed.results[0].execution.instructionsExecuted);
}

TEST(ScriptRuntimeUVETest, TickWithEntityQueryDetailedUVE_RefreshesFactsAndAccountsAdapterFailures) {
    Memory::MemoryManagerUVE memoryManager;
    Events::EventSystemUVE eventSystem;
    Scene::EntityManagerUVE entityManager{memoryManager.GetDefaultAllocatorUVE(), eventSystem};
    const Scene::EntityUVE aliveEntity = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<Scene::NameComponentUVE>(aliveEntity, Scene::NameComponentUVE{"Hero"});
    const Scene::EntityUVE notAliveEntity{99U, 1U};

    ScriptBytecodeProgramUVE program;
    program.instructions.push_back(
        {ScriptIrInstructionKindUVE::ExecuteNode, 70U, 0U, "query.entity.has_component", {}, {}});
    ScriptRuntimeUVE runtime;
    ASSERT_TRUE(runtime.AttachUVE(aliveEntity, program));
    ASSERT_TRUE(runtime.AttachUVE(notAliveEntity, program));

    ScriptRuntimeStateUVE aliveState;
    ASSERT_TRUE(aliveState.executionContext.SetInputUVE(70U, "Entity", ScriptEntityValueUVE{aliveEntity}));
    ASSERT_TRUE(aliveState.executionContext.SetInputUVE(
        70U, "Component", ScriptComponentValueUVE{Scene::kInvalidEntityUVE, "NameComponentUVE", false}));
    ASSERT_TRUE(runtime.SetStateUVE(aliveEntity, aliveState));
    const std::vector<ScriptEntityComponentTypeBindingUVE> bindings{
        {"NameComponentUVE", std::type_index(typeid(Scene::NameComponentUVE))},
    };

    const ScriptRuntimeTickBatchResultUVE detailed =
        runtime.TickWithEntityQueryDetailedUVE(entityManager, bindings);
    ASSERT_EQ(detailed.results.size(), 2U);
    EXPECT_EQ(detailed.results[0].entity, aliveEntity);
    EXPECT_EQ(detailed.results[1].entity, notAliveEntity);
    EXPECT_EQ(detailed.summary.enabledInstanceCount, 2U);
    EXPECT_EQ(detailed.summary.completedCount, 1U);
    EXPECT_EQ(detailed.summary.nodeExecutionFailedCount, 1U);
    EXPECT_EQ(detailed.summary.diagnosticCount, 1U);
    EXPECT_FALSE(detailed.IsSuccessUVE());
    ASSERT_TRUE(detailed.results[0].execution.IsSuccessUVE());
    ASSERT_GE(detailed.results[0].execution.trace.size(), 3U);
    EXPECT_EQ(detailed.results[0].execution.trace[0].kind,
              ScriptVmTraceEventKindUVE::QueryFactsRefreshed);
    EXPECT_EQ(detailed.results[0].execution.trace[0].entity, aliveEntity);
    EXPECT_EQ(detailed.results[1].execution.status, ScriptVmStatusUVE::NodeExecutionFailed);
    ASSERT_EQ(detailed.results[1].execution.trace.size(), 1U);
    EXPECT_EQ(detailed.results[1].execution.trace.front().kind, ScriptVmTraceEventKindUVE::Failed);
    EXPECT_EQ(detailed.results[1].execution.trace.front().entity, notAliveEntity);
    EXPECT_FALSE(detailed.results[1].execution.trace.front().message.empty());

    const auto refreshedState = runtime.GetStateUVE(aliveEntity);
    ASSERT_TRUE(refreshedState.has_value());
    const auto fact = refreshedState->executionContext.FindComponentFactUVE(aliveEntity, "NameComponentUVE");
    ASSERT_TRUE(fact.has_value());
    EXPECT_TRUE(fact->present);
    EXPECT_TRUE(runtime.SetEnabledUVE(notAliveEntity, false));
    const ScriptRuntimeTickBatchResultUVE disabled =
        runtime.TickWithEntityQueryDetailedUVE(entityManager, bindings);
    ASSERT_EQ(disabled.results.size(), 1U);
    EXPECT_EQ(disabled.summary.enabledInstanceCount, 1U);
    EXPECT_EQ(disabled.summary.completedCount, 1U);
    EXPECT_EQ(disabled.summary.nodeExecutionFailedCount, 0U);
    EXPECT_TRUE(disabled.IsSuccessUVE());
}

TEST(ScriptRuntimeUVETest, TickDetailedUVE_ExecutesTypedVector3ThroughPerEntityContext) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back(
        {ScriptIrInstructionKindUVE::ExecuteNode, 21U, 0U, "math.vector3.make", {}, {}});
    const Scene::EntityUVE entity{21U, 1U};
    ASSERT_TRUE(runtime.AttachUVE(entity, program));

    ScriptRuntimeStateUVE state;
    ASSERT_TRUE(state.executionContext.SetInputUVE(21U, "X", 2.0F));
    ASSERT_TRUE(state.executionContext.SetInputUVE(21U, "Y", -4.0F));
    ASSERT_TRUE(state.executionContext.SetInputUVE(21U, "Z", 8.0F));
    ASSERT_TRUE(runtime.SetStateUVE(entity, state));

    const ScriptRuntimeTickBatchResultUVE detailed = runtime.TickDetailedUVE();
    ASSERT_TRUE(detailed.IsSuccessUVE());
    ASSERT_EQ(detailed.results.size(), 1U);
    EXPECT_EQ(detailed.summary.completedCount, 1U);
    EXPECT_EQ(detailed.results.front().execution.instructionsExecuted, 1U);

    const auto stored = runtime.GetStateUVE(entity);
    ASSERT_TRUE(stored.has_value());
    const auto output = stored->executionContext.FindOutputUVE(21U, "Vector");
    ASSERT_TRUE(output.has_value());
    EXPECT_EQ(std::get<ScriptVector3ValueUVE>(*output), (ScriptVector3ValueUVE{{2.0F, -4.0F, 8.0F}}));
}

TEST(ScriptRuntimeUVETest, SetStateDetailedUVE_RejectsInvalidVmBindingWithoutMutation) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE program;
    const Scene::EntityUVE entity{22U, 1U};
    ASSERT_TRUE(runtime.AttachUVE(entity, program));

    ScriptRuntimeStateUVE valid;
    ASSERT_TRUE(valid.executionContext.SetInputUVE(22U, "Value", 3.0F));
    ASSERT_TRUE(runtime.SetStateUVE(entity, valid));

    ScriptRuntimeStateUVE invalid = valid;
    invalid.executionContext.inputs.front().value = std::numeric_limits<float>::infinity();
    const ScriptRuntimeStateUpdateResultUVE rejected = runtime.SetStateDetailedUVE(entity, invalid);
    EXPECT_EQ(rejected.code, ScriptRuntimeStateUpdateCodeUVE::InvalidVmBinding);
    EXPECT_FALSE(rejected.IsAcceptedUVE());
    EXPECT_EQ(runtime.GetStateUVE(entity), std::optional<ScriptRuntimeStateUVE>(valid));
}

TEST(ScriptRuntimeUVETest, TickDetailedUVE_SummarizesInstructionBudgetDiagnostics) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE program;
    program.instructions.resize(3U);
    ASSERT_TRUE(runtime.AttachUVE({4U, 2U}, program));

    const ScriptRuntimeTickBatchResultUVE detailed = runtime.TickDetailedUVE({2U});
    ASSERT_EQ(detailed.results.size(), 1U);
    EXPECT_EQ(detailed.summary.enabledInstanceCount, 1U);
    EXPECT_EQ(detailed.summary.completedCount, 0U);
    EXPECT_EQ(detailed.summary.instructionBudgetExceededCount, 1U);
    EXPECT_EQ(detailed.summary.invalidInstructionCount, 0U);
    EXPECT_EQ(detailed.summary.diagnosticCount, 1U);
    EXPECT_FALSE(detailed.IsSuccessUVE());
    EXPECT_EQ(detailed.results[0].execution.status, ScriptVmStatusUVE::InstructionBudgetExceeded);
}

TEST(ScriptRuntimeUVETest, GetSnapshotUVE_ReturnsDeterministicCopiedInstanceMetadata) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE firstProgram;
    firstProgram.instructions.resize(2U);
    ScriptBytecodeProgramUVE secondProgram;
    secondProgram.instructions.resize(1U);
    const Scene::EntityUVE firstEntity{9U, 1U};
    const Scene::EntityUVE secondEntity{2U, 4U};
    ASSERT_TRUE(runtime.AttachUVE(firstEntity, firstProgram));
    ASSERT_TRUE(runtime.AttachUVE(secondEntity, secondProgram));

    ScriptRuntimeStateUVE firstState;
    firstState.values = {3, 5};
    ASSERT_TRUE(runtime.SetStateUVE(firstEntity, firstState));
    ASSERT_TRUE(runtime.SetEnabledUVE(firstEntity, false));

    const auto initialSnapshot = runtime.GetSnapshotUVE();
    ASSERT_EQ(initialSnapshot.size(), 2U);
    EXPECT_EQ(initialSnapshot[0].entity, secondEntity);
    EXPECT_EQ(initialSnapshot[0].generation, 1U);
    EXPECT_EQ(initialSnapshot[0].programVersion, ScriptBytecodeProgramUVE::kCurrentVersionUVE);
    EXPECT_EQ(initialSnapshot[0].instructionCount, 1U);
    EXPECT_EQ(initialSnapshot[0].stateValueCount, 0U);
    EXPECT_TRUE(initialSnapshot[0].enabled);
    EXPECT_EQ(initialSnapshot[1].entity, firstEntity);
    EXPECT_EQ(initialSnapshot[1].instructionCount, 2U);
    EXPECT_EQ(initialSnapshot[1].stateValueCount, 2U);
    EXPECT_FALSE(initialSnapshot[1].enabled);

    const ScriptRuntimeReloadResultUVE reload = runtime.ReloadUVE(firstEntity, firstProgram);
    ASSERT_TRUE(reload.IsAcceptedUVE());
    const auto reloadedSnapshot = runtime.GetSnapshotUVE();
    ASSERT_EQ(reloadedSnapshot.size(), 2U);
    EXPECT_EQ(reloadedSnapshot[1].generation, 2U);
    EXPECT_EQ(reloadedSnapshot[1].stateValueCount, 2U);
    EXPECT_FALSE(reloadedSnapshot[1].enabled);

    ASSERT_TRUE(runtime.DetachUVE(secondEntity));
    const auto detachedSnapshot = runtime.GetSnapshotUVE();
    ASSERT_EQ(detachedSnapshot.size(), 1U);
    EXPECT_EQ(detachedSnapshot.front().entity, firstEntity);
}

TEST(ScriptRuntimeUVETest, SetEnabledDetailedUVEReturnsStructuredDiagnosticsAndControlsTicking) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE program;
    const Scene::EntityUVE entity{6U, 1U};
    ASSERT_TRUE(runtime.AttachUVE(entity, program));

    const ScriptRuntimeEnabledUpdateResultUVE unchanged = runtime.SetEnabledDetailedUVE(entity, true);
    EXPECT_EQ(unchanged.code, ScriptRuntimeEnabledUpdateCodeUVE::Unchanged);
    EXPECT_TRUE(unchanged.IsAcceptedUVE());
    EXPECT_FALSE(unchanged.message.empty());

    const ScriptRuntimeEnabledUpdateResultUVE disabled = runtime.SetEnabledDetailedUVE(entity, false);
    EXPECT_EQ(disabled.code, ScriptRuntimeEnabledUpdateCodeUVE::Applied);
    EXPECT_TRUE(disabled.IsAcceptedUVE());
    EXPECT_TRUE(runtime.TickUVE().empty());
    EXPECT_TRUE(runtime.SetEnabledUVE(entity, false));

    const ScriptRuntimeEnabledUpdateResultUVE enabled = runtime.SetEnabledDetailedUVE(entity, true);
    EXPECT_EQ(enabled.code, ScriptRuntimeEnabledUpdateCodeUVE::Applied);
    EXPECT_TRUE(enabled.IsAcceptedUVE());
    EXPECT_EQ(runtime.TickUVE().size(), 1U);

    const ScriptRuntimeEnabledUpdateResultUVE missing = runtime.SetEnabledDetailedUVE({9U, 1U}, true);
    EXPECT_EQ(missing.code, ScriptRuntimeEnabledUpdateCodeUVE::NoActiveInstance);
    EXPECT_FALSE(missing.IsAcceptedUVE());
    EXPECT_FALSE(missing.message.empty());
}

TEST(ScriptRuntimeUVETest, DetachDetailedUVEReturnsStructuredLifecycleDiagnostics) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE program;
    const Scene::EntityUVE entity{4U, 1U};
    ASSERT_TRUE(runtime.AttachUVE(entity, program));

    const ScriptRuntimeDetachResultUVE wrongGeneration = runtime.DetachDetailedUVE({4U, 2U});
    EXPECT_EQ(wrongGeneration.code, ScriptRuntimeDetachCodeUVE::NoActiveInstance);
    EXPECT_FALSE(wrongGeneration.IsAcceptedUVE());
    EXPECT_FALSE(wrongGeneration.message.empty());
    EXPECT_TRUE(runtime.HasInstanceUVE(entity));

    const ScriptRuntimeDetachResultUVE applied = runtime.DetachDetailedUVE(entity);
    EXPECT_EQ(applied.code, ScriptRuntimeDetachCodeUVE::Applied);
    EXPECT_TRUE(applied.IsAcceptedUVE());
    EXPECT_FALSE(applied.message.empty());
    EXPECT_FALSE(runtime.HasInstanceUVE(entity));
    EXPECT_FALSE(runtime.DetachUVE(entity));
}

TEST(ScriptRuntimeUVETest, ReloadUVE_RejectsInvalidCandidateAndRetainsLastKnownGoodProgram) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE initial;
    initial.instructions.resize(1U);
    ASSERT_TRUE(runtime.AttachUVE({7U, 1U}, initial));

    ScriptBytecodeProgramUVE invalid;
    invalid.version = 99U;
    const ScriptRuntimeReloadResultUVE rejected = runtime.ReloadUVE({7U, 1U}, invalid);
    EXPECT_EQ(rejected.code, ScriptRuntimeReloadCodeUVE::RejectedInvalidProgram);
    EXPECT_EQ(rejected.activeGeneration, 1U);
    EXPECT_TRUE(rejected.lastKnownGoodRetained);
    ASSERT_EQ(rejected.diagnostics.size(), 1U);
    EXPECT_EQ(rejected.diagnostics[0].code, ScriptBytecodeDiagnosticCodeUVE::UnsupportedVersion);
    EXPECT_EQ(runtime.TickUVE({8U}).front().execution.instructionsExecuted, 1U);
}

TEST(ScriptRuntimeUVETest, StateUVE_IsBoundedAndPreservedAcrossCompatibleReload) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE initial;
    initial.instructions.resize(1U);
    const Scene::EntityUVE entity{8U, 1U};
    ASSERT_TRUE(runtime.AttachUVE(entity, initial));

    ScriptRuntimeStateUVE state;
    state.values = {11, -7, 42};
    const ScriptRuntimeStateUpdateResultUVE appliedState = runtime.SetStateDetailedUVE(entity, state);
    EXPECT_EQ(appliedState.code, ScriptRuntimeStateUpdateCodeUVE::Applied);
    EXPECT_TRUE(appliedState.IsAcceptedUVE());
    EXPECT_FALSE(appliedState.message.empty());
    ASSERT_EQ(runtime.GetStateUVE(entity), std::optional<ScriptRuntimeStateUVE>(state));

    const ScriptRuntimeStateUpdateResultUVE unchangedState = runtime.SetStateDetailedUVE(entity, state);
    EXPECT_EQ(unchangedState.code, ScriptRuntimeStateUpdateCodeUVE::Unchanged);
    EXPECT_TRUE(unchangedState.IsAcceptedUVE());
    EXPECT_TRUE(runtime.SetStateUVE(entity, state));

    ScriptBytecodeProgramUVE replacement;
    replacement.instructions.resize(2U);
    const ScriptRuntimeReloadResultUVE accepted = runtime.ReloadUVE(entity, replacement);
    EXPECT_TRUE(accepted.IsAcceptedUVE());
    EXPECT_TRUE(accepted.compatibleStatePreserved);
    EXPECT_EQ(accepted.activeGeneration, 2U);
    EXPECT_EQ(runtime.GetStateUVE(entity), std::optional<ScriptRuntimeStateUVE>(state));

    ScriptBytecodeProgramUVE invalid;
    invalid.version = 99U;
    const ScriptRuntimeReloadResultUVE rejected = runtime.ReloadUVE(entity, invalid);
    EXPECT_EQ(rejected.code, ScriptRuntimeReloadCodeUVE::RejectedInvalidProgram);
    EXPECT_FALSE(rejected.compatibleStatePreserved);
    EXPECT_TRUE(rejected.lastKnownGoodRetained);
    EXPECT_EQ(runtime.GetStateUVE(entity), std::optional<ScriptRuntimeStateUVE>(state));

    ScriptRuntimeStateUVE oversized;
    oversized.values.resize(ScriptRuntimeUVE::kMaximumStateValuesUVE + 1U);
    const ScriptRuntimeStateUpdateResultUVE capacity = runtime.SetStateDetailedUVE(entity, std::move(oversized));
    EXPECT_EQ(capacity.code, ScriptRuntimeStateUpdateCodeUVE::CapacityExceeded);
    EXPECT_FALSE(capacity.IsAcceptedUVE());
    EXPECT_FALSE(capacity.message.empty());

    const ScriptRuntimeStateUpdateResultUVE missing = runtime.SetStateDetailedUVE({9U, 1U}, state);
    EXPECT_EQ(missing.code, ScriptRuntimeStateUpdateCodeUVE::NoActiveInstance);
    EXPECT_FALSE(missing.IsAcceptedUVE());
    EXPECT_FALSE(missing.message.empty());
    EXPECT_FALSE(runtime.SetStateUVE({9U, 1U}, state));
}

TEST(ScriptRuntimeUVETest, StateUVE_SupportsTypedVector3ValuesWithoutChangingScalarSlots) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE program;
    const Scene::EntityUVE entity{10U, 1U};
    ASSERT_TRUE(runtime.AttachUVE(entity, program));

    const ScriptVector3ValueUVE position{{1.0F, -2.0F, 3.5F}};
    const ScriptVector3ValueUVE direction{{0.0F, 1.0F, 0.0F}};
    ScriptRuntimeStateUVE state;
    state.values = {17, -4};
    state.vector3Values = {position, direction};

    const ScriptRuntimeStateUpdateResultUVE applied = runtime.SetStateDetailedUVE(entity, state);
    EXPECT_EQ(applied.code, ScriptRuntimeStateUpdateCodeUVE::Applied);
    EXPECT_TRUE(applied.IsAcceptedUVE());

    const auto stored = runtime.GetStateUVE(entity);
    ASSERT_EQ(stored, std::optional<ScriptRuntimeStateUVE>(state));
    ASSERT_EQ(stored->values, (std::vector<std::int64_t>{17, -4}));
    ASSERT_EQ(stored->vector3Values.size(), 2U);
    EXPECT_EQ(stored->vector3Values[0], position);
    EXPECT_EQ(stored->vector3Values[1], direction);
}

TEST(ScriptRuntimeUVETest, StateUVE_RejectsNonFiniteTypedVector3WithoutMutation) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE program;
    const Scene::EntityUVE entity{11U, 1U};
    ASSERT_TRUE(runtime.AttachUVE(entity, program));

    ScriptRuntimeStateUVE validState;
    validState.vector3Values = {ScriptVector3ValueUVE{{1.0F, 2.0F, 3.0F}}};
    ASSERT_TRUE(runtime.SetStateUVE(entity, validState));

    ScriptRuntimeStateUVE invalidState = validState;
    invalidState.vector3Values.front().value.y = std::numeric_limits<float>::quiet_NaN();
    const ScriptRuntimeStateUpdateResultUVE rejected = runtime.SetStateDetailedUVE(entity, invalidState);
    EXPECT_EQ(rejected.code, ScriptRuntimeStateUpdateCodeUVE::NonFiniteVector3);
    EXPECT_FALSE(rejected.IsAcceptedUVE());
    EXPECT_FALSE(rejected.message.empty());
    EXPECT_EQ(runtime.GetStateUVE(entity), std::optional<ScriptRuntimeStateUVE>(validState));
}

TEST(ScriptRuntimeUVETest, StateUVE_BoundsTypedVector3ValuesIndependentlyFromScalarSlots) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE program;
    const Scene::EntityUVE entity{12U, 1U};
    ASSERT_TRUE(runtime.AttachUVE(entity, program));

    ScriptRuntimeStateUVE oversized;
    oversized.values = {1};
    oversized.vector3Values.resize(ScriptRuntimeUVE::kMaximumStateVector3ValuesUVE + 1U);
    const ScriptRuntimeStateUpdateResultUVE rejected = runtime.SetStateDetailedUVE(entity, oversized);
    EXPECT_EQ(rejected.code, ScriptRuntimeStateUpdateCodeUVE::CapacityExceeded);
    EXPECT_FALSE(rejected.IsAcceptedUVE());
    EXPECT_FALSE(rejected.message.empty());
    EXPECT_EQ(runtime.GetStateUVE(entity), std::optional<ScriptRuntimeStateUVE>(ScriptRuntimeStateUVE{}));

    ScriptRuntimeStateUVE valid;
    valid.values.resize(ScriptRuntimeUVE::kMaximumStateValuesUVE);
    valid.vector3Values = {ScriptVector3ValueUVE{{4.0F, 5.0F, 6.0F}}};
    const ScriptRuntimeStateUpdateResultUVE applied = runtime.SetStateDetailedUVE(entity, valid);
    EXPECT_EQ(applied.code, ScriptRuntimeStateUpdateCodeUVE::Applied);
    EXPECT_EQ(runtime.GetStateUVE(entity), std::optional<ScriptRuntimeStateUVE>(valid));
}

TEST(ScriptRuntimeUVETest, StateUVE_PreservesTypedValuesAcrossCompatibleReload) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE initial;
    const Scene::EntityUVE entity{13U, 1U};
    ASSERT_TRUE(runtime.AttachUVE(entity, initial));

    ScriptRuntimeStateUVE state;
    state.values = {8};
    state.vector3Values = {ScriptVector3ValueUVE{{-1.0F, 0.5F, 9.0F}}};
    ASSERT_TRUE(runtime.SetStateUVE(entity, state));

    ScriptBytecodeProgramUVE replacement;
    replacement.instructions.resize(2U);
    const ScriptRuntimeReloadResultUVE reloaded = runtime.ReloadUVE(entity, replacement);
    ASSERT_TRUE(reloaded.IsAcceptedUVE());
    EXPECT_TRUE(reloaded.compatibleStatePreserved);
    EXPECT_EQ(runtime.GetStateUVE(entity), std::optional<ScriptRuntimeStateUVE>(state));
}

TEST(ScriptRuntimeUVETest, ReloadUVE_AcceptsValidReplacementAndRejectsMissingInstance) {
    ScriptRuntimeUVE runtime;
    ScriptBytecodeProgramUVE initial;
    ASSERT_TRUE(runtime.AttachUVE({8U, 1U}, initial));

    ScriptBytecodeProgramUVE replacement;
    replacement.instructions.resize(2U);
    const ScriptRuntimeReloadResultUVE accepted = runtime.ReloadUVE({8U, 1U}, replacement);
    EXPECT_TRUE(accepted.IsAcceptedUVE());
    EXPECT_EQ(accepted.activeGeneration, 2U);
    EXPECT_FALSE(accepted.lastKnownGoodRetained);
    ASSERT_EQ(runtime.TickUVE({8U}).size(), 1U);
    EXPECT_EQ(runtime.TickUVE({8U}).front().execution.instructionsExecuted, 2U);

    const ScriptRuntimeReloadResultUVE missing = runtime.ReloadUVE({9U, 1U}, replacement);
    EXPECT_EQ(missing.code, ScriptRuntimeReloadCodeUVE::NoActiveInstance);
    EXPECT_EQ(missing.activeGeneration, 0U);
}

} // namespace UVE::Scripting


namespace UVE::Scripting {

TEST(ScriptGraphPersistenceUVETest, EncodeDecodeSchema_RoundTripsNodesLinksLayoutMetadataDeterministically) {
    ScriptGraphSchemaUVE schema{};
    ASSERT_TRUE(schema.graph.AddNodeUVE({2U, "test.sink"}));
    ASSERT_TRUE(schema.graph.AddNodeUVE({1U, "test.source"}));
    ASSERT_TRUE(schema.graph.AddLinkUVE({{2U, "Out"}, {1U, "In"}}));
    ASSERT_TRUE(schema.graph.AddLinkUVE({{1U, "Out"}, {2U, "In"}}));
    schema.layout = {{2U, 30.0F, 40.0F}, {1U, 10.0F, 20.0F}};
    schema.metadata.emplace("zeta", "last");
    schema.metadata.emplace("alpha", "first");

    std::vector<ScriptPersistenceDiagnosticUVE> diagnostics;
    const std::string encoded = EncodeScriptGraphSchemaUVE(schema, diagnostics);
    ASSERT_TRUE(diagnostics.empty());
    EXPECT_EQ(encoded,
              R"({"schemaVersion":1,"nodes":[{"id":1,"typeId":"test.source"},{"id":2,"typeId":"test.sink"}],"links":[{"output":{"nodeId":1,"pinName":"Out"},"input":{"nodeId":2,"pinName":"In"}},{"output":{"nodeId":2,"pinName":"Out"},"input":{"nodeId":1,"pinName":"In"}}],"layout":[{"nodeId":1,"x":10.0,"y":20.0},{"nodeId":2,"x":30.0,"y":40.0}],"metadata":{"alpha":"first","zeta":"last"}})");

    const ScriptGraphSchemaDecodeResultUVE decoded = DecodeScriptGraphSchemaUVE(encoded);
    ASSERT_TRUE(decoded.IsSuccessUVE());
    EXPECT_EQ(decoded.schema->schemaVersion, kScriptGraphSchemaVersionUVE);
    EXPECT_EQ(decoded.schema->graph.GetNodesUVE().size(), 2U);
    EXPECT_EQ(decoded.schema->graph.GetLinksUVE().size(), 2U);
    EXPECT_EQ(decoded.schema->layout, (std::vector<ScriptGraphLayoutEntryUVE>{{1U, 10.0F, 20.0F}, {2U, 30.0F, 40.0F}}));
    EXPECT_EQ(decoded.schema->metadata.at("alpha"), "first");

    std::vector<ScriptPersistenceDiagnosticUVE> secondDiagnostics;
    EXPECT_EQ(EncodeScriptGraphSchemaUVE(*decoded.schema, secondDiagnostics), encoded);
    EXPECT_TRUE(secondDiagnostics.empty());
}

TEST(ScriptGraphPersistenceUVETest, DecodeSchema_RejectsUnknownFieldsMalformedInputAndFutureVersion) {
    const ScriptGraphSchemaDecodeResultUVE unknown = DecodeScriptGraphSchemaUVE(
        R"({"schemaVersion":1,"nodes":[],"links":[],"layout":[],"metadata":{},"future":true})");
    ASSERT_FALSE(unknown.IsSuccessUVE());
    EXPECT_EQ(unknown.diagnostics[0].code, ScriptPersistenceDiagnosticCodeUVE::UnknownField);

    const ScriptGraphSchemaDecodeResultUVE malformed = DecodeScriptGraphSchemaUVE(
        R"({"schemaVersion":1,"nodes":[{"id":1,"typeId":"test"}],"links":[],"layout":[],"metadata":{}})");
    ASSERT_TRUE(malformed.IsSuccessUVE());

    const ScriptGraphSchemaDecodeResultUVE missing = DecodeScriptGraphSchemaUVE(
        R"({"schemaVersion":1,"nodes":[],"links":[],"metadata":{}})");
    ASSERT_FALSE(missing.IsSuccessUVE());
    EXPECT_EQ(missing.diagnostics[0].code, ScriptPersistenceDiagnosticCodeUVE::MissingField);

    const ScriptGraphSchemaDecodeResultUVE future = DecodeScriptGraphSchemaUVE(
        R"({"schemaVersion":2,"nodes":[],"links":[],"layout":[],"metadata":{}})");
    ASSERT_FALSE(future.IsSuccessUVE());
    EXPECT_EQ(future.diagnostics[0].code, ScriptPersistenceDiagnosticCodeUVE::UnsupportedVersion);

    const ScriptGraphSchemaDecodeResultUVE duplicateLayout = DecodeScriptGraphSchemaUVE(
        R"({"schemaVersion":1,"nodes":[{"id":1,"typeId":"test"}],"links":[],"layout":[{"nodeId":1,"x":0,"y":0},{"nodeId":1,"x":1,"y":1}],"metadata":{}})");
    ASSERT_FALSE(duplicateLayout.IsSuccessUVE());
    EXPECT_EQ(duplicateLayout.diagnostics[0].code, ScriptPersistenceDiagnosticCodeUVE::DuplicateEntry);
}

TEST(ScriptGraphPersistenceUVETest, EncodeDecodeScriptGraphUVE_RoundTripsDeterministically) {
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({2U, "test.sink"}));
    ASSERT_TRUE(graph.AddNodeUVE({1U, "test.source"}));
    ASSERT_TRUE(graph.AddLinkUVE({{1U, "Out"}, {2U, "In"}}));
    std::vector<ScriptPersistenceDiagnosticUVE> diagnostics;
    const std::string encoded = EncodeScriptGraphUVE(graph, diagnostics);
    ASSERT_TRUE(diagnostics.empty());
    const ScriptGraphDecodeResultUVE decoded = DecodeScriptGraphUVE(encoded);
    ASSERT_TRUE(decoded.IsSuccessUVE());
    EXPECT_EQ(decoded.graph->GetNodesUVE().size(), 2U);
    EXPECT_EQ(decoded.graph->GetLinksUVE().size(), 1U);
    std::vector<ScriptPersistenceDiagnosticUVE> secondDiagnostics;
    EXPECT_EQ(EncodeScriptGraphUVE(*decoded.graph, secondDiagnostics), encoded);
    EXPECT_TRUE(secondDiagnostics.empty());
}

TEST(ScriptGraphPersistenceUVETest, DecodeScriptGraphUVE_RejectsMalformedVersionAndDuplicates) {
    const ScriptGraphDecodeResultUVE malformed = DecodeScriptGraphUVE("{not-json");
    ASSERT_FALSE(malformed.IsSuccessUVE());
    EXPECT_EQ(malformed.diagnostics[0].code, ScriptPersistenceDiagnosticCodeUVE::InvalidJson);
    const ScriptGraphDecodeResultUVE unsupported = DecodeScriptGraphUVE(
        R"({"schemaVersion":99,"nodes":[],"links":[]})");
    ASSERT_FALSE(unsupported.IsSuccessUVE());
    EXPECT_EQ(unsupported.diagnostics[0].code, ScriptPersistenceDiagnosticCodeUVE::UnsupportedVersion);
    const ScriptGraphDecodeResultUVE duplicate = DecodeScriptGraphUVE(
        R"({"schemaVersion":1,"nodes":[{"id":1,"typeId":"test"},{"id":1,"typeId":"test"}],"links":[],"layout":[],"metadata":{}})");
    ASSERT_FALSE(duplicate.IsSuccessUVE());
    EXPECT_EQ(duplicate.diagnostics[0].code, ScriptPersistenceDiagnosticCodeUVE::DuplicateEntry);
}

TEST(ScriptGraphPersistenceUVETest, EncodeScriptGraphUVE_EnforcesTextLimitWithoutOutput) {
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "test.source"}));
    std::vector<ScriptPersistenceDiagnosticUVE> diagnostics;
    EXPECT_TRUE(EncodeScriptGraphUVE(graph, diagnostics, {4096U, 8192U, 4096U, 128U, 4096U, 4U}).empty());
    ASSERT_EQ(diagnostics.size(), 1U);
    EXPECT_EQ(diagnostics[0].code, ScriptPersistenceDiagnosticCodeUVE::LimitExceeded);
}

} // namespace UVE::Scripting


namespace UVE::Scripting {

TEST(ScriptGraphEditorBackendUVETest, CommandsAreTransactionalAndNodeRemovalCleansIncidentLinks) {
    ScriptGraphEditorBackendUVE backend;
    ASSERT_TRUE(backend.AddNodeUVE({1U, "test.source"}).IsAppliedUVE());
    ASSERT_TRUE(backend.AddNodeUVE({2U, "test.sink"}).IsAppliedUVE());
    ASSERT_TRUE(backend.AddLinkUVE({{1U, "Out"}, {2U, "In"}}).IsAppliedUVE());
    EXPECT_EQ(backend.GetGraphUVE().GetLinksUVE().size(), 1U);
    EXPECT_FALSE(backend.AddLinkUVE({{1U, "Out"}, {2U, "In"}}).IsAppliedUVE());
    EXPECT_EQ(backend.GetGraphUVE().GetLinksUVE().size(), 1U);
    ASSERT_TRUE(backend.RemoveNodeUVE(1U).IsAppliedUVE());
    EXPECT_TRUE(backend.GetGraphUVE().GetNodesUVE().size() == 1U);
    EXPECT_TRUE(backend.GetGraphUVE().GetLinksUVE().empty());
}

TEST(ScriptGraphEditorBackendUVETest, UndoRedoRestoresSnapshotsAndNewEditClearsRedo) {
    ScriptGraphEditorBackendUVE backend;
    ASSERT_TRUE(backend.AddNodeUVE({1U, "test.source"}).IsAppliedUVE());
    ASSERT_TRUE(backend.AddNodeUVE({2U, "test.sink"}).IsAppliedUVE());
    EXPECT_EQ(backend.GetUndoCountUVE(), 2U);
    ASSERT_TRUE(backend.UndoUVE().IsAppliedUVE());
    EXPECT_EQ(backend.GetGraphUVE().GetNodesUVE().size(), 1U);
    EXPECT_EQ(backend.GetRedoCountUVE(), 1U);
    ASSERT_TRUE(backend.RedoUVE().IsAppliedUVE());
    EXPECT_EQ(backend.GetGraphUVE().GetNodesUVE().size(), 2U);
    ASSERT_TRUE(backend.UndoUVE().IsAppliedUVE());
    ASSERT_TRUE(backend.AddNodeUVE({3U, "test.source"}).IsAppliedUVE());
    EXPECT_EQ(backend.GetRedoCountUVE(), 0U);
    EXPECT_EQ(backend.RedoUVE().code, ScriptGraphCommandCodeUVE::NoHistory);
}

TEST(ScriptGraphCanvasUVETest, ApplyGraphSchemaReplacesAuthoritativeGraphAndLayoutWithUndo) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphCanvasUVE canvas(registry);
    ASSERT_TRUE(canvas.AddNodeUVE({1U, "test.source"}, {1.0F, 2.0F}).IsAppliedUVE());

    ScriptGraphSchemaUVE schema{};
    ASSERT_TRUE(schema.graph.AddNodeUVE({10U, "test.source"}));
    ASSERT_TRUE(schema.graph.AddNodeUVE({20U, "test.sink"}));
    ASSERT_TRUE(schema.graph.AddLinkUVE({{10U, "Out"}, {20U, "In"}}));
    schema.layout = {{20U, 40.0F, 50.0F}, {10U, 10.0F, 20.0F}};
    const ScriptGraphCanvasCommandResultUVE applied = canvas.ApplyGraphSchemaUVE(std::move(schema));
    ASSERT_TRUE(applied.IsAppliedUVE());
    EXPECT_TRUE(canvas.GetSnapshotUVE().dirty);
    EXPECT_EQ(canvas.GetGraphUVE().GetNodesUVE().front().id, 10U);
    EXPECT_EQ(canvas.GetLayoutSnapshotUVE().entries.size(), 2U);
    EXPECT_TRUE(canvas.UndoUVE().IsAppliedUVE());
    EXPECT_EQ(canvas.GetGraphUVE().GetNodesUVE().front().id, 1U);
}

TEST(ScriptGraphCanvasUVETest, RemoveNodeUndoRestoresExactGraphLayoutSelectionAndLinkOrder) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphCanvasUVE canvas(registry);
    ASSERT_TRUE(canvas.AddNodeUVE({1U, "test.source"}, {10.0F, 20.0F}).IsAppliedUVE());
    ASSERT_TRUE(canvas.AddNodeUVE({2U, "test.sink"}, {30.0F, 40.0F}).IsAppliedUVE());
    ASSERT_TRUE(canvas.AddNodeUVE({3U, "test.sink"}, {50.0F, 60.0F}).IsAppliedUVE());
    ASSERT_TRUE(canvas.AddLinkUVE({{1U, "Out"}, {2U, "In"}}).IsAppliedUVE());
    ASSERT_TRUE(canvas.AddLinkUVE({{1U, "Exec"}, {3U, "Exec"}}).IsAppliedUVE());
    ASSERT_TRUE(canvas.SetSelectionUVE({1U, 2U, 3U}).IsAppliedUVE());
    const ScriptGraphCanvasSnapshotUVE before = canvas.GetSnapshotUVE();

    ASSERT_TRUE(canvas.RemoveNodeUVE(2U).IsAppliedUVE());
    EXPECT_EQ(canvas.GetSnapshotUVE().selectedNodeIds, (std::vector<std::uint32_t>{1U, 3U}));
    ASSERT_TRUE(canvas.UndoUVE().IsAppliedUVE());
    const ScriptGraphCanvasSnapshotUVE restored = canvas.GetSnapshotUVE();
    EXPECT_EQ(restored.nodes, before.nodes);
    EXPECT_EQ(restored.links, before.links);
    EXPECT_EQ(restored.selectedNodeIds, before.selectedNodeIds);
    ASSERT_TRUE(canvas.RedoUVE().IsAppliedUVE());
    EXPECT_EQ(canvas.GetSnapshotUVE().nodes.size(), 2U);
    EXPECT_EQ(canvas.GetSnapshotUVE().links.size(), 1U);
}

TEST(ScriptGraphCanvasUVETest, ViewChangesAreNotUndoableAndRejectInvalidValues) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphCanvasUVE canvas(registry);
    const std::uint64_t initialRevision = canvas.GetSnapshotUVE().revision;
    ASSERT_TRUE(canvas.SetViewUVE({{12.0F, -4.0F}, 2.0F}).IsAppliedUVE());
    EXPECT_EQ(canvas.GetUndoCountUVE(), 0U);
    EXPECT_EQ(canvas.GetSnapshotUVE().view.pan, (ScriptGraphCanvasPointUVE{12.0F, -4.0F}));
    EXPECT_EQ(canvas.GetSnapshotUVE().view.zoom, 2.0F);
    EXPECT_GT(canvas.GetSnapshotUVE().revision, initialRevision);
    EXPECT_FALSE(canvas.SetViewUVE({{0.0F, 0.0F}, 0.0F}).IsAppliedUVE());
    EXPECT_FALSE(canvas.SetViewUVE({{std::numeric_limits<float>::quiet_NaN(), 0.0F}, 1.0F}).IsAppliedUVE());
}

TEST(ScriptGraphCanvasPersistenceUVETest, EncodeDecodeAndApplyLayout_RoundTripsDeterministicallyWithUndo) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphCanvasUVE canvas(registry);
    ASSERT_TRUE(canvas.AddNodeUVE({1U, "test.source"}, {10.0F, 20.0F}).IsAppliedUVE());
    ASSERT_TRUE(canvas.AddNodeUVE({2U, "test.sink"}, {30.0F, 40.0F}).IsAppliedUVE());
    ASSERT_TRUE(canvas.SetViewUVE({{12.0F, -4.0F}, 2.0F}).IsAppliedUVE());

    const ScriptGraphCanvasLayoutSnapshotUVE expected = canvas.GetLayoutSnapshotUVE();
    std::vector<ScriptPersistenceDiagnosticUVE> diagnostics;
    const std::string encoded = EncodeScriptGraphCanvasLayoutUVE(expected, diagnostics);
    ASSERT_TRUE(diagnostics.empty());
    const ScriptGraphCanvasLayoutDecodeResultUVE decoded = DecodeScriptGraphCanvasLayoutUVE(encoded);
    ASSERT_TRUE(decoded.IsSuccessUVE());
    ASSERT_EQ(*decoded.layout, expected);
    ScriptGraphCanvasLayoutSnapshotUVE incomplete = expected;
    incomplete.entries.pop_back();
    EXPECT_EQ(canvas.ApplyLayoutUVE(std::move(incomplete)).code,
              ScriptGraphCanvasCommandCodeUVE::Rejected);

    ASSERT_TRUE(canvas.MoveNodeUVE(1U, {100.0F, 200.0F}).IsAppliedUVE());
    ASSERT_TRUE(canvas.SetViewUVE({{-8.0F, 6.0F}, 1.0F}).IsAppliedUVE());
    ASSERT_TRUE(canvas.ApplyLayoutUVE(*decoded.layout).IsAppliedUVE());
    EXPECT_EQ(canvas.GetLayoutSnapshotUVE(), expected);
    ASSERT_TRUE(canvas.UndoUVE().IsAppliedUVE());
    EXPECT_EQ(canvas.GetLayoutSnapshotUVE().view, expected.view);
    EXPECT_EQ(canvas.GetLayoutSnapshotUVE().entries[0].position,
              (ScriptGraphCanvasPointUVE{100.0F, 200.0F}));
}

TEST(ScriptGraphCanvasPersistenceUVETest, DecodeLayout_RejectsMalformedVersionDuplicateAndTextLimit) {
    const ScriptGraphCanvasLayoutDecodeResultUVE malformed = DecodeScriptGraphCanvasLayoutUVE("{not-json");
    ASSERT_FALSE(malformed.IsSuccessUVE());
    EXPECT_EQ(malformed.diagnostics[0].code, ScriptPersistenceDiagnosticCodeUVE::InvalidJson);

    const ScriptGraphCanvasLayoutDecodeResultUVE duplicate = DecodeScriptGraphCanvasLayoutUVE(
        R"({"schemaVersion":1,"view":{"pan":{"x":0,"y":0},"zoom":1},"entries":[{"nodeId":1,"x":0,"y":0},{"nodeId":1,"x":1,"y":1}]})");
    ASSERT_FALSE(duplicate.IsSuccessUVE());
    EXPECT_EQ(duplicate.diagnostics[0].code, ScriptPersistenceDiagnosticCodeUVE::DuplicateEntry);

    ScriptGraphCanvasLayoutSnapshotUVE layout;
    layout.entries.push_back({1U, {0.0F, 0.0F}});
    std::vector<ScriptPersistenceDiagnosticUVE> diagnostics;
    EXPECT_TRUE(EncodeScriptGraphCanvasLayoutUVE(layout, diagnostics,
                                                  {128U, 8U}).empty());
    ASSERT_EQ(diagnostics.size(), 1U);
    EXPECT_EQ(diagnostics[0].code, ScriptPersistenceDiagnosticCodeUVE::LimitExceeded);
}

TEST(ScriptGraphCanvasUVETest, CommandsRejectStaleRevisionAndInvalidSelectionWithoutMutation) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphCanvasUVE canvas(registry);
    ASSERT_TRUE(canvas.AddNodeUVE({1U, "test.source"}, {0.0F, 0.0F}).IsAppliedUVE());
    const std::uint64_t revision = canvas.GetSnapshotUVE().revision;
    ASSERT_TRUE(canvas.MoveNodeUVE(1U, {5.0F, 5.0F}, revision).IsAppliedUVE());
    const ScriptGraphCanvasSnapshotUVE beforeReject = canvas.GetSnapshotUVE();
    EXPECT_EQ(canvas.MoveNodeUVE(1U, {8.0F, 8.0F}, revision).code,
              ScriptGraphCanvasCommandCodeUVE::StaleRevision);
    EXPECT_EQ(canvas.SetSelectionUVE({1U, 1U}).code, ScriptGraphCanvasCommandCodeUVE::Rejected);
    EXPECT_EQ(canvas.GetSnapshotUVE(), beforeReject);
}

TEST(ScriptGraphCanvasUVETest, SnapshotProvidesSortedPaletteAndTypedNodePins) {
    ScriptNodeRegistryUVE registry;
    RegisterTestNodesUVE(registry);
    ScriptGraphCanvasUVE canvas(registry);
    const ScriptGraphCanvasSnapshotUVE initial = canvas.GetSnapshotUVE();
    EXPECT_FALSE(initial.dirty);
    EXPECT_FALSE(initial.canUndo);
    EXPECT_FALSE(initial.canRedo);
    ASSERT_TRUE(canvas.AddNodeUVE({1U, "test.sink"}, {0.0F, 0.0F}).IsAppliedUVE());
    const ScriptGraphCanvasSnapshotUVE snapshot = canvas.GetSnapshotUVE();
    EXPECT_EQ(snapshot.paletteNodeTypeIds, (std::vector<std::string>{"test.sink", "test.source"}));
    ASSERT_EQ(snapshot.nodes.size(), 1U);
    EXPECT_EQ(snapshot.nodes[0].displayName, "Test Sink");
    ASSERT_EQ(snapshot.nodes[0].pins.size(), 2U);
    EXPECT_EQ(snapshot.nodes[0].pins[0].name, "In");
    EXPECT_EQ(snapshot.nodes[0].pins[0].direction, ScriptPinDirectionUVE::Input);
    EXPECT_EQ(snapshot.nodes[0].category, "Uncategorized");
    EXPECT_EQ(snapshot.nodes[0].iconId, "node.default");
    EXPECT_EQ(snapshot.nodes[0].pins[0].role, ScriptPinRoleUVE::Data);
    EXPECT_TRUE(snapshot.dirty);
    EXPECT_TRUE(snapshot.canUndo);
    EXPECT_FALSE(snapshot.canRedo);

    ScriptGraphCanvasUVE selectionCanvas(registry);
    ASSERT_TRUE(selectionCanvas.AddNodeUVE({1U, "test.sink"}, {0.0F, 0.0F}).IsAppliedUVE());
    ASSERT_TRUE(selectionCanvas.SetSelectionUVE({}).IsAppliedUVE());
    EXPECT_TRUE(selectionCanvas.GetSnapshotUVE().dirty);

    ASSERT_TRUE(canvas.UndoUVE().IsAppliedUVE());
    const ScriptGraphCanvasSnapshotUVE undone = canvas.GetSnapshotUVE();
    EXPECT_TRUE(undone.dirty);
    EXPECT_FALSE(undone.canUndo);
    EXPECT_TRUE(undone.canRedo);
}

} // namespace UVE::Scripting


namespace UVE::Scripting {

TEST(ScriptDebuggerUVETest, ContinueUVE_PausesAtSourceNodeBreakpointAndContinueResumes) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 10U, 0U, "test.source", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::TransferValue, 20U, 30U, {}, "Out", "In"});
    ScriptDebuggerUVE debugger;
    ASSERT_TRUE(debugger.AttachUVE(program));
    ASSERT_TRUE(debugger.SetBreakpointUVE(20U, true));
    const ScriptDebuggerSnapshotUVE paused = debugger.ContinueUVE();
    EXPECT_EQ(paused.state, ScriptDebuggerStateUVE::Paused);
    EXPECT_EQ(paused.instructionIndex, 1U);
    EXPECT_EQ(paused.sourceNodeId, 20U);
    EXPECT_EQ(paused.pauseReason, "Breakpoint reached.");
    ASSERT_EQ(paused.trace.size(), 1U);
    EXPECT_EQ(paused.trace.front().kind, ScriptVmTraceEventKindUVE::NodeExecuted);
    EXPECT_EQ(paused.trace.front().sourceNodeId, 10U);
    EXPECT_EQ(paused.trace.front().nodeTypeId, "test.source");
    const ScriptDebuggerSnapshotUVE completed = debugger.ContinueUVE();
    EXPECT_EQ(completed.state, ScriptDebuggerStateUVE::Completed);
    EXPECT_EQ(completed.executedInstructions, 2U);
    ASSERT_EQ(completed.trace.size(), 3U);
    EXPECT_EQ(completed.trace[1].kind, ScriptVmTraceEventKindUVE::ValueTransferred);
    EXPECT_EQ(completed.trace[2].kind, ScriptVmTraceEventKindUVE::Completed);
}

TEST(ScriptDebuggerUVETest, StepUVE_AdvancesOneInstructionAndReportsCompletion) {
    ScriptBytecodeProgramUVE program;
    program.instructions.resize(2U);
    ScriptDebuggerUVE debugger;
    ASSERT_TRUE(debugger.AttachUVE(program));
    const ScriptDebuggerSnapshotUVE first = debugger.StepUVE();
    EXPECT_EQ(first.state, ScriptDebuggerStateUVE::Paused);
    EXPECT_EQ(first.instructionIndex, 1U);
    EXPECT_EQ(first.executedInstructions, 1U);
    const ScriptDebuggerSnapshotUVE second = debugger.StepUVE();
    EXPECT_EQ(second.state, ScriptDebuggerStateUVE::Completed);
    EXPECT_EQ(second.instructionIndex, 2U);
}

TEST(ScriptDebuggerUVETest, StepUVE_ConditionalJumpUsesAttachedCopiedContext) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::ConditionalJump, 1U, 0U, "flow.branch",
                                    "Condition", {}, 2U, 1U});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 20U, 0U, "test.false", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 30U, 0U, "test.true", {}, {}});
    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(1U, "Condition", true));

    ScriptDebuggerUVE debugger;
    ASSERT_TRUE(debugger.AttachWithContextUVE(program, context));
    const ScriptDebuggerSnapshotUVE stepped = debugger.StepUVE();
    EXPECT_EQ(stepped.state, ScriptDebuggerStateUVE::Paused);
    EXPECT_EQ(stepped.instructionIndex, 2U);
    EXPECT_EQ(stepped.executedInstructions, 1U);
    ASSERT_EQ(stepped.trace.size(), 1U);
    EXPECT_EQ(stepped.trace.front().kind, ScriptVmTraceEventKindUVE::NodeExecuted);
    EXPECT_EQ(stepped.trace.front().message, "ConditionalJump evaluated true.");
}

TEST(ScriptDebuggerUVETest, StepUVE_SequenceDispatchContinuesToSecondTarget) {
    ScriptBytecodeProgramUVE program;
    program.instructions.push_back({ScriptIrInstructionKindUVE::SequenceDispatch, 1U, 0U, "flow.sequence",
                                    "Then", "Then2", 0U, 0U, 1U, 2U});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 20U, 0U, "test.first", {}, {}});
    program.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 30U, 0U, "test.second", {}, {}});

    ScriptDebuggerUVE debugger;
    ASSERT_TRUE(debugger.AttachUVE(program));
    const ScriptDebuggerSnapshotUVE sequence = debugger.StepUVE();
    EXPECT_EQ(sequence.state, ScriptDebuggerStateUVE::Paused);
    EXPECT_EQ(sequence.instructionIndex, 1U);
    ASSERT_EQ(sequence.trace.size(), 1U);
    EXPECT_EQ(sequence.trace.front().message, "SequenceDispatch selected ordered execution targets.");

    const ScriptDebuggerSnapshotUVE first = debugger.StepUVE();
    EXPECT_EQ(first.state, ScriptDebuggerStateUVE::Paused);
    EXPECT_EQ(first.instructionIndex, 2U);
    EXPECT_EQ(first.executedInstructions, 2U);

    const ScriptDebuggerSnapshotUVE second = debugger.StepUVE();
    EXPECT_EQ(second.state, ScriptDebuggerStateUVE::Completed);
    EXPECT_EQ(second.instructionIndex, 3U);
    EXPECT_EQ(second.executedInstructions, 3U);
}

TEST(ScriptDebuggerUVETest, ContinueUVE_BoundsCopiedTraceHistory) {
    ScriptBytecodeProgramUVE program;
    program.instructions.resize(ScriptDebuggerUVE::kMaximumTraceEventsUVE + 1U);
    ScriptDebuggerUVE debugger;
    ASSERT_TRUE(debugger.AttachUVE(std::move(program)));

    const ScriptDebuggerSnapshotUVE snapshot = debugger.ContinueUVE();
    EXPECT_EQ(snapshot.state, ScriptDebuggerStateUVE::Completed);
    EXPECT_EQ(snapshot.trace.size(), ScriptDebuggerUVE::kMaximumTraceEventsUVE);
    EXPECT_TRUE(snapshot.traceTruncated);
}

TEST(ScriptDebuggerUVETest, SetBreakpointUVE_ProvidesSortedSnapshotAndRejectsEmptyNodeId) {
    ScriptDebuggerUVE debugger;
    EXPECT_FALSE(debugger.SetBreakpointUVE(0U, true));
    EXPECT_TRUE(debugger.SetBreakpointUVE(20U, true));
    EXPECT_TRUE(debugger.SetBreakpointUVE(10U, true));
    const ScriptDebuggerSnapshotUVE snapshot = debugger.GetSnapshotUVE();
    EXPECT_EQ(snapshot.breakpointNodeIds, (std::vector<std::uint32_t>{10U, 20U}));
}

} // namespace UVE::Scripting
namespace UVE::Scripting {
TEST(ScriptGraphCanvasUVETest, SetPinDefaultValueValidatesAndRecordsNativeHistory) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(ScriptNodeTypeDescriptorUVE{
        "test.default", "Default", {{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number,
                                        ScriptPinRoleUVE::Data, std::string("1.0")},
                                       {"Out", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Number}}}));
    ScriptGraphCanvasUVE canvas(registry);
    ASSERT_TRUE(canvas.AddNodeUVE({1U, "test.default"}, {0.0F, 0.0F}).IsAppliedUVE());

    const auto applied = canvas.SetPinDefaultValueUVE(1U, "Value", "2.5");
    ASSERT_TRUE(applied.IsAppliedUVE());
    const ScriptGraphCanvasSnapshotUVE changed = canvas.GetSnapshotUVE();
    ASSERT_EQ(changed.nodes.size(), 1U);
    ASSERT_EQ(changed.nodes[0].pins.size(), 2U);
    EXPECT_EQ(changed.nodes[0].pins[0].defaultValue, std::optional<std::string>("2.5"));
    EXPECT_TRUE(changed.dirty);
    EXPECT_TRUE(changed.canUndo);

    EXPECT_FALSE(canvas.SetPinDefaultValueUVE(1U, "Value", "not-a-number").IsAppliedUVE());
    EXPECT_FALSE(canvas.SetPinDefaultValueUVE(1U, "Out", "2.5").IsAppliedUVE());
    ASSERT_TRUE(canvas.UndoUVE(changed.revision).IsAppliedUVE());
    const ScriptGraphCanvasSnapshotUVE restored = canvas.GetSnapshotUVE();
    ASSERT_EQ(restored.nodes.size(), 1U);
    EXPECT_EQ(restored.nodes[0].pins[0].defaultValue, std::optional<std::string>("1.0"));
}

TEST(ScriptGraphCanvasUVETest, SnapshotExposesDescriptorRichPaletteInDeterministicOrder) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(ScriptNodeTypeDescriptorUVE{
        "test.late", "Late", {{"Value", ScriptPinDirectionUVE::Input, ScriptValueTypeUVE::Number}},
        "FLOW", "node.branch", 20U, kScriptNodePresentationFlagCollapsibleUVE}));
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(ScriptNodeTypeDescriptorUVE{
        "test.early", "Early", {{"Exec", ScriptPinDirectionUVE::Output, ScriptValueTypeUVE::Execution}},
        "EVENT", "node.event", 10U, kScriptNodePresentationFlagCompactUVE}));
    ScriptGraphCanvasUVE canvas(registry);
    ASSERT_TRUE(canvas.AddNodeTypeUVE("test.early", {24.0F, 48.0F}).IsAppliedUVE());

    const ScriptGraphCanvasSnapshotUVE snapshot = canvas.GetSnapshotUVE();

    ASSERT_EQ(snapshot.paletteNodeTypeIds.size(), 2U);
    ASSERT_EQ(snapshot.paletteDescriptors.size(), 2U);
    EXPECT_EQ(snapshot.paletteDescriptors[0].typeId, "test.early");
    EXPECT_EQ(snapshot.paletteDescriptors[0].category, "EVENT");
    EXPECT_EQ(snapshot.paletteDescriptors[0].displayOrder, 10U);
    ASSERT_EQ(snapshot.paletteDescriptors[0].pins.size(), 1U);
    EXPECT_EQ(snapshot.paletteDescriptors[0].pins[0].name, "Exec");
    EXPECT_EQ(snapshot.paletteDescriptors[1].typeId, "test.late");
    EXPECT_EQ(snapshot.paletteDescriptors[1].category, "FLOW");
}

TEST(ScriptHotReloadManagerUVETest, LoadInitialAndReloadUVE_PublishOnlyValidatedCandidates) {

    ScriptBytecodeProgramUVE initial;
    initial.instructions.push_back({ScriptIrInstructionKindUVE::ExecuteNode, 10U, 0U, "test.source", {}, {}});
    std::vector<std::uint8_t> initialBytes;
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    initialBytes = EncodeScriptBytecodeUVE(initial, diagnostics);
    ASSERT_TRUE(diagnostics.empty());

    ScriptHotReloadManagerUVE manager;
    const ScriptHotReloadResultUVE loaded = manager.LoadInitialUVE(initialBytes);
    ASSERT_TRUE(loaded.IsAcceptedUVE());
    EXPECT_EQ(loaded.activeGeneration, 1U);
    EXPECT_TRUE(manager.GetSnapshotUVE().hasActiveProgram);

    std::vector<std::uint8_t> invalid = initialBytes;
    invalid[0U] = static_cast<std::uint8_t>('X');
    const ScriptHotReloadResultUVE rejected = manager.ReloadUVE(invalid);
    EXPECT_FALSE(rejected.IsAcceptedUVE());
    EXPECT_TRUE(rejected.lastKnownGoodRetained);
    EXPECT_EQ(rejected.activeGeneration, 1U);
    EXPECT_EQ(manager.GetActiveProgramUVE()->instructions.size(), 1U);

    initial.instructions.push_back({ScriptIrInstructionKindUVE::TransferValue, 10U, 20U, {}, "Out", "In"});
    diagnostics.clear();
    const std::vector<std::uint8_t> replacement = EncodeScriptBytecodeUVE(initial, diagnostics);
    ASSERT_TRUE(diagnostics.empty());
    const ScriptHotReloadResultUVE accepted = manager.ReloadUVE(replacement);
    EXPECT_TRUE(accepted.IsAcceptedUVE());
    EXPECT_EQ(accepted.activeGeneration, 2U);
    EXPECT_TRUE(accepted.compatibleStatePreserved);
    EXPECT_EQ(manager.GetSnapshotUVE().instructionCount, 2U);
}

TEST(ScriptHotReloadManagerUVETest, ReloadUVE_ReportsNoActiveProgramWhenInitialCandidateIsInvalid) {
    ScriptHotReloadManagerUVE manager;
    const ScriptHotReloadResultUVE result = manager.ReloadUVE({0x00U, 0x01U});
    EXPECT_EQ(result.code, ScriptHotReloadCodeUVE::NoActiveProgram);
    EXPECT_FALSE(result.lastKnownGoodRetained);
    EXPECT_FALSE(manager.GetSnapshotUVE().hasActiveProgram);
}

} // namespace UVE::Scripting
