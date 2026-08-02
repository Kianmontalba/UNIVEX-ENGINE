//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <array>
#include <cstddef>

#include "uve/math/vector3_uve.h"
#include "uve/scene/components/light_component_uve.h"
#include "uve/scene/i_entity_manager_uve.h"

namespace UVE::Render {

/// The fixed maximum number of simultaneously active lights ILightSystemUVE extracts per frame
/// (Increment 25). No light culling/prioritization system exists — if more than this many light
/// entities exist in a scene, which ones are kept is first-encountered order (see
/// ILightSystemUVE::ExtractActiveLightsUVE's own doc comment), not a distance- or
/// importance-based selection. A compile-time constant, not an EngineConfigUVE field — mirrors
/// the existing fixed-texture-slot-constant precedent (kAlbedoTextureSlotUVE etc. in
/// renderer_3d_uve.cpp) rather than adding runtime configurability nobody asked for.
constexpr std::size_t kMaxLightsUVE = 4;

/// One light slot's data for this frame, extracted by ILightSystemUVE::ExtractActiveLightsUVE().
/// `intensity == 0.0F` is the deliberate "this slot is empty" sentinel — no std::optional,
/// matching this codebase's existing no-shader-branching philosophy (compare Renderer3DUVE's
/// fallback-texture pattern, Increment 22): a shader multiplying by uLights[i].intensity
/// naturally goes dark with no special-casing needed on either the C++ or GLSL side. Always a
/// full record regardless of `type` — e.g. a Directional slot's `position`/`range`/
/// `spotAngleDegrees` are simply unused by the (documented, not implemented) GLSL type-branch,
/// rather than the C++ side omitting fields based on type.
struct LightDataUVE {
    Scene::LightTypeUVE type = Scene::LightTypeUVE::Directional;

    /// Point/Spot: world position. Unused (meaningless) for Directional.
    Math::Vector3UVE position{};

    // direction = where light PHOTONS travel (NOT surface-to-light). GLSL usage:
    // max(dot(N, -direction), 0.0) for Lambertian diffuse (negate: 'direction' points
    // toward the surface, N·L needs the surface-to-light vector). Meaningful for
    // Directional/Spot; unused for Point (radiates in all directions).
    Math::Vector3UVE direction{0.0F, 0.0F, -1.0F};

    Math::Vector3UVE color{1.0F, 1.0F, 1.0F};
    float intensity = 0.0F;

    /// Point/Spot falloff distance. Unused for Directional.
    float range = 10.0F;

    /// Spot cone half-angle, in degrees. Unused for Directional/Point.
    float spotAngleDegrees = 45.0F;
};

/// A fixed-size list of this frame's active lights — see kMaxLightsUVE. Trailing unused slots
/// hold the default LightDataUVE{} sentinel (intensity == 0.0F).
using LightListUVE = std::array<LightDataUVE, kMaxLightsUVE>;

/// ILightSystemUVE extracts this frame's active lights from the ECS (the spec's `LightSystemUVE`,
/// Part 7.2 — "Light culling, IBL (diffuse + specular probes)"). Culling and IBL remain deferred
/// future work; Increment 25 adds Point/Spot light types and support for up to kMaxLightsUVE
/// simultaneous lights (v1, Increment 23, supported at most one Directional light only). Reads
/// every entity with both `Scene::WorldTransformComponentUVE` and `Scene::LightComponentUVE` via
/// `IEntityManagerUVE::ForEachUVE`.
/// Thread-safety: implementations should be stateless (holding no members), matching
/// `ICameraSystemUVE`'s/`IMeshRendererUVE`'s contract — every method only reads the
/// `IEntityManagerUVE` passed in.
class ILightSystemUVE {
public:
    virtual ~ILightSystemUVE() = default;

    /// Fills up to kMaxLightsUVE slots with the first light entities
    /// `ForEachUVE<WorldTransformComponentUVE, LightComponentUVE>` encounters this call; any
    /// slot beyond the number of light entities present stays at the default LightDataUVE{}
    /// sentinel (intensity 0.0F). `IEntityManagerUVE::ForEachUVE` only guarantees "every matching
    /// entity exactly once, order unspecified" — if more than kMaxLightsUVE light entities exist,
    /// which ones are kept is first-encountered/arbitrary this v1 (no distance- or
    /// importance-based selection — that's future work, same deferral v1 already documented).
    /// Deterministic and stable for any fixed set of light entities that all share one archetype
    /// (the common case): within a single archetype, iteration order is chunk/row creation order.
    /// Across different archetypes, order is `std::unordered_map`-hash-dependent — not a
    /// documented guarantee.
    /// Non-`const` `entityManager`, unlike `ICameraSystemUVE`'s methods: `ForEachUVE` has no
    /// `const` overload (same reason `IMeshRendererUVE::ExtractRenderQueueUVE` takes a non-`const`
    /// reference too).
    [[nodiscard]] virtual LightListUVE ExtractActiveLightsUVE(Scene::IEntityManagerUVE& entityManager) const = 0;
};

} // namespace UVE::Render
