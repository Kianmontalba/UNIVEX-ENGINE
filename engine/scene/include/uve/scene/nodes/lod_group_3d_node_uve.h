// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/scene/components/expanded_3d_node_components_uve.h"

namespace UVE::Scene::Nodes {

/// User-facing LODGroup3D state with bounded monotonic thresholds and no per-frame allocation requirement.
using LODGroup3DNodeUVE = LodGroup3DNodeComponentUVE;
using LODGroup3DNodeFacadeUVE = LODGroup3DNodeUVE;

} // namespace UVE::Scene::Nodes
