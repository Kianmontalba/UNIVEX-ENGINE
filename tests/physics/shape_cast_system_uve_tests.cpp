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
