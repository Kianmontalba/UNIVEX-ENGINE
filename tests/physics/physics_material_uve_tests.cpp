// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/physics/physics_material_uve.h"

#include <gtest/gtest.h>

#include "uve/scene/components/collider_component_uve.h"

namespace UVE::Physics::Tests {
namespace {

constexpr float kEpsilon = 1e-5F;

TEST(PhysicsMaterialUVETest, CombineMaterialsUVE_KnownValues_AverageCorrectly) {
    constexpr PhysicsMaterialUVE a{0.2F, 0.4F, 1.0F};
    constexpr PhysicsMaterialUVE b{0.6F, 0.8F, 3.0F};

    constexpr PhysicsMaterialUVE combined = CombineMaterialsUVE(a, b);

    EXPECT_NEAR(combined.friction, 0.4F, kEpsilon);
    EXPECT_NEAR(combined.restitution, 0.6F, kEpsilon);
    EXPECT_NEAR(combined.density, 2.0F, kEpsilon);
}

TEST(PhysicsMaterialUVETest, CombineMaterialsUVE_BothZero_StaysZero) {
    constexpr PhysicsMaterialUVE a{0.0F, 0.0F, 0.0F};
    constexpr PhysicsMaterialUVE b{0.0F, 0.0F, 0.0F};

    constexpr PhysicsMaterialUVE combined = CombineMaterialsUVE(a, b);

    EXPECT_NEAR(combined.friction, 0.0F, kEpsilon);
    EXPECT_NEAR(combined.restitution, 0.0F, kEpsilon);
    EXPECT_NEAR(combined.density, 0.0F, kEpsilon);
}

TEST(PhysicsMaterialUVETest, MaterialOfUVE_InRangeValues_PassThroughUnchanged) {
    Scene::ColliderComponentUVE collider;
    collider.friction = 0.3F;
    collider.restitution = 0.7F;
    collider.density = 2.5F;

    const PhysicsMaterialUVE material = MaterialOfUVE(collider);

    EXPECT_NEAR(material.friction, 0.3F, kEpsilon);
    EXPECT_NEAR(material.restitution, 0.7F, kEpsilon);
    EXPECT_NEAR(material.density, 2.5F, kEpsilon);
}

TEST(PhysicsMaterialUVETest, MaterialOfUVE_OutOfRangeRestitution_ClampsToOne) {
    Scene::ColliderComponentUVE collider;
    collider.restitution = 5.0F;

    EXPECT_NEAR(MaterialOfUVE(collider).restitution, 1.0F, kEpsilon);
}

TEST(PhysicsMaterialUVETest, MaterialOfUVE_NegativeFriction_ClampsToZero) {
    Scene::ColliderComponentUVE collider;
    collider.friction = -1.0F;

    EXPECT_NEAR(MaterialOfUVE(collider).friction, 0.0F, kEpsilon);
}

} // namespace
} // namespace UVE::Physics::Tests
