// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/physics/shape_cast_system_uve.h"

#include <cstdint>
#include <limits>
#include <optional>

#include <gtest/gtest.h>

#include "uve/events/event_system_uve.h"
#include "uve/memory/memory_manager_uve.h"
#include "uve/scene/components/collider_component_uve.h"
#include "uve/scene/components/transform_component_uve.h"
#include "uve/scene/entity_manager_uve.h"
#include "uve/scene/scene_graph_uve.h"

namespace UVE::Physics::Tests {
namespace {

constexpr float kEpsilon = 1e-3F;

class ShapeCastSystemUVETest : public ::testing::Test {
protected:
    Memory::MemoryManagerUVE memoryManager;
    Events::EventSystemUVE eventSystem;
    Scene::EntityManagerUVE entityManager{memoryManager.GetDefaultAllocatorUVE(), eventSystem};
    Scene::SceneGraphUVE sceneGraph;

    Scene::EntityUVE MakeColliderEntityUVE(Math::Vector3UVE position, Math::Vector3UVE halfExtents,
                                            std::uint32_t collisionLayer = 1U, float friction = 0.0F,
                                            float restitution = 0.0F, float density = 1.0F) {
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE local;
        local.localPosition = position;
        sceneGraph.AttachTransformUVE(entityManager, entity, local);
        sceneGraph.UpdateUVE(entityManager);
        Scene::ColliderComponentUVE collider{halfExtents};
        collider.collisionLayer = collisionLayer;
        collider.friction = friction;
        collider.restitution = restitution;
        collider.density = density;
        entityManager.AddComponentUVE<Scene::ColliderComponentUVE>(entity, collider);
        return entity;
    }

    [[nodiscard]] static SphereCastQueryUVE MakeXAxisQueryUVE(float originX = -10.0F,
                                                               float radius = 0.5F,
                                                               float maxDistance = 100.0F) {
        SphereCastQueryUVE query;
        query.ray = Math::RayUVE{Math::Vector3UVE{originX, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 0.0F, 0.0F}};
        query.radius = radius;
        query.maxDistance = maxDistance;
        return query;
    }
};

TEST_F(ShapeCastSystemUVETest, SphereCastUVE_ExpandsAabbAndReportsMovingCenter) {
    const Scene::EntityUVE entity = MakeColliderEntityUVE(Math::Vector3UVE{}, Math::Vector3UVE{1.0F, 1.0F, 1.0F},
                                                           1U, 0.4F, 0.9F, 3.0F);
    SphereCastQueryUVE query = MakeXAxisQueryUVE(-5.0F, 1.0F);

    const std::optional<SphereCastHitUVE> hit = ShapeCastSystemUVE::SphereCastUVE(entityManager, query);

    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->entity, entity);
    EXPECT_NEAR(hit->distance, 3.0F, kEpsilon);
    EXPECT_NEAR(hit->center.x, -2.0F, kEpsilon);
    EXPECT_NEAR(hit->normal.x, -1.0F, kEpsilon);
    EXPECT_NEAR(hit->material.friction, 0.4F, kEpsilon);
    EXPECT_NEAR(hit->material.restitution, 0.9F, kEpsilon);
    EXPECT_NEAR(hit->material.density, 3.0F, kEpsilon);
}

TEST_F(ShapeCastSystemUVETest, SphereCastUVE_MultipleCollidersReturnsClosestDeterministically) {
    MakeColliderEntityUVE(Math::Vector3UVE{5.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});
    const Scene::EntityUVE closer =
        MakeColliderEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});

    const std::optional<SphereCastHitUVE> hit =
        ShapeCastSystemUVE::SphereCastUVE(entityManager, MakeXAxisQueryUVE(-10.0F, 0.5F));

    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->entity, closer);
}

TEST_F(ShapeCastSystemUVETest, SphereCastUVE_LayerAndIgnoreFiltersAreApplied) {
    const Scene::EntityUVE ignored =
        MakeColliderEntityUVE(Math::Vector3UVE{}, Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 1U);
    MakeColliderEntityUVE(Math::Vector3UVE{4.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 2U);

    SphereCastQueryUVE layerQuery = MakeXAxisQueryUVE(-10.0F, 0.5F);
    layerQuery.layerMask = 1U;
    layerQuery.ignoreEntity = ignored;
    EXPECT_FALSE(ShapeCastSystemUVE::SphereCastUVE(entityManager, layerQuery).has_value());

    layerQuery.layerMask = 2U;
    const std::optional<SphereCastHitUVE> hit = ShapeCastSystemUVE::SphereCastUVE(entityManager, layerQuery);
    ASSERT_TRUE(hit.has_value());
    EXPECT_NE(hit->entity, ignored);
}

TEST_F(ShapeCastSystemUVETest, BoxCastUVE_ExpandsTargetByMoverHalfExtentsAndReportsCenter) {
    const Scene::EntityUVE entity = MakeColliderEntityUVE(Math::Vector3UVE{}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});
    BoxCastQueryUVE query;
    query.ray = Math::RayUVE{Math::Vector3UVE{-5.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 0.0F, 0.0F}};
    query.halfExtents = Math::Vector3UVE{0.5F, 0.25F, 0.75F};
    query.maxDistance = 100.0F;

    const std::optional<BoxCastHitUVE> hit = ShapeCastSystemUVE::BoxCastUVE(entityManager, query);
    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->entity, entity);
    EXPECT_NEAR(hit->distance, 3.5F, kEpsilon);
    EXPECT_NEAR(hit->center.x, -1.5F, kEpsilon);
    EXPECT_NEAR(hit->normal.x, -1.0F, kEpsilon);
}

