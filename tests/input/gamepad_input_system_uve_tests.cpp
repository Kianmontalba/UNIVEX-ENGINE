// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/input/gamepad_input_system_uve.h"
#include "uve/input/input_frame_counter_uve.h"

#include <cmath>
#include <limits>

#include <gtest/gtest.h>

namespace UVE::Input::Tests {
namespace {

constexpr float kEpsilon = 1e-5F;

TEST(InputFrameCounterUVETest, AdvanceInputFrameNumberUVE_SaturatesWithoutWrap) {
    std::uint64_t frameNumber = std::numeric_limits<std::uint64_t>::max() - 1U;
    AdvanceInputFrameNumberUVE(frameNumber);
    EXPECT_EQ(frameNumber, std::numeric_limits<std::uint64_t>::max());
    AdvanceInputFrameNumberUVE(frameNumber);
    EXPECT_EQ(frameNumber, std::numeric_limits<std::uint64_t>::max());
}

TEST(GamepadInputSystemUVETest, EvaluateGamepadConnectionTransitionUVE_ClassifiesEdges) {
    GamepadStateSnapshotUVE previous{};
    GamepadStateSnapshotUVE current{};
    current.connected = true;
    GamepadConnectionTransitionUVE transition = GamepadConnectionTransitionUVE::Disconnected;
    ASSERT_TRUE(EvaluateGamepadConnectionTransitionUVE(previous, current, transition));
    EXPECT_EQ(transition, GamepadConnectionTransitionUVE::Connected);

    previous = current;
    ASSERT_TRUE(EvaluateGamepadConnectionTransitionUVE(previous, current, transition));
    EXPECT_EQ(transition, GamepadConnectionTransitionUVE::None);

    current.connected = false;
    ASSERT_TRUE(EvaluateGamepadConnectionTransitionUVE(previous, current, transition));
    EXPECT_EQ(transition, GamepadConnectionTransitionUVE::Disconnected);
}

TEST(GamepadInputSystemUVETest, EvaluateGamepadConnectionTransitionUVE_RejectsInvalidAxesAtomically) {
    GamepadStateSnapshotUVE previous{};
    GamepadStateSnapshotUVE current{};
    current.connected = true;
    current.axes[static_cast<std::size_t>(GamepadAxisUVE::LeftX)] =
        std::numeric_limits<float>::quiet_NaN();
    GamepadConnectionTransitionUVE transition = GamepadConnectionTransitionUVE::Disconnected;
    EXPECT_FALSE(EvaluateGamepadConnectionTransitionUVE(previous, current, transition));
    EXPECT_EQ(transition, GamepadConnectionTransitionUVE::Disconnected);

    current.axes[static_cast<std::size_t>(GamepadAxisUVE::LeftX)] = 2.0F;
    EXPECT_FALSE(EvaluateGamepadConnectionTransitionUVE(previous, current, transition));
    EXPECT_EQ(transition, GamepadConnectionTransitionUVE::Disconnected);
}

TEST(GamepadInputSystemUVETest, UpdateUVE_NormalizesFiniteAxesAndCopiesSnapshot) {
    GamepadInputSystemUVE inputSystem(0.2F);

    inputSystem.SetConnectedUVE(0U, true);
    inputSystem.SetAxisStateUVE(0U, GamepadAxisUVE::LeftX, 0.1F);
    inputSystem.SetAxisStateUVE(0U, GamepadAxisUVE::LeftY, 0.6F);
    inputSystem.SetAxisStateUVE(0U, GamepadAxisUVE::RightX, -1.0F);
    inputSystem.SetAxisStateUVE(0U, GamepadAxisUVE::RightY, 2.0F);
    inputSystem.SetAxisStateUVE(0U, GamepadAxisUVE::LeftTrigger,
                                std::numeric_limits<float>::quiet_NaN());
    inputSystem.UpdateUVE();

    const GamepadStateSnapshotUVE snapshot = inputSystem.GetSnapshotUVE(0U);
    EXPECT_TRUE(snapshot.connected);
    EXPECT_EQ(snapshot.frameNumber, 1U);
    EXPECT_NEAR(snapshot.axes[static_cast<std::size_t>(GamepadAxisUVE::LeftX)], 0.0F, kEpsilon);
    EXPECT_NEAR(snapshot.axes[static_cast<std::size_t>(GamepadAxisUVE::LeftY)], 0.5F, kEpsilon);
    EXPECT_NEAR(snapshot.axes[static_cast<std::size_t>(GamepadAxisUVE::RightX)], -1.0F, kEpsilon);
    EXPECT_NEAR(snapshot.axes[static_cast<std::size_t>(GamepadAxisUVE::RightY)], 1.0F, kEpsilon);
    EXPECT_NEAR(snapshot.axes[static_cast<std::size_t>(GamepadAxisUVE::LeftTrigger)], 0.0F, kEpsilon);

    inputSystem.SetAxisStateUVE(0U, GamepadAxisUVE::LeftY, -0.2F);
    EXPECT_NEAR(inputSystem.GetSnapshotUVE(0U).axes[static_cast<std::size_t>(GamepadAxisUVE::LeftY)], 0.5F,
                kEpsilon);
    inputSystem.UpdateUVE();
    EXPECT_NEAR(inputSystem.GetAxisValueUVE(0U, GamepadAxisUVE::LeftY), 0.0F, kEpsilon);
    EXPECT_EQ(inputSystem.GetPreviousSnapshotUVE(0U), snapshot);
    EXPECT_EQ(inputSystem.GetDeadZoneUVE(), 0.2F);
}

TEST(GamepadInputSystemUVETest, ButtonEdges_AreDeterministicAcrossCopiedFrames) {
    GamepadInputSystemUVE inputSystem;
    inputSystem.SetConnectedUVE(1U, true);
    inputSystem.SetButtonStateUVE(1U, GamepadButtonUVE::South, true);
    inputSystem.UpdateUVE();

    EXPECT_TRUE(inputSystem.IsButtonDownUVE(1U, GamepadButtonUVE::South));
    EXPECT_TRUE(inputSystem.WasButtonPressedThisFrameUVE(1U, GamepadButtonUVE::South));
    EXPECT_FALSE(inputSystem.WasButtonReleasedThisFrameUVE(1U, GamepadButtonUVE::South));

    inputSystem.UpdateUVE();
    EXPECT_TRUE(inputSystem.IsButtonDownUVE(1U, GamepadButtonUVE::South));
    EXPECT_FALSE(inputSystem.WasButtonPressedThisFrameUVE(1U, GamepadButtonUVE::South));

    inputSystem.SetButtonStateUVE(1U, GamepadButtonUVE::South, false);
    inputSystem.UpdateUVE();
    EXPECT_FALSE(inputSystem.IsButtonDownUVE(1U, GamepadButtonUVE::South));
    EXPECT_TRUE(inputSystem.WasButtonReleasedThisFrameUVE(1U, GamepadButtonUVE::South));
    EXPECT_EQ(inputSystem.GetSnapshotUVE(1U).frameNumber, 3U);
}

TEST(GamepadInputSystemUVETest, Disconnect_ClearsStateAndInvalidIndicesAreSafeNoOps) {
    GamepadInputSystemUVE inputSystem;
    inputSystem.SetConnectedUVE(0U, true);
    inputSystem.SetAxisStateUVE(0U, GamepadAxisUVE::RightTrigger, 0.75F);
    inputSystem.SetButtonStateUVE(0U, GamepadButtonUVE::Start, true);
    inputSystem.UpdateUVE();

    inputSystem.SetConnectedUVE(0U, false);
    inputSystem.UpdateUVE();
    const GamepadStateSnapshotUVE disconnected = inputSystem.GetSnapshotUVE(0U);
    EXPECT_FALSE(disconnected.connected);
    EXPECT_NEAR(disconnected.axes[static_cast<std::size_t>(GamepadAxisUVE::RightTrigger)], 0.0F, kEpsilon);
    EXPECT_FALSE(disconnected.buttons[static_cast<std::size_t>(GamepadButtonUVE::Start)]);
    // Disconnect clears the current state and emits one release edge for any previously held button.
    EXPECT_TRUE(inputSystem.WasButtonReleasedThisFrameUVE(0U, GamepadButtonUVE::Start));

    inputSystem.SetAxisStateUVE(0U, GamepadAxisUVE::RightTrigger, 1.0F);
    inputSystem.SetButtonStateUVE(0U, GamepadButtonUVE::Start, true);
    inputSystem.SetConnectedUVE(0U, true);
    inputSystem.UpdateUVE();
    const GamepadStateSnapshotUVE reconnected = inputSystem.GetSnapshotUVE(0U);
    EXPECT_TRUE(reconnected.connected);
    EXPECT_NEAR(reconnected.axes[static_cast<std::size_t>(GamepadAxisUVE::RightTrigger)], 0.0F, kEpsilon);
    EXPECT_FALSE(reconnected.buttons[static_cast<std::size_t>(GamepadButtonUVE::Start)]);

    inputSystem.SetConnectedUVE(kMaximumGamepadCountUVE, true);
    inputSystem.SetAxisStateUVE(kMaximumGamepadCountUVE, GamepadAxisUVE::LeftX, 1.0F);
    inputSystem.SetButtonStateUVE(kMaximumGamepadCountUVE, GamepadButtonUVE::South, true);
    inputSystem.UpdateUVE();
    EXPECT_EQ(inputSystem.GetSnapshotUVE(kMaximumGamepadCountUVE), GamepadStateSnapshotUVE{});
    EXPECT_NEAR(inputSystem.GetAxisValueUVE(0U, GamepadAxisUVE::Count), 0.0F, kEpsilon);
    EXPECT_FALSE(inputSystem.IsButtonDownUVE(0U, GamepadButtonUVE::Count));
}

} // namespace
} // namespace UVE::Input::Tests
