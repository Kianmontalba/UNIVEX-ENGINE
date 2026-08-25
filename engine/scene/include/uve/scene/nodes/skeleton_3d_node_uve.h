// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/scene/components/expanded_3d_node_components_uve.h"

namespace UVE::Scene::Nodes {

/// User-facing Skeleton3D state. Animation and skinning systems consume this bounded data later.
using Skeleton3DNodeUVE = Skeleton3DNodeComponentUVE;
using Skeleton3DNodeFacadeUVE = Skeleton3DNodeUVE;
using SkeletonBoneUVE = ::UVE::Scene::SkeletonBoneUVE;

} // namespace UVE::Scene::Nodes
