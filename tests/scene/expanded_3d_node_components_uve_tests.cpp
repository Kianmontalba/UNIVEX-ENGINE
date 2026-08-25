// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <limits>

#include <gtest/gtest.h>

#include "uve/scene/components/area_component_uve.h"
#include "uve/scene/components/expanded_3d_node_components_uve.h"

namespace UVE::Scene::Tests {
namespace {

TEST(Expanded3DNodeComponentsUVETest, DefaultContractsAreValid) {
    EXPECT_TRUE(IsAreaComponentValidUVE(AreaComponentUVE{}));
    EXPECT_TRUE(IsRayCast3DNodeComponentValidUVE(RayCast3DNodeComponentUVE{}));
    EXPECT_TRUE(IsAnimatableBody3DNodeComponentValidUVE(AnimatableBody3DNodeComponentUVE{}));
    EXPECT_TRUE(IsNavigationRegion3DNodeComponentValidUVE(NavigationRegion3DNodeComponentUVE{}));
    EXPECT_TRUE(IsNavigationAgent3DNodeComponentValidUVE(NavigationAgent3DNodeComponentUVE{}));
    EXPECT_TRUE(IsSkeleton3DNodeComponentValidUVE(Skeleton3DNodeComponentUVE{}));
    EXPECT_TRUE(IsBoneAttachment3DNodeComponentValidUVE(BoneAttachment3DNodeComponentUVE{}));
    EXPECT_TRUE(IsSpringArm3DNodeComponentValidUVE(SpringArm3DNodeComponentUVE{}));
    EXPECT_TRUE(IsMarker3DNodeComponentValidUVE(Marker3DNodeComponentUVE{}));
    EXPECT_TRUE(IsHitbox3DNodeComponentValidUVE(Hitbox3DNodeComponentUVE{}));
    EXPECT_TRUE(IsHurtbox3DNodeComponentValidUVE(Hurtbox3DNodeComponentUVE{}));
    EXPECT_TRUE(IsProjectile3DNodeComponentValidUVE(Projectile3DNodeComponentUVE{}));
    EXPECT_TRUE(IsInteractionArea3DNodeComponentValidUVE(InteractionArea3DNodeComponentUVE{}));
    EXPECT_TRUE(IsWorldEnvironment3DNodeComponentValidUVE(WorldEnvironment3DNodeComponentUVE{}));
    EXPECT_TRUE(IsReflectionProbe3DNodeComponentValidUVE(ReflectionProbe3DNodeComponentUVE{}));
    EXPECT_TRUE(IsDecal3DNodeComponentValidUVE(Decal3DNodeComponentUVE{}));
    EXPECT_TRUE(IsLodGroup3DNodeComponentValidUVE(LodGroup3DNodeComponentUVE{}));
    EXPECT_TRUE(IsOccluder3DNodeComponentValidUVE(Occluder3DNodeComponentUVE{}));
    EXPECT_TRUE(IsVisibilityRegion3DNodeComponentValidUVE(VisibilityRegion3DNodeComponentUVE{}));
    EXPECT_TRUE(IsSpawnPoint3DNodeComponentValidUVE(SpawnPoint3DNodeComponentUVE{}));
    EXPECT_TRUE(IsLevelStreamer3DNodeComponentValidUVE(LevelStreamer3DNodeComponentUVE{}));
    EXPECT_TRUE(IsWorldPartition3DNodeComponentValidUVE(WorldPartition3DNodeComponentUVE{}));
}

TEST(Expanded3DNodeComponentsUVETest, BoundedContractsRejectUnsafeValues) {
    RayCast3DNodeComponentUVE ray;
    ray.exclusionCount = static_cast<std::uint8_t>(kMaximumRayCastExclusionsUVE + 1U);
    EXPECT_FALSE(IsRayCast3DNodeComponentValidUVE(ray));

    Skeleton3DNodeComponentUVE skeleton;
    skeleton.bones.push_back(SkeletonBoneUVE{"root", -1, {}, {}, {1.0F, 1.0F, 1.0F}});
    skeleton.bones.push_back(SkeletonBoneUVE{"root", 0, {}, {}, {1.0F, 1.0F, 1.0F}});
    EXPECT_FALSE(IsSkeleton3DNodeComponentValidUVE(skeleton));

    LodGroup3DNodeComponentUVE lod;
    lod.distanceThresholds[1] = lod.distanceThresholds[0];
    EXPECT_FALSE(IsLodGroup3DNodeComponentValidUVE(lod));

    LevelStreamer3DNodeComponentUVE streamer;
    streamer.enabled = true;
    streamer.levelPath.clear();
    EXPECT_FALSE(IsLevelStreamer3DNodeComponentValidUVE(streamer));

    WorldPartition3DNodeComponentUVE partition;
    partition.cellCounts[1] = 0U;
    EXPECT_FALSE(IsWorldPartition3DNodeComponentValidUVE(partition));
}

TEST(Expanded3DNodeComponentsUVETest, FiniteAndBoundedValuesRejectNonFinitePayloads) {
    WorldEnvironment3DNodeComponentUVE environment;
    environment.exposure = std::numeric_limits<float>::quiet_NaN();
    EXPECT_FALSE(IsWorldEnvironment3DNodeComponentValidUVE(environment));

    SpringArm3DNodeComponentUVE springArm;
    springArm.currentLength = std::numeric_limits<float>::infinity();
    EXPECT_FALSE(IsSpringArm3DNodeComponentValidUVE(springArm));
}

} // namespace
} // namespace UVE::Scene::Tests
