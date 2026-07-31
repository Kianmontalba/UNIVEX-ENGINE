//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/utilities/timer_uve.h"

#include <chrono>
#include <thread>

#include <gtest/gtest.h>

namespace UVE::Utilities::Tests {
namespace {

TEST(TimerUVETest, InitialState_ZeroTotalTime) {
    const TimerUVE timer;
    EXPECT_DOUBLE_EQ(timer.GetTotalTimeUVE(), 0.0);
}

TEST(TimerUVETest, Tick_ProducesPositiveDelta) {
    TimerUVE timer;
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    timer.Tick();
    EXPECT_GT(timer.GetDeltaTimeUVE(), 0.0);
}

TEST(TimerUVETest, Tick_AccumulatesTotalTime) {
    TimerUVE timer;
    for (int i = 0; i < 3; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        timer.Tick();
    }
    EXPECT_GT(timer.GetTotalTimeUVE(), 0.001);
}

TEST(TimerUVETest, DeltaTime_ClampedToConfiguredMax) {
    TimerUVE timer;
    timer.SetMaxDeltaTimeUVE(0.01);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    timer.Tick();
    EXPECT_LE(timer.GetDeltaTimeUVE(), 0.01 + 1e-6);
}

TEST(TimerUVETest, Reset_ZerosTotalTime) {
    TimerUVE timer;
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    timer.Tick();
    ASSERT_GT(timer.GetTotalTimeUVE(), 0.0);

    timer.Reset();
    EXPECT_DOUBLE_EQ(timer.GetTotalTimeUVE(), 0.0);
}

TEST(TimerUVETest, GetDeltaTimeFloatUVE_MatchesDoubleValue) {
    TimerUVE timer;
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    timer.Tick();
    EXPECT_FLOAT_EQ(timer.GetDeltaTimeFloatUVE(), static_cast<float>(timer.GetDeltaTimeUVE()));
}

TEST(TimerUVETest, FixedStep_AccumulatesWholeStepsOnly) {
    TimerUVE timer;
    timer.SetFixedTimestepUVE(0.01); // 100 Hz
    std::this_thread::sleep_for(std::chrono::milliseconds(35));
    timer.Tick();

    const FixedStepResultUVE result = timer.AdvanceFixedStepUVE();
    EXPECT_GE(result.stepsToRun, 2);
    EXPECT_LE(result.stepsToRun, 4);
    EXPECT_GE(result.alpha, 0.0);
    EXPECT_LT(result.alpha, 1.0);
}

TEST(TimerUVETest, FixedStep_CapsStepsAtMaximum) {
    TimerUVE timer;
    timer.SetMaxDeltaTimeUVE(10.0); // disable the per-tick clamp for this test
    timer.SetFixedTimestepUVE(0.001);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    timer.Tick();

    const FixedStepResultUVE result = timer.AdvanceFixedStepUVE();
    EXPECT_LE(result.stepsToRun, 8); // matches TimerUVE::kMaxStepsPerTick
}

} // namespace
} // namespace UVE::Utilities::Tests
