// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/scene/particle_runtime_uve.h"

#include <algorithm>
#include <limits>
#include <utility>

namespace UVE::Scene {
namespace {

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

    m_instances.emplace(entity, InstanceUVE{entity, component.maxParticles, 0U, 1U, true});
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

