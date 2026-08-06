// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstdint>

namespace UVE::Scene {

/// One of the master spec's named built-in components (Part 7.3). Deliberately minimal
/// placeholder data — a particle budget is the one universally-understood basic of a particle
/// emitter. Will be extended once a particle system (Part 7.2's ComputeSystemUVE / a future VFX
/// system) exists to consume it.
struct ParticleEmitterComponentUVE final {
    std::uint32_t maxParticles = 100;
};

} // namespace UVE::Scene
