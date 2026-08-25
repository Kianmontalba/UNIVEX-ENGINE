// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/scene/components/expanded_3d_node_components_uve.h"

namespace UVE::Scene::Nodes {

/// User-facing Decal3D state. Projection and GPU submission remain renderer-owned concerns.
using Decal3DNodeUVE = Decal3DNodeComponentUVE;
using Decal3DNodeFacadeUVE = Decal3DNodeUVE;
using DecalProjectionModeUVE = ::UVE::Scene::DecalProjectionModeUVE;

} // namespace UVE::Scene::Nodes
