// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/scene/components/transform_component_uve.h"

namespace UVE::Scene::Nodes {

/// User-facing Transform façade. Hierarchy and transform propagation remain owned by SceneGraph.
using TransformNodeUVE = TransformComponentUVE;
using TransformNodeFacadeUVE = TransformNodeUVE;

} // namespace UVE::Scene::Nodes
