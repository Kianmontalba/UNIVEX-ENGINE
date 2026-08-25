// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/scene/components/rigid_body_component_uve.h"

namespace UVE::Scene::Nodes {

/// User-facing RigidBody3D façade. Authored body state remains owned by RigidBodyComponentUVE.
using RigidBody3DNodeUVE = RigidBodyComponentUVE;
using RigidBody3DNodeFacadeUVE = RigidBody3DNodeUVE;

} // namespace UVE::Scene::Nodes
