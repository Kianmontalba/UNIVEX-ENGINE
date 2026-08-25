// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/physics/physics_query_system_uve.h"

#include <gtest/gtest.h>

#include "uve/events/event_system_uve.h"
#include "uve/math/ray_uve.h"
#include "uve/memory/memory_manager_uve.h"
#include "uve/physics/collision_system_uve.h"
#include "uve/scene/components/area_component_uve.h"
#include "uve/scene/components/collider_component_uve.h"
#include "uve/scene/components/transform_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"
#include "uve/scene/entity_manager_uve.h"
#include "uve/scene/scene_graph_uve.h"

namespace UVE::Physics::Tests {
namespace {

class PhysicsQuerySystemUVETest : public ::testing::Test {
protected:
    Memory::MemoryManagerUVE memoryManager;
    Events::EventSystemUVE eventSystem;
    Scene::EntityManagerUVE entityManager{memoryManager.GetDefaultAllocatorUVE(), eventSystem};
    Scene::SceneGraphUVE sceneGraph;
    CollisionSystemUVE collisionSystem;
    PhysicsQuerySystemUVE querySystem{collisionSystem};

    Scene::EntityUVE MakeColliderUVE(Math::Vector3UVE position, Math::Vector3UVE halfExtents) {
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE transform;
        transform.localPosition = position;
        sceneGraph.AttachTransformUVE(entityManager, entity, transform);
        sceneGraph.UpdateUVE(entityManager);
        entityManager.AddComponentUVE<Scene::ColliderComponentUVE>(
            entity, Scene::ColliderComponentUVE{halfExtents});
        return entity;
    }

    Scene::EntityUVE MakeAreaUVE(Math::Vector3UVE position, Math::Vector3UVE halfExtents) {
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE transform;
        transform.localPosition = position;
        sceneGraph.AttachTransformUVE(entityManager, entity, transform);
        sceneGraph.UpdateUVE(entityManager);
        entityManager.AddComponentUVE<Scene::AreaComponentUVE>(
            entity, Scene::AreaComponentUVE{halfExtents, 1U, 0xFFFFFFFFU});
        return entity;
    }

    [[nodiscard]] Math::Vector3UVE WorldPositionUVE(Scene::EntityUVE entity) const {
        return entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(entity).worldPosition;
    }
};

TEST_F(PhysicsQuerySystemUVETest, SphereCastAreaOverlapAndCharacterMove_DelegateThroughOneFacade) {
    const Scene::EntityUVE castTarget = MakeColliderUVE({5.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F});
    const Scene::EntityUVE area = MakeAreaUVE({0.0F, 0.0F, 0.0F}, {2.0F, 2.0F, 2.0F});
    const Scene::EntityUVE overlapTarget = MakeColliderUVE({0.5F, 0.0F, 0.0F}, {0.5F, 0.5F, 0.5F});
    const Scene::EntityUVE controller = MakeColliderUVE({-5.0F, 0.0F, 0.0F}, {0.5F, 0.5F, 0.5F});

    SphereCastQueryUVE castQuery;
    castQuery.ray = Math::RayUVE{{0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}};
    castQuery.radius = 0.25F;
    castQuery.maxDistance = 20.0F;
    castQuery.ignoreEntity = overlapTarget;
    const std::optional<SphereCastHitUVE> hit = querySystem.SphereCastUVE(entityManager, castQuery);
    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->entity, castTarget);

    const AreaOverlapQueryResultUVE overlaps = querySystem.QueryAreaOverlapsUVE(entityManager, 8U);
    EXPECT_FALSE(overlaps.IsTruncatedUVE());
    ASSERT_EQ(overlaps.overlaps.size(), 1U);
    EXPECT_EQ(overlaps.overlaps.front().area, area);
    EXPECT_EQ(overlaps.overlaps.front().other, overlapTarget);

    CharacterControllerInputUVE input;
    input.entity = controller;
    input.desiredDisplacement = {1.0F, 0.0F, 0.0F};
    input.maximumSubstepDistance = 1.0F;
    const CharacterControllerMoveResultUVE moved =
        querySystem.MoveCharacterUVE(entityManager, sceneGraph, input);
    EXPECT_TRUE(moved.IsAcceptedUVE());
    EXPECT_FLOAT_EQ(WorldPositionUVE(controller).x, -4.0F);
}

TEST_F(PhysicsQuerySystemUVETest, InvalidCharacterInput_IsRejectedWithoutMovingAnyEntity) {
    const Scene::EntityUVE entity = MakeColliderUVE({2.0F, 0.0F, 0.0F}, {0.5F, 0.5F, 0.5F});
    CharacterControllerInputUVE input;
    input.entity = Scene::kInvalidEntityUVE;
    input.desiredDisplacement = {1.0F, 0.0F, 0.0F};

    const CharacterControllerMoveResultUVE moved =
        querySystem.MoveCharacterWithToIUVE(entityManager, sceneGraph, input);
    EXPECT_EQ(moved.code, CharacterControllerMoveCodeUVE::InvalidEntity);
    EXPECT_FLOAT_EQ(WorldPositionUVE(entity).x, 2.0F);
}

} // namespace
} // namespace UVE::Physics::Tests