TEST_F(ShapeCastSystemUVETest, BoxCastUVE_ReturnsClosestHitDeterministically) {
    MakeColliderEntityUVE(Math::Vector3UVE{5.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});
    const Scene::EntityUVE closer =
        MakeColliderEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});
    BoxCastQueryUVE query;
    query.ray = Math::RayUVE{Math::Vector3UVE{-10.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 0.0F, 0.0F}};
    query.halfExtents = Math::Vector3UVE{0.5F, 0.5F, 0.5F};
    query.maxDistance = 100.0F;

    const std::optional<BoxCastHitUVE> hit = ShapeCastSystemUVE::BoxCastUVE(entityManager, query);
    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->entity, closer);
}

TEST_F(ShapeCastSystemUVETest, BoxCastUVE_AppliesLayerIgnoreAndMaterialFacts) {
    const Scene::EntityUVE ignored =
        MakeColliderEntityUVE(Math::Vector3UVE{}, Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 1U);
    const Scene::EntityUVE visible = MakeColliderEntityUVE(
        Math::Vector3UVE{4.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 2U, 0.4F, 0.9F, 3.0F);
    BoxCastQueryUVE query;
    query.ray = Math::RayUVE{Math::Vector3UVE{-10.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 0.0F, 0.0F}};
    query.halfExtents = Math::Vector3UVE{0.5F, 0.5F, 0.5F};
    query.maxDistance = 100.0F;
    query.layerMask = 2U;
    query.ignoreEntity = ignored;

    const std::optional<BoxCastHitUVE> hit = ShapeCastSystemUVE::BoxCastUVE(entityManager, query);
    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->entity, visible);
    EXPECT_NEAR(hit->material.friction, 0.4F, kEpsilon);
    EXPECT_NEAR(hit->material.restitution, 0.9F, kEpsilon);
    EXPECT_NEAR(hit->material.density, 3.0F, kEpsilon);
}

TEST_F(ShapeCastSystemUVETest, BoxCastUVE_RejectsInvalidFiniteInputs) {
    MakeColliderEntityUVE(Math::Vector3UVE{}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});
    BoxCastQueryUVE negativeExtents;
    negativeExtents.ray = Math::RayUVE{Math::Vector3UVE{-5.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 0.0F, 0.0F}};
    negativeExtents.halfExtents = Math::Vector3UVE{-1.0F, 0.0F, 0.0F};
    negativeExtents.maxDistance = 100.0F;
    EXPECT_FALSE(ShapeCastSystemUVE::BoxCastUVE(entityManager, negativeExtents).has_value());

    BoxCastQueryUVE zeroDirection = negativeExtents;
    zeroDirection.halfExtents = Math::Vector3UVE{0.5F, 0.5F, 0.5F};
    zeroDirection.ray.direction = Math::Vector3UVE{};
    EXPECT_FALSE(ShapeCastSystemUVE::BoxCastUVE(entityManager, zeroDirection).has_value());

    BoxCastQueryUVE nonFiniteDistance = zeroDirection;
    nonFiniteDistance.ray.direction = Math::Vector3UVE{1.0F, 0.0F, 0.0F};
    nonFiniteDistance.maxDistance = std::numeric_limits<float>::quiet_NaN();
    EXPECT_FALSE(ShapeCastSystemUVE::BoxCastUVE(entityManager, nonFiniteDistance).has_value());
}

TEST_F(ShapeCastSystemUVETest, CapsuleCastUVE_ExpandsTargetByLocalYCapsuleBounds) {
    const Scene::EntityUVE entity = MakeColliderEntityUVE(Math::Vector3UVE{}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});
    CapsuleCastQueryUVE query;
    query.ray = Math::RayUVE{Math::Vector3UVE{-5.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 0.0F, 0.0F}};
    query.radius = 0.5F;
    query.height = 2.0F;
    query.maxDistance = 100.0F;

    const std::optional<CapsuleCastHitUVE> hit = ShapeCastSystemUVE::CapsuleCastUVE(entityManager, query);
    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->entity, entity);
    EXPECT_NEAR(hit->distance, 3.5F, kEpsilon);
    EXPECT_NEAR(hit->center.x, -1.5F, kEpsilon);
    EXPECT_NEAR(hit->normal.x, -1.0F, kEpsilon);
}

