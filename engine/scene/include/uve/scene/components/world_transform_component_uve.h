//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include "uve/math/quaternion_uve.h"
#include "uve/math/vector3_uve.h"

namespace UVE::Scene {

/// The cached, derived world-space position/rotation/scale of a scene-graph entity — internal
/// infrastructure SceneGraphUVE owns, not one of the master spec's named built-in components.
/// Kept as a component separate from TransformComponentUVE so setting the authored local
/// transform never has to touch this cached data (and vice versa). `dirty` is set whenever the
/// authored local transform or the parent link changes (see SceneGraphUVE::
/// SetLocalTransformUVE()/SetParentUVE()); SceneGraphUVE::UpdateUVE() clears it after
/// recomputing worldPosition/worldRotation/worldScale from the parent chain. Only
/// SceneGraphUVE writes to this component — treat it as read-only elsewhere.
struct WorldTransformComponentUVE final {
    Math::Vector3UVE worldPosition{};
    Math::QuaternionUVE worldRotation{};
    Math::Vector3UVE worldScale{1.0F, 1.0F, 1.0F};
    bool dirty = true;
};

} // namespace UVE::Scene
