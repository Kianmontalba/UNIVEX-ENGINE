//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/core/engine_state_uve.h"

#include <gtest/gtest.h>

// EngineStateUVE's role as a formal, enforced lifecycle (invalid
// transitions triggering UVE_ASSERT) is exercised end-to-end against the
// real EngineCoreUVE in engine_core_uve_tests.cpp — this file covers the
// pure, state-machine-only logic in IsValidTransitionUVE()/ToStringUVE().

namespace UVE::Core::Tests {
namespace {

TEST(EngineStateUVETest, ValidTransitions_FollowLifecycleOrder) {
    EXPECT_TRUE(IsValidTransitionUVE(EngineStateUVE::Uninitialized, EngineStateUVE::Initializing));
    EXPECT_TRUE(IsValidTransitionUVE(EngineStateUVE::Initializing, EngineStateUVE::Running));
    EXPECT_TRUE(IsValidTransitionUVE(EngineStateUVE::Running, EngineStateUVE::ShuttingDown));
    EXPECT_TRUE(IsValidTransitionUVE(EngineStateUVE::ShuttingDown, EngineStateUVE::Shutdown));
}

TEST(EngineStateUVETest, InvalidTransitions_AreRejected) {
    EXPECT_FALSE(IsValidTransitionUVE(EngineStateUVE::Uninitialized, EngineStateUVE::Running));
    EXPECT_FALSE(IsValidTransitionUVE(EngineStateUVE::Running, EngineStateUVE::Initializing));
    EXPECT_FALSE(IsValidTransitionUVE(EngineStateUVE::Shutdown, EngineStateUVE::Uninitialized));
    EXPECT_FALSE(IsValidTransitionUVE(EngineStateUVE::Running, EngineStateUVE::Running));
    EXPECT_FALSE(IsValidTransitionUVE(EngineStateUVE::Shutdown, EngineStateUVE::Initializing));
    EXPECT_FALSE(IsValidTransitionUVE(EngineStateUVE::Uninitialized, EngineStateUVE::ShuttingDown));
}

TEST(EngineStateUVETest, ToStringUVE_ReturnsExpectedNames) {
    EXPECT_EQ(ToStringUVE(EngineStateUVE::Uninitialized), "Uninitialized");
    EXPECT_EQ(ToStringUVE(EngineStateUVE::Initializing), "Initializing");
    EXPECT_EQ(ToStringUVE(EngineStateUVE::Running), "Running");
    EXPECT_EQ(ToStringUVE(EngineStateUVE::ShuttingDown), "ShuttingDown");
    EXPECT_EQ(ToStringUVE(EngineStateUVE::Shutdown), "Shutdown");
}

} // namespace
} // namespace UVE::Core::Tests
