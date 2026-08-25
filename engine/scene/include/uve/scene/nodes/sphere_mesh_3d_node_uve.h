// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/scene/components/primitive_mesh_component_uve.h"

namespace UVE::Scene::Nodes {

/// User-facing SphereMesh3D façade. Primitive mesh data remains owned by PrimitiveMeshComponentUVE.
using SphereMesh3DNodeUVE = PrimitiveMeshComponentUVE;
using SphereMesh3DNodeFacadeUVE = SphereMesh3DNodeUVE;

} // namespace UVE::Scene::Nodes
