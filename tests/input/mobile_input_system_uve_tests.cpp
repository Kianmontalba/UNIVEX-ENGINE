// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/input/mobile_input_system_uve.h"

#include <gtest/gtest.h>

#include <limits>

namespace UVE::Input::Tests {
namespace {

constexpr float kEpsilon = 1e-5F;

TEST(MobileInputSystemUVETest, EvaluateMobileLifecycleTransitionUVE_ClassifiesPlatformEdges) {
    MobileLifecycleTransitionUVE transition = MobileLifecycleTransitionUVE::Terminated;
    ASSERT_TRUE(EvaluateMobileLifecycleTransitionUVE(
        MobileLifecycleStateUVE::Inactive, MobileLifecycleStateUVE::Active, transition));
    EXPECT_EQ(transition, MobileLifecycleTransitionUVE::Activated);
    ASSERT_TRUE(EvaluateMobileLifecycleTransitionUVE(
        MobileLifecycleStateUVE::Active, MobileLifecycleStateUVE::Suspended, transition));
    EXPECT_EQ(transition, MobileLifecycleTransitionUVE::Suspended);
    ASSERT_TRUE(EvaluateMobileLifecycleTransitionUVE(
        MobileLifecycleStateUVE::Suspended, MobileLifecycleStateUVE::Active, transition));
    EXPECT_EQ(transition, MobileLifecycleTransitionUVE::Resumed);
    ASSERT_TRUE(EvaluateMobileLifecycleTransitionUVE(
        MobileLifecycleStateUVE::Active, MobileLifecycleStateUVE::Inactive, transition));
    EXPECT_EQ(transition, MobileLifecycleTransitionUVE::Deactivated);
    ASSERT_TRUE(EvaluateMobileLifecycleTransitionUVE(
        MobileLifecycleStateUVE::Active, MobileLifecycleStateUVE::Terminated, transition));
    EXPECT_EQ(transition, MobileLifecycleTransitionUVE::Terminated);
    ASSERT_TRUE(EvaluateMobileLifecycleTransitionUVE(
        MobileLifecycleStateUVE::Terminated, MobileLifecycleStateUVE::Active, transition));
    EXPECT_EQ(transition, MobileLifecycleTransitionUVE::Reinitialized);
    ASSERT_TRUE(EvaluateMobileLifecycleTransitionUVE(
        MobileLifecycleStateUVE::Active, MobileLifecycleStateUVE::Active, transition));
    EXPECT_EQ(transition, MobileLifecycleTransitionUVE::None);
}

TEST(MobileInputSystemUVETest, EvaluateMobileLifecycleTransitionUVE_RejectsInvalidStatesAtomically) {
    MobileLifecycleTransitionUVE transition = MobileLifecycleTransitionUVE::Resumed;
    const auto invalidState = static_cast<MobileLifecycleStateUVE>(
        static_cast<std::uint8_t>(MobileLifecycleStateUVE::Count));
    EXPECT_FALSE(EvaluateMobileLifecycleTransitionUVE(
        invalidState, MobileLifecycleStateUVE::Active, transition));
    EXPECT_EQ(transition, MobileLifecycleTransitionUVE::Resumed);
    EXPECT_FALSE(EvaluateMobileLifecycleTransitionUVE(
        MobileLifecycleStateUVE::Active, invalidState, transition));
    EXPECT_EQ(transition, MobileLifecycleTransitionUVE::Resumed);
}

TEST(MobileInputSystemUVETest, EvaluateTouchLifecycleTransitionUVE_ClassifiesSlotChanges) {
    TouchLifecycleTransitionUVE transition = TouchLifecycleTransitionUVE::Replaced;
    const TouchPointStateUVE inactive{};
    const TouchPointStateUVE began{true, 7U, Math::Vector2UVE{1.0F, 2.0F}, {}, 0.5F};
    const TouchPointStateUVE moved{true, 7U, Math::Vector2UVE{2.0F, 2.0F}, Math::Vector2UVE{1.0F, 0.0F}, 0.75F};
    const TouchPointStateUVE replaced{true, 8U, Math::Vector2UVE{2.0F, 2.0F}, {}, 0.75F};
    ASSERT_TRUE(EvaluateTouchLifecycleTransitionUVE(inactive, inactive, transition));
    EXPECT_EQ(transition, TouchLifecycleTransitionUVE::None);
    ASSERT_TRUE(EvaluateTouchLifecycleTransitionUVE(inactive, began, transition));
    EXPECT_EQ(transition, TouchLifecycleTransitionUVE::Began);
    ASSERT_TRUE(EvaluateTouchLifecycleTransitionUVE(began, moved, transition));
    EXPECT_EQ(transition, TouchLifecycleTransitionUVE::Moved);
    ASSERT_TRUE(EvaluateTouchLifecycleTransitionUVE(moved, inactive, transition));
    EXPECT_EQ(transition, TouchLifecycleTransitionUVE::Ended);
    ASSERT_TRUE(EvaluateTouchLifecycleTransitionUVE(moved, replaced, transition));
    EXPECT_EQ(transition, TouchLifecycleTransitionUVE::Replaced);
    ASSERT_TRUE(EvaluateTouchLifecycleTransitionUVE(began, began, transition));
    EXPECT_EQ(transition, TouchLifecycleTransitionUVE::None);
}

TEST(MobileInputSystemUVETest, EvaluateTouchLifecycleTransitionUVE_RejectsInvalidInputsAtomically) {
    TouchLifecycleTransitionUVE transition = TouchLifecycleTransitionUVE::Moved;
    const TouchPointStateUVE invalidIdentifier{true, 0U, Math::Vector2UVE{}, {}, 0.5F};
    const TouchPointStateUVE invalidPressure{true, 1U, Math::Vector2UVE{}, {}, 1.1F};
    EXPECT_FALSE(EvaluateTouchLifecycleTransitionUVE(invalidIdentifier, TouchPointStateUVE{}, transition));
    EXPECT_EQ(transition, TouchLifecycleTransitionUVE::Moved);
    EXPECT_FALSE(EvaluateTouchLifecycleTransitionUVE(invalidPressure, TouchPointStateUVE{}, transition));
    EXPECT_EQ(transition, TouchLifecycleTransitionUVE::Moved);
}

TEST(MobileInputSystemUVETest, SnapshotCopiesTouchDeltaPressureAndGyroscopeDeterministically) {
    MobileInputSystemUVE mobileInput;

    mobileInput.SetTouchStateUVE(0U, true, 42U, Math::Vector2UVE{100.0F, 200.0F}, 1.5F);
    mobileInput.SetGyroscopeRotationRateUVE(Math::Vector3UVE{1.0F, 2.0F, 3.0F});
    mobileInput.UpdateUVE();

    const MobileInputSnapshotUVE first = mobileInput.GetSnapshotUVE();
    EXPECT_EQ(first.frameNumber, 1U);
    EXPECT_TRUE(first.touches[0U].active);
    EXPECT_EQ(first.touches[0U].identifier, 42U);
    EXPECT_NEAR(first.touches[0U].position.x, 100.0F, kEpsilon);
    EXPECT_NEAR(first.touches[0U].position.y, 200.0F, kEpsilon);
    EXPECT_NEAR(first.touches[0U].delta.x, 0.0F, kEpsilon);
    EXPECT_NEAR(first.touches[0U].delta.y, 0.0F, kEpsilon);
    EXPECT_NEAR(first.touches[0U].pressure, 1.0F, kEpsilon);
    EXPECT_NEAR(first.gyroscopeRotationRate.z, 3.0F, kEpsilon);
    EXPECT_EQ(mobileInput.GetPreviousSnapshotUVE().frameNumber, 0U);

    mobileInput.SetTouchStateUVE(0U, true, 42U, Math::Vector2UVE{110.0F, 190.0F}, -0.5F);
    mobileInput.SetGyroscopeRotationRateUVE(Math::Vector3UVE{-1.0F, 0.0F, 2.0F});
    mobileInput.UpdateUVE();

    const MobileInputSnapshotUVE second = mobileInput.GetSnapshotUVE();
    EXPECT_EQ(second.frameNumber, 2U);
    EXPECT_NEAR(second.touches[0U].delta.x, 10.0F, kEpsilon);
    EXPECT_NEAR(second.touches[0U].delta.y, -10.0F, kEpsilon);
    EXPECT_NEAR(second.touches[0U].pressure, 0.0F, kEpsilon);
    EXPECT_NEAR(second.gyroscopeRotationRate.x, -1.0F, kEpsilon);
    EXPECT_EQ(mobileInput.GetPreviousSnapshotUVE().touches[0U].identifier, 42U);
}

TEST(MobileInputSystemUVETest, InvalidAndNonFiniteInputFailsClosedAndReleaseClearsTouch) {
    MobileInputSystemUVE mobileInput;
    const float infinity = std::numeric_limits<float>::infinity();
    const float nan = std::numeric_limits<float>::quiet_NaN();

    mobileInput.SetTouchStateUVE(kMaximumTouchCountUVE, true, 99U, Math::Vector2UVE{1.0F, 2.0F}, 1.0F);
    mobileInput.SetTouchStateUVE(1U, true, 7U, Math::Vector2UVE{infinity, nan}, nan);
    mobileInput.SetGyroscopeRotationRateUVE(Math::Vector3UVE{nan, infinity, -infinity});
    mobileInput.UpdateUVE();

    MobileInputSnapshotUVE active = mobileInput.GetSnapshotUVE();
    EXPECT_TRUE(active.touches[1U].active);
    EXPECT_EQ(active.touches[1U].identifier, 7U);
    EXPECT_NEAR(active.touches[1U].position.x, 0.0F, kEpsilon);
    EXPECT_NEAR(active.touches[1U].position.y, 0.0F, kEpsilon);
    EXPECT_NEAR(active.touches[1U].pressure, 0.0F, kEpsilon);
    EXPECT_NEAR(active.gyroscopeRotationRate.x, 0.0F, kEpsilon);
    EXPECT_NEAR(active.gyroscopeRotationRate.y, 0.0F, kEpsilon);
    EXPECT_NEAR(active.gyroscopeRotationRate.z, 0.0F, kEpsilon);
    EXPECT_FALSE(active.touches[0U].active);

    mobileInput.SetTouchStateUVE(1U, false, 0U, {}, 0.0F);
    mobileInput.UpdateUVE();
    const MobileInputSnapshotUVE released = mobileInput.GetSnapshotUVE();
    EXPECT_FALSE(released.touches[1U].active);
    EXPECT_EQ(released.touches[1U].identifier, 0U);
    EXPECT_NEAR(released.touches[1U].delta.x, 0.0F, kEpsilon);
    EXPECT_NEAR(released.touches[1U].delta.y, 0.0F, kEpsilon);
    EXPECT_TRUE(mobileInput.GetPreviousSnapshotUVE().touches[1U].active);
}

} // namespace
} // namespace UVE::Input::Tests
