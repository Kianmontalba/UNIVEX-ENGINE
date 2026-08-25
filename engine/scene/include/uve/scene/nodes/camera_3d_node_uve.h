// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/scene/components/camera_component_uve.h"

namespace UVE::Scene::Nodes {

/// User-facing Camera3D façade. Camera authored state remains owned by Scene::CameraComponentUVE.
using Camera3DNodeUVE = CameraComponentUVE;
using Camera3DNodeFacadeUVE = Camera3DNodeUVE;

} // namespace UVE::Scene::Nodes
