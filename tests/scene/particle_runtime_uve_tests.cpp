// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/scene/particle_runtime_uve.h"

#include <gtest/gtest.h>

namespace UVE::Scene::Tests {

TEST(ParticleRuntimeUVETest, AttachUVE_RejectsInvalidDuplicateAndPreservesBudgetAtomicity) {
    ParticleRuntimeUVE runtime;
    EXPECT_EQ(runtime.AttachDetailedUVE(kInvalidEntityUVE, ParticleEmitterComponentUVE{10U}).code,
              ParticleRuntimeCodeUVE::InvalidEntity);
    EXPECT_EQ(runtime.AttachDetailedUVE({1U, 1U}, ParticleEmitterComponentUVE{0U}).code,
              ParticleRuntimeCodeUVE::InvalidComponent);

    ASSERT_TRUE(runtime.AttachUVE({1U, 1U}, ParticleEmitterComponentUVE{100U}));
    EXPECT_EQ(runtime.AttachDetailedUVE({1U, 1U}, ParticleEmitterComponentUVE{100U}).code,
              ParticleRuntimeCodeUVE::DuplicateInstance);
    EXPECT_EQ(runtime.GetInstanceCountUVE(), 1U);
    EXPECT_EQ(runtime.GetTotalBudgetUVE(), 100U);
}

TEST(ParticleRuntimeUVETest, AttachUVE_EnforcesAggregateBudgetAndReleasesItOnDetach) {
    ParticleRuntimeUVE runtime;
    const ParticleEmitterComponentUVE maximum{kMaximumParticleEmitterParticlesUVE};
    ASSERT_TRUE(runtime.AttachUVE({2U, 1U}, maximum));
    ASSERT_TRUE(runtime.AttachUVE({3U, 1U}, maximum));
    ASSERT_TRUE(runtime.AttachUVE({4U, 1U}, maximum));
    ASSERT_TRUE(runtime.AttachUVE({5U, 1U}, maximum));
    EXPECT_EQ(runtime.GetTotalBudgetUVE(), ParticleRuntimeUVE::kMaximumTotalParticleBudgetUVE);
    EXPECT_EQ(runtime.AttachDetailedUVE({6U, 1U}, ParticleEmitterComponentUVE{1U}).code,
              ParticleRuntimeCodeUVE::CapacityExceeded);
    EXPECT_EQ(runtime.GetInstanceCountUVE(), 4U);

    ASSERT_TRUE(runtime.DetachUVE({3U, 1U}));
    EXPECT_EQ(runtime.GetTotalBudgetUVE(), 3'000'000U);
    ASSERT_TRUE(runtime.AttachUVE({6U, 1U}, ParticleEmitterComponentUVE{1'000'000U}));
    EXPECT_EQ(runtime.GetTotalBudgetUVE(), ParticleRuntimeUVE::kMaximumTotalParticleBudgetUVE);
}

TEST(ParticleRuntimeUVETest, StateUVE_EnforcesLiveCountAndEnabledLifecycle) {
    ParticleRuntimeUVE runtime;
    const EntityUVE entity{7U, 1U};
    ASSERT_TRUE(runtime.AttachUVE(entity, ParticleEmitterComponentUVE{128U}));

    EXPECT_EQ(runtime.SetLiveParticleCountDetailedUVE(entity, 129U).code,
              ParticleRuntimeCodeUVE::LiveParticleCountExceeded);
    EXPECT_EQ(runtime.SetLiveParticleCountDetailedUVE(entity, 64U).code,
              ParticleRuntimeCodeUVE::Applied);
    EXPECT_EQ(runtime.SetLiveParticleCountDetailedUVE(entity, 64U).code,
              ParticleRuntimeCodeUVE::Unchanged);
    EXPECT_EQ(runtime.SetEnabledDetailedUVE(entity, false).code, ParticleRuntimeCodeUVE::Applied);
    EXPECT_EQ(runtime.SetEnabledDetailedUVE(entity, false).code, ParticleRuntimeCodeUVE::Unchanged);

    const ParticleRuntimeSnapshotUVE snapshot = runtime.GetSnapshotUVE();
    ASSERT_EQ(snapshot.instances.size(), 1U);
    EXPECT_EQ(snapshot.instances.front().entity, entity);
    EXPECT_EQ(snapshot.instances.front().maxParticles, 128U);
    EXPECT_EQ(snapshot.instances.front().liveParticles, 64U);
    EXPECT_FALSE(snapshot.instances.front().enabled);
}

TEST(ParticleRuntimeUVETest, GetSnapshotUVE_IsDeterministicByGenerationalEntityOrder) {
    ParticleRuntimeUVE runtime;
    ASSERT_TRUE(runtime.AttachUVE({11U, 2U}, ParticleEmitterComponentUVE{20U}));
    ASSERT_TRUE(runtime.AttachUVE({10U, 3U}, ParticleEmitterComponentUVE{30U}));
    ASSERT_TRUE(runtime.AttachUVE({10U, 1U}, ParticleEmitterComponentUVE{40U}));

    const ParticleRuntimeSnapshotUVE snapshot = runtime.GetSnapshotUVE();
    ASSERT_EQ(snapshot.instances.size(), 3U);
    EXPECT_EQ(snapshot.instances[0].entity, (EntityUVE{10U, 1U}));
    EXPECT_EQ(snapshot.instances[1].entity, (EntityUVE{10U, 3U}));
    EXPECT_EQ(snapshot.instances[2].entity, (EntityUVE{11U, 2U}));
    EXPECT_EQ(snapshot.totalBudget, 90U);
}

TEST(ParticleRuntimeUVETest, DetachUVE_ReportsMissingInstancesAndRemovesOwnership) {
    ParticleRuntimeUVE runtime;
    EXPECT_EQ(runtime.DetachDetailedUVE({12U, 1U}).code, ParticleRuntimeCodeUVE::NoActiveInstance);
    ASSERT_TRUE(runtime.AttachUVE({12U, 1U}, ParticleEmitterComponentUVE{12U}));
    ASSERT_TRUE(runtime.DetachUVE({12U, 1U}));
    EXPECT_FALSE(runtime.HasInstanceUVE({12U, 1U}));
    EXPECT_EQ(runtime.GetInstanceCountUVE(), 0U);
    EXPECT_EQ(runtime.GetTotalBudgetUVE(), 0U);
}

} // namespace UVE::Scene::Tests

