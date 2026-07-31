//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include "uve/math/frustum_uve.h"
#include "uve/math/matrix4x4_uve.h"
#include "uve/scene/entity_uve.h"
#include "uve/scene/i_entity_manager_uve.h"

namespace UVE::Render {

/// ICameraSystemUVE computes view/projection matrices from a camera entity's ECS data and
/// extracts a culling frustum from them (the spec's `CameraSystemUVE`, Part 7.2 — "Camera
/// management, frustum culling, occlusion queries"; occlusion queries are future work, deferred
/// until a real GPU backend exists to query). Every method reads `cameraEntity`'s
/// `Scene::WorldTransformComponentUVE` (position/rotation) and/or `Scene::CameraComponentUVE`
/// (fov/near/far) — the entity must have both, exactly the same contract
/// `IEntityManagerUVE::GetComponentUVE<T>()` already enforces (asserts alive + has the
/// component). There is no graceful-missing-component fallback: entities reach `CameraSystemUVE`
/// only via `SceneGraphUVE::AttachTransformUVE()`, which always adds `WorldTransformComponentUVE`
/// alongside the authored `TransformComponentUVE` in one call, so a camera entity missing its
/// world transform shouldn't occur in practice.
/// Thread-safety: implementations should be stateless (holding no members) and thread-safe,
/// matching `SceneGraphUVE`'s contract — every method only reads the `IEntityManagerUVE` passed
/// in.
class ICameraSystemUVE {
public:
    virtual ~ICameraSystemUVE() = default;

    /// The world-to-view transform for `cameraEntity`, built directly from its world position and
    /// rotation (see `Math::Matrix4x4UVE::ViewFromPositionAndRotationUVE`).
    [[nodiscard]] virtual Math::Matrix4x4UVE ComputeViewMatrixUVE(const Scene::IEntityManagerUVE& entityManager,
                                                                   Scene::EntityUVE cameraEntity) const = 0;

    /// The view-to-clip perspective projection for `cameraEntity`, built from its
    /// `CameraComponentUVE` fov/near/far and the caller-supplied `aspectRatio` (width / height of
    /// the target being rendered into — not itself part of any component, since the same camera
    /// entity may render into differently-shaped targets, e.g. an editor viewport vs. the game
    /// window).
    [[nodiscard]] virtual Math::Matrix4x4UVE ComputeProjectionMatrixUVE(const Scene::IEntityManagerUVE& entityManager,
                                                                         Scene::EntityUVE cameraEntity,
                                                                         float aspectRatio) const = 0;

    /// Convenience combining `ComputeProjectionMatrixUVE(...) * ComputeViewMatrixUVE(...)` — the
    /// single matrix most callers (e.g. frustum extraction, a future `Renderer3DUVE`) actually
    /// need.
    [[nodiscard]] virtual Math::Matrix4x4UVE ComputeViewProjectionUVE(const Scene::IEntityManagerUVE& entityManager,
                                                                       Scene::EntityUVE cameraEntity,
                                                                       float aspectRatio) const = 0;

    /// Extracts the 6-plane culling frustum from a combined view-projection matrix (thin
    /// pass-through to `Math::FrustumUVE::FromViewProjectionUVE`, kept as a method here so
    /// callers can go straight from a camera entity to a usable frustum without reaching into
    /// `UVE::Math` directly).
    [[nodiscard]] virtual Math::FrustumUVE ExtractFrustumUVE(const Math::Matrix4x4UVE& viewProjection) const = 0;
};

} // namespace UVE::Render
