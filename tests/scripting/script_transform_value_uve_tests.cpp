// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/scripting/script_bytecode_uve.h"
#include "uve/scripting/script_builtin_nodes_uve.h"
#include "uve/scripting/script_compiler_ir_uve.h"
#include "uve/scripting/script_graph_uve.h"
#include "uve/scripting/script_transform_value_uve.h"
#include "uve/scripting/script_vm_uve.h"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <numbers>
#include <optional>
#include <variant>

namespace UVE::Scripting {
namespace {

constexpr float kTolerance = 1.0e-4F;

TEST(ScriptTransformValueUVETest, MakeBreakRoundTripPreservesNormalizedTrs) {
    const ScriptTransformValueResultUVE made = EvaluateScriptTransformMakeUVE(
        ScriptVector3ValueUVE{{2.0F, 3.0F, 4.0F}}, ScriptRotationValueUVE{},
        ScriptVector3ValueUVE{{1.0F, 2.0F, 3.0F}});
    ASSERT_TRUE(made.IsAppliedUVE());

    const ScriptTransformValueResultUVE broken = EvaluateScriptTransformBreakUVE(made.value);
    ASSERT_TRUE(broken.IsAppliedUVE());
    EXPECT_FLOAT_EQ(broken.value.position.value.x, 2.0F);
    EXPECT_FLOAT_EQ(broken.value.position.value.y, 3.0F);
    EXPECT_FLOAT_EQ(broken.value.position.value.z, 4.0F);
    EXPECT_FLOAT_EQ(broken.value.scale.value.x, 1.0F);
    EXPECT_FLOAT_EQ(broken.value.scale.value.y, 2.0F);
    EXPECT_FLOAT_EQ(broken.value.scale.value.z, 3.0F);
    EXPECT_FLOAT_EQ(broken.value.rotation.value.w, 1.0F);
}

TEST(ScriptTransformValueUVETest, TransformPointAppliesTranslationScaleAndRotation) {
    const ScriptTransformValueResultUVE translated = EvaluateScriptTransformMakeUVE(
        ScriptVector3ValueUVE{{5.0F, 6.0F, 7.0F}}, ScriptRotationValueUVE{},
        ScriptVector3ValueUVE{{1.0F, 1.0F, 1.0F}});
    ASSERT_TRUE(translated.IsAppliedUVE());
    const ScriptTransformVectorResultUVE translatedPoint = EvaluateScriptTransformPointUVE(
        translated.value, ScriptVector3ValueUVE{{1.0F, 2.0F, 3.0F}});
    ASSERT_TRUE(translatedPoint.IsAppliedUVE());
    EXPECT_NEAR(translatedPoint.value.value.x, 6.0F, kTolerance);
    EXPECT_NEAR(translatedPoint.value.value.y, 8.0F, kTolerance);
    EXPECT_NEAR(translatedPoint.value.value.z, 10.0F, kTolerance);

    const ScriptTransformValueResultUVE scaled = EvaluateScriptTransformMakeUVE(
        ScriptVector3ValueUVE{}, ScriptRotationValueUVE{}, ScriptVector3ValueUVE{{2.0F, 3.0F, 4.0F}});
    ASSERT_TRUE(scaled.IsAppliedUVE());
    const ScriptTransformVectorResultUVE scaledPoint = EvaluateScriptTransformPointUVE(
        scaled.value, ScriptVector3ValueUVE{{1.0F, 1.0F, 1.0F}});
    ASSERT_TRUE(scaledPoint.IsAppliedUVE());
    EXPECT_NEAR(scaledPoint.value.value.x, 2.0F, kTolerance);
    EXPECT_NEAR(scaledPoint.value.value.y, 3.0F, kTolerance);
    EXPECT_NEAR(scaledPoint.value.value.z, 4.0F, kTolerance);

    const ScriptRotationValueResultUVE rotation = EvaluateScriptRotationMakeUVE(
        ScriptVector3ValueUVE{{0.0F, 0.0F, 1.0F}}, std::numbers::pi_v<float> * 0.5F);
    ASSERT_TRUE(rotation.IsAppliedUVE());
    const ScriptTransformValueResultUVE rotated = EvaluateScriptTransformMakeUVE(
        ScriptVector3ValueUVE{}, rotation.value, ScriptVector3ValueUVE{{1.0F, 1.0F, 1.0F}});
    ASSERT_TRUE(rotated.IsAppliedUVE());
    const ScriptTransformVectorResultUVE rotatedPoint = EvaluateScriptTransformPointUVE(
        rotated.value, ScriptVector3ValueUVE{{1.0F, 0.0F, 0.0F}});
    ASSERT_TRUE(rotatedPoint.IsAppliedUVE());
    EXPECT_NEAR(rotatedPoint.value.value.x, 0.0F, kTolerance);
    EXPECT_NEAR(rotatedPoint.value.value.y, 1.0F, kTolerance);
    EXPECT_NEAR(rotatedPoint.value.value.z, 0.0F, kTolerance);
}

TEST(ScriptTransformValueUVETest, TransformPointPreservesFiniteExtremeRotationTranslation) {
    const float maximum = std::numeric_limits<float>::max();
    const float halfAngle = std::numbers::pi_v<float> * 0.125F;
    const ScriptRotationValueResultUVE rotation = EvaluateScriptRotationQuaternionUVE(
        0.0F, 0.0F, std::sin(halfAngle), std::cos(halfAngle));
    ASSERT_TRUE(rotation.IsAppliedUVE());
    const ScriptTransformValueResultUVE transform = EvaluateScriptTransformMakeUVE(
        ScriptVector3ValueUVE{{-maximum, 0.0F, 0.0F}}, rotation.value,
        ScriptVector3ValueUVE{{1.0F, 1.0F, 1.0F}});
    ASSERT_TRUE(transform.IsAppliedUVE());

    const ScriptTransformVectorResultUVE point = EvaluateScriptTransformPointUVE(
        transform.value, ScriptVector3ValueUVE{{maximum, -maximum, 0.0F}});
    ASSERT_TRUE(point.IsAppliedUVE());
    EXPECT_TRUE(std::isfinite(point.value.value.x));
    EXPECT_TRUE(std::isfinite(point.value.value.y));
    EXPECT_TRUE(std::isfinite(point.value.value.z));
    EXPECT_GT(point.value.value.x, 0.0F);
    EXPECT_LT(point.value.value.x, maximum);
    EXPECT_NEAR(point.value.value.y, 0.0F, maximum * 1.0e-6F);
    EXPECT_FLOAT_EQ(point.value.value.z, 0.0F);
}

TEST(ScriptTransformValueUVETest, DegenerateRotationIsRejectedByMakeAndRotate) {
    const ScriptRotationValueUVE degenerate{{0.0F, 0.0F, 0.0F, 0.0F}};
    const ScriptTransformValueResultUVE made = EvaluateScriptTransformMakeUVE(
        ScriptVector3ValueUVE{}, degenerate, ScriptVector3ValueUVE{{1.0F, 1.0F, 1.0F}});
    EXPECT_EQ(made.code, ScriptTransformEvaluationCodeUVE::DegenerateRotation);

    const ScriptTransformValueResultUVE identity = EvaluateScriptTransformMakeUVE(
        ScriptVector3ValueUVE{}, ScriptRotationValueUVE{}, ScriptVector3ValueUVE{{1.0F, 1.0F, 1.0F}});
    ASSERT_TRUE(identity.IsAppliedUVE());
    const ScriptTransformValueResultUVE rotated = EvaluateScriptTransformRotateUVE(identity.value, degenerate);
    EXPECT_EQ(rotated.code, ScriptTransformEvaluationCodeUVE::DegenerateRotation);
}

TEST(ScriptTransformValueUVETest, CompiledMakeToTransformPointExecutesThroughVm) {
    ScriptNodeRegistryUVE registry;
    ASSERT_TRUE(RegisterBuiltInScriptNodesUVE(registry));
    ScriptGraphUVE graph;
    ASSERT_TRUE(graph.AddNodeUVE({1U, "math.transform.make"}));
    ASSERT_TRUE(graph.AddNodeUVE({2U, "math.transform.transform_point"}));
    ASSERT_TRUE(graph.AddLinkUVE(ScriptLinkUVE{{1U, "Transform"}, {2U, "Transform"}}));

    const ScriptIrCompileResultUVE compiled = CompileScriptGraphToIrUVE(graph, registry);
    ASSERT_TRUE(compiled.IsSuccessUVE());
    std::vector<ScriptBytecodeDiagnosticUVE> diagnostics;
    const std::optional<ScriptBytecodeProgramUVE> bytecode = LowerIrToBytecodeUVE(*compiled.program, diagnostics);
    ASSERT_TRUE(bytecode.has_value());
    ASSERT_TRUE(diagnostics.empty());

    ScriptVmExecutionContextUVE context;
    ASSERT_TRUE(context.SetInputUVE(1U, "Position", ScriptVector3ValueUVE{{10.0F, 20.0F, 30.0F}}));
    ASSERT_TRUE(context.SetInputUVE(1U, "Rotation", ScriptRotationValueUVE{}));
    ASSERT_TRUE(context.SetInputUVE(1U, "Scale", ScriptVector3ValueUVE{{2.0F, 2.0F, 2.0F}}));
    ASSERT_TRUE(context.SetInputUVE(2U, "Point", ScriptVector3ValueUVE{{1.0F, 2.0F, 3.0F}}));
    const ScriptVmExecutionResultUVE result = ExecuteScriptBytecodeUVE(*bytecode, context);
    ASSERT_TRUE(result.IsSuccessUVE()) << (result.diagnostics.empty() ? "no diagnostic" : result.diagnostics.front().message);
    const auto output = context.FindOutputUVE(2U, "Result");
    ASSERT_TRUE(output.has_value());
    const ScriptVector3ValueUVE transformed = std::get<ScriptVector3ValueUVE>(*output);
    EXPECT_NEAR(transformed.value.x, 12.0F, kTolerance);
    EXPECT_NEAR(transformed.value.y, 24.0F, kTolerance);
    EXPECT_NEAR(transformed.value.z, 36.0F, kTolerance);
}

} // namespace
} // namespace UVE::Scripting
