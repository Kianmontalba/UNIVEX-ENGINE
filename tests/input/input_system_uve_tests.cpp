//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/input/input_system_uve.h"

#include <gtest/gtest.h>

#include "uve/events/event_system_uve.h"
#include "uve/input/input_action_triggered_event_uve.h"

namespace UVE::Input::Tests {
namespace {

constexpr float kEpsilon = 1e-5F;

class InputSystemUVETest : public ::testing::Test {
protected:
    Events::EventSystemUVE eventSystem;
    InputSystemUVE inputSystem{eventSystem};
};

TEST_F(InputSystemUVETest, SetKeyStateUVE_IsKeyDownUVE_RoundTrips) {
    EXPECT_FALSE(inputSystem.IsKeyDownUVE(KeyCodeUVE::A));

    inputSystem.SetKeyStateUVE(KeyCodeUVE::A, true);
    inputSystem.UpdateUVE();
    EXPECT_TRUE(inputSystem.IsKeyDownUVE(KeyCodeUVE::A));

    inputSystem.SetKeyStateUVE(KeyCodeUVE::A, false);
    inputSystem.UpdateUVE();
    EXPECT_FALSE(inputSystem.IsKeyDownUVE(KeyCodeUVE::A));
}

TEST_F(InputSystemUVETest, KeyEdgeDetection_PressedHeldReleasedNotHeld) {
    inputSystem.SetKeyStateUVE(KeyCodeUVE::Space, true);
    inputSystem.UpdateUVE();
    EXPECT_TRUE(inputSystem.WasKeyPressedThisFrameUVE(KeyCodeUVE::Space));
    EXPECT_FALSE(inputSystem.WasKeyReleasedThisFrameUVE(KeyCodeUVE::Space));
    EXPECT_TRUE(inputSystem.IsKeyDownUVE(KeyCodeUVE::Space));

    inputSystem.UpdateUVE(); // held, no new Set call
    EXPECT_FALSE(inputSystem.WasKeyPressedThisFrameUVE(KeyCodeUVE::Space));
    EXPECT_TRUE(inputSystem.IsKeyDownUVE(KeyCodeUVE::Space));

    inputSystem.SetKeyStateUVE(KeyCodeUVE::Space, false);
    inputSystem.UpdateUVE();
    EXPECT_TRUE(inputSystem.WasKeyReleasedThisFrameUVE(KeyCodeUVE::Space));
    EXPECT_FALSE(inputSystem.IsKeyDownUVE(KeyCodeUVE::Space));

    inputSystem.UpdateUVE(); // not held, no new Set call
    EXPECT_FALSE(inputSystem.WasKeyReleasedThisFrameUVE(KeyCodeUVE::Space));
}

TEST_F(InputSystemUVETest, MouseButtonEdgeDetection_PressedHeldReleased) {
    inputSystem.SetMouseButtonStateUVE(MouseButtonUVE::Left, true);
    inputSystem.UpdateUVE();
    EXPECT_TRUE(inputSystem.WasMouseButtonPressedThisFrameUVE(MouseButtonUVE::Left));
    EXPECT_TRUE(inputSystem.IsMouseButtonDownUVE(MouseButtonUVE::Left));

    inputSystem.UpdateUVE();
    EXPECT_FALSE(inputSystem.WasMouseButtonPressedThisFrameUVE(MouseButtonUVE::Left));

    inputSystem.SetMouseButtonStateUVE(MouseButtonUVE::Left, false);
    inputSystem.UpdateUVE();
    EXPECT_TRUE(inputSystem.WasMouseButtonReleasedThisFrameUVE(MouseButtonUVE::Left));
    EXPECT_FALSE(inputSystem.IsMouseButtonDownUVE(MouseButtonUVE::Left));
}

TEST_F(InputSystemUVETest, MousePositionAndDelta_ComputedAcrossUpdateUVE) {
    inputSystem.SetMousePositionUVE(Math::Vector2UVE{10.0F, 20.0F});
    inputSystem.UpdateUVE();
    EXPECT_NEAR(inputSystem.GetMousePositionUVE().x, 10.0F, kEpsilon);
    EXPECT_NEAR(inputSystem.GetMousePositionUVE().y, 20.0F, kEpsilon);
    EXPECT_NEAR(inputSystem.GetMouseDeltaUVE().x, 10.0F, kEpsilon); // moved from default (0,0)
    EXPECT_NEAR(inputSystem.GetMouseDeltaUVE().y, 20.0F, kEpsilon);

    inputSystem.SetMousePositionUVE(Math::Vector2UVE{13.0F, 15.0F});
    inputSystem.UpdateUVE();
    EXPECT_NEAR(inputSystem.GetMouseDeltaUVE().x, 3.0F, kEpsilon);
    EXPECT_NEAR(inputSystem.GetMouseDeltaUVE().y, -5.0F, kEpsilon);

    inputSystem.UpdateUVE(); // no movement this frame
    EXPECT_NEAR(inputSystem.GetMouseDeltaUVE().x, 0.0F, kEpsilon);
    EXPECT_NEAR(inputSystem.GetMouseDeltaUVE().y, 0.0F, kEpsilon);
}

TEST_F(InputSystemUVETest, ScrollDelta_AccumulatesThenResetsAfterUpdateUVE) {
    inputSystem.SetMouseScrollDeltaUVE(1.0F);
    inputSystem.SetMouseScrollDeltaUVE(0.5F);
    inputSystem.UpdateUVE();
    EXPECT_NEAR(inputSystem.GetMouseScrollDeltaUVE(), 1.5F, kEpsilon);

    inputSystem.UpdateUVE(); // no new scroll input
    EXPECT_NEAR(inputSystem.GetMouseScrollDeltaUVE(), 0.0F, kEpsilon);
}

TEST_F(InputSystemUVETest, ButtonAction_SingleBinding_TriggeredHeldReleased) {
    inputSystem.RegisterActionUVE(
        InputActionUVE{"Jump", InputActionTypeUVE::Button, {KeyBindingUVE(KeyCodeUVE::Space)}, {}});

    inputSystem.SetKeyStateUVE(KeyCodeUVE::Space, true);
    inputSystem.UpdateUVE();
    EXPECT_TRUE(inputSystem.IsActionTriggeredUVE("Jump"));
    EXPECT_TRUE(inputSystem.IsActionHeldUVE("Jump"));
    EXPECT_FALSE(inputSystem.IsActionReleasedUVE("Jump"));

    inputSystem.UpdateUVE(); // still held
    EXPECT_FALSE(inputSystem.IsActionTriggeredUVE("Jump"));
    EXPECT_TRUE(inputSystem.IsActionHeldUVE("Jump"));

    inputSystem.SetKeyStateUVE(KeyCodeUVE::Space, false);
    inputSystem.UpdateUVE();
    EXPECT_FALSE(inputSystem.IsActionHeldUVE("Jump"));
    EXPECT_TRUE(inputSystem.IsActionReleasedUVE("Jump"));
}

TEST_F(InputSystemUVETest, ButtonAction_MultiBindingOr_DoesNotRetriggerOnSecondKey) {
    inputSystem.RegisterActionUVE(InputActionUVE{"Jump",
                                                   InputActionTypeUVE::Button,
                                                   {KeyBindingUVE(KeyCodeUVE::Space), KeyBindingUVE(KeyCodeUVE::Enter)},
                                                   {}});

    inputSystem.SetKeyStateUVE(KeyCodeUVE::Space, true);
    inputSystem.UpdateUVE();
    EXPECT_TRUE(inputSystem.IsActionTriggeredUVE("Jump"));

    // Enter goes down too while Space is still held — must not re-trigger.
    inputSystem.SetKeyStateUVE(KeyCodeUVE::Enter, true);
    inputSystem.UpdateUVE();
    EXPECT_FALSE(inputSystem.IsActionTriggeredUVE("Jump"));
    EXPECT_TRUE(inputSystem.IsActionHeldUVE("Jump"));
}

TEST_F(InputSystemUVETest, AxisAction_ComputesPositiveNegativeBothNeither) {
    inputSystem.RegisterActionUVE(InputActionUVE{"MoveHorizontal",
                                                   InputActionTypeUVE::Axis1D,
                                                   {KeyBindingUVE(KeyCodeUVE::D)},
                                                   {KeyBindingUVE(KeyCodeUVE::A)}});

    inputSystem.UpdateUVE();
    EXPECT_NEAR(inputSystem.GetAxisValueUVE("MoveHorizontal"), 0.0F, kEpsilon);

    inputSystem.SetKeyStateUVE(KeyCodeUVE::D, true);
    inputSystem.UpdateUVE();
    EXPECT_NEAR(inputSystem.GetAxisValueUVE("MoveHorizontal"), 1.0F, kEpsilon);

    inputSystem.SetKeyStateUVE(KeyCodeUVE::D, false);
    inputSystem.SetKeyStateUVE(KeyCodeUVE::A, true);
    inputSystem.UpdateUVE();
    EXPECT_NEAR(inputSystem.GetAxisValueUVE("MoveHorizontal"), -1.0F, kEpsilon);

    inputSystem.SetKeyStateUVE(KeyCodeUVE::D, true); // both held
    inputSystem.UpdateUVE();
    EXPECT_NEAR(inputSystem.GetAxisValueUVE("MoveHorizontal"), 0.0F, kEpsilon);
}

TEST_F(InputSystemUVETest, UnregisteredActionName_ReturnsFalseOrZeroWithoutAsserting) {
    EXPECT_FALSE(inputSystem.IsActionTriggeredUVE("DoesNotExist"));
    EXPECT_FALSE(inputSystem.IsActionHeldUVE("DoesNotExist"));
    EXPECT_FALSE(inputSystem.IsActionReleasedUVE("DoesNotExist"));
    EXPECT_NEAR(inputSystem.GetAxisValueUVE("DoesNotExist"), 0.0F, kEpsilon);
}

TEST_F(InputSystemUVETest, UnregisterActionUVE_RemovesActionAndReportsWhetherItExisted) {
    inputSystem.RegisterActionUVE(
        InputActionUVE{"Jump", InputActionTypeUVE::Button, {KeyBindingUVE(KeyCodeUVE::Space)}, {}});

    EXPECT_TRUE(inputSystem.UnregisterActionUVE("Jump"));
    EXPECT_FALSE(inputSystem.UnregisterActionUVE("Jump")); // already gone

    inputSystem.SetKeyStateUVE(KeyCodeUVE::Space, true);
    inputSystem.UpdateUVE();
    EXPECT_FALSE(inputSystem.IsActionTriggeredUVE("Jump"));
    EXPECT_FALSE(inputSystem.IsActionHeldUVE("Jump"));
}

TEST_F(InputSystemUVETest, ButtonActionTriggered_QueuesAndDeliversInputActionTriggeredEventUVE) {
    std::string receivedActionName;
    InputActionTypeUVE receivedType = InputActionTypeUVE::Axis1D;
    float receivedAxisValue = -1.0F;
    int receivedCount = 0;
    eventSystem.Subscribe<InputActionTriggeredEventUVE>(
        [&](const InputActionTriggeredEventUVE& event) {
            receivedActionName = event.actionName;
            receivedType = event.type;
            receivedAxisValue = event.axisValue;
            ++receivedCount;
        });

    inputSystem.RegisterActionUVE(
        InputActionUVE{"Jump", InputActionTypeUVE::Button, {KeyBindingUVE(KeyCodeUVE::Space)}, {}});

    inputSystem.SetKeyStateUVE(KeyCodeUVE::Space, true);
    inputSystem.UpdateUVE();
    ASSERT_EQ(receivedCount, 0); // queued, not yet dispatched
    eventSystem.DispatchQueuedUVE();

    EXPECT_EQ(receivedCount, 1);
    EXPECT_EQ(receivedActionName, "Jump");
    EXPECT_EQ(receivedType, InputActionTypeUVE::Button);
    EXPECT_NEAR(receivedAxisValue, 0.0F, kEpsilon);
}

TEST_F(InputSystemUVETest, AxisActionValueChange_DoesNotQueueAnEvent) {
    int receivedCount = 0;
    eventSystem.Subscribe<InputActionTriggeredEventUVE>(
        [&receivedCount](const InputActionTriggeredEventUVE&) { ++receivedCount; });

    inputSystem.RegisterActionUVE(InputActionUVE{"MoveHorizontal",
                                                   InputActionTypeUVE::Axis1D,
                                                   {KeyBindingUVE(KeyCodeUVE::D)},
                                                   {KeyBindingUVE(KeyCodeUVE::A)}});

    inputSystem.SetKeyStateUVE(KeyCodeUVE::D, true);
    inputSystem.UpdateUVE();
    eventSystem.DispatchQueuedUVE();

    EXPECT_EQ(receivedCount, 0);
}

} // namespace
} // namespace UVE::Input::Tests
