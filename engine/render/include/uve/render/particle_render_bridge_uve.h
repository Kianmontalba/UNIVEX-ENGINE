// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "uve/math/vector3_uve.h"
#include "uve/scene/particle_runtime_uve.h"

namespace UVE::Render {

inline constexpr std::size_t kMaximumParticleRenderItemsUVE = 1'000'000U;

struct ParticleRenderItemUVE final {
    Scene::EntityUVE entity;
    Math::Vector3UVE position{};
    float remainingLifetimeSeconds = 0.0F;
    float sortDepth = 0.0F;
    std::uint64_t sequence = 0U;

    [[nodiscard]] bool operator==(const ParticleRenderItemUVE&) const = default;
};

struct ParticleRenderSnapshotUVE final {
    std::size_t sourceParticleCount = 0U;
    bool truncated = false;
    std::vector<ParticleRenderItemUVE> items;

    [[nodiscard]] bool operator==(const ParticleRenderSnapshotUVE&) const = default;
};

/// Copies enabled CPU particle state into the renderer-owned value path. It performs no GPU work,
/// asset lookup, renderer registration, culling against a camera, or mutation of ParticleRuntimeUVE.
class ParticleRenderBridgeUVE final {
public:
    [[nodiscard]] static ParticleRenderSnapshotUVE ExtractUVE(
        const Scene::ParticleRuntimeUVE& runtime,
        std::size_t maximumItems = kMaximumParticleRenderItemsUVE);
};

} // namespace UVE::Render

