#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
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

/// IParticleRuntimeUVE is the gameplay/content-facing boundary for bounded CPU particle state.
/// Implementations own only explicitly emitted particle values and deterministic integration;
/// GPU resources, renderer registration, asset lifetime, and backend selection remain outside.
/// Thread-safety: not thread-safe; callers use it from the scene/runtime thread.
class IParticleRuntimeUVE {
public:
    virtual ~IParticleRuntimeUVE() = default;

    [[nodiscard]] virtual ParticleRuntimeResultUVE AttachDetailedUVE(
        EntityUVE entity, const ParticleEmitterComponentUVE& component) = 0;
    [[nodiscard]] bool AttachUVE(EntityUVE entity, const ParticleEmitterComponentUVE& component) {
        return AttachDetailedUVE(entity, component).IsAcceptedUVE();
    }

    [[nodiscard]] virtual ParticleRuntimeResultUVE DetachDetailedUVE(EntityUVE entity) noexcept = 0;
    [[nodiscard]] bool DetachUVE(EntityUVE entity) noexcept {
        return DetachDetailedUVE(entity).IsAcceptedUVE();
    }

    [[nodiscard]] virtual ParticleRuntimeResultUVE SetEnabledDetailedUVE(
        EntityUVE entity, bool enabled) noexcept = 0;
    [[nodiscard]] virtual ParticleRuntimeResultUVE SetLiveParticleCountDetailedUVE(
        EntityUVE entity, std::uint32_t liveParticles) noexcept = 0;
    [[nodiscard]] virtual ParticleRuntimeResultUVE EmitDetailedUVE(
        EntityUVE entity, const ParticleEmissionUVE& emission) = 0;
    [[nodiscard]] virtual ParticleRuntimeResultUVE SimulateDetailedUVE(
        float deltaSeconds, const Math::Vector3UVE& acceleration) noexcept = 0;
    [[nodiscard]] virtual ParticleRuntimeSnapshotUVE GetSnapshotUVE() const = 0;
    [[nodiscard]] virtual std::optional<ParticleStateSnapshotUVE> GetParticleSnapshotUVE(
        EntityUVE entity) const = 0;
    [[nodiscard]] virtual bool HasInstanceUVE(EntityUVE entity) const noexcept = 0;
    [[nodiscard]] virtual std::size_t GetInstanceCountUVE() const noexcept = 0;
    [[nodiscard]] virtual std::uint64_t GetTotalBudgetUVE() const noexcept = 0;
};

} // namespace UVE::Scene
