// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "uve/math/vector3_uve.h"
#include "uve/scene/components/particle_emitter_component_uve.h"
#include "uve/scene/entity_uve.h"

namespace UVE::Scene {

enum class ParticleRuntimeCodeUVE : std::uint8_t {
    Applied = 0,
    Unchanged,
    InvalidEntity,
    InvalidComponent,
    DuplicateInstance,
    NoActiveInstance,
    CapacityExceeded,
    LiveParticleCountExceeded,
    DisabledInstance,
    InvalidSimulationInput,
    NonFiniteSimulation,
};

struct ParticleRuntimeResultUVE final {
    ParticleRuntimeCodeUVE code = ParticleRuntimeCodeUVE::InvalidComponent;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == ParticleRuntimeCodeUVE::Applied || code == ParticleRuntimeCodeUVE::Unchanged;
    }
};

struct ParticleEmissionUVE final {
    std::uint32_t count = 0U;
    Math::Vector3UVE position{};
    Math::Vector3UVE velocity{};
    float lifetimeSeconds = 0.0F;
};

struct ParticleStateUVE final {
    Math::Vector3UVE position{};
    Math::Vector3UVE velocity{};
    float remainingLifetimeSeconds = 0.0F;
    std::uint64_t sequence = 0U;

    [[nodiscard]] bool operator==(const ParticleStateUVE&) const = default;
};

struct ParticleRuntimeInstanceSnapshotUVE final {
    EntityUVE entity;
    std::uint32_t maxParticles = 0U;
    std::uint32_t liveParticles = 0U;
    std::uint64_t generation = 0U;
    bool enabled = false;

    [[nodiscard]] bool operator==(const ParticleRuntimeInstanceSnapshotUVE&) const = default;
};

struct ParticleRuntimeSnapshotUVE final {
    std::size_t instanceCount = 0U;
    std::uint64_t totalBudget = 0U;
    std::vector<ParticleRuntimeInstanceSnapshotUVE> instances;

    [[nodiscard]] bool operator==(const ParticleRuntimeSnapshotUVE&) const = default;
};

struct ParticleStateSnapshotUVE final {
    EntityUVE entity;
    std::uint64_t generation = 0U;
    std::vector<ParticleStateUVE> particles;

    [[nodiscard]] bool operator==(const ParticleStateSnapshotUVE&) const = default;
};

/// Bounded CPU particle state for authored emitters. This v1 owns only explicitly emitted particle
/// values and deterministic integration; GPU resources, renderer registration, asset lifetime, and
/// backend selection remain outside the runtime.
class ParticleRuntimeUVE final {
public:
    static constexpr std::size_t kMaximumInstancesUVE = 4096U;
    static constexpr std::uint64_t kMaximumTotalParticleBudgetUVE = 4'000'000U;
    static constexpr float kMaximumParticleLifetimeSecondsUVE = 3600.0F;
    static constexpr float kMaximumSimulationDeltaSecondsUVE = 0.25F;

    ParticleRuntimeUVE() = default;
    ParticleRuntimeUVE(const ParticleRuntimeUVE&) = delete;
    ParticleRuntimeUVE& operator=(const ParticleRuntimeUVE&) = delete;

    [[nodiscard]] ParticleRuntimeResultUVE AttachDetailedUVE(EntityUVE entity,
                                                              const ParticleEmitterComponentUVE& component);
    [[nodiscard]] bool AttachUVE(EntityUVE entity, const ParticleEmitterComponentUVE& component) {
        return AttachDetailedUVE(entity, component).IsAcceptedUVE();
    }
    [[nodiscard]] ParticleRuntimeResultUVE DetachDetailedUVE(EntityUVE entity) noexcept;
    [[nodiscard]] bool DetachUVE(EntityUVE entity) noexcept {
        return DetachDetailedUVE(entity).IsAcceptedUVE();
    }
    [[nodiscard]] ParticleRuntimeResultUVE SetEnabledDetailedUVE(EntityUVE entity, bool enabled) noexcept;
    [[nodiscard]] ParticleRuntimeResultUVE SetLiveParticleCountDetailedUVE(
        EntityUVE entity, std::uint32_t liveParticles) noexcept;
    [[nodiscard]] ParticleRuntimeResultUVE EmitDetailedUVE(EntityUVE entity,
                                                            const ParticleEmissionUVE& emission);
    [[nodiscard]] ParticleRuntimeResultUVE SimulateDetailedUVE(
        float deltaSeconds, const Math::Vector3UVE& acceleration) noexcept;
    [[nodiscard]] ParticleRuntimeSnapshotUVE GetSnapshotUVE() const;
    [[nodiscard]] std::optional<ParticleStateSnapshotUVE> GetParticleSnapshotUVE(EntityUVE entity) const;
    [[nodiscard]] bool HasInstanceUVE(EntityUVE entity) const noexcept;
    [[nodiscard]] std::size_t GetInstanceCountUVE() const noexcept;
    [[nodiscard]] std::uint64_t GetTotalBudgetUVE() const noexcept;

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

