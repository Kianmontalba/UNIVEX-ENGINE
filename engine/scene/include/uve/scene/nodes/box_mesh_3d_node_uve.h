// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/scene/components/primitive_mesh_component_uve.h"

namespace UVE::Scene::Nodes {

/// User-facing BoxMesh3D façade. Primitive mesh data remains owned by PrimitiveMeshComponentUVE.
using BoxMesh3DNodeUVE = PrimitiveMeshComponentUVE;
using BoxMesh3DNodeFacadeUVE = BoxMesh3DNodeUVE;

} // namespace UVE::Scene::Nodes
