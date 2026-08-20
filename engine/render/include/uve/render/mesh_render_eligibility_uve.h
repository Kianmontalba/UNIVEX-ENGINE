// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstdint>

#include "uve/asset/mesh_asset_uve.h"
#include "uve/math/aabb_uve.h"
#include "uve/math/frustum_uve.h"
#include "uve/math/matrix4x4_uve.h"
#include "uve/scene/components/mesh_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"

namespace UVE::Render {

enum class MeshRenderEligibilityReasonUVE : std::uint8_t {
    Eligible = 0,
    InvalidAssetReferences,
    InvalidWorldTransform,
    InvalidLocalBounds,
    OutsideFrustum,
};

struct MeshRenderEligibilityUVE final {
    MeshRenderEligibilityReasonUVE reason = MeshRenderEligibilityReasonUVE::InvalidAssetReferences;
    Math::Matrix4x4UVE worldMatrix{};
    Math::AabbUVE worldBounds{};
    float sortDepth = 0.0F;

    [[nodiscard]] bool IsEligibleUVE() const noexcept {
        return reason == MeshRenderEligibilityReasonUVE::Eligible;
    }
};

/// Evaluates one loaded scene MeshComponentUVE for render-queue handoff. The contract validates
/// asset GUID coherence, finite world transform/quaternion normalization, finite ordered mesh bounds,
/// transformed bounds, frustum visibility, and finite sort depth, then publishes copied matrix/bounds
/// facts. It performs no asset loading, GPU allocation, command submission, or ownership transfer.
[[nodiscard]] bool EvaluateMeshRenderEligibilityUVE(
    const Scene::MeshComponentUVE& meshComponent,
    const Scene::WorldTransformComponentUVE& worldTransform,
    const Asset::MeshAssetUVE& mesh,
    const Math::FrustumUVE& cullFrustum,
    MeshRenderEligibilityUVE& outEligibility) noexcept;

} // namespace UVE::Render
