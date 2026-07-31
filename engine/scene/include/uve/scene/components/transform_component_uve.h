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

/// The authored, parent-relative (local) position/rotation/scale of a scene-graph entity —
/// exactly what a Node3D "requires" per the master spec (Position, Rotation, Scale). Attach via
/// SceneGraphUVE::AttachTransformUVE() rather than directly, so the paired
/// WorldTransformComponentUVE/HierarchyComponentUVE are never forgotten. Setting this directly
/// via IEntityManagerUVE::GetComponentUVE() does NOT mark the entity dirty — use
/// SceneGraphUVE::SetLocalTransformUVE() to change it so world-transform propagation stays
/// correct.
struct TransformComponentUVE final {
    Math::Vector3UVE localPosition{};
    Math::QuaternionUVE localRotation{};
    Math::Vector3UVE localScale{1.0F, 1.0F, 1.0F};
};

} // namespace UVE::Scene
