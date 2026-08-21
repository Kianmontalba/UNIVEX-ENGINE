// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/render/particle_render_bridge_uve.h"
#include "uve/render/render_queue_uve.h"

#include <limits>

#include <gtest/gtest.h>

namespace UVE::Render::Tests {

TEST(ParticleRenderBridgeUVETest, ValidateParticleRenderSnapshotUVE_AcceptsValidAndTruncatedBatches) {
    ParticleRenderSnapshotUVE valid;
    valid.sourceParticleCount = 1U;
    valid.items.push_back({Scene::EntityUVE{2U, 1U}, Math::Vector3UVE{1.0F, 2.0F, 3.0F}, 1.0F, 3.0F, 7U});
    EXPECT_TRUE(ValidateParticleRenderSnapshotUVE(valid));
    valid.truncated = true;
    valid.sourceParticleCount = 3U;
    EXPECT_TRUE(ValidateParticleRenderSnapshotUVE(valid, 1U));
}

TEST(ParticleRenderBridgeUVETest, ValidateParticleRenderSnapshotUVE_RejectsInconsistentOrUnsafeBatches) {
    ParticleRenderSnapshotUVE snapshot;
    snapshot.sourceParticleCount = 1U;
    snapshot.items.push_back({Scene::EntityUVE{2U, 1U}, {}, 1.0F, 0.0F, 1U});
    EXPECT_FALSE(ValidateParticleRenderSnapshotUVE(snapshot, 0U));
    snapshot.items[0].sequence = 0U;
    EXPECT_FALSE(ValidateParticleRenderSnapshotUVE(snapshot));
    snapshot.items[0].sequence = 1U;
    snapshot.items[0].entity = Scene::kInvalidEntityUVE;
    EXPECT_FALSE(ValidateParticleRenderSnapshotUVE(snapshot));
    snapshot.items[0].entity = Scene::EntityUVE{2U, 1U};
    snapshot.sourceParticleCount = 0U;
    EXPECT_FALSE(ValidateParticleRenderSnapshotUVE(snapshot));
}

TEST(ParticleRenderBridgeUVETest, ExtractUVE_CopiesEnabledParticlesWithStableOrdering) {
    Scene::ParticleRuntimeUVE runtime;
    const Scene::EntityUVE first{2U, 1U};
    const Scene::EntityUVE second{1U, 1U};
    ASSERT_TRUE(runtime.AttachUVE(first, Scene::ParticleEmitterComponentUVE{4U}));
    ASSERT_TRUE(runtime.AttachUVE(second, Scene::ParticleEmitterComponentUVE{4U}));
    ASSERT_TRUE(runtime.EmitDetailedUVE(
                       first, Scene::ParticleEmissionUVE{1U, Math::Vector3UVE{0.0F, 0.0F, 3.0F}, {}, 2.0F})
                    .IsAcceptedUVE());
    ASSERT_TRUE(runtime.EmitDetailedUVE(
                       second, Scene::ParticleEmissionUVE{1U, Math::Vector3UVE{0.0F, 0.0F, 5.0F}, {}, 2.0F})
                    .IsAcceptedUVE());

    const ParticleRenderSnapshotUVE snapshot = ParticleRenderBridgeUVE::ExtractUVE(runtime);
    ASSERT_EQ(snapshot.sourceParticleCount, 2U);
    ASSERT_EQ(snapshot.items.size(), 2U);
    EXPECT_FALSE(snapshot.truncated);
    EXPECT_EQ(snapshot.items[0].entity, second);
    EXPECT_EQ(snapshot.items[0].sortDepth, 5.0F);
    EXPECT_EQ(snapshot.items[1].entity, first);
    EXPECT_EQ(snapshot.items[1].sortDepth, 3.0F);
}

TEST(ParticleRenderBridgeUVETest, ExtractUVE_FiltersDisabledAndHonorsHardCap) {
    Scene::ParticleRuntimeUVE runtime;
    const Scene::EntityUVE enabled{3U, 1U};
    const Scene::EntityUVE disabled{4U, 1U};
    ASSERT_TRUE(runtime.AttachUVE(enabled, Scene::ParticleEmitterComponentUVE{4U}));
    ASSERT_TRUE(runtime.AttachUVE(disabled, Scene::ParticleEmitterComponentUVE{4U}));
    ASSERT_TRUE(runtime.EmitDetailedUVE(enabled, Scene::ParticleEmissionUVE{3U, {}, {}, 2.0F}).IsAcceptedUVE());
    ASSERT_TRUE(runtime.EmitDetailedUVE(disabled, Scene::ParticleEmissionUVE{2U, {}, {}, 2.0F}).IsAcceptedUVE());
    ASSERT_EQ(runtime.SetEnabledDetailedUVE(disabled, false).code, Scene::ParticleRuntimeCodeUVE::Applied);

    const ParticleRenderSnapshotUVE snapshot = ParticleRenderBridgeUVE::ExtractUVE(runtime, 2U);
    EXPECT_EQ(snapshot.sourceParticleCount, 3U);
    EXPECT_EQ(snapshot.items.size(), 2U);
    EXPECT_TRUE(snapshot.truncated);
    for (const ParticleRenderItemUVE& item : snapshot.items) {
        EXPECT_EQ(item.entity, enabled);
    }
}

TEST(ParticleRenderBridgeUVETest, ExtractUVE_ZeroCapDoesNotCopyState) {
    Scene::ParticleRuntimeUVE runtime;
    ASSERT_TRUE(runtime.AttachUVE({5U, 1U}, Scene::ParticleEmitterComponentUVE{2U}));
    ASSERT_TRUE(runtime.EmitDetailedUVE({5U, 1U}, Scene::ParticleEmissionUVE{1U, {}, {}, 2.0F}).IsAcceptedUVE());

    const ParticleRenderSnapshotUVE snapshot = ParticleRenderBridgeUVE::ExtractUVE(runtime, 0U);
    EXPECT_TRUE(snapshot.items.empty());
    EXPECT_EQ(snapshot.sourceParticleCount, 0U);
    EXPECT_TRUE(snapshot.truncated);
}

TEST(ParticleRenderBridgeUVETest, RenderQueueUVE_RejectsInvalidSnapshotWithoutMutatingExistingItems) {
    RenderQueueUVE queue;
    queue.particleItems = {{{8U, 1U}, {}, 1.0F, 2.0F, 1U}};
    const RenderQueueUVE before = queue;

    ParticleRenderSnapshotUVE invalidSnapshot;
    invalidSnapshot.sourceParticleCount = 1U;
    invalidSnapshot.items = {{{9U, 1U}, Math::Vector3UVE{std::numeric_limits<float>::infinity(), 0.0F, 0.0F},
                               1.0F, 2.0F, 2U}};

    queue.AppendParticleSnapshotUVE(invalidSnapshot);

    EXPECT_EQ(queue.particleItems, before.particleItems);
    EXPECT_TRUE(queue.particleItemsTruncated);
}

TEST(ParticleRenderBridgeUVETest, RenderQueueUVE_AppendsCopiesAndSortsParticleItemsDeterministically) {
    RenderQueueUVE queue;
    ParticleRenderSnapshotUVE snapshot;
    snapshot.sourceParticleCount = 3U;
    snapshot.items = {
        {{9U, 1U}, {}, 1.0F, 1.0F, 2U},
        {{8U, 1U}, {}, 1.0F, 3.0F, 1U},
        {{8U, 1U}, {}, 1.0F, 3.0F, 2U},
    };
    snapshot.truncated = true;

    queue.AppendParticleSnapshotUVE(snapshot);
    queue.SortUVE();
    ASSERT_EQ(queue.particleItems.size(), 3U);
    EXPECT_TRUE(queue.particleItemsTruncated);
    EXPECT_EQ(queue.particleItems[0].sequence, 1U);
    EXPECT_EQ(queue.particleItems[1].sequence, 2U);
    EXPECT_EQ(queue.particleItems[2].entity, (Scene::EntityUVE{9U, 1U}));
}

} // namespace UVE::Render::Tests

