// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/scene/components/expanded_3d_node_components_uve.h"

namespace UVE::Scene::Nodes {

/// User-facing Occluder3D state. Occlusion data is consumed by a future renderer culling service.
using Occluder3DNodeUVE = Occluder3DNodeComponentUVE;
using Occluder3DNodeFacadeUVE = Occluder3DNodeUVE;
using Occluder3DNodeModeUVE = ::UVE::Scene::Occluder3DNodeModeUVE;

} // namespace UVE::Scene::Nodes
