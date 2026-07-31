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

/// Published (immediately, via IEventSystemUVE::Publish<T>() — not queued, since entity
/// creation already happens synchronously on the main thread) by
/// IEntityManagerUVE::CreateEntityUVE() once `entity` is fully created. The minimal, cheap
/// lifecycle hook a future Editor Scene Outliner or other system can subscribe to.
struct EntityCreatedEventUVE {
    EntityUVE entity;
};

/// Published (immediately) by IEntityManagerUVE::DestroyEntityUVE() just before `entity`'s
/// slot is returned to the free list (so subscribers can still safely inspect `entity` as "the
/// entity that just went away," though it is already dead by the time this delivers).
struct EntityDestroyedEventUVE {
    EntityUVE entity;
};

} // namespace UVE::Scene