TEST_F(ShapeCastSystemUVETest, CapsuleCastUVE_ReturnsClosestHitAndHonorsFilters) {
    const Scene::EntityUVE ignored =
        MakeColliderEntityUVE(Math::Vector3UVE{}, Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 1U);
    const Scene::EntityUVE visible =
        MakeColliderEntityUVE(Math::Vector3UVE{4.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 2U);
    CapsuleCastQueryUVE query;
    query.ray = Math::RayUVE{Math::Vector3UVE{-10.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 0.0F, 0.0F}};
    query.radius = 0.5F;
    query.height = 3.0F;
    query.maxDistance = 100.0F;
    query.layerMask = 2U;
    query.ignoreEntity = ignored;

    const std::optional<CapsuleCastHitUVE> hit = ShapeCastSystemUVE::CapsuleCastUVE(entityManager, query);
    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->entity, visible);
}

TEST_F(ShapeCastSystemUVETest, CapsuleCastUVE_RejectsInvalidDimensionsAndFiniteInputs) {
    MakeColliderEntityUVE(Math::Vector3UVE{}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});
    CapsuleCastQueryUVE tooShort;
    tooShort.ray = Math::RayUVE{Math::Vector3UVE{-5.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 0.0F, 0.0F}};
    tooShort.radius = 1.0F;
    tooShort.height = 1.5F;
    tooShort.maxDistance = 100.0F;
    EXPECT_FALSE(ShapeCastSystemUVE::CapsuleCastUVE(entityManager, tooShort).has_value());

    CapsuleCastQueryUVE nonFinite = tooShort;
    nonFinite.height = 2.0F;
    nonFinite.radius = std::numeric_limits<float>::quiet_NaN();
    EXPECT_FALSE(ShapeCastSystemUVE::CapsuleCastUVE(entityManager, nonFinite).has_value());
}

TEST_F(ShapeCastSystemUVETest, ShapeCasts_SkipNonFiniteExpandedAabbs) {
    MakeColliderEntityUVE(Math::Vector3UVE{1.0e38F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});
    SphereCastQueryUVE sphere = MakeXAxisQueryUVE(0.0F, std::numeric_limits<float>::max(),
                                                   std::numeric_limits<float>::max());
    EXPECT_FALSE(ShapeCastSystemUVE::SphereCastUVE(entityManager, sphere).has_value());

    BoxCastQueryUVE box;
    box.ray = Math::RayUVE{Math::Vector3UVE{}, Math::Vector3UVE{1.0F, 0.0F, 0.0F}};
    box.halfExtents = Math::Vector3UVE{std::numeric_limits<float>::max(), 0.0F, 0.0F};
    box.maxDistance = std::numeric_limits<float>::max();
    EXPECT_FALSE(ShapeCastSystemUVE::BoxCastUVE(entityManager, box).has_value());

    MakeColliderEntityUVE(Math::Vector3UVE{0.0F, 2.0e38F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});
    CapsuleCastQueryUVE capsule;
    capsule.ray = Math::RayUVE{Math::Vector3UVE{}, Math::Vector3UVE{0.0F, 1.0F, 0.0F}};
    capsule.radius = 1.0F;
    capsule.height = std::numeric_limits<float>::max();
    capsule.maxDistance = std::numeric_limits<float>::max();
    EXPECT_FALSE(ShapeCastSystemUVE::CapsuleCastUVE(entityManager, capsule).has_value());
}

TEST_F(ShapeCastSystemUVETest, SphereCastUVE_RejectsInvalidFiniteInputs) {
    MakeColliderEntityUVE(Math::Vector3UVE{}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});

    SphereCastQueryUVE negativeRadius = MakeXAxisQueryUVE();
    negativeRadius.radius = -1.0F;
    EXPECT_FALSE(ShapeCastSystemUVE::SphereCastUVE(entityManager, negativeRadius).has_value());

    SphereCastQueryUVE nonFiniteDistance = MakeXAxisQueryUVE();
    nonFiniteDistance.maxDistance = std::numeric_limits<float>::quiet_NaN();
    EXPECT_FALSE(ShapeCastSystemUVE::SphereCastUVE(entityManager, nonFiniteDistance).has_value());

    SphereCastQueryUVE emptyMask = MakeXAxisQueryUVE();
    emptyMask.layerMask = 0U;
    EXPECT_FALSE(ShapeCastSystemUVE::SphereCastUVE(entityManager, emptyMask).has_value());
}

} // namespace
} // namespace UVE::Physics::Tests
