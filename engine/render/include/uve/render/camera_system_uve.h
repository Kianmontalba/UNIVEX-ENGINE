// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include "uve/render/i_camera_system_uve.h"

namespace UVE::Render {

/// CameraSystemUVE is the concrete, engine-standard implementation of ICameraSystemUVE.
/// Deliberately stateless (no members, no PIMPL) — matches SceneGraphUVE's precedent for
/// utility services that always take the IEntityManagerUVE& they operate on as an explicit
/// parameter rather than owning a reference to one.
class CameraSystemUVE final : public ICameraSystemUVE {
public:
    [[nodiscard]] Math::Matrix4x4UVE ComputeViewMatrixUVE(const Scene::IEntityManagerUVE& entityManager,
                                                            Scene::EntityUVE cameraEntity) const override;
    [[nodiscard]] Math::Matrix4x4UVE ComputeProjectionMatrixUVE(const Scene::IEntityManagerUVE& entityManager,
                                                                  Scene::EntityUVE cameraEntity,
                                                                  float aspectRatio) const override;
    [[nodiscard]] Math::Matrix4x4UVE ComputeViewProjectionUVE(const Scene::IEntityManagerUVE& entityManager,
                                                                Scene::EntityUVE cameraEntity,
                                                                float aspectRatio) const override;
    [[nodiscard]] Math::FrustumUVE ExtractFrustumUVE(const Math::Matrix4x4UVE& viewProjection) const override;
    [[nodiscard]] Math::Vector3UVE GetWorldPositionUVE(const Scene::IEntityManagerUVE& entityManager,
                                                          Scene::EntityUVE cameraEntity) const override;
};

} // namespace UVE::Render
