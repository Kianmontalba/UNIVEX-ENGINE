// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "uve/scene/i_particle_runtime_uve.h"

namespace UVE::Scene {

/// Bounded CPU particle state for authored emitters. This v1 owns only explicitly emitted particle
/// values and deterministic integration; GPU resources, renderer registration, asset lifetime, and
/// backend selection remain outside the runtime.
class ParticleRuntimeUVE final : public IParticleRuntimeUVE {
public:
    static constexpr std::size_t kMaximumInstancesUVE = 4096U;
    static constexpr std::uint64_t kMaximumTotalParticleBudgetUVE = 4'000'000U;
    static constexpr float kMaximumParticleLifetimeSecondsUVE = 3600.0F;
    static constexpr float kMaximumSimulationDeltaSecondsUVE = 0.25F;

    ParticleRuntimeUVE() = default;
    ParticleRuntimeUVE(const ParticleRuntimeUVE&) = delete;
    ParticleRuntimeUVE& operator=(const ParticleRuntimeUVE&) = delete;

    [[nodiscard]] ParticleRuntimeResultUVE AttachDetailedUVE(
        EntityUVE entity, const ParticleEmitterComponentUVE& component) override;
    [[nodiscard]] bool AttachUVE(EntityUVE entity, const ParticleEmitterComponentUVE& component) {
        return AttachDetailedUVE(entity, component).IsAcceptedUVE();
    }
    [[nodiscard]] ParticleRuntimeResultUVE DetachDetailedUVE(EntityUVE entity) noexcept override;
    [[nodiscard]] bool DetachUVE(EntityUVE entity) noexcept {
        return DetachDetailedUVE(entity).IsAcceptedUVE();
    }
    [[nodiscard]] ParticleRuntimeResultUVE SetEnabledDetailedUVE(EntityUVE entity, bool enabled) noexcept override;
    [[nodiscard]] ParticleRuntimeResultUVE SetLiveParticleCountDetailedUVE(
        EntityUVE entity, std::uint32_t liveParticles) noexcept override;
    [[nodiscard]] ParticleRuntimeResultUVE EmitDetailedUVE(
        EntityUVE entity, const ParticleEmissionUVE& emission) override;
    [[nodiscard]] ParticleRuntimeResultUVE SimulateDetailedUVE(
        float deltaSeconds, const Math::Vector3UVE& acceleration) noexcept override;
    [[nodiscard]] ParticleRuntimeSnapshotUVE GetSnapshotUVE() const override;
    [[nodiscard]] std::optional<ParticleStateSnapshotUVE> GetParticleSnapshotUVE(EntityUVE entity) const override;
    [[nodiscard]] bool HasInstanceUVE(EntityUVE entity) const noexcept override;
    [[nodiscard]] std::size_t GetInstanceCountUVE() const noexcept override;
    [[nodiscard]] std::uint64_t GetTotalBudgetUVE() const noexcept override;

private:
    struct InstanceUVE final {
        EntityUVE entity;
        std::uint32_t maxParticles = 0U;
        std::uint32_t liveParticles = 0U;
        std::uint64_t generation = 1U;
        std::uint64_t nextSequence = 1U;
        bool enabled = true;
        std::vector<ParticleStateUVE> particles;
    };

    std::unordered_map<EntityUVE, InstanceUVE> m_instances;
    std::uint64_t m_totalBudget = 0U;
};

} // namespace UVE::Scene

