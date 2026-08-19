// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/render/particle_draw_command_uve.h"

#include <gtest/gtest.h>

#include <limits>

namespace UVE::Render::Tests {

TEST(ParticleDrawRecorderUVETest, RecordUVE_CopiesSortedQueueItemsWithoutMutatingQueue) {
    RenderQueueUVE queue;
    queue.particleItems = {
        {{7U, 1U}, Math::Vector3UVE{1.0F, 2.0F, 3.0F}, 2.0F, 3.0F, 4U},
        {{8U, 1U}, Math::Vector3UVE{4.0F, 5.0F, 6.0F}, 1.0F, 1.0F, 5U},
    };
    const RenderQueueUVE before = queue;

    const ParticleDrawRecordingUVE recording = ParticleDrawRecorderUVE::RecordUVE(queue);
    ASSERT_EQ(recording.sourceItemCount, 2U);
    ASSERT_EQ(recording.commands.size(), 2U);
    EXPECT_FALSE(recording.truncated);
    EXPECT_EQ(recording.commands[0].entity, queue.particleItems[0].entity);
    EXPECT_EQ(recording.commands[0].position, queue.particleItems[0].position);
    EXPECT_EQ(recording.commands[0].sequence, 4U);
    EXPECT_EQ(queue.particleItems, before.particleItems);
    EXPECT_EQ(queue.particleItemsTruncated, before.particleItemsTruncated);
}

TEST(ParticleDrawRecorderUVETest, IsValidParticleDrawCommandUVE_AcceptsFiniteLiveCommand) {
    const ParticleDrawCommandUVE command{{1U, 1U}, Math::Vector3UVE{1.0F, 2.0F, 3.0F}, 0.5F, -2.0F, 7U};
    EXPECT_TRUE(IsValidParticleDrawCommandUVE(command));
}

TEST(ParticleDrawRecorderUVETest, IsValidParticleDrawCommandUVE_RejectsUnsafeFields) {
    EXPECT_FALSE(IsValidParticleDrawCommandUVE(
        ParticleDrawCommandUVE{{1U, 1U}, Math::Vector3UVE{}, -0.1F, 0.0F, 1U}));
    EXPECT_FALSE(IsValidParticleDrawCommandUVE(
        ParticleDrawCommandUVE{{1U, 1U}, Math::Vector3UVE{std::numeric_limits<float>::quiet_NaN(), 0.0F, 0.0F},
                               1.0F, 0.0F, 1U}));
    EXPECT_FALSE(IsValidParticleDrawCommandUVE(
        ParticleDrawCommandUVE{{1U, 1U}, Math::Vector3UVE{}, 1.0F, 0.0F, 0U}));
}

TEST(ParticleDrawRecorderUVETest, RecordUVE_HonorsHardCapAndQueueTruncationFact) {
    RenderQueueUVE queue;
    queue.particleItems = {
        {{1U, 1U}, {}, 1.0F, 3.0F, 1U},
        {{2U, 1U}, {}, 1.0F, 2.0F, 2U},
        {{3U, 1U}, {}, 1.0F, 1.0F, 3U},
    };
    queue.particleItemsTruncated = true;

    const ParticleDrawRecordingUVE recording = ParticleDrawRecorderUVE::RecordUVE(queue, 2U);
    EXPECT_EQ(recording.sourceItemCount, 3U);
    EXPECT_EQ(recording.commands.size(), 2U);
    EXPECT_TRUE(recording.truncated);
    EXPECT_EQ(recording.commands[1].sequence, 2U);
}

TEST(ParticleDrawRecorderUVETest, RecordUVE_ZeroCapProducesNoCommandsAndPreservesSourceCount) {
    RenderQueueUVE queue;
    queue.particleItems.push_back({{4U, 1U}, {}, 1.0F, 0.0F, 1U});

    const ParticleDrawRecordingUVE recording = ParticleDrawRecorderUVE::RecordUVE(queue, 0U);
    EXPECT_EQ(recording.sourceItemCount, 1U);
    EXPECT_TRUE(recording.commands.empty());
    EXPECT_TRUE(recording.truncated);
}

} // namespace UVE::Render::Tests
