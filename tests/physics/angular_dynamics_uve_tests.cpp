// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/physics/angular_dynamics_uve.h"

#include <cmath>
#include <limits>

#include <gtest/gtest.h>

namespace UVE::Physics::Tests {
namespace {

constexpr float kEpsilon = 1.0e-5F;

TEST(AngularDynamicsUVETest, ComputeBoxInverseInertiaUVE_MatchesDiagonalBoxFormula) {
    const auto inverseInertia = ComputeBoxInverseInertiaUVE(2.0F, Math::Vector3UVE{1.0F, 2.0F, 3.0F});

    ASSERT_TRUE(inverseInertia.has_value());
    EXPECT_NEAR(inverseInertia->x, 3.0F / 26.0F, kEpsilon);
    EXPECT_NEAR(inverseInertia->y, 3.0F / 20.0F, kEpsilon);
    EXPECT_NEAR(inverseInertia->z, 3.0F / 10.0F, kEpsilon);
}

TEST(AngularDynamicsUVETest, ComputeBoxInverseInertiaUVE_ZeroMassReturnsStaticZeroTensor) {
    const auto inverseInertia = ComputeBoxInverseInertiaUVE(0.0F, Math::Vector3UVE{1.0F, 1.0F, 1.0F});

    ASSERT_TRUE(inverseInertia.has_value());
    EXPECT_EQ(*inverseInertia, Math::Vector3UVE{});
}

TEST(AngularDynamicsUVETest, IntegrateAngularVelocityUVE_AppliesTorqueOverFiniteStep) {
    const auto result = IntegrateAngularVelocityUVE(
        Math::Vector3UVE{1.0F, 2.0F, 3.0F}, Math::Vector3UVE{4.0F, 5.0F, 6.0F},
        Math::Vector3UVE{0.5F, 0.25F, 0.1F}, 0.2F);

    ASSERT_TRUE(result.has_value());
    EXPECT_NEAR(result->x, 1.4F, kEpsilon);
    EXPECT_NEAR(result->y, 2.25F, kEpsilon);
    EXPECT_NEAR(result->z, 3.12F, kEpsilon);
}

TEST(AngularDynamicsUVETest, ApplyAngularImpulseUVE_AppliesDiagonalInverseInertia) {
    const auto result = ApplyAngularImpulseUVE(
        Math::Vector3UVE{1.0F, 2.0F, 3.0F}, Math::Vector3UVE{4.0F, 5.0F, 6.0F},
        Math::Vector3UVE{0.5F, 0.25F, 0.1F});

    ASSERT_TRUE(result.has_value());
    EXPECT_NEAR(result->x, 3.0F, kEpsilon);
    EXPECT_NEAR(result->y, 3.25F, kEpsilon);
    EXPECT_NEAR(result->z, 3.6F, kEpsilon);
}

TEST(AngularDynamicsUVETest, AngularHelpers_RejectUnsafeOrNonFiniteInputs) {
    EXPECT_FALSE(ComputeBoxInverseInertiaUVE(-1.0F, Math::Vector3UVE{1.0F, 1.0F, 1.0F}).has_value());
    EXPECT_FALSE(ComputeBoxInverseInertiaUVE(1.0F, Math::Vector3UVE{0.0F, 1.0F, 1.0F}).has_value());
    EXPECT_FALSE(ComputeBoxInverseInertiaUVE(
        std::numeric_limits<float>::quiet_NaN(), Math::Vector3UVE{1.0F, 1.0F, 1.0F}).has_value());
    EXPECT_FALSE(IntegrateAngularVelocityUVE(
        Math::Vector3UVE{}, Math::Vector3UVE{}, Math::Vector3UVE{}, -0.1F).has_value());
    EXPECT_FALSE(ApplyAngularImpulseUVE(
        Math::Vector3UVE{}, Math::Vector3UVE{}, Math::Vector3UVE{-1.0F, 0.0F, 0.0F}).has_value());
    EXPECT_FALSE(ApplyAngularImpulseUVE(
        Math::Vector3UVE{std::numeric_limits<float>::infinity(), 0.0F, 0.0F},
        Math::Vector3UVE{}, Math::Vector3UVE{}).has_value());
}

} // namespace
} // namespace UVE::Physics::Tests
