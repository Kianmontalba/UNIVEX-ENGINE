// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/scripting/script_bytecode_uve.h"
#include "uve/scripting/script_builtin_nodes_uve.h"
#include "uve/scripting/script_compiler_ir_uve.h"
#include "uve/scripting/script_graph_uve.h"
#include "uve/scripting/script_rotation_value_uve.h"
#include "uve/scripting/script_vm_uve.h"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <numbers>

namespace UVE::Scripting {
namespace {

constexpr float kTolerance = 1.0e-4F;

TEST(ScriptRotationValueUVETest, AxisAngleBreakAndRotateAreDeterministic) {
    const ScriptRotationValueResultUVE rotation = EvaluateScriptRotationMakeUVE(
        ScriptVector3ValueUVE{{0.0F, 0.0F, 1.0F}}, std::numbers::pi_v<float> * 0.5F);
    ASSERT_TRUE(rotation.IsAppliedUVE());

    const ScriptRotationBreakResultUVE broken = EvaluateScriptRotationBreakUVE(rotation.value);
    ASSERT_TRUE(broken.IsAppliedUVE());
    EXPECT_NEAR(broken.axis.value.z, 1.0F, kTolerance);
    EXPECT_NEAR(broken.radians, std::numbers::pi_v<float> * 0.5F, kTolerance);

    const ScriptRotationVectorResultUVE rotated = EvaluateScriptRotationRotateUVE(
        rotation.value, ScriptVector3ValueUVE{{1.0F, 0.0F, 0.0F}});
    ASSERT_TRUE(rotated.IsAppliedUVE());
    EXPECT_NEAR(rotated.value.value.x, 0.0F, kTolerance);
    EXPECT_NEAR(rotated.value.value.y, 1.0F, kTolerance);
    EXPECT_NEAR(rotated.value.value.z, 0.0F, kTolerance);
}

TEST(ScriptRotationValueUVETest, RotatePreservesFiniteExtremeVectorUnderHalfTurn) {
    const float maximum = std::numeric_limits<float>::max();
    const ScriptRotationValueResultUVE rotation = EvaluateScriptRotationQuaternionUVE(
        1.0F, 0.0F, 0.0F, 0.0F);
    ASSERT_TRUE(rotation.IsAppliedUVE());

    const ScriptRotationVectorResultUVE rotated = EvaluateScriptRotationRotateUVE(
        rotation.value, ScriptVector3ValueUVE{{0.0F, maximum, 0.0F}});
    ASSERT_TRUE(rotated.IsAppliedUVE());
    EXPECT_EQ(rotated.value.value, (Math::Vector3UVE{0.0F, -maximum, 0.0F}));
}

TEST(ScriptRotationValueUVETest, EulerQuaternionLookAtAndSlerpProduceUnitRotations) {
    const ScriptRotationValueResultUVE euler = EvaluateScriptRotationEulerUVE(
        ScriptVector3ValueUVE{{0.0F, 0.0F, std::numbers::pi_v<float> * 0.5F}});
    const ScriptRotationValueResultUVE quaternion = EvaluateScriptRotationQuaternionUVE(
        0.0F, 0.0F, std::sin(std::numbers::pi_v<float> * 0.25F),
        std::cos(std::numbers::pi_v<float> * 0.25F));
    ASSERT_TRUE(euler.IsAppliedUVE());
    ASSERT_TRUE(quaternion.IsAppliedUVE());
    EXPECT_NEAR(euler.value.value.z, quaternion.value.value.z, kTolerance);
    EXPECT_NEAR(euler.value.value.w, quaternion.value.value.w, kTolerance);

    const ScriptRotationValueResultUVE lookAt = EvaluateScriptRotationLookAtUVE(
        ScriptVector3ValueUVE{{0.0F, 0.0F, 1.0F}}, ScriptVector3ValueUVE{{0.0F, 1.0F, 0.0F}});
    ASSERT_TRUE(lookAt.IsAppliedUVE());
    const ScriptRotationVectorResultUVE forward = EvaluateScriptRotationRotateUVE(
        lookAt.value, ScriptVector3ValueUVE{{0.0F, 0.0F, 1.0F}});
    ASSERT_TRUE(forward.IsAppliedUVE());
    EXPECT_NEAR(forward.value.value.x, 0.0F, kTolerance);
    EXPECT_NEAR(forward.value.value.y, 0.0F, kTolerance);
    EXPECT_NEAR(forward.value.value.z, 1.0F, kTolerance);

    const ScriptRotationValueResultUVE halfway = EvaluateScriptRotationSlerpUVE(
        ScriptRotationValueUVE{}, quaternion.value, 0.5F);
    ASSERT_TRUE(halfway.IsAppliedUVE());
    EXPECT_NEAR(std::sqrt(Math::LengthSquaredUVE(halfway.value.value)), 1.0F, kTolerance);
}

TEST(ScriptRotationValueUVETest, DegreesAndRadiansRoundTripAndRejectDegenerateInputs) {
    const float degrees = 90.0F;
    const ScriptRotationNumberResultUVE radians = EvaluateScriptRotationRadiansUVE(degrees);
    ASSERT_TRUE(radians.IsAppliedUVE());
    const ScriptRotationNumberResultUVE roundTrip = EvaluateScriptRotationDegreesUVE(radians.value);
    ASSERT_TRUE(roundTrip.IsAppliedUVE());
    EXPECT_NEAR(roundTrip.value, degrees, kTolerance);
    EXPECT_EQ(EvaluateScriptRotationMakeUVE(ScriptVector3ValueUVE{{0.0F, 0.0F, 0.0F}}, 1.0F).code,
              ScriptRotationEvaluationCodeUVE::DegenerateInput);
    EXPECT_EQ(EvaluateScriptRotationQuaternionUVE(0.0F, 0.0F, 0.0F, 0.0F).code,
              ScriptRotationEvaluationCodeUVE::DegenerateInput);
}

TEST(ScriptRotationValueUVETest, CompiledMakeToRotateVectorExecutesThroughVm) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "math.rotation.make"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "math.rotation.rotate"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{1U, "Rotation"}, {2U, "Rotation"}}));

    const ScriptIrCompileResultUVE compiled = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(compiled.IsSuccessUVE());
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    const std::optional<ScriptBytecodeProgramUVE> bytecode = LowerIrToBytecodeUVE(*compiled.program, diagnostics);
    ASSERT_TRUE(bytecode.has_value());
    ASSERT_TRUE(diagnostics.empty());

    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(1U, "Axis", ScriptVector3ValueUVE{{0.0F, 0.0F, 1.0F}}));
    ASSERT_TRUE(context.SetInputUVE(1U, "Radians", std::numbers::pi_v<float> * 0.5F));
    ASSERT_TRUE(context.SetInputUVE(2U, "Vector", ScriptVector3ValueUVE{{1.0F, 0.0F, 0.0F}}));
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(*bytecode, context);
    ASSERT_TRUE(result.IsSuccessUVE()) << (result.diagnostics.empty() ? "no diagnostic" : result.diagnostics.front().message);
    const auto output = context.FindOutputUVE(2U, "Result");
    ASSERT_TRUE(output.has_value());
    const ScriptVector3ValueUVE rotated = std::get<ScriptVector3ValueUVE>(*output);
    EXPECT_NEAR(rotated.value.x, 0.0F, kTolerance);
    EXPECT_NEAR(rotated.value.y, 1.0F, kTolerance);
    EXPECT_NEAR(rotated.value.z, 0.0F, kTolerance);
}

} // namespace
} // namespace UVE::Scripting
