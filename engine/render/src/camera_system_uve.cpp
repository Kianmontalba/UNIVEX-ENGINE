// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/render/camera_system_uve.h"

#include <cmath>
#include <limits>
#include <numbers>

#include "uve/debug/assert_uve.h"
#include "uve/debug/logging_macros_uve.h"
#include "uve/scene/components/camera_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"

namespace UVE::Render {
namespace {

[[nodiscard]] bool IsFiniteVectorUVE(const Math::Vector3UVE& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] bool TryNormalizeWorldTransformRotationUVE(const Scene::WorldTransformComponentUVE& worldTransform,
                                                          Math::QuaternionUVE& outRotation) noexcept {
    return IsFiniteVectorUVE(worldTransform.worldPosition) &&
           Math::TryNormalizeUVE(worldTransform.worldRotation, outRotation);
}

} // namespace

Math::Matrix4x4UVE CameraSystemUVE::ComputeViewMatrixUVE(const Scene::IEntityManagerUVE& entityManager,
                                                           Scene::EntityUVE cameraEntity) const {
    const Scene::WorldTransformComponentUVE& worldTransform =
        entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(cameraEntity);
    Math::QuaternionUVE normalizedRotation;
    const bool validTransform = TryNormalizeWorldTransformRotationUVE(worldTransform, normalizedRotation);
    UVE_ASSERT(validTransform);
    if (!validTransform) {
        UVE_ERROR("CameraSystemUVE: ComputeViewMatrixUVE received an invalid world transform");
        return Math::Matrix4x4UVE::IdentityUVE();
    }
    return Math::Matrix4x4UVE::ViewFromPositionAndRotationUVE(worldTransform.worldPosition, normalizedRotation);
}

Math::Matrix4x4UVE CameraSystemUVE::ComputeProjectionMatrixUVE(const Scene::IEntityManagerUVE& entityManager,
                                                                 Scene::EntityUVE cameraEntity,
                                                                 float aspectRatio) const {
    const Scene::CameraComponentUVE& camera = entityManager.GetComponentUVE<Scene::CameraComponentUVE>(cameraEntity);
    const bool cameraValid = Scene::IsCameraComponentValidUVE(camera);
    UVE_ASSERT(cameraValid);
    if (!cameraValid) {
        UVE_ERROR("CameraSystemUVE: ComputeProjectionMatrixUVE received invalid camera parameters");
        return Math::Matrix4x4UVE::IdentityUVE();
    }
    const bool aspectRatioValid = std::isfinite(aspectRatio) && aspectRatio > 0.0F;
    UVE_ASSERT(aspectRatioValid);
    if (!aspectRatioValid) {
        UVE_ERROR("CameraSystemUVE: ComputeProjectionMatrixUVE received an invalid aspect ratio");
        return Math::Matrix4x4UVE::IdentityUVE();
    }
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
    Math::QuaternionUVE normalizedRotation;
    const bool transformValid = TryNormalizeWorldTransformRotationUVE(worldTransform, normalizedRotation);
    UVE_ASSERT(transformValid);
    if (!transformValid) {
        UVE_ERROR("CameraSystemUVE: ComputeFrustumCornersUVE received an invalid world transform");
        return CameraFrustumCornersUVE{};
    }
    const Scene::CameraComponentUVE& camera = entityManager.GetComponentUVE<Scene::CameraComponentUVE>(cameraEntity);
    const bool cameraValid = Scene::IsCameraComponentValidUVE(camera);
    UVE_ASSERT(cameraValid);
    if (!cameraValid) {
        UVE_ERROR("CameraSystemUVE: ComputeFrustumCornersUVE received invalid camera parameters");
        return CameraFrustumCornersUVE{};
    }
    const bool aspectRatioValid = std::isfinite(aspectRatio) && aspectRatio > 0.0F;
    UVE_ASSERT(aspectRatioValid);
    if (!aspectRatioValid) {
        UVE_ERROR("CameraSystemUVE: ComputeFrustumCornersUVE received an invalid aspect ratio");
        return CameraFrustumCornersUVE{};
    }
    const float tangent = std::tan(camera.fieldOfViewDegrees * (std::numbers::pi_v<float> / 360.0F));
    const float nearHalfHeight = camera.nearPlane * tangent;
    const float nearHalfWidth = nearHalfHeight * aspectRatio;
    const float farHalfHeight = camera.farPlane * tangent;
    const float farHalfWidth = farHalfHeight * aspectRatio;

    const Math::Vector3UVE forward = Math::RotateVectorUVE(normalizedRotation, {0.0F, 0.0F, -1.0F});
    const Math::Vector3UVE right = Math::RotateVectorUVE(normalizedRotation, {1.0F, 0.0F, 0.0F});
    const Math::Vector3UVE up = Math::RotateVectorUVE(normalizedRotation, {0.0F, 1.0F, 0.0F});
    const Math::Vector3UVE nearCenter = worldTransform.worldPosition + forward * camera.nearPlane;
    const Math::Vector3UVE farCenter = worldTransform.worldPosition + forward * camera.farPlane;

    const CameraFrustumCornersUVE floatCorners{
        nearCenter - right * nearHalfWidth - up * nearHalfHeight,
        nearCenter + right * nearHalfWidth - up * nearHalfHeight,
        nearCenter - right * nearHalfWidth + up * nearHalfHeight,
        nearCenter + right * nearHalfWidth + up * nearHalfHeight,
        farCenter - right * farHalfWidth - up * farHalfHeight,
        farCenter + right * farHalfWidth - up * farHalfHeight,
        farCenter - right * farHalfWidth + up * farHalfHeight,
        farCenter + right * farHalfWidth + up * farHalfHeight,
    };
    bool floatCornersFinite = true;
    for (const Math::Vector3UVE corner : floatCorners) {
        floatCornersFinite = floatCornersFinite && std::isfinite(corner.x) && std::isfinite(corner.y) &&
                              std::isfinite(corner.z);
    }
    if (floatCornersFinite) {
        return floatCorners;
    }

    const double tangentWide = std::tan(static_cast<double>(camera.fieldOfViewDegrees) *
                                        (static_cast<double>(std::numbers::pi_v<float>) / 360.0));
    const double nearPlaneWide = static_cast<double>(camera.nearPlane);
    const double farPlaneWide = static_cast<double>(camera.farPlane);
    const double aspectWide = static_cast<double>(aspectRatio);
    const double nearHalfHeightWide = nearPlaneWide * tangentWide;
    const double nearHalfWidthWide = nearHalfHeightWide * aspectWide;
    const double farHalfHeightWide = farPlaneWide * tangentWide;
    const double farHalfWidthWide = farHalfHeightWide * aspectWide;
    const double positionX = static_cast<double>(worldTransform.worldPosition.x);
    const double positionY = static_cast<double>(worldTransform.worldPosition.y);
    const double positionZ = static_cast<double>(worldTransform.worldPosition.z);
    const double forwardX = static_cast<double>(forward.x);
    const double forwardY = static_cast<double>(forward.y);
    const double forwardZ = static_cast<double>(forward.z);
    const double rightX = static_cast<double>(right.x);
    const double rightY = static_cast<double>(right.y);
    const double rightZ = static_cast<double>(right.z);
    const double upX = static_cast<double>(up.x);
    const double upY = static_cast<double>(up.y);
    const double upZ = static_cast<double>(up.z);
    const double nearCenterX = positionX + forwardX * nearPlaneWide;
    const double nearCenterY = positionY + forwardY * nearPlaneWide;
    const double nearCenterZ = positionZ + forwardZ * nearPlaneWide;
    const double farCenterX = positionX + forwardX * farPlaneWide;
    const double farCenterY = positionY + forwardY * farPlaneWide;
    const double farCenterZ = positionZ + forwardZ * farPlaneWide;
    const double nearRightX = rightX * nearHalfWidthWide;
    const double nearRightY = rightY * nearHalfWidthWide;
    const double nearRightZ = rightZ * nearHalfWidthWide;
    const double nearUpX = upX * nearHalfHeightWide;
    const double nearUpY = upY * nearHalfHeightWide;
    const double nearUpZ = upZ * nearHalfHeightWide;
    const double farRightX = rightX * farHalfWidthWide;
    const double farRightY = rightY * farHalfWidthWide;
    const double farRightZ = rightZ * farHalfWidthWide;
    const double farUpX = upX * farHalfHeightWide;
    const double farUpY = upY * farHalfHeightWide;
    const double farUpZ = upZ * farHalfHeightWide;
    const auto NarrowFiniteUVE = [](const double value) noexcept {
        if (std::isnan(value)) {
            return 0.0F;
        }
        const double maximum = static_cast<double>(std::numeric_limits<float>::max());
        if (value >= maximum) {
            return std::numeric_limits<float>::max();
        }
        if (value <= -maximum) {
            return -std::numeric_limits<float>::max();
        }
        return static_cast<float>(value);
    };
    const auto MakeCornerUVE = [NarrowFiniteUVE](const double centerX, const double centerY, const double centerZ,
                                                  const double rightComponentX, const double rightComponentY,
                                                  const double rightComponentZ, const double upComponentX,
                                                  const double upComponentY, const double upComponentZ,
                                                  const double rightSign, const double upSign) {
        return Math::Vector3UVE{
            NarrowFiniteUVE(centerX + rightSign * rightComponentX + upSign * upComponentX),
            NarrowFiniteUVE(centerY + rightSign * rightComponentY + upSign * upComponentY),
            NarrowFiniteUVE(centerZ + rightSign * rightComponentZ + upSign * upComponentZ),
        };
    };

    return {
        MakeCornerUVE(nearCenterX, nearCenterY, nearCenterZ, nearRightX, nearRightY, nearRightZ,
                      nearUpX, nearUpY, nearUpZ, -1.0, -1.0),
        MakeCornerUVE(nearCenterX, nearCenterY, nearCenterZ, nearRightX, nearRightY, nearRightZ,
                      nearUpX, nearUpY, nearUpZ, 1.0, -1.0),
        MakeCornerUVE(nearCenterX, nearCenterY, nearCenterZ, nearRightX, nearRightY, nearRightZ,
                      nearUpX, nearUpY, nearUpZ, -1.0, 1.0),
        MakeCornerUVE(nearCenterX, nearCenterY, nearCenterZ, nearRightX, nearRightY, nearRightZ,
                      nearUpX, nearUpY, nearUpZ, 1.0, 1.0),
        MakeCornerUVE(farCenterX, farCenterY, farCenterZ, farRightX, farRightY, farRightZ,
                      farUpX, farUpY, farUpZ, -1.0, -1.0),
        MakeCornerUVE(farCenterX, farCenterY, farCenterZ, farRightX, farRightY, farRightZ,
                      farUpX, farUpY, farUpZ, 1.0, -1.0),
        MakeCornerUVE(farCenterX, farCenterY, farCenterZ, farRightX, farRightY, farRightZ,
                      farUpX, farUpY, farUpZ, -1.0, 1.0),
        MakeCornerUVE(farCenterX, farCenterY, farCenterZ, farRightX, farRightY, farRightZ,
                      farUpX, farUpY, farUpZ, 1.0, 1.0),
    };
}

Math::Vector3UVE CameraSystemUVE::GetWorldPositionUVE(const Scene::IEntityManagerUVE& entityManager,
                                                         Scene::EntityUVE cameraEntity) const {
    const Scene::WorldTransformComponentUVE& worldTransform =
        entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(cameraEntity);
    Math::QuaternionUVE normalizedRotation;
    const bool validTransform = TryNormalizeWorldTransformRotationUVE(worldTransform, normalizedRotation);
    UVE_ASSERT(validTransform);
    if (!validTransform) {
        UVE_ERROR("CameraSystemUVE: GetWorldPositionUVE received an invalid world transform");
        return Math::Vector3UVE{};
    }
    return worldTransform.worldPosition;
}

} // namespace UVE::Render
