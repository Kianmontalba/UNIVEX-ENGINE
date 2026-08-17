// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/render/particle_render_bridge_uve.h"

#include <algorithm>

namespace UVE::Render {

ParticleRenderSnapshotUVE ParticleRenderBridgeUVE::ExtractUVE(
    const Scene::ParticleRuntimeUVE& runtime, const std::size_t maximumItems) {
    ParticleRenderSnapshotUVE snapshot;
    if (maximumItems == 0U) {
        snapshot.truncated = runtime.GetInstanceCountUVE() != 0U;
        return snapshot;
    }

    const Scene::ParticleRuntimeSnapshotUVE runtimeSnapshot = runtime.GetSnapshotUVE();
    for (const Scene::ParticleRuntimeInstanceSnapshotUVE& instance : runtimeSnapshot.instances) {
        if (!instance.enabled) {
            continue;
        }
        const std::optional<Scene::ParticleStateSnapshotUVE> particleSnapshot =
            runtime.GetParticleSnapshotUVE(instance.entity);
        if (!particleSnapshot.has_value()) {
            continue;
        }
        snapshot.sourceParticleCount += particleSnapshot->particles.size();
        for (const Scene::ParticleStateUVE& particle : particleSnapshot->particles) {
            if (snapshot.items.size() >= maximumItems) {
                snapshot.truncated = true;
                continue;
            }
            snapshot.items.push_back({instance.entity, particle.position, particle.remainingLifetimeSeconds,
                                      particle.position.z, particle.sequence});
        }
    }
    if (snapshot.sourceParticleCount > snapshot.items.size()) {
        snapshot.truncated = true;
    }
    return snapshot;
}

} // namespace UVE::Render

