// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/render/mesh_render_eligibility_uve.h"

#include <cmath>

#include "uve/asset/asset_guid_uve.h"
#include "uve/math/quaternion_uve.h"
#include "uve/scene/components/mesh_component_uve.h"

namespace UVE::Render {
namespace {

constexpr std::size_t kNearPlaneIndexUVE = 4U;

[[nodiscard]] bool IsFiniteVectorUVE(const Math::Vector3UVE& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] bool IsOrderedFiniteAabbUVE(const Math::AabbUVE& bounds) noexcept {
    return IsFiniteVectorUVE(bounds.min) && IsFiniteVectorUVE(bounds.max) && bounds.min.x <= bounds.max.x &&
           bounds.min.y <= bounds.max.y && bounds.min.z <= bounds.max.z;
}

} // namespace

bool EvaluateMeshRenderEligibilityUVE(const Scene::MeshComponentUVE& meshComponent,
                                      const Scene::WorldTransformComponentUVE& worldTransform,
                                      const Asset::MeshAssetUVE& mesh,
                                      const Math::FrustumUVE& cullFrustum,
                                      MeshRenderEligibilityUVE& outEligibility) noexcept {
    MeshRenderEligibilityUVE candidate;
    if (!Scene::IsMeshComponentValidUVE(meshComponent) ||
        meshComponent.meshGuid == Asset::kInvalidAssetGuidUVE ||
        meshComponent.materialGuid == Asset::kInvalidAssetGuidUVE) {
        candidate.reason = MeshRenderEligibilityReasonUVE::InvalidAssetReferences;
        outEligibility = candidate;
        return false;
    }
    if (!IsFiniteVectorUVE(worldTransform.worldPosition) || !IsFiniteVectorUVE(worldTransform.worldScale) ||
        !Math::IsFiniteUVE(worldTransform.worldRotation)) {
        candidate.reason = MeshRenderEligibilityReasonUVE::InvalidWorldTransform;
        outEligibility = candidate;
        return false;
    }
    Math::QuaternionUVE normalizedRotation;
    if (!Math::TryNormalizeUVE(worldTransform.worldRotation, normalizedRotation)) {
        candidate.reason = MeshRenderEligibilityReasonUVE::InvalidWorldTransform;
        outEligibility = candidate;
        return false;
    }
    if (!IsOrderedFiniteAabbUVE(mesh.localBounds)) {
        candidate.reason = MeshRenderEligibilityReasonUVE::InvalidLocalBounds;
        outEligibility = candidate;
        return false;
    }

    candidate.worldMatrix = Math::Matrix4x4UVE::ComposeTrsUVE(worldTransform.worldPosition, normalizedRotation,
                                                               worldTransform.worldScale);
    candidate.worldBounds = mesh.localBounds.TransformUVE(candidate.worldMatrix);
    if (!IsOrderedFiniteAabbUVE(candidate.worldBounds)) {
        candidate.reason = MeshRenderEligibilityReasonUVE::InvalidWorldTransform;
        outEligibility = candidate;
        return false;
    }
    if (!cullFrustum.IntersectsUVE(candidate.worldBounds)) {
        candidate.reason = MeshRenderEligibilityReasonUVE::OutsideFrustum;
        outEligibility = candidate;
        return false;
    }
    candidate.sortDepth = cullFrustum.planes[kNearPlaneIndexUVE].GetSignedDistanceUVE(
        candidate.worldBounds.GetCenterUVE());
    if (!std::isfinite(candidate.sortDepth)) {
        candidate.reason = MeshRenderEligibilityReasonUVE::InvalidWorldTransform;
        outEligibility = candidate;
        return false;
    }
    candidate.reason = MeshRenderEligibilityReasonUVE::Eligible;
    outEligibility = candidate;
    return true;
}

} // namespace UVE::Render
