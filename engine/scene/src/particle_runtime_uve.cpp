// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/scene/particle_runtime_uve.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace UVE::Scene {
namespace {

[[nodiscard]] bool IsFiniteVectorUVE(const Math::Vector3UVE& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] ParticleRuntimeResultUVE MakeResultUVE(const ParticleRuntimeCodeUVE code,
                                                      std::string message) {
    return {code, std::move(message)};
}

} // namespace

ParticleRuntimeResultUVE ParticleRuntimeUVE::AttachDetailedUVE(
    const EntityUVE entity, const ParticleEmitterComponentUVE& component) {
    if (entity == kInvalidEntityUVE) {
        return MakeResultUVE(ParticleRuntimeCodeUVE::InvalidEntity,
                             "Particle runtime attach requires a valid generational entity.");
    }
    if (!IsParticleEmitterComponentValidUVE(component)) {
        return MakeResultUVE(ParticleRuntimeCodeUVE::InvalidComponent,
                             "Particle runtime attach rejected an invalid emitter budget.");
    }
    if (m_instances.find(entity) != m_instances.end()) {
        return MakeResultUVE(ParticleRuntimeCodeUVE::DuplicateInstance,
                             "Particle runtime attach rejected a duplicate entity instance.");
    }
    if (m_instances.size() >= kMaximumInstancesUVE) {
        return MakeResultUVE(ParticleRuntimeCodeUVE::CapacityExceeded,
                             "Particle runtime attach rejected the instance capacity limit.");
    }
    if (component.maxParticles > kMaximumTotalParticleBudgetUVE - m_totalBudget) {
        return MakeResultUVE(ParticleRuntimeCodeUVE::CapacityExceeded,
                             "Particle runtime attach rejected the aggregate particle budget limit.");
    }

    m_instances.emplace(entity, InstanceUVE{entity, component.maxParticles, 0U, 1U, 1U, true, {}});
    m_totalBudget += component.maxParticles;
    return MakeResultUVE(ParticleRuntimeCodeUVE::Applied,
                         "Particle emitter ownership attached without allocating particle storage.");
}

ParticleRuntimeResultUVE ParticleRuntimeUVE::DetachDetailedUVE(const EntityUVE entity) noexcept {
    const auto iterator = m_instances.find(entity);
    if (iterator == m_instances.end()) {
        return MakeResultUVE(ParticleRuntimeCodeUVE::NoActiveInstance,
                             "Particle runtime detach found no active entity instance.");
    }
    m_totalBudget -= iterator->second.maxParticles;
    m_instances.erase(iterator);
    return MakeResultUVE(ParticleRuntimeCodeUVE::Applied,
                         "Particle emitter ownership detached and its budget was released.");
}

ParticleRuntimeResultUVE ParticleRuntimeUVE::SetEnabledDetailedUVE(const EntityUVE entity,
                                                                    const bool enabled) noexcept {
    const auto iterator = m_instances.find(entity);
    if (iterator == m_instances.end()) {
        return MakeResultUVE(ParticleRuntimeCodeUVE::NoActiveInstance,
                             "Particle runtime enable update found no active entity instance.");
    }
    if (iterator->second.enabled == enabled) {
        return MakeResultUVE(ParticleRuntimeCodeUVE::Unchanged,
                             "Particle runtime enabled state was already unchanged.");
    }
    iterator->second.enabled = enabled;
    return MakeResultUVE(ParticleRuntimeCodeUVE::Applied,
                         "Particle runtime enabled state updated without changing ownership budget.");
}

