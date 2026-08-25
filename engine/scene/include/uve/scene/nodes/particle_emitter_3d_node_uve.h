// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/scene/components/particle_emitter_component_uve.h"

namespace UVE::Scene::Nodes {

/// User-facing ParticleEmitter3D façade. Particle runtime ownership remains in scene/core systems.
using ParticleEmitter3DNodeUVE = ParticleEmitterComponentUVE;
using ParticleEmitter3DNodeFacadeUVE = ParticleEmitter3DNodeUVE;

} // namespace UVE::Scene::Nodes
