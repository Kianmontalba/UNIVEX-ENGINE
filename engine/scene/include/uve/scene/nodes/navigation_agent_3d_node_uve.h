// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/scene/components/expanded_3d_node_components_uve.h"

namespace UVE::Scene::Nodes {

/// User-facing NavigationAgent3D state. Path calculation is delegated to a future navigation service.
using NavigationAgent3DNodeUVE = NavigationAgent3DNodeComponentUVE;
using NavigationAgent3DNodeFacadeUVE = NavigationAgent3DNodeUVE;
using NavigationAgentPathStatusUVE = ::UVE::Scene::NavigationAgentPathStatusUVE;

} // namespace UVE::Scene::Nodes
