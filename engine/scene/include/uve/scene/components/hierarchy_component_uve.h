//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include "uve/scene/entity_uve.h"

namespace UVE::Scene {

/// The parent link for a scene-graph entity — internal infrastructure SceneGraphUVE owns, not
/// one of the master spec's named built-in components. Deliberately holds only the parent (not
/// a children list too), so there is exactly one place the parent/child relationship is
/// recorded; SceneGraphUVE::GetChildrenUVE() derives the children of a given entity by
/// scanning for this component rather than maintaining a second, redundant index. `parent ==
/// kInvalidEntityUVE` means "this entity is a scene root."
struct HierarchyComponentUVE final {
    EntityUVE parent = kInvalidEntityUVE;
};

} // namespace UVE::Scene
