// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/render/light_system_uve.h"

#include <cmath>
#include <cstddef>
#include <limits>

#include <gtest/gtest.h>

#include "uve/events/event_system_uve.h"
#include "uve/memory/memory_manager_uve.h"
#include "uve/scene/components/light_component_uve.h"
#include "uve/scene/components/mesh_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"
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

TEST(LightComponentUVETest, IsLightComponentValidUVE_RejectsUnsafeValues) {
    EXPECT_TRUE(Scene::IsLightComponentValidUVE(Scene::LightComponentUVE{}));

    Scene::LightComponentUVE invalid = {};
    invalid.color.x = -0.1F;
    EXPECT_FALSE(Scene::IsLightComponentValidUVE(invalid));
    invalid = {};
    invalid.color.y = std::numeric_limits<float>::quiet_NaN();
    EXPECT_FALSE(Scene::IsLightComponentValidUVE(invalid));
    invalid = {};
    invalid.intensity = -1.0F;
    EXPECT_FALSE(Scene::IsLightComponentValidUVE(invalid));
    invalid = {};
    invalid.type = static_cast<Scene::LightTypeUVE>(99U);
    EXPECT_FALSE(Scene::IsLightComponentValidUVE(invalid));
    invalid = {};
    invalid.range = 0.0F;
    EXPECT_FALSE(Scene::IsLightComponentValidUVE(invalid));
    invalid = {};
    invalid.spotAngleDegrees = 180.0F;
    EXPECT_FALSE(Scene::IsLightComponentValidUVE(invalid));
    invalid = {};
    invalid.spotAngleDegrees = std::numeric_limits<float>::infinity();
    EXPECT_FALSE(Scene::IsLightComponentValidUVE(invalid));
}

TEST_F(LightSystemUVETest, ExtractActiveLightsUVE_NoLightEntities_AllSlotsReturnIntensityZeroSentinel) {
    const LightListUVE result = lightSystem.ExtractActiveLightsUVE(entityManager);

    for (const LightDataUVE& slot : result) {
        EXPECT_FLOAT_EQ(slot.intensity, 0.0F);
    }
}

TEST_F(LightSystemUVETest, ExtractActiveLightsUVE_OneDirectionalLight_PopulatesSlotZeroOnly) {
    Scene::LightComponentUVE light{Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 2.0F};
    light.type = Scene::LightTypeUVE::Directional;
    static_cast<void>(MakeLightEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::QuaternionUVE{}, light));

    const LightListUVE result = lightSystem.ExtractActiveLightsUVE(entityManager);

    EXPECT_EQ(result[0].type, Scene::LightTypeUVE::Directional);
    EXPECT_EQ(result[0].direction, (Math::Vector3UVE{0.0F, 0.0F, -1.0F}));
    EXPECT_EQ(result[0].rotation, Math::QuaternionUVE{});
    EXPECT_FLOAT_EQ(result[0].intensity, 2.0F);
    for (std::size_t i = 1; i < kMaxLightsUVE; ++i) {
        EXPECT_FLOAT_EQ(result[i].intensity, 0.0F);
    }
}

TEST_F(LightSystemUVETest, ExtractActiveLightsUVE_RotatedDirectionalLight_DirectionMatchesRotateVectorUVE) {
    // 180 degrees about Y: x=0, y=sin(90deg)=1, z=0, w=cos(90deg)=0 — negates x and z, so the
    // light's forward {0,0,-1} becomes {0,0,1}.
    const Math::QuaternionUVE rotation{0.0F, 1.0F, 0.0F, 0.0F};
    const Scene::LightComponentUVE light{Math::Vector3UVE{0.2F, 0.4F, 0.6F}, 3.5F};
    static_cast<void>(MakeLightEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, rotation, light));

    const LightListUVE result = lightSystem.ExtractActiveLightsUVE(entityManager);

    EXPECT_EQ(result[0].direction, (Math::Vector3UVE{0.0F, 0.0F, 1.0F}));
    EXPECT_EQ(result[0].rotation, rotation);
    EXPECT_EQ(result[0].color, light.color);
    EXPECT_FLOAT_EQ(result[0].intensity, light.intensity);
}

