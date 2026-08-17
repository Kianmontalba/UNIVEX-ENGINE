// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

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
};

struct ParticleRuntimeResultUVE final {
    ParticleRuntimeCodeUVE code = ParticleRuntimeCodeUVE::InvalidComponent;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == ParticleRuntimeCodeUVE::Applied || code == ParticleRuntimeCodeUVE::Unchanged;
    }
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

/// Bounded ownership/runtime bookkeeping for authored particle emitters. This v1 deliberately owns
/// no particle arrays, GPU resources, simulation backend, renderer registration, or asset lifetime;
/// it only establishes a deterministic lifecycle and budget contract for later simulation work.
class ParticleRuntimeUVE final {
public:
    static constexpr std::size_t kMaximumInstancesUVE = 4096U;
    static constexpr std::uint64_t kMaximumTotalParticleBudgetUVE = 4'000'000U;

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
    [[nodiscard]] ParticleRuntimeSnapshotUVE GetSnapshotUVE() const;
    [[nodiscard]] bool HasInstanceUVE(EntityUVE entity) const noexcept;
    [[nodiscard]] std::size_t GetInstanceCountUVE() const noexcept;
    [[nodiscard]] std::uint64_t GetTotalBudgetUVE() const noexcept;

private:
    struct InstanceUVE final {
        EntityUVE entity;
        std::uint32_t maxParticles = 0U;
        std::uint32_t liveParticles = 0U;
        std::uint64_t generation = 1U;
        bool enabled = true;
    };

    std::unordered_map<EntityUVE, InstanceUVE> m_instances;
    std::uint64_t m_totalBudget = 0U;
};

} // namespace UVE::Scene

