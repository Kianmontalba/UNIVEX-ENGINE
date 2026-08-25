// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/scene/components/collider_component_uve.h"

namespace UVE::Scene::Nodes {

/// User-facing StaticBody3D façade. A collider without a rigid body remains static in Physics.
using StaticBody3DNodeUVE = ColliderComponentUVE;
using StaticBody3DNodeFacadeUVE = StaticBody3DNodeUVE;

} // namespace UVE::Scene::Nodes
