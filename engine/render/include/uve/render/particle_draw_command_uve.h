// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "uve/render/render_queue_uve.h"

namespace UVE::Render {

inline constexpr std::size_t kMaximumParticleDrawCommandsUVE = 1'000'000U;

struct ParticleDrawCommandUVE final {
    Scene::EntityUVE entity;
    Math::Vector3UVE position{};
    float remainingLifetimeSeconds = 0.0F;
    float sortDepth = 0.0F;
    std::uint64_t sequence = 0U;

    [[nodiscard]] bool operator==(const ParticleDrawCommandUVE&) const = default;
};

struct ParticleDrawRecordingUVE final {
    std::size_t sourceItemCount = 0U;
    bool truncated = false;
    std::vector<ParticleDrawCommandUVE> commands;

    [[nodiscard]] bool operator==(const ParticleDrawRecordingUVE&) const = default;
};

/// Copies renderer-owned particle queue values into a bounded command description. This v1 does
/// not allocate GPU buffers, bind pipelines, submit OpenGL calls, resolve assets, or mutate the queue.
class ParticleDrawRecorderUVE final {
public:
    [[nodiscard]] static ParticleDrawRecordingUVE RecordUVE(
        const RenderQueueUVE& queue,
        std::size_t maximumCommands = kMaximumParticleDrawCommandsUVE);
};

} // namespace UVE::Render
