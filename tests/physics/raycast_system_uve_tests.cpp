//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/physics/raycast_system_uve.h"

#include <cstdint>
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

class RaycastSystemUVETest : public ::testing::Test {
protected:
    Memory::MemoryManagerUVE memoryManager;
    Events::EventSystemUVE eventSystem;
    Scene::EntityManagerUVE entityManager{memoryManager.GetDefaultAllocatorUVE(), eventSystem};
    Scene::SceneGraphUVE sceneGraph;
    RaycastSystemUVE raycastSystem;

    Scene::EntityUVE MakeColliderEntityUVE(Math::Vector3UVE position, Math::Vector3UVE halfExtents,
                                            std::uint32_t collisionLayer = 1, float friction = 0.0F,
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

    [[nodiscard]] static RaycastQueryUVE MakeXAxisQueryUVE(float originX = -10.0F, float maxDistance = 100.0F) {
        RaycastQueryUVE query;
        query.ray = Math::RayUVE{Math::Vector3UVE{originX, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 0.0F, 0.0F}};
        query.maxDistance = maxDistance;
        return query;
    }
};

TEST_F(RaycastSystemUVETest, RaycastUVE_SingleCollider_ReturnsHit) {
    const Scene::EntityUVE entity = MakeColliderEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});
    const RaycastQueryUVE query = MakeXAxisQueryUVE(-5.0F);

    const std::optional<RaycastHitUVE> hit = raycastSystem.RaycastUVE(entityManager, query);

    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->entity, entity);
    EXPECT_NEAR(hit->distance, 4.0F, kEpsilon);
    EXPECT_NEAR(hit->point.x, -1.0F, kEpsilon);
    EXPECT_NEAR(hit->normal.x, -1.0F, kEpsilon);
}

TEST_F(RaycastSystemUVETest, RaycastUVE_NoColliders_ReturnsNullopt) {
    EXPECT_FALSE(raycastSystem.RaycastUVE(entityManager, MakeXAxisQueryUVE()).has_value());
}

TEST_F(RaycastSystemUVETest, RaycastUVE_MultipleCollidersAlongRay_ReturnsClosest) {
    MakeColliderEntityUVE(Math::Vector3UVE{5.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});
    const Scene::EntityUVE closer = MakeColliderEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});

    const std::optional<RaycastHitUVE> hit = raycastSystem.RaycastUVE(entityManager, MakeXAxisQueryUVE());

    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->entity, closer);
}

TEST_F(RaycastSystemUVETest, RaycastUVE_EntityMissingColliderComponent_IsExcluded) {
    const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
    sceneGraph.AttachTransformUVE(entityManager, entity, Scene::TransformComponentUVE{});
    sceneGraph.UpdateUVE(entityManager);

    EXPECT_FALSE(raycastSystem.RaycastUVE(entityManager, MakeXAxisQueryUVE()).has_value());
}

TEST_F(RaycastSystemUVETest, RaycastUVE_LayerMaskExcludesNonMatchingCollider) {
    MakeColliderEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F}, /*collisionLayer=*/2U);
    RaycastQueryUVE query = MakeXAxisQueryUVE();
    query.layerMask = 1U; // (2 & 1) == 0 — collider's layer isn't in the query's mask.

    EXPECT_FALSE(raycastSystem.RaycastUVE(entityManager, query).has_value());
}

TEST_F(RaycastSystemUVETest, RaycastUVE_IgnoreEntity_ExcludesSpecificCollider) {
    const Scene::EntityUVE entity = MakeColliderEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});
    RaycastQueryUVE query = MakeXAxisQueryUVE();
    query.ignoreEntity = entity;

    EXPECT_FALSE(raycastSystem.RaycastUVE(entityManager, query).has_value());
}

TEST_F(RaycastSystemUVETest, RaycastUVE_EqualDistanceHits_ResolveDeterministically) {
    const Scene::EntityUVE first = MakeColliderEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});
    MakeColliderEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F}); // identical duplicate
    const RaycastQueryUVE query = MakeXAxisQueryUVE();

    const std::optional<RaycastHitUVE> firstCall = raycastSystem.RaycastUVE(entityManager, query);
    const std::optional<RaycastHitUVE> secondCall = raycastSystem.RaycastUVE(entityManager, query);

    ASSERT_TRUE(firstCall.has_value());
    ASSERT_TRUE(secondCall.has_value());
    EXPECT_EQ(firstCall->entity, first);
    EXPECT_EQ(secondCall->entity, first);
}

TEST_F(RaycastSystemUVETest, RaycastUVE_HitMaterial_MatchesColliderFields) {
    const Scene::EntityUVE entity = MakeColliderEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F},
                                                            1U, 0.4F, 0.9F, 3.0F);

    const std::optional<RaycastHitUVE> hit = raycastSystem.RaycastUVE(entityManager, MakeXAxisQueryUVE());

    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->entity, entity);
    EXPECT_NEAR(hit->material.friction, 0.4F, kEpsilon);
    EXPECT_NEAR(hit->material.restitution, 0.9F, kEpsilon);
    EXPECT_NEAR(hit->material.density, 3.0F, kEpsilon);
}

TEST_F(RaycastSystemUVETest, RaycastUVE_HitMaterial_ClampsOutOfRangeRestitution) {
    MakeColliderEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 1U, 0.0F, 5.0F, 1.0F);

    const std::optional<RaycastHitUVE> hit = raycastSystem.RaycastUVE(entityManager, MakeXAxisQueryUVE());

    ASSERT_TRUE(hit.has_value());
    EXPECT_NEAR(hit->material.restitution, 1.0F, kEpsilon);
}

} // namespace
} // namespace UVE::Physics::Tests
