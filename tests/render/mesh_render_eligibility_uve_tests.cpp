// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/render/mesh_render_eligibility_uve.h"

#include <cmath>
#include <limits>
#include <numbers>

#include <gtest/gtest.h>

namespace UVE::Render::Tests {
namespace {

[[nodiscard]] Math::FrustumUVE MakeEligibilityFrustumUVE() {
    const Math::Matrix4x4UVE view =
        Math::Matrix4x4UVE::ViewFromPositionAndRotationUVE(Math::Vector3UVE{}, Math::QuaternionUVE{});
    const Math::Matrix4x4UVE projection =
        Math::Matrix4x4UVE::PerspectiveUVE(std::numbers::pi_v<float> / 2.0F, 1.0F, 1.0F, 100.0F);
    return Math::FrustumUVE::FromViewProjectionUVE(projection * view);
}

[[nodiscard]] Scene::MeshComponentUVE MakeEligibleMeshComponentUVE() {
    return Scene::MeshComponentUVE{Asset::AssetGuidUVE{1U}, Asset::AssetGuidUVE{2U}};
}

[[nodiscard]] Asset::MeshAssetUVE MakeUnitMeshAssetUVE() {
    Asset::MeshAssetUVE mesh;
    mesh.localBounds = Math::AabbUVE::FromCenterExtentsUVE(Math::Vector3UVE{}, Math::Vector3UVE{0.5F, 0.5F, 0.5F});
    return mesh;
}

TEST(MeshRenderEligibilityUVETest, EvaluateUVE_NormalizesFiniteWorldTransformAndPublishesCopiedFacts) {
    const Scene::MeshComponentUVE component = MakeEligibleMeshComponentUVE();
    Scene::WorldTransformComponentUVE transform;
    transform.worldPosition = Math::Vector3UVE{0.0F, 0.0F, -10.0F};
    transform.worldRotation = Math::QuaternionUVE{0.0F, 0.0F, 0.0F, 2.0F};
    transform.worldScale = Math::Vector3UVE{2.0F, 1.0F, 1.0F};
    const Asset::MeshAssetUVE mesh = MakeUnitMeshAssetUVE();
    MeshRenderEligibilityUVE eligibility;

    ASSERT_TRUE(EvaluateMeshRenderEligibilityUVE(component, transform, mesh, MakeEligibilityFrustumUVE(), eligibility));
    EXPECT_TRUE(eligibility.IsEligibleUVE());
    EXPECT_EQ(eligibility.reason, MeshRenderEligibilityReasonUVE::Eligible);
    EXPECT_FLOAT_EQ(eligibility.worldMatrix.m[0][0], 2.0F);
    EXPECT_FLOAT_EQ(eligibility.worldBounds.min.x, -1.0F);
    EXPECT_FLOAT_EQ(eligibility.worldBounds.max.x, 1.0F);
    EXPECT_TRUE(std::isfinite(eligibility.sortDepth));
}

TEST(MeshRenderEligibilityUVETest, EvaluateUVE_RejectsNonFiniteWorldTransformAtomically) {
    const Scene::MeshComponentUVE component = MakeEligibleMeshComponentUVE();
    Scene::WorldTransformComponentUVE transform;
    transform.worldPosition = Math::Vector3UVE{std::numeric_limits<float>::quiet_NaN(), 0.0F, -10.0F};
    const Asset::MeshAssetUVE mesh = MakeUnitMeshAssetUVE();
    MeshRenderEligibilityUVE eligibility;
    eligibility.reason = MeshRenderEligibilityReasonUVE::Eligible;

    EXPECT_FALSE(EvaluateMeshRenderEligibilityUVE(component, transform, mesh, MakeEligibilityFrustumUVE(), eligibility));
    EXPECT_EQ(eligibility.reason, MeshRenderEligibilityReasonUVE::InvalidWorldTransform);
    EXPECT_FALSE(eligibility.IsEligibleUVE());
}

TEST(MeshRenderEligibilityUVETest, EvaluateUVE_RejectsUnorderedLocalBounds) {
    const Scene::MeshComponentUVE component = MakeEligibleMeshComponentUVE();
    const Scene::WorldTransformComponentUVE transform{
        Math::Vector3UVE{0.0F, 0.0F, -10.0F}, Math::QuaternionUVE{}, Math::Vector3UVE{1.0F, 1.0F, 1.0F}, false};
    Asset::MeshAssetUVE mesh = MakeUnitMeshAssetUVE();
    mesh.localBounds.min.x = 1.0F;
    mesh.localBounds.max.x = -1.0F;
    MeshRenderEligibilityUVE eligibility;

    EXPECT_FALSE(EvaluateMeshRenderEligibilityUVE(component, transform, mesh, MakeEligibilityFrustumUVE(), eligibility));
    EXPECT_EQ(eligibility.reason, MeshRenderEligibilityReasonUVE::InvalidLocalBounds);
}

TEST(MeshRenderEligibilityUVETest, EvaluateUVE_RejectsOutsideFrustumWithoutAssetFailure) {
    const Scene::MeshComponentUVE component = MakeEligibleMeshComponentUVE();
    const Scene::WorldTransformComponentUVE transform{
        Math::Vector3UVE{0.0F, 0.0F, 10.0F}, Math::QuaternionUVE{}, Math::Vector3UVE{1.0F, 1.0F, 1.0F}, false};
    const Asset::MeshAssetUVE mesh = MakeUnitMeshAssetUVE();
    MeshRenderEligibilityUVE eligibility;

    EXPECT_FALSE(EvaluateMeshRenderEligibilityUVE(component, transform, mesh, MakeEligibilityFrustumUVE(), eligibility));
    EXPECT_EQ(eligibility.reason, MeshRenderEligibilityReasonUVE::OutsideFrustum);
}

} // namespace
} // namespace UVE::Render::Tests
