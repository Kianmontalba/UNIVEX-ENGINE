// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/scripting/script_source_mapping_uve.h"
#include "uve/scripting/script_compiler_ir_uve.h"
#include "uve/scripting/script_bytecode_uve.h"

#include <gtest/gtest.h>

namespace UVE::Scripting {

TEST(ScriptSourceMappingUVETest, BuildPresentationUVE_BuildsNodeLabelsBreakpointsAndWatches) {
    ScriptGraphUVE graph;
    EXPECT_TRUE(graph.AddNodeUVE(ScriptNodeUVE{10U, "Action"}));
    EXPECT_TRUE(graph.AddNodeUVE(ScriptNodeUVE{20U, "Branch"}));

    ScriptSourceMappingUVE mapping(graph);
    EXPECT_TRUE(mapping.AddWatchUVE("w1", "player.health"));
    EXPECT_TRUE(mapping.AddWatchUVE("w2", "speed * 2.0"));

    ScriptDebuggerUVE debugger;
    ScriptNodeRegistryUVE registry;
    EXPECT_TRUE(registry.RegisterNodeTypeUVE(ScriptNodeTypeDescriptorUVE{"Action", "Action", {}}));
    EXPECT_TRUE(registry.RegisterNodeTypeUVE(ScriptNodeTypeDescriptorUVE{"Branch", "Branch", {}}));

    const auto irResult = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(irResult.IsSuccessUVE());
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    const auto bytecode = LowerIrToBytecodeUVE(*irResult.program, diagnostics);
    ASSERT_TRUE(bytecode.has_value());

    EXPECT_TRUE(debugger.AttachUVE(*bytecode));
    EXPECT_TRUE(debugger.SetBreakpointUVE(20U, true));

    const ScriptDebugPresentationSnapshotUVE snapshot = mapping.BuildPresentationUVE(debugger);
    EXPECT_TRUE(snapshot.available);
    EXPECT_EQ(snapshot.entries.size(), 2U);
    EXPECT_EQ(snapshot.entries[0].nodeId, 10U);
    EXPECT_FALSE(snapshot.entries[0].hasBreakpoint);
    EXPECT_EQ(snapshot.entries[1].nodeId, 20U);
    EXPECT_TRUE(snapshot.entries[1].hasBreakpoint);

    EXPECT_EQ(snapshot.watches.size(), 2U);
    EXPECT_EQ(snapshot.watches[0].watchId, "w1");
    EXPECT_EQ(snapshot.watches[0].expression, "player.health");
    EXPECT_TRUE(snapshot.watches[0].valid);
    EXPECT_TRUE(snapshot.trace.empty());
    EXPECT_FALSE(snapshot.traceTruncated);

    ASSERT_EQ(debugger.StepUVE().trace.size(), 1U);
    const ScriptDebugPresentationSnapshotUVE stepped = mapping.BuildPresentationUVE(debugger);
    ASSERT_EQ(stepped.trace.size(), 1U);
    EXPECT_EQ(stepped.trace.front().kind, ScriptVmTraceEventKindUVE::NodeExecuted);
    EXPECT_EQ(stepped.trace.front().sourceNodeId, 10U);
    EXPECT_FALSE(stepped.traceTruncated);
}

TEST(ScriptSourceMappingUVETest, WatchManagement_EnforcesCapacityAndUniqueness) {
    ScriptGraphUVE graph;
    ScriptSourceMappingUVE mapping(graph);

    EXPECT_TRUE(mapping.AddWatchUVE("a", "x"));
    EXPECT_FALSE(mapping.AddWatchUVE("a", "y")); // duplicate ID
    EXPECT_FALSE(mapping.AddWatchUVE("", "z"));   // empty ID
    EXPECT_FALSE(mapping.AddWatchUVE("b", ""));   // empty expression

    EXPECT_TRUE(mapping.RemoveWatchUVE("a"));
    EXPECT_FALSE(mapping.RemoveWatchUVE("nonexistent"));
}

} // namespace UVE::Scripting