TEST_F(LightSystemUVETest, ExtractActiveLightsUVE_OnePointLight_PositionTypeAndRangeMatch) {
    Scene::LightComponentUVE light{Math::Vector3UVE{0.5F, 0.6F, 0.7F}, 4.0F};
    light.type = Scene::LightTypeUVE::Point;
    light.range = 25.0F;
    const Math::Vector3UVE position{3.0F, 1.0F, -2.0F};
    static_cast<void>(MakeLightEntityUVE(position, Math::QuaternionUVE{}, light));

    const LightListUVE result = lightSystem.ExtractActiveLightsUVE(entityManager);

    EXPECT_EQ(result[0].type, Scene::LightTypeUVE::Point);
    EXPECT_EQ(result[0].position, position);
    EXPECT_EQ(result[0].rotation, Math::QuaternionUVE{});
    EXPECT_FLOAT_EQ(result[0].range, 25.0F);
}

TEST_F(LightSystemUVETest, ExtractActiveLightsUVE_OneSpotLight_PositionDirectionAndAngleMatch) {
    Scene::LightComponentUVE light{Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 6.0F};
    light.type = Scene::LightTypeUVE::Spot;
    light.range = 12.0F;
    light.spotAngleDegrees = 20.0F;
    const Math::Vector3UVE position{-1.0F, 2.0F, 0.5F};
    static_cast<void>(MakeLightEntityUVE(position, Math::QuaternionUVE{}, light));

    const LightListUVE result = lightSystem.ExtractActiveLightsUVE(entityManager);

    EXPECT_EQ(result[0].type, Scene::LightTypeUVE::Spot);
    EXPECT_EQ(result[0].position, position);
    EXPECT_EQ(result[0].direction, (Math::Vector3UVE{0.0F, 0.0F, -1.0F}));
    EXPECT_EQ(result[0].rotation, Math::QuaternionUVE{});
    EXPECT_FLOAT_EQ(result[0].range, 12.0F);
    EXPECT_FLOAT_EQ(result[0].spotAngleDegrees, 20.0F);
}

TEST_F(LightSystemUVETest, ExtractActiveLightsUVE_LightComponentWithoutWorldTransform_SkippedNotMatched) {
    // Unlike ICameraSystemUVE (which asserts a required component is missing), ForEachUVE simply
    // never invokes the callback for an entity that doesn't match every requested component type
    // — no assert, no error, just silently excluded from the result.
    const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<Scene::LightComponentUVE>(
        entity, Scene::LightComponentUVE{Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 5.0F});

    const LightListUVE result = lightSystem.ExtractActiveLightsUVE(entityManager);

    for (const LightDataUVE& slot : result) {
        EXPECT_FLOAT_EQ(slot.intensity, 0.0F);
    }
}

TEST_F(LightSystemUVETest, ExtractActiveLightsUVE_MultipleLightsSameArchetype_EachOccupiesDistinctSlotInEncounterOrder) {
    // All three entities go through the identical MakeLightEntityUVE path (Transform +
    // WorldTransform + Hierarchy + Light, nothing else) so they share one archetype — within a
    // single archetype, ForEachUVE's iteration order is chunk/row creation order, so this pins
    // "slot N == the Nth created entity" as a deterministic, testable outcome.
    static_cast<void>(MakeLightEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::QuaternionUVE{},
                                          Scene::LightComponentUVE{Math::Vector3UVE{1.0F, 0.0F, 0.0F}, 10.0F}));
    static_cast<void>(MakeLightEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::QuaternionUVE{},
                                          Scene::LightComponentUVE{Math::Vector3UVE{0.0F, 1.0F, 0.0F}, 20.0F}));
    static_cast<void>(MakeLightEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::QuaternionUVE{},
                                          Scene::LightComponentUVE{Math::Vector3UVE{0.0F, 0.0F, 1.0F}, 30.0F}));

    const LightListUVE result = lightSystem.ExtractActiveLightsUVE(entityManager);

    EXPECT_FLOAT_EQ(result[0].intensity, 10.0F);
    EXPECT_EQ(result[0].color, (Math::Vector3UVE{1.0F, 0.0F, 0.0F}));
    EXPECT_FLOAT_EQ(result[1].intensity, 20.0F);
    EXPECT_EQ(result[1].color, (Math::Vector3UVE{0.0F, 1.0F, 0.0F}));
    EXPECT_FLOAT_EQ(result[2].intensity, 30.0F);
    EXPECT_EQ(result[2].color, (Math::Vector3UVE{0.0F, 0.0F, 1.0F}));
    EXPECT_FLOAT_EQ(result[3].intensity, 0.0F); // unfilled sentinel
}

