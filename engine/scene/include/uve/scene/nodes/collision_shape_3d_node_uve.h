// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/scene/components/collider_component_uve.h"

namespace UVE::Scene::Nodes {

/// User-facing CollisionShape3D/Collider3D façade. Shape data remains owned by the scene collider component.
using CollisionShape3DNodeUVE = ColliderComponentUVE;
using CollisionShape3DNodeFacadeUVE = CollisionShape3DNodeUVE;
using CollisionShape3DTypeUVE = ColliderShapeTypeUVE;
using Collider3DNodeUVE = CollisionShape3DNodeUVE;
using Collider3DNodeFacadeUVE = CollisionShape3DNodeFacadeUVE;

} // namespace UVE::Scene::Nodes
