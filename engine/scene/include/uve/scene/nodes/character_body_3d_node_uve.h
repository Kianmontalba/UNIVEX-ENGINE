// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/physics/character_controller_uve.h"

namespace UVE::Scene::Nodes {

/// User-facing CharacterBody3D façade. Movement and collision solving remain owned by Physics.
using CharacterBody3DNodeUVE = Physics::CharacterControllerInputUVE;
using CharacterBody3DNodeFacadeUVE = CharacterBody3DNodeUVE;
using CharacterBody3DMoveResultUVE = Physics::CharacterControllerMoveResultUVE;
using CharacterBody3DControllerUVE = Physics::CharacterControllerUVE;

} // namespace UVE::Scene::Nodes
