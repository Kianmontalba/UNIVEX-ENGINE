// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/scene/components/light_component_uve.h"

namespace UVE::Scene::Nodes {

/// User-facing Light3D façade. Light state remains owned by Scene::LightComponentUVE.
using Light3DNodeUVE = LightComponentUVE;
using Light3DNodeFacadeUVE = Light3DNodeUVE;
using Light3DTypeUVE = LightTypeUVE;

} // namespace UVE::Scene::Nodes