ParticleRuntimeResultUVE ParticleRuntimeUVE::SetLiveParticleCountDetailedUVE(
    const EntityUVE entity, const std::uint32_t liveParticles) noexcept {
    const auto iterator = m_instances.find(entity);
    if (iterator == m_instances.end()) {
        return MakeResultUVE(ParticleRuntimeCodeUVE::NoActiveInstance,
                             "Particle runtime live-count update found no active entity instance.");
    }
    if (liveParticles > iterator->second.maxParticles) {
        return MakeResultUVE(ParticleRuntimeCodeUVE::LiveParticleCountExceeded,
                             "Particle runtime live count exceeds the emitter budget.");
    }
    if (iterator->second.liveParticles == liveParticles) {
        return MakeResultUVE(ParticleRuntimeCodeUVE::Unchanged,
                             "Particle runtime live count was already unchanged.");
    }
    iterator->second.liveParticles = liveParticles;
    return MakeResultUVE(ParticleRuntimeCodeUVE::Applied,
                         "Particle runtime live count updated within the authored budget.");
}

ParticleRuntimeResultUVE ParticleRuntimeUVE::EmitDetailedUVE(const EntityUVE entity,
                                                              const ParticleEmissionUVE& emission) {
    const auto iterator = m_instances.find(entity);
    if (iterator == m_instances.end()) {
        return MakeResultUVE(ParticleRuntimeCodeUVE::NoActiveInstance,
                             "Particle emission found no active entity instance.");
    }
    if (!iterator->second.enabled) {
        return MakeResultUVE(ParticleRuntimeCodeUVE::DisabledInstance,
                             "Particle emission was rejected for a disabled entity instance.");
    }
    if (emission.count == 0U) {
        return MakeResultUVE(ParticleRuntimeCodeUVE::Unchanged,
                             "Particle emission requested zero new particles.");
    }
    if (emission.lifetimeSeconds <= 0.0F ||
        emission.lifetimeSeconds > kMaximumParticleLifetimeSecondsUVE ||
        !std::isfinite(emission.lifetimeSeconds) || !IsFiniteVectorUVE(emission.position) ||
        !IsFiniteVectorUVE(emission.velocity)) {
        return MakeResultUVE(ParticleRuntimeCodeUVE::InvalidSimulationInput,
                             "Particle emission requires finite position/velocity and bounded positive lifetime.");
    }
    const std::size_t currentCount = iterator->second.particles.size();
    if (static_cast<std::uint64_t>(emission.count) >
        static_cast<std::uint64_t>(iterator->second.maxParticles) - currentCount) {
        return MakeResultUVE(ParticleRuntimeCodeUVE::LiveParticleCountExceeded,
                             "Particle emission exceeds the emitter particle budget.");
    }
    if (iterator->second.nextSequence == 0U ||
        static_cast<std::uint64_t>(emission.count) >
            std::numeric_limits<std::uint64_t>::max() - iterator->second.nextSequence + 1U) {
        return MakeResultUVE(ParticleRuntimeCodeUVE::CapacityExceeded,
                             "Particle emission would exhaust the deterministic sequence range.");
    }

    iterator->second.particles.reserve(currentCount + emission.count);
    for (std::uint32_t index = 0U; index < emission.count; ++index) {
        const std::uint64_t sequence = iterator->second.nextSequence;
        iterator->second.nextSequence =
            sequence == std::numeric_limits<std::uint64_t>::max() ? 0U : sequence + 1U;
        iterator->second.particles.push_back(
            {emission.position, emission.velocity, emission.lifetimeSeconds, sequence});
    }
    iterator->second.liveParticles = static_cast<std::uint32_t>(iterator->second.particles.size());
    return MakeResultUVE(ParticleRuntimeCodeUVE::Applied,
                         "Particle emission appended deterministic CPU particle state.");
}