TEST_F(LightSystemUVETest, ExtractActiveLightsUVE_MoreThanMaxLights_OnlyFirstFourCreatedAreKept) {
    // kMaxLightsUVE == 4: create 6 lights, all in the same archetype, so encounter order equals
    // creation order — only the first 4 created should be kept, matching the confirmed
    // first-N-encountered policy (no distance/importance sorting).
    for (int i = 0; i < 6; ++i) {
        static_cast<void>(MakeLightEntityUVE(
            Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::QuaternionUVE{},
            Scene::LightComponentUVE{Math::Vector3UVE{1.0F, 1.0F, 1.0F}, static_cast<float>(i + 1)}));
    }

    const LightListUVE result = lightSystem.ExtractActiveLightsUVE(entityManager);

    ASSERT_EQ(result.size(), kMaxLightsUVE);
    EXPECT_FLOAT_EQ(result[0].intensity, 1.0F);
    EXPECT_FLOAT_EQ(result[1].intensity, 2.0F);
    EXPECT_FLOAT_EQ(result[2].intensity, 3.0F);
    EXPECT_FLOAT_EQ(result[3].intensity, 4.0F);
}

TEST_F(LightSystemUVETest, ExtractActiveLightsUVE_EntityWithExtraComponents_StillMatched) {
    const Scene::EntityUVE entity =
        MakeLightEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::QuaternionUVE{},
                            Scene::LightComponentUVE{Math::Vector3UVE{1.0F, 1.0F, 1.0F}, 7.0F});
    entityManager.AddComponentUVE<Scene::MeshComponentUVE>(entity);

    const LightListUVE result = lightSystem.ExtractActiveLightsUVE(entityManager);

    EXPECT_FLOAT_EQ(result[0].intensity, 7.0F);
}

#if UVE_DEBUG
TEST_F(LightSystemUVETest, ExtractActiveLightsUVE_InvalidLightParameters_Asserts) {
    Scene::LightComponentUVE invalid = {};
    invalid.intensity = -1.0F;
    static_cast<void>(MakeLightEntityUVE(Math::Vector3UVE{}, Math::QuaternionUVE{}, invalid));

    EXPECT_DEATH({ static_cast<void>(lightSystem.ExtractActiveLightsUVE(entityManager)); }, "");
}
#else
TEST_F(LightSystemUVETest, ExtractActiveLightsUVE_InvalidLightParameters_SkipsWithoutPublishing) {
    Scene::LightComponentUVE invalid = {};
    invalid.intensity = -1.0F;
    static_cast<void>(MakeLightEntityUVE(Math::Vector3UVE{}, Math::QuaternionUVE{}, invalid));

    const LightListUVE result = lightSystem.ExtractActiveLightsUVE(entityManager);

    for (const LightDataUVE& slot : result) {
        EXPECT_FLOAT_EQ(slot.intensity, 0.0F);
    }
}
#endif

#if UVE_DEBUG
TEST_F(LightSystemUVETest, ExtractActiveLightsUVE_InvalidWorldTransform_Asserts) {
    const Scene::EntityUVE entity =
        MakeLightEntityUVE(Math::Vector3UVE{}, Math::QuaternionUVE{}, Scene::LightComponentUVE{});
    entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(entity).worldPosition.x =
        std::numeric_limits<float>::quiet_NaN();

    EXPECT_DEATH({ static_cast<void>(lightSystem.ExtractActiveLightsUVE(entityManager)); }, "");
}
#else
TEST_F(LightSystemUVETest, ExtractActiveLightsUVE_InvalidWorldTransform_SkipsWithoutPublishing) {
    const Scene::EntityUVE entity =
        MakeLightEntityUVE(Math::Vector3UVE{}, Math::QuaternionUVE{}, Scene::LightComponentUVE{});
    entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(entity).worldRotation =
        Math::QuaternionUVE{0.0F, 0.0F, 0.0F, 0.0F};

    const LightListUVE result = lightSystem.ExtractActiveLightsUVE(entityManager);

    for (const LightDataUVE& slot : result) {
        EXPECT_FLOAT_EQ(slot.intensity, 0.0F);
    }
}
#endif

} // namespace
} // namespace UVE::Render::Tests
