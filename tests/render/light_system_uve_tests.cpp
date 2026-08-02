//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/render/light_system_uve.h"

#include <gtest/gtest.h>

#include "uve/events/event_system_uve.h"
#include "uve/memory/memory_manager_uve.h"
#include "uve/scene/components/light_component_uve.h"
#include "uve/scene/components/mesh_component_uve.h"
#include "uve/scene/entity_manager_uve.h"
#include "uve/scene/scene_graph_uve.h"

namespace UVE::Render::Tests {
namespace {

class LightSystemUVETest : public ::testing::Test {
protected:
    Memory::MemoryManagerUVE memoryManager;
    Events::EventSystemUVE eventSystem;
    Scene::EntityManagerUVE entityManager{memoryManager.GetDefaultAllocatorUVE(), eventSystem};
    Scene::SceneGraphUVE sceneGraph;
    LightSystemUVE lightSystem;

    [[nodiscard]] Scene::EntityUVE MakeLightEntityUVE(Math::Vector3UVE position, Math::QuaternionUVE rotation,
                                                        Scene::LightComponentUVE light) {
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE local;
        local.localPosition = position;
        local.localRotation = rotation;
        sceneGraph.AttachTransformUVE(entityManager, entity, local);
        sceneGraph.UpdateUVE(entityManager);
        entityManager.AddComponentUVE<Scene::LightComponentUVE>(entity, light);
        return entity;
    }
};

TEST_F(LightSystemUVETest, ExtractActiveLightUVE_NoLightEntity_ReturnsIntensityZeroSentinel) {
    const DirectionalLightDataUVE result = lightSystem.ExtractActiveLightUVE(entityManager);

    EXPECT_FLOAT_EQ(result.intensity, 0.0F);
}

TEST_F(LightSystemUVETest, ExtractActiveLightUVE_IdentityRotation_DirectionIsNegativeZ) {
    static_cast<void>(MakeLightEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::QuaternionUVE{},
                                          Scene::LightComponentUVE{Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 2.0F}));

    const DirectionalLightDataUVE result = lightSystem.ExtractActiveLightUVE(entityManager);

    EXPECT_EQ(result.direction, (Math::Vector3UVE{0.0F, 0.0F, -1.0F}));
}

TEST_F(LightSystemUVETest, ExtractActiveLightUVE_RotatedLightEntity_DirectionMatchesRotateVectorUVE) {
    // 180 degrees about Y: x=0, y=sin(90deg)=1, z=0, w=cos(90deg)=0 — negates x and z, so the
    // light's forward {0,0,-1} becomes {0,0,1}.
    const Math::QuaternionUVE rotation{0.0F, 1.0F, 0.0F, 0.0F};
    const Scene::LightComponentUVE light{Math::Vector3UVE{0.2F, 0.4F, 0.6F}, 3.5F};
    static_cast<void>(MakeLightEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, rotation, light));

    const DirectionalLightDataUVE result = lightSystem.ExtractActiveLightUVE(entityManager);

    EXPECT_EQ(result.direction, (Math::Vector3UVE{0.0F, 0.0F, 1.0F}));
    EXPECT_EQ(result.color, light.color);
    EXPECT_FLOAT_EQ(result.intensity, light.intensity);
}

TEST_F(LightSystemUVETest, ExtractActiveLightUVE_LightComponentWithoutWorldTransform_SkippedNotMatched) {
    // Unlike ICameraSystemUVE (which asserts a required component is missing), ForEachUVE simply
    // never invokes the callback for an entity that doesn't match every requested component type
    // — no assert, no error, just silently excluded from the result.
    const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<Scene::LightComponentUVE>(
        entity, Scene::LightComponentUVE{Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 5.0F});

    const DirectionalLightDataUVE result = lightSystem.ExtractActiveLightUVE(entityManager);

    EXPECT_FLOAT_EQ(result.intensity, 0.0F);
}

TEST_F(LightSystemUVETest, ExtractActiveLightUVE_MultipleLightEntitiesSameArchetype_ReturnsFirstCreatedDeterministically) {
    // Both entities go through the identical MakeLightEntityUVE path (Transform + WorldTransform +
    // Hierarchy + Light, nothing else) so they share one archetype — within a single archetype,
    // ForEachUVE's iteration order is chunk/row creation order, so this pins "first created wins"
    // as a deterministic, testable (if v1-arbitrary) outcome, not a general IEntityManagerUVE-wide
    // guarantee.
    static_cast<void>(MakeLightEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::QuaternionUVE{},
                                          Scene::LightComponentUVE{Math::Vector3UVE{1.0F, 0.0F, 0.0F}, 10.0F}));
    static_cast<void>(MakeLightEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::QuaternionUVE{},
                                          Scene::LightComponentUVE{Math::Vector3UVE{0.0F, 1.0F, 0.0F}, 20.0F}));

    const DirectionalLightDataUVE result = lightSystem.ExtractActiveLightUVE(entityManager);

    EXPECT_FLOAT_EQ(result.intensity, 10.0F);
    EXPECT_EQ(result.color, (Math::Vector3UVE{1.0F, 0.0F, 0.0F}));
}

TEST_F(LightSystemUVETest, ExtractActiveLightUVE_EntityWithExtraComponents_StillMatched) {
    const Scene::EntityUVE entity =
        MakeLightEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::QuaternionUVE{},
                            Scene::LightComponentUVE{Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 7.0F});
    entityManager.AddComponentUVE<Scene::MeshComponentUVE>(entity);

    const DirectionalLightDataUVE result = lightSystem.ExtractActiveLightUVE(entityManager);

    EXPECT_FLOAT_EQ(result.intensity, 7.0F);
}

} // namespace
} // namespace UVE::Render::Tests
