// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/scene/entity_uve.h"

namespace UVE::Scene::Nodes {

/// User-facing Empty node façade. Entity identity and lifecycle remain owned by Scene ECS.
using EmptyNodeUVE = EntityUVE;
using EmptyNodeFacadeUVE = EmptyNodeUVE;

} // namespace UVE::Scene::Nodes
