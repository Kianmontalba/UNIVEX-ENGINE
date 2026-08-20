// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/input/touch_coordinate_transform_uve.h"
#include <gtest/gtest.h>
#include <limits>
namespace UVE::Input::Tests {
namespace {
TEST(TouchCoordinateTransformUVETest, AppliesCardinalTouchOrientations) {
    const Math::Vector2UVE input{0.25F, 0.75F};
    Math::Vector2UVE output;
    ASSERT_TRUE(ApplyTouchOrientationUVE(input, TouchOrientationUVE::Deg0, output));
    EXPECT_FLOAT_EQ(output.x, 0.25F); EXPECT_FLOAT_EQ(output.y, 0.75F);
    ASSERT_TRUE(ApplyTouchOrientationUVE(input, TouchOrientationUVE::Deg90, output));
    EXPECT_FLOAT_EQ(output.x, 0.25F); EXPECT_FLOAT_EQ(output.y, 0.25F);
    ASSERT_TRUE(ApplyTouchOrientationUVE(input, TouchOrientationUVE::Deg180, output));
    EXPECT_FLOAT_EQ(output.x, 0.75F); EXPECT_FLOAT_EQ(output.y, 0.25F);
    ASSERT_TRUE(ApplyTouchOrientationUVE(input, TouchOrientationUVE::Deg270, output));
    EXPECT_FLOAT_EQ(output.x, 0.75F); EXPECT_FLOAT_EQ(output.y, 0.75F);
}
TEST(TouchCoordinateTransformUVETest, RejectsInvalidOrientationInputsAtomically) {
    Math::Vector2UVE output{0.4F, 0.6F};
    EXPECT_FALSE(ApplyTouchOrientationUVE(Math::Vector2UVE{-0.1F, 0.5F}, TouchOrientationUVE::Deg0, output));
    EXPECT_FLOAT_EQ(output.x, 0.4F); EXPECT_FLOAT_EQ(output.y, 0.6F);
    EXPECT_FALSE(ApplyTouchOrientationUVE(Math::Vector2UVE{0.5F, std::numeric_limits<float>::quiet_NaN()}, TouchOrientationUVE::Deg0, output));
    EXPECT_FLOAT_EQ(output.x, 0.4F); EXPECT_FLOAT_EQ(output.y, 0.6F);
}
TEST(TouchCoordinateTransformUVETest, MapsSafeAreaPixelsToNormalizedCoordinates) {
    const TouchCoordinateViewportUVE viewport{1000.0F, 800.0F, 100.0F, 50.0F, 100.0F, 50.0F};
    Math::Vector2UVE normalized{0.0F, 0.0F};
    ASSERT_TRUE(NormalizeTouchCoordinateUVE(Math::Vector2UVE{550.0F, 400.0F}, viewport, normalized));
    EXPECT_FLOAT_EQ(normalized.x, 0.5625F);
    EXPECT_FLOAT_EQ(normalized.y, 0.5F);
}
TEST(TouchCoordinateTransformUVETest, ClampsOutsidePixelsToSafeAreaBounds) {
    const TouchCoordinateViewportUVE viewport{100.0F, 100.0F, 10.0F, 10.0F, 10.0F, 10.0F};
    Math::Vector2UVE normalized{0.0F, 0.0F};
    ASSERT_TRUE(NormalizeTouchCoordinateUVE(Math::Vector2UVE{-20.0F, 200.0F}, viewport, normalized));
    EXPECT_FLOAT_EQ(normalized.x, 0.0F);
    EXPECT_FLOAT_EQ(normalized.y, 1.0F);
}
TEST(TouchCoordinateTransformUVETest, InvalidViewportOrPixelLeavesOutputUnchanged) {
    const TouchCoordinateViewportUVE invalid{100.0F, 100.0F, 60.0F, 0.0F, 50.0F, 0.0F};
    Math::Vector2UVE normalized{0.25F, 0.75F};
    EXPECT_FALSE(NormalizeTouchCoordinateUVE(Math::Vector2UVE{1.0F, 1.0F}, invalid, normalized));
    EXPECT_FLOAT_EQ(normalized.x, 0.25F);
    EXPECT_FLOAT_EQ(normalized.y, 0.75F);
    const TouchCoordinateViewportUVE valid{100.0F, 100.0F, 0.0F, 0.0F, 0.0F, 0.0F};
    EXPECT_FALSE(NormalizeTouchCoordinateUVE(Math::Vector2UVE{std::numeric_limits<float>::quiet_NaN(), 1.0F}, valid, normalized));
    EXPECT_FLOAT_EQ(normalized.x, 0.25F);
    EXPECT_FLOAT_EQ(normalized.y, 0.75F);
}
} // namespace
} // namespace UVE::Input::Tests
