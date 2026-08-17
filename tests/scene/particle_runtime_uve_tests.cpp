// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/scene/particle_runtime_uve.h"

#include <gtest/gtest.h>

#include <limits>

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

TEST(ParticleRuntimeUVETest, EmitUVE_AppendsStableParticleStateWithinBudget) {
    ParticleRuntimeUVE runtime;
    const EntityUVE entity{13U, 1U};
    ASSERT_TRUE(runtime.AttachUVE(entity, ParticleEmitterComponentUVE{2U}));
    const ParticleEmissionUVE emission{2U, Math::Vector3UVE{1.0F, 2.0F, 3.0F},
                                       Math::Vector3UVE{4.0F, 5.0F, 6.0F}, 5.0F};

    ASSERT_TRUE(runtime.EmitDetailedUVE(entity, emission).IsAcceptedUVE());
    const std::optional<ParticleStateSnapshotUVE> snapshot = runtime.GetParticleSnapshotUVE(entity);
    ASSERT_TRUE(snapshot.has_value());
    ASSERT_EQ(snapshot->particles.size(), 2U);
    EXPECT_EQ(snapshot->particles[0].sequence, 1U);
    EXPECT_EQ(snapshot->particles[1].sequence, 2U);
    EXPECT_EQ(snapshot->particles[0].position, emission.position);
    EXPECT_EQ(snapshot->particles[0].velocity, emission.velocity);
    EXPECT_EQ(snapshot->particles[0].remainingLifetimeSeconds, 5.0F);
    EXPECT_EQ(runtime.GetSnapshotUVE().instances.front().liveParticles, 2U);

    const ParticleRuntimeSnapshotUVE beforeRejectedEmission = runtime.GetSnapshotUVE();
    EXPECT_EQ(runtime.EmitDetailedUVE(entity, emission).code, ParticleRuntimeCodeUVE::LiveParticleCountExceeded);
    EXPECT_EQ(runtime.GetSnapshotUVE(), beforeRejectedEmission);
}

TEST(ParticleRuntimeUVETest, EmitUVE_RejectsDisabledAndInvalidInputWithoutMutation) {
    ParticleRuntimeUVE runtime;
    const EntityUVE entity{14U, 1U};
    ASSERT_TRUE(runtime.AttachUVE(entity, ParticleEmitterComponentUVE{8U}));
    ASSERT_EQ(runtime.SetEnabledDetailedUVE(entity, false).code, ParticleRuntimeCodeUVE::Applied);
    EXPECT_EQ(runtime.EmitDetailedUVE(entity, ParticleEmissionUVE{1U, {}, {}, 1.0F}).code,
              ParticleRuntimeCodeUVE::DisabledInstance);
    ASSERT_EQ(runtime.SetEnabledDetailedUVE(entity, true).code, ParticleRuntimeCodeUVE::Applied);

    const ParticleRuntimeSnapshotUVE beforeInvalid = runtime.GetSnapshotUVE();
    ParticleEmissionUVE invalid{1U, {}, {}, 1.0F};
    invalid.velocity.x = std::numeric_limits<float>::infinity();
    EXPECT_EQ(runtime.EmitDetailedUVE(entity, invalid).code, ParticleRuntimeCodeUVE::InvalidSimulationInput);
    EXPECT_EQ(runtime.GetSnapshotUVE(), beforeInvalid);
}

TEST(ParticleRuntimeUVETest, SimulateUVE_UsesDeterministicSemiImplicitIntegrationAndLifetimeCulling) {
    ParticleRuntimeUVE runtime;
    const EntityUVE entity{15U, 1U};
    ASSERT_TRUE(runtime.AttachUVE(entity, ParticleEmitterComponentUVE{4U}));
    ASSERT_TRUE(runtime.EmitDetailedUVE(
                       entity, ParticleEmissionUVE{1U, {}, Math::Vector3UVE{1.0F, 0.0F, 0.0F}, 1.0F})
                    .IsAcceptedUVE());

    ASSERT_TRUE(runtime.SimulateDetailedUVE(0.25F, Math::Vector3UVE{0.0F, -2.0F, 0.0F}).IsAcceptedUVE());
    auto midpoint = runtime.GetParticleSnapshotUVE(entity);
    ASSERT_TRUE(midpoint.has_value());
    ASSERT_EQ(midpoint->particles.size(), 1U);
    EXPECT_EQ(midpoint->particles.front().position, (Math::Vector3UVE{0.25F, -0.125F, 0.0F}));
    EXPECT_EQ(midpoint->particles.front().velocity, (Math::Vector3UVE{1.0F, -0.5F, 0.0F}));
    EXPECT_EQ(midpoint->particles.front().remainingLifetimeSeconds, 0.75F);

    ASSERT_TRUE(runtime.SimulateDetailedUVE(0.25F, Math::Vector3UVE{0.0F, -2.0F, 0.0F}).IsAcceptedUVE());
    auto secondStep = runtime.GetParticleSnapshotUVE(entity);
    ASSERT_TRUE(secondStep.has_value());
    ASSERT_EQ(secondStep->particles.size(), 1U);
    EXPECT_EQ(secondStep->particles.front().position, (Math::Vector3UVE{0.5F, -0.375F, 0.0F}));
    EXPECT_EQ(secondStep->particles.front().velocity, (Math::Vector3UVE{1.0F, -1.0F, 0.0F}));
    EXPECT_EQ(secondStep->particles.front().remainingLifetimeSeconds, 0.5F);

    ASSERT_TRUE(runtime.SimulateDetailedUVE(0.25F, Math::Vector3UVE{0.0F, -2.0F, 0.0F}).IsAcceptedUVE());
    ASSERT_TRUE(runtime.SimulateDetailedUVE(0.25F, Math::Vector3UVE{0.0F, -2.0F, 0.0F}).IsAcceptedUVE());
    EXPECT_TRUE(runtime.GetParticleSnapshotUVE(entity)->particles.empty());
    EXPECT_EQ(runtime.GetSnapshotUVE().instances.front().liveParticles, 0U);
}

TEST(ParticleRuntimeUVETest, SimulateUVE_RejectsNonFiniteInputAtomically) {
    ParticleRuntimeUVE runtime;
    const EntityUVE entity{16U, 1U};
    ASSERT_TRUE(runtime.AttachUVE(entity, ParticleEmitterComponentUVE{4U}));
    ASSERT_TRUE(runtime.EmitDetailedUVE(entity, ParticleEmissionUVE{
                                               1U,
                                               {},
                                               Math::Vector3UVE{std::numeric_limits<float>::max(), 0.0F, 0.0F},
                                               2.0F})
                    .IsAcceptedUVE());
    const std::optional<ParticleStateSnapshotUVE> before = runtime.GetParticleSnapshotUVE(entity);
    ASSERT_TRUE(before.has_value());

    const ParticleRuntimeResultUVE result = runtime.SimulateDetailedUVE(
        0.1F, Math::Vector3UVE{std::numeric_limits<float>::max(), 0.0F, 0.0F});
    EXPECT_EQ(result.code, ParticleRuntimeCodeUVE::NonFiniteSimulation);
    EXPECT_EQ(runtime.GetParticleSnapshotUVE(entity), before);
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

