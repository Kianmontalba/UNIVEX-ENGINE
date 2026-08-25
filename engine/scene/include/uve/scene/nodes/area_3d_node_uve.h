// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/scene/components/area_component_uve.h"

namespace UVE::Scene::Nodes {

/// User-facing Area3D façade. Overlap queries and lifecycle transitions remain owned by Physics.
using Area3DNodeUVE = AreaComponentUVE;
using Area3DNodeFacadeUVE = Area3DNodeUVE;

} // namespace UVE::Scene::Nodes
