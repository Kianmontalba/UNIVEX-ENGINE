// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/core/animation_tree_uve.h"

namespace UVE::Scene::Nodes {

/// User-facing AnimationTree runtime façade. No authored scene component exists yet, so the
/// registry intentionally exposes this node as non-creatable until persistence is integrated.
using AnimationTreeNodeUVE = Core::AnimationTreeUVE;
using AnimationTreeNodeFacadeUVE = AnimationTreeNodeUVE;

} // namespace UVE::Scene::Nodes
