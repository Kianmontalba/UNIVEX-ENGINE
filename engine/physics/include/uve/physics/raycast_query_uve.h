//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <cstdint>

#include "uve/math/ray_uve.h"
#include "uve/scene/entity_uve.h"

namespace UVE::Physics {

/// Bundles IRaycastSystemUVE::RaycastUVE()'s query parameters, matching this codebase's existing
/// `Desc`/`Settings`-struct convention (Render::BufferDescUVE, Asset::AssetImportSettingsUVE) —
/// growing this struct with a new field later never breaks the call signature, unlike growing a
/// parameter list would.
struct RaycastQueryUVE {
    Math::RayUVE ray;
    float maxDistance = 0.0F;

    /// Filters via `(collider.collisionLayer & layerMask) != 0` — defaults to "everything."
    std::uint32_t layerMask = 0xFFFFFFFFU;

    /// An entity to exclude from consideration entirely, regardless of layer or distance.
    /// Defaults to Scene::kInvalidEntityUVE (nothing excluded). Added so a future
    /// CharacterController's ground-check ray doesn't hit its own collider — a near-certain, not
    /// speculative, need; retrofitting this later would mean a breaking signature change.
    Scene::EntityUVE ignoreEntity = Scene::kInvalidEntityUVE;
};

} // namespace UVE::Physics
