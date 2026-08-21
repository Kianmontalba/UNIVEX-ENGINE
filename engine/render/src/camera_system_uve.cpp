// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/render/camera_system_uve.h"

#include <cmath>
#include <numbers>

#include "uve/debug/assert_uve.h"
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
    UVE_ASSERT(Scene::IsCameraComponentValidUVE(camera));
    UVE_ASSERT(std::isfinite(aspectRatio) && aspectRatio > 0.0F);
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

CameraFrustumCornersUVE CameraSystemUVE::ComputeFrustumCornersUVE(const Scene::IEntityManagerUVE& entityManager,
                                                                    Scene::EntityUVE cameraEntity,
                                                                    float aspectRatio) const {
    const Scene::WorldTransformComponentUVE& worldTransform =
        entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(cameraEntity);
    const Scene::CameraComponentUVE& camera = entityManager.GetComponentUVE<Scene::CameraComponentUVE>(cameraEntity);
    UVE_ASSERT(Scene::IsCameraComponentValidUVE(camera));
    UVE_ASSERT(std::isfinite(aspectRatio) && aspectRatio > 0.0F);
    const float tangent = std::tan(camera.fieldOfViewDegrees * (std::numbers::pi_v<float> / 360.0F));
    const float nearHalfHeight = camera.nearPlane * tangent;
    const float nearHalfWidth = nearHalfHeight * aspectRatio;
    const float farHalfHeight = camera.farPlane * tangent;
    const float farHalfWidth = farHalfHeight * aspectRatio;

    const Math::Vector3UVE forward = Math::RotateVectorUVE(worldTransform.worldRotation, {0.0F, 0.0F, -1.0F});
    const Math::Vector3UVE right = Math::RotateVectorUVE(worldTransform.worldRotation, {1.0F, 0.0F, 0.0F});
    const Math::Vector3UVE up = Math::RotateVectorUVE(worldTransform.worldRotation, {0.0F, 1.0F, 0.0F});
    const Math::Vector3UVE nearCenter = worldTransform.worldPosition + forward * camera.nearPlane;
    const Math::Vector3UVE farCenter = worldTransform.worldPosition + forward * camera.farPlane;

    return {
        nearCenter - right * nearHalfWidth - up * nearHalfHeight,
        nearCenter + right * nearHalfWidth - up * nearHalfHeight,
        nearCenter - right * nearHalfWidth + up * nearHalfHeight,
        nearCenter + right * nearHalfWidth + up * nearHalfHeight,
        farCenter - right * farHalfWidth - up * farHalfHeight,
        farCenter + right * farHalfWidth - up * farHalfHeight,
        farCenter - right * farHalfWidth + up * farHalfHeight,
        farCenter + right * farHalfWidth + up * farHalfHeight,
    };
}

Math::Vector3UVE CameraSystemUVE::GetWorldPositionUVE(const Scene::IEntityManagerUVE& entityManager,
                                                         Scene::EntityUVE cameraEntity) const {
    return entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(cameraEntity).worldPosition;
}

} // namespace UVE::Render
