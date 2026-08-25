// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/scene/components/animation_player_component_uve.h"

namespace UVE::Scene::Nodes {

/// User-facing AnimationPlayer façade. Authored animation-player state remains serialized by Scene.
using AnimationPlayerNodeUVE = AnimationPlayerComponentUVE;
using AnimationPlayerNodeFacadeUVE = AnimationPlayerNodeUVE;

} // namespace UVE::Scene::Nodes
