//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/render/camera_system_uve.h"

#include <cmath>

#include <gtest/gtest.h>

#include "uve/events/event_system_uve.h"
#include "uve/memory/memory_manager_uve.h"
#include "uve/platform/platform_uve.h"
#include "uve/scene/components/camera_component_uve.h"
#include "uve/scene/entity_manager_uve.h"
#include "uve/scene/scene_graph_uve.h"

namespace UVE::Render::Tests {
namespace {

constexpr float kEpsilon = 1e-3F;

class CameraSystemUVETest : public ::testing::Test {
protected:
    // Entities reach CameraSystemUVE only via SceneGraphUVE::AttachTransformUVE (see
    // ICameraSystemUVE's doc comment), so every fixture entity goes through the same path a real
    // scene would use, exactly like SceneGraphUVETest's own fixture.
    Memory::MemoryManagerUVE memoryManager;
    Events::EventSystemUVE eventSystem;
    Scene::EntityManagerUVE entityManager{memoryManager.GetDefaultAllocatorUVE(), eventSystem};
    Scene::SceneGraphUVE sceneGraph;
    CameraSystemUVE cameraSystem;

    [[nodiscard]] Scene::EntityUVE MakeCameraEntityUVE(Math::Vector3UVE position, Math::QuaternionUVE rotation,
                                                        Scene::CameraComponentUVE camera) {
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE local;
        local.localPosition = position;
        local.localRotation = rotation;
        sceneGraph.AttachTransformUVE(entityManager, entity, local);
        sceneGraph.UpdateUVE(entityManager);
        entityManager.AddComponentUVE<Scene::CameraComponentUVE>(entity, camera);
        return entity;
    }
};

TEST_F(CameraSystemUVETest, ComputeViewMatrixUVE_IdentityRotation_TransformsWorldPointRelativeToEye) {
    const Scene::EntityUVE camera =
        MakeCameraEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 5.0F}, Math::QuaternionUVE{}, Scene::CameraComponentUVE{});

    const Math::Matrix4x4UVE view = cameraSystem.ComputeViewMatrixUVE(entityManager, camera);
    const Math::Vector3UVE viewSpaceOrigin = Math::TransformPointUVE(view, Math::Vector3UVE{0.0F, 0.0F, 0.0F});

    EXPECT_NEAR(viewSpaceOrigin.x, 0.0F, kEpsilon);
    EXPECT_NEAR(viewSpaceOrigin.y, 0.0F, kEpsilon);
    EXPECT_NEAR(viewSpaceOrigin.z, -5.0F, kEpsilon);
}

TEST_F(CameraSystemUVETest, ComputeProjectionMatrixUVE_MapsNearAndFarPlanesToExpectedDepth) {
    Scene::CameraComponentUVE camera;
    camera.fieldOfViewDegrees = 90.0F;
    camera.nearPlane = 1.0F;
    camera.farPlane = 100.0F;
    const Scene::EntityUVE cameraEntity =
        MakeCameraEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::QuaternionUVE{}, camera);

    const Math::Matrix4x4UVE projection = cameraSystem.ComputeProjectionMatrixUVE(entityManager, cameraEntity, 1.0F);

    // Manual homogeneous multiply (TransformPointUVE assumes an affine w=1 matrix and doesn't
    // perform a perspective divide, so it can't be used for a projection matrix).
    const auto clipZ = [&](float viewZ) {
        return projection.m[2][2] * viewZ + projection.m[2][3];
    };
    const auto clipW = [&](float viewZ) { return projection.m[3][2] * viewZ + projection.m[3][3]; };

    EXPECT_NEAR(clipZ(-1.0F) / clipW(-1.0F), 0.0F, kEpsilon);
    EXPECT_NEAR(clipZ(-100.0F) / clipW(-100.0F), 1.0F, kEpsilon);
}

TEST_F(CameraSystemUVETest, ComputeViewProjectionUVE_EqualsProjectionTimesView) {
    Scene::CameraComponentUVE camera;
    camera.fieldOfViewDegrees = 60.0F;
    const Scene::EntityUVE cameraEntity =
        MakeCameraEntityUVE(Math::Vector3UVE{1.0F, 2.0F, 3.0F}, Math::QuaternionUVE{}, camera);

    const Math::Matrix4x4UVE view = cameraSystem.ComputeViewMatrixUVE(entityManager, cameraEntity);
    const Math::Matrix4x4UVE projection = cameraSystem.ComputeProjectionMatrixUVE(entityManager, cameraEntity, 1.5F);
    const Math::Matrix4x4UVE expected = projection * view;

    const Math::Matrix4x4UVE actual = cameraSystem.ComputeViewProjectionUVE(entityManager, cameraEntity, 1.5F);

    EXPECT_EQ(actual, expected);
}

TEST_F(CameraSystemUVETest, ExtractFrustumUVE_EndToEnd_ClassifiesKnownBoxesCorrectly) {
    Scene::CameraComponentUVE camera;
    camera.fieldOfViewDegrees = 90.0F;
    camera.nearPlane = 1.0F;
    camera.farPlane = 100.0F;
    const Scene::EntityUVE cameraEntity =
        MakeCameraEntityUVE(Math::Vector3UVE{0.0F, 0.0F, 0.0F}, Math::QuaternionUVE{}, camera);

    const Math::Matrix4x4UVE viewProjection = cameraSystem.ComputeViewProjectionUVE(entityManager, cameraEntity, 1.0F);
    const Math::FrustumUVE frustum = cameraSystem.ExtractFrustumUVE(viewProjection);

    const Math::AabbUVE visibleBox =
        Math::AabbUVE::FromCenterExtentsUVE(Math::Vector3UVE{0.0F, 0.0F, -10.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});
    const Math::AabbUVE behindCameraBox =
        Math::AabbUVE::FromCenterExtentsUVE(Math::Vector3UVE{0.0F, 0.0F, 10.0F}, Math::Vector3UVE{1.0F, 1.0F, 1.0F});
    const Math::AabbUVE offToTheSideBox = Math::AabbUVE::FromCenterExtentsUVE(Math::Vector3UVE{1000.0F, 0.0F, -10.0F},
                                                                               Math::Vector3UVE{1.0F, 1.0F, 1.0F});

    EXPECT_TRUE(frustum.IntersectsUVE(visibleBox));
    EXPECT_FALSE(frustum.IntersectsUVE(behindCameraBox));
    EXPECT_FALSE(frustum.IntersectsUVE(offToTheSideBox));
}

#if UVE_DEBUG
TEST_F(CameraSystemUVETest, ComputeViewMatrixUVE_EntityWithoutWorldTransform_Asserts) {
    const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
    EXPECT_DEATH({ static_cast<void>(cameraSystem.ComputeViewMatrixUVE(entityManager, entity)); }, "");
}

TEST_F(CameraSystemUVETest, ComputeProjectionMatrixUVE_EntityWithoutCameraComponent_Asserts) {
    const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
    sceneGraph.AttachTransformUVE(entityManager, entity, Scene::TransformComponentUVE{});
    EXPECT_DEATH({ static_cast<void>(cameraSystem.ComputeProjectionMatrixUVE(entityManager, entity, 1.0F)); }, "");
}
#endif

} // namespace
} // namespace UVE::Render::Tests
