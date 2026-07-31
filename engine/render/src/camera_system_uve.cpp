//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/render/camera_system_uve.h"

#include <numbers>

#include "uve/scene/components/camera_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"

namespace UVE::Render {

Math::Matrix4x4UVE CameraSystemUVE::ComputeViewMatrixUVE(const Scene::IEntityManagerUVE& entityManager,
                                                           Scene::EntityUVE cameraEntity) const {
    const Scene::WorldTransformComponentUVE& worldTransform =
        entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(cameraEntity);
    return Math::Matrix4x4UVE::ViewFromPositionAndRotationUVE(worldTransform.worldPosition, worldTransform.worldRotation);
}

Math::Matrix4x4UVE CameraSystemUVE::ComputeProjectionMatrixUVE(const Scene::IEntityManagerUVE& entityManager,
                                                                 Scene::EntityUVE cameraEntity,
                                                                 float aspectRatio) const {
    const Scene::CameraComponentUVE& camera = entityManager.GetComponentUVE<Scene::CameraComponentUVE>(cameraEntity);
    const float fovYRadians = camera.fieldOfViewDegrees * (std::numbers::pi_v<float> / 180.0F);
    return Math::Matrix4x4UVE::PerspectiveUVE(fovYRadians, aspectRatio, camera.nearPlane, camera.farPlane);
}

Math::Matrix4x4UVE CameraSystemUVE::ComputeViewProjectionUVE(const Scene::IEntityManagerUVE& entityManager,
                                                               Scene::EntityUVE cameraEntity,
                                                               float aspectRatio) const {
    return ComputeProjectionMatrixUVE(entityManager, cameraEntity, aspectRatio) *
           ComputeViewMatrixUVE(entityManager, cameraEntity);
}

Math::FrustumUVE CameraSystemUVE::ExtractFrustumUVE(const Math::Matrix4x4UVE& viewProjection) const {
    return Math::FrustumUVE::FromViewProjectionUVE(viewProjection);
}

} // namespace UVE::Render
