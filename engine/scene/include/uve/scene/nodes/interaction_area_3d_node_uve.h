// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/scene/components/expanded_3d_node_components_uve.h"

namespace UVE::Scene::Nodes {

/// User-facing InteractionArea3D state. Game-specific interaction dispatch stays outside the node.
using InteractionArea3DNodeUVE = InteractionArea3DNodeComponentUVE;
using InteractionArea3DNodeFacadeUVE = InteractionArea3DNodeUVE;

} // namespace UVE::Scene::Nodes
