// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/input/mobile_gesture_recognizer_uve.h"

#include <gtest/gtest.h>

#include <limits>

namespace UVE::Input::Tests {
namespace {

constexpr float kEpsilon = 1e-5F;

TEST(MobileGestureRecognizerUVETest, ClassifiesTouchChordLifecycleAcrossCopiedFrames) {
    constexpr std::size_t kRequiredTouches = 2U;
    MobileInputSnapshotUVE previous{};
    MobileInputSnapshotUVE current{};
    current.touches[0U] = TouchPointStateUVE{true, 11U, Math::Vector2UVE{0.0F, 2.0F}, {}, 1.0F};
    current.touches[3U] = TouchPointStateUVE{true, 22U, Math::Vector2UVE{4.0F, 6.0F}, {}, 1.0F};
    TouchChordLifecycleTransitionUVE transition = TouchChordLifecycleTransitionUVE::Replaced;
    ASSERT_TRUE(EvaluateTouchChordLifecycleTransitionUVE(
        previous, current, kRequiredTouches, transition));
    EXPECT_EQ(transition, TouchChordLifecycleTransitionUVE::Began);

    previous = current;
    current.touches[0U].position.x = 1.0F;
    ASSERT_TRUE(EvaluateTouchChordLifecycleTransitionUVE(
        previous, current, kRequiredTouches, transition));
    EXPECT_EQ(transition, TouchChordLifecycleTransitionUVE::Moved);

    previous = current;
    ASSERT_TRUE(EvaluateTouchChordLifecycleTransitionUVE(
        previous, current, kRequiredTouches, transition));
    EXPECT_EQ(transition, TouchChordLifecycleTransitionUVE::None);

    current.touches[3U].identifier = 33U;
    ASSERT_TRUE(EvaluateTouchChordLifecycleTransitionUVE(
        previous, current, kRequiredTouches, transition));
    EXPECT_EQ(transition, TouchChordLifecycleTransitionUVE::Replaced);

    previous = current;
    current.touches[3U] = {};
    ASSERT_TRUE(EvaluateTouchChordLifecycleTransitionUVE(
        previous, current, kRequiredTouches, transition));
    EXPECT_EQ(transition, TouchChordLifecycleTransitionUVE::Ended);
}

TEST(MobileGestureRecognizerUVETest, RejectsInvalidTouchChordLifecycleAtomically) {
    MobileInputSnapshotUVE previous{};
    MobileInputSnapshotUVE current{};
    current.touches[0U] = TouchPointStateUVE{true, 11U, Math::Vector2UVE{}, {}, 1.0F};
    current.touches[1U] = TouchPointStateUVE{true, 11U, Math::Vector2UVE{1.0F, 1.0F}, {}, 1.0F};
    TouchChordLifecycleTransitionUVE transition = TouchChordLifecycleTransitionUVE::Moved;
    EXPECT_FALSE(EvaluateTouchChordLifecycleTransitionUVE(previous, current, 2U, transition));
    EXPECT_EQ(transition, TouchChordLifecycleTransitionUVE::Moved);

    current.touches[1U].identifier = 22U;
    current.touches[1U].position.x = std::numeric_limits<float>::quiet_NaN();
    EXPECT_FALSE(EvaluateTouchChordLifecycleTransitionUVE(previous, current, 2U, transition));
    EXPECT_EQ(transition, TouchChordLifecycleTransitionUVE::Moved);
    EXPECT_FALSE(EvaluateTouchChordLifecycleTransitionUVE(previous, current, 1U, transition));
}

TEST(MobileGestureRecognizerUVETest, EvaluatesExactTouchChordCentroid) {
    MobileInputSnapshotUVE snapshot{};
    snapshot.touches[0U] = TouchPointStateUVE{true, 11U, Math::Vector2UVE{0.0F, 2.0F}, {}, 1.0F};
    snapshot.touches[3U] = TouchPointStateUVE{true, 22U, Math::Vector2UVE{4.0F, 6.0F}, {}, 1.0F};
    Math::Vector2UVE centroid{9.0F, 9.0F};
    ASSERT_TRUE(EvaluateTouchChordUVE(snapshot, 2U, centroid));
    EXPECT_NEAR(centroid.x, 2.0F, kEpsilon);
    EXPECT_NEAR(centroid.y, 4.0F, kEpsilon);
}

TEST(MobileGestureRecognizerUVETest, RejectsChordCountDuplicatesAndInvalidPositionAtomically) {
    MobileInputSnapshotUVE snapshot{};
    snapshot.touches[0U] = TouchPointStateUVE{true, 11U, Math::Vector2UVE{0.0F, 2.0F}, {}, 1.0F};
    snapshot.touches[1U] = TouchPointStateUVE{true, 11U, Math::Vector2UVE{4.0F, 6.0F}, {}, 1.0F};
    Math::Vector2UVE centroid{9.0F, 9.0F};
    EXPECT_FALSE(EvaluateTouchChordUVE(snapshot, 2U, centroid));
    EXPECT_EQ(centroid, (Math::Vector2UVE{9.0F, 9.0F}));
    snapshot.touches[1U].identifier = 22U;
    snapshot.touches[1U].position.x = std::numeric_limits<float>::quiet_NaN();
    EXPECT_FALSE(EvaluateTouchChordUVE(snapshot, 2U, centroid));
    EXPECT_EQ(centroid, (Math::Vector2UVE{9.0F, 9.0F}));
    EXPECT_FALSE(EvaluateTouchChordUVE(snapshot, 1U, centroid));
}

TEST(MobileGestureRecognizerUVETest, ClassifiesTapOnReleaseAndSuppressesDuplicateFrame) {
    MobileGestureRecognizerUVE recognizer;

    MobileInputSnapshotUVE pressed{};
    pressed.frameNumber = 1U;
    pressed.touches[0U] = TouchPointStateUVE{true, 7U, Math::Vector2UVE{25.0F, 30.0F}, {}, 1.0F};
    EXPECT_EQ(recognizer.ConsumeSnapshotUVE(pressed, 0.016F).count, 0U);

    MobileInputSnapshotUVE released{};
    released.frameNumber = 2U;
    const MobileGestureReportUVE report = recognizer.ConsumeSnapshotUVE(released, 0.016F);
    ASSERT_EQ(report.count, 1U);
    EXPECT_FALSE(report.truncated);
    EXPECT_EQ(report.events[0U].type, MobileGestureTypeUVE::Tap);
    EXPECT_EQ(report.events[0U].touchIdentifier, 7U);
    EXPECT_NEAR(report.events[0U].startPosition.x, 25.0F, kEpsilon);
    EXPECT_NEAR(report.events[0U].endPosition.y, 30.0F, kEpsilon);

    EXPECT_EQ(recognizer.ConsumeSnapshotUVE(released, 0.016F).count, 0U);
}

TEST(MobileGestureRecognizerUVETest, ClassifiesDirectionalSwipeFromStableTouchIdentity) {
    MobileGestureRecognizerUVE recognizer;

    MobileInputSnapshotUVE start{};
    start.frameNumber = 10U;
    start.touches[2U] = TouchPointStateUVE{true, 99U, Math::Vector2UVE{0.0F, 0.0F}, {}, 0.5F};
    EXPECT_EQ(recognizer.ConsumeSnapshotUVE(start, 0.1F).count, 0U);

    MobileInputSnapshotUVE moved{};
    moved.frameNumber = 11U;
    moved.touches[2U] = TouchPointStateUVE{true, 99U, Math::Vector2UVE{60.0F, 10.0F}, {}, 0.5F};
    EXPECT_EQ(recognizer.ConsumeSnapshotUVE(moved, 0.2F).count, 0U);

    MobileInputSnapshotUVE released{};
    released.frameNumber = 12U;
    const MobileGestureReportUVE report = recognizer.ConsumeSnapshotUVE(released, 0.2F);
    ASSERT_EQ(report.count, 1U);
    EXPECT_EQ(report.events[0U].type, MobileGestureTypeUVE::Swipe);
    EXPECT_EQ(report.events[0U].direction, MobileSwipeDirectionUVE::PositiveX);
    EXPECT_EQ(report.events[0U].touchIdentifier, 99U);
    EXPECT_NEAR(report.events[0U].delta.x, 60.0F, kEpsilon);
    EXPECT_NEAR(report.events[0U].delta.y, 10.0F, kEpsilon);
    EXPECT_NEAR(report.events[0U].durationSeconds, 0.2F, kEpsilon);
}

TEST(MobileGestureRecognizerUVETest, RejectsOverflowedReleaseDeltaWithoutPublishingGesture) {
    MobileGestureRecognizerUVE recognizer;
    MobileInputSnapshotUVE pressed{};
    pressed.frameNumber = 30U;
    pressed.touches[0U] = TouchPointStateUVE{
        true, 5U, Math::Vector2UVE{-std::numeric_limits<float>::max(), 0.0F}, {}, 0.0F};
    EXPECT_EQ(recognizer.ConsumeSnapshotUVE(pressed, 0.1F).count, 0U);

    MobileInputSnapshotUVE moved = pressed;
    moved.frameNumber = 31U;
    moved.touches[0U].position = Math::Vector2UVE{std::numeric_limits<float>::max(), 0.0F};
    EXPECT_EQ(recognizer.ConsumeSnapshotUVE(moved, 0.1F).count, 0U);

    MobileInputSnapshotUVE released{};
    released.frameNumber = 32U;
    const MobileGestureReportUVE report = recognizer.ConsumeSnapshotUVE(released, 0.1F);
    EXPECT_EQ(report.count, 0U);
    EXPECT_FALSE(report.truncated);
}

TEST(MobileGestureRecognizerUVETest, RejectsInvalidFrameDeltaAndResetClearsTrackedTouch) {
    MobileGestureRecognizerUVE recognizer;
    MobileInputSnapshotUVE pressed{};
    pressed.frameNumber = 20U;
    pressed.touches[0U] = TouchPointStateUVE{true, 5U, Math::Vector2UVE{1.0F, 1.0F}, {}, 0.0F};
    EXPECT_EQ(recognizer.ConsumeSnapshotUVE(pressed, 0.1F).count, 0U);

    MobileInputSnapshotUVE next = pressed;
    next.frameNumber = 21U;
    next.touches[0U].position = Math::Vector2UVE{100.0F, 0.0F};
    EXPECT_EQ(recognizer.ConsumeSnapshotUVE(next, std::numeric_limits<float>::quiet_NaN()).count, 0U);

    recognizer.ResetUVE();
    MobileInputSnapshotUVE released{};
    released.frameNumber = 22U;
    EXPECT_EQ(recognizer.ConsumeSnapshotUVE(released, 0.1F).count, 0U);
}

} // namespace
} // namespace UVE::Input::Tests
