// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/input/mobile_gesture_system_uve.h"
#include "uve/input/mobile_input_system_uve.h"

#include <gtest/gtest.h>

namespace UVE::Input::Tests {

TEST(MobileGestureSystemUVETest, ConsumesMobileSnapshotsAndPublishesTapOnRelease) {
    MobileInputSystemUVE mobileInput;
    MobileGestureSystemUVE gestureSystem(mobileInput);

    mobileInput.SetTouchStateUVE(0U, true, 7U, Math::Vector2UVE{10.0F, 20.0F}, 0.5F);
    mobileInput.UpdateUVE();
    gestureSystem.UpdateUVE(0.016F);
    EXPECT_EQ(gestureSystem.GetLastReportUVE().count, 0U);

    mobileInput.SetTouchStateUVE(0U, false, 0U, Math::Vector2UVE{}, 0.0F);
    mobileInput.UpdateUVE();
    gestureSystem.UpdateUVE(0.016F);

    const MobileGestureReportUVE report = gestureSystem.GetLastReportUVE();
    ASSERT_EQ(report.count, 1U);
    EXPECT_EQ(report.events[0].type, MobileGestureTypeUVE::Tap);
    EXPECT_EQ(report.events[0].touchIdentifier, 7U);
    EXPECT_EQ(report.events[0].startPosition, (Math::Vector2UVE{10.0F, 20.0F}));
}

TEST(MobileGestureSystemUVETest, ResetClearsLatestReportAndRecognizerState) {
    MobileInputSystemUVE mobileInput;
    MobileGestureSystemUVE gestureSystem(mobileInput);

    mobileInput.SetTouchStateUVE(0U, true, 9U, Math::Vector2UVE{1.0F, 2.0F}, 0.5F);
    mobileInput.UpdateUVE();
    gestureSystem.UpdateUVE(0.016F);
    gestureSystem.ResetUVE();

    EXPECT_EQ(gestureSystem.GetLastReportUVE(), MobileGestureReportUVE{});
}

} // namespace UVE::Input::Tests
