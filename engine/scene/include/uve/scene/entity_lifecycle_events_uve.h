// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


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
