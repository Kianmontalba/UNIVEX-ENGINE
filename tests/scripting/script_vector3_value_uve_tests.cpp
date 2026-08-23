// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/scripting/script_vector3_value_uve.h"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>

namespace UVE::Scripting {
namespace {

ScriptVector3ValueUVE ValueUVE(const float x, const float y, const float z) {
    return ScriptVector3ValueUVE{Math::Vector3UVE{x, y, z}};
}

TEST(ScriptVector3ValueUVETest, MakeAddSubtractAndMultiplyUseTypedVectorValues) {
    const ScriptVector3ValueResultUVE made = EvaluateScriptVector3MakeUVE(1.0F, 2.0F, 3.0F);
    ASSERT_TRUE(made.IsAppliedUVE());
    EXPECT_EQ(made.value, ValueUVE(1.0F, 2.0F, 3.0F));

    const ScriptVector3ValueResultUVE added =
        EvaluateScriptVector3AddUVE(ValueUVE(1.0F, 2.0F, 3.0F), ValueUVE(4.0F, 5.0F, 6.0F));
    ASSERT_TRUE(added.IsAppliedUVE());
    EXPECT_EQ(added.value, ValueUVE(5.0F, 7.0F, 9.0F));

    const ScriptVector3ValueResultUVE subtracted =
        EvaluateScriptVector3SubtractUVE(ValueUVE(4.0F, 5.0F, 6.0F), ValueUVE(1.0F, 2.0F, 3.0F));
    ASSERT_TRUE(subtracted.IsAppliedUVE());
    EXPECT_EQ(subtracted.value, ValueUVE(3.0F, 3.0F, 3.0F));

    const ScriptVector3ValueResultUVE multiplied =
        EvaluateScriptVector3MultiplyUVE(ValueUVE(1.0F, -2.0F, 3.0F), 2.5F);
    ASSERT_TRUE(multiplied.IsAppliedUVE());
    EXPECT_EQ(multiplied.value, ValueUVE(2.5F, -5.0F, 7.5F));
}

TEST(ScriptVector3ValueUVETest, DotCrossAndLengthReuseEngineMathSemantics) {
    const ScriptVector3ValueUVE xAxis = ValueUVE(1.0F, 0.0F, 0.0F);
    const ScriptVector3ValueUVE yAxis = ValueUVE(0.0F, 1.0F, 0.0F);

    const ScriptVector3NumberResultUVE dot = EvaluateScriptVector3DotUVE(xAxis, yAxis);
    ASSERT_TRUE(dot.IsAppliedUVE());
    EXPECT_FLOAT_EQ(dot.value, 0.0F);

    const ScriptVector3ValueResultUVE cross = EvaluateScriptVector3CrossUVE(xAxis, yAxis);
    ASSERT_TRUE(cross.IsAppliedUVE());
    EXPECT_EQ(cross.value, ValueUVE(0.0F, 0.0F, 1.0F));

    const ScriptVector3NumberResultUVE length = EvaluateScriptVector3LengthUVE(ValueUVE(3.0F, 4.0F, 0.0F));
    ASSERT_TRUE(length.IsAppliedUVE());
    EXPECT_FLOAT_EQ(length.value, 5.0F);
}

TEST(ScriptVector3ValueUVETest, NormalizeRejectsZeroLengthAndReturnsUnitVector) {
    const ScriptVector3ValueResultUVE zero = EvaluateScriptVector3NormalizeUVE(ValueUVE(0.0F, 0.0F, 0.0F));
    EXPECT_EQ(zero.code, ScriptVector3EvaluationCodeUVE::ZeroLengthNormalize);

    const ScriptVector3ValueResultUVE normalized =
        EvaluateScriptVector3NormalizeUVE(ValueUVE(0.0F, 3.0F, 4.0F));
    ASSERT_TRUE(normalized.IsAppliedUVE());
    EXPECT_FLOAT_EQ(normalized.value.value.x, 0.0F);
    EXPECT_FLOAT_EQ(normalized.value.value.y, 0.6F);
    EXPECT_FLOAT_EQ(normalized.value.value.z, 0.8F);
}

TEST(ScriptVector3ValueUVETest, DirectionUsesFinitePrecisionForLargeEndpointDelta) {
    const float maximum = std::numeric_limits<float>::max();
    const float endpointMagnitude = maximum * 0.99F;
    const ScriptVector3ValueResultUVE direction = EvaluateScriptVector3DirectionUVE(
        ValueUVE(-endpointMagnitude, 0.0F, 0.0F), ValueUVE(endpointMagnitude, 0.0F, 0.0F));

    ASSERT_TRUE(direction.IsAppliedUVE());
    EXPECT_TRUE(std::isfinite(direction.value.value.x));
    EXPECT_TRUE(std::isfinite(direction.value.value.y));
    EXPECT_TRUE(std::isfinite(direction.value.value.z));
    EXPECT_FLOAT_EQ(direction.value.value.x, 1.0F);
    EXPECT_FLOAT_EQ(direction.value.value.y, 0.0F);
    EXPECT_FLOAT_EQ(direction.value.value.z, 0.0F);
}

TEST(ScriptVector3ValueUVETest, LerpUsesFinitePrecisionForLargeEndpointDelta) {
    const float maximum = std::numeric_limits<float>::max();
    const ScriptVector3ValueResultUVE midpoint = EvaluateScriptVector3LerpUVE(
        ValueUVE(-maximum, 2.0F, 0.0F), ValueUVE(maximum, 4.0F, 0.0F), 0.5F);

    ASSERT_TRUE(midpoint.IsAppliedUVE());
    EXPECT_EQ(midpoint.value, ValueUVE(0.0F, 3.0F, 0.0F));
}

TEST(ScriptVector3ValueUVETest, RejectsNonFiniteInputsAndOverflowedOutputs) {
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float infinity = std::numeric_limits<float>::infinity();
    EXPECT_EQ(EvaluateScriptVector3MakeUVE(nan, 0.0F, 0.0F).code,
              ScriptVector3EvaluationCodeUVE::NonFiniteInput);
    EXPECT_EQ(EvaluateScriptVector3MultiplyUVE(ValueUVE(1.0F, 0.0F, 0.0F), infinity).code,
              ScriptVector3EvaluationCodeUVE::NonFiniteInput);
    EXPECT_EQ(EvaluateScriptVector3AddUVE(ValueUVE(std::numeric_limits<float>::max(), 0.0F, 0.0F),
                                          ValueUVE(std::numeric_limits<float>::max(), 0.0F, 0.0F)).code,
              ScriptVector3EvaluationCodeUVE::NonFiniteInput);
    EXPECT_EQ(EvaluateScriptVector3LerpUVE(ValueUVE(0.0F, 0.0F, 0.0F),
                                           ValueUVE(std::numeric_limits<float>::max(), 0.0F, 0.0F),
                                           2.0F).code,
              ScriptVector3EvaluationCodeUVE::NonFiniteInput);
    EXPECT_EQ(EvaluateScriptVector3DotUVE(ValueUVE(infinity, 0.0F, 0.0F), ValueUVE(1.0F, 0.0F, 0.0F)).code,
              ScriptVector3EvaluationCodeUVE::NonFiniteInput);
}

} // namespace
} // namespace UVE::Scripting