ParticleRuntimeResultUVE ParticleRuntimeUVE::SimulateDetailedUVE(
    const float deltaSeconds, const Math::Vector3UVE& acceleration) noexcept {
    if (deltaSeconds < 0.0F || deltaSeconds > kMaximumSimulationDeltaSecondsUVE ||
        !std::isfinite(deltaSeconds) || !IsFiniteVectorUVE(acceleration)) {
        return MakeResultUVE(ParticleRuntimeCodeUVE::InvalidSimulationInput,
                             "Particle simulation requires finite acceleration and bounded non-negative delta time.");
    }
    if (deltaSeconds == 0.0F) {
        return MakeResultUVE(ParticleRuntimeCodeUVE::Unchanged,
                             "Particle simulation received a zero delta time.");
    }

    bool hasWork = false;
    for (const auto& [entity, instance] : m_instances) {
        static_cast<void>(entity);
        if (!instance.enabled || instance.particles.empty()) {
            continue;
        }
        for (const ParticleStateUVE& particle : instance.particles) {
            const Math::Vector3UVE nextVelocity = particle.velocity + acceleration * deltaSeconds;
            const Math::Vector3UVE nextPosition = particle.position + nextVelocity * deltaSeconds;
            const float nextLifetime = particle.remainingLifetimeSeconds - deltaSeconds;
            if (!IsFiniteVectorUVE(nextVelocity) || !IsFiniteVectorUVE(nextPosition) ||
                !std::isfinite(nextLifetime)) {
                return MakeResultUVE(ParticleRuntimeCodeUVE::NonFiniteSimulation,
                                     "Particle simulation rejected a non-finite integrated state atomically.");
            }
        }
        hasWork = true;
    }
    if (!hasWork) {
        return MakeResultUVE(ParticleRuntimeCodeUVE::Unchanged,
                             "Particle simulation found no enabled live particle state.");
    }

    for (auto& [entity, instance] : m_instances) {
        static_cast<void>(entity);
        if (!instance.enabled || instance.particles.empty()) {
            continue;
        }
        std::size_t writeIndex = 0U;
        for (const ParticleStateUVE& particle : instance.particles) {
            const Math::Vector3UVE nextVelocity = particle.velocity + acceleration * deltaSeconds;
            const Math::Vector3UVE nextPosition = particle.position + nextVelocity * deltaSeconds;
            const float nextLifetime = particle.remainingLifetimeSeconds - deltaSeconds;
            if (nextLifetime <= 0.0F) {
                continue;
            }
            instance.particles[writeIndex++] =
                {nextPosition, nextVelocity, nextLifetime, particle.sequence};
        }
        instance.particles.resize(writeIndex);
        instance.liveParticles = static_cast<std::uint32_t>(writeIndex);
    }
    return MakeResultUVE(ParticleRuntimeCodeUVE::Applied,
                         "Particle simulation advanced enabled CPU particle state deterministically.");
}

ParticleRuntimeSnapshotUVE ParticleRuntimeUVE::GetSnapshotUVE() const {
    ParticleRuntimeSnapshotUVE snapshot;
    snapshot.instanceCount = m_instances.size();
    snapshot.totalBudget = m_totalBudget;
    snapshot.instances.reserve(m_instances.size());
    for (const auto& [entity, instance] : m_instances) {
        snapshot.instances.push_back(
            {entity, instance.maxParticles, instance.liveParticles, instance.generation, instance.enabled});
    }
    std::sort(snapshot.instances.begin(), snapshot.instances.end(),
              [](const ParticleRuntimeInstanceSnapshotUVE& lhs, const ParticleRuntimeInstanceSnapshotUVE& rhs) {
                  if (lhs.entity.index != rhs.entity.index) {
                      return lhs.entity.index < rhs.entity.index;
                  }
                  return lhs.entity.generation < rhs.entity.generation;
              });
    return snapshot;
}

std::optional<ParticleStateSnapshotUVE> ParticleRuntimeUVE::GetParticleSnapshotUVE(
    const EntityUVE entity) const {
    const auto iterator = m_instances.find(entity);
    if (iterator == m_instances.end()) {
        return std::nullopt;
    }
    return ParticleStateSnapshotUVE{entity, iterator->second.generation, iterator->second.particles};
}

bool ParticleRuntimeUVE::HasInstanceUVE(const EntityUVE entity) const noexcept {
    return m_instances.find(entity) != m_instances.end();
}

std::size_t ParticleRuntimeUVE::GetInstanceCountUVE() const noexcept {
    return m_instances.size();
}

std::uint64_t ParticleRuntimeUVE::GetTotalBudgetUVE() const noexcept {
    return m_totalBudget;
}

} // namespace UVE::Scene

