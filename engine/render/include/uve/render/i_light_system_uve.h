//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include "uve/math/vector3_uve.h"
#include "uve/scene/i_entity_manager_uve.h"

namespace UVE::Render {

/// This frame's single active directional light, extracted by ILightSystemUVE::ExtractActiveLightUVE().
/// `intensity == 0.0F` is the deliberate "no active light this frame" sentinel — no std::optional,
/// matching this codebase's existing no-shader-branching philosophy (compare Renderer3DUVE's
/// fallback-texture pattern, Increment 22): a shader multiplying by uLightIntensity naturally goes
/// dark with no special-casing needed on either the C++ or GLSL side.
struct DirectionalLightDataUVE {
    // direction = where light PHOTONS travel (NOT surface-to-light). GLSL usage:
    // max(dot(N, -direction), 0.0) for Lambertian diffuse (negate: 'direction' points
    // toward the surface, N·L needs the surface-to-light vector).
    Math::Vector3UVE direction{0.0F, 0.0F, -1.0F};
    Math::Vector3UVE color{1.0F, 1.0F, 1.0F};
    float intensity = 0.0F;
};

/// ILightSystemUVE extracts this frame's single active directional light from the ECS (the spec's
/// `LightSystemUVE`, Part 7.2 — "Light culling, IBL (diffuse + specular probes)"). Culling and IBL
/// are multi-light features, explicitly deferred until a future increment: this v1 supports at
/// most one active light, with no point/spot lights. Reads every entity with both
/// `Scene::WorldTransformComponentUVE` and `Scene::LightComponentUVE` via
/// `IEntityManagerUVE::ForEachUVE`.
/// Thread-safety: implementations should be stateless (holding no members), matching
/// `ICameraSystemUVE`'s/`IMeshRendererUVE`'s contract — every method only reads the
/// `IEntityManagerUVE` passed in.
class ILightSystemUVE {
public:
    virtual ~ILightSystemUVE() = default;

    /// Returns the first light entity `ForEachUVE<WorldTransformComponentUVE, LightComponentUVE>`
    /// encounters this call, or a default-constructed DirectionalLightDataUVE{} (intensity 0.0F)
    /// if none exists. `IEntityManagerUVE::ForEachUVE` only guarantees "every matching entity
    /// exactly once, order unspecified" — if more than one light entity exists, which one is
    /// returned is arbitrary/unspecified this v1 (a real "which light wins" policy is future
    /// multi-light work). Deterministic and stable for any fixed set of light entities that all
    /// share one archetype (the common case): within a single archetype, iteration order is
    /// chunk/row creation order. Across different archetypes, order is
    /// `std::unordered_map`-hash-dependent — not a documented guarantee.
    /// Non-`const` `entityManager`, unlike `ICameraSystemUVE`'s methods: `ForEachUVE` has no
    /// `const` overload (same reason `IMeshRendererUVE::ExtractRenderQueueUVE` takes a non-`const`
    /// reference too).
    [[nodiscard]] virtual DirectionalLightDataUVE ExtractActiveLightUVE(
        Scene::IEntityManagerUVE& entityManager) const = 0;
};

} // namespace UVE::Render
