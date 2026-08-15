// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/scripting/script_compiler_ir_uve.h"
#include "uve/scripting/script_graph_uve.h"

#include <gtest/gtest.h>

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

TEST(ScriptNodeRegistryUVETest, FindNodeTypeUVE_ReturnsCopiedStableDescriptorView) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(registry.RegisterNodeTypeUVE(MakeSourceNodeUVE()));
    const ScriptNodeTypeDescriptorUVE* descriptor = registry.FindNodeTypeUVE("test.source");
    ASSERT_NE(descriptor, nullptr);
    EXPECT_EQ(descriptor->displayName, "Test Source");
    EXPECT_EQ(descriptor->pins.size(), 2U);
    EXPECT_EQ(registry.FindNodeTypeUVE("missing"), nullptr);
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
    EXPECT_EQ(diagnostics[1].code, ScriptValidationCodeUVE::IncompatiblePinTypes);
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
    EXPECT_EQ(diagnostics[1].code, ScriptValidationCodeUVE::WrongPinDirection);
    EXPECT_EQ(diagnostics[2].code, ScriptValidationCodeUVE::EmptyLinkEndpoint);
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
}

} // namespace UVE::Scripting
