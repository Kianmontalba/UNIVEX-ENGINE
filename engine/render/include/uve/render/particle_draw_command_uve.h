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

[[nodiscard]] bool IsValidParticleDrawCommandUVE(const ParticleDrawCommandUVE& command) noexcept;

struct ParticleDrawRecordingUVE final {
    std::size_t sourceItemCount = 0U;
    bool truncated = false;
    std::vector<ParticleDrawCommandUVE> commands;

    [[nodiscard]] bool operator==(const ParticleDrawRecordingUVE&) const = default;
};

struct ParticleGpuUploadPlanUVE final {
    static constexpr std::size_t kParticleStrideBytesUVE = sizeof(float) * 5U;
    std::size_t commandCount = 0U;
    std::size_t byteCount = 0U;
    bool truncated = false;
};

/// Plans a bounded fixed-stride particle upload from copied draw commands. It does not allocate
/// GPU buffers, resolve materials, bind pipelines, submit commands, or mutate the recording.
[[nodiscard]] inline bool PlanParticleGpuUploadUVE(
    const ParticleDrawRecordingUVE& recording, const std::size_t maximumBytes,
    ParticleGpuUploadPlanUVE& outPlan) noexcept {
    if (recording.commands.size() > kMaximumParticleDrawCommandsUVE ||
        recording.sourceItemCount < recording.commands.size() ||
        (!recording.truncated && recording.sourceItemCount != recording.commands.size()) ||
        maximumBytes < ParticleGpuUploadPlanUVE::kParticleStrideBytesUVE) {
        return false;
    }
    const std::size_t commandCount = recording.commands.size();
    if (commandCount > maximumBytes / ParticleGpuUploadPlanUVE::kParticleStrideBytesUVE) {
        return false;
    }
    for (const ParticleDrawCommandUVE& command : recording.commands) {
        if (!IsValidParticleDrawCommandUVE(command)) {
            return false;
        }
    }
    ParticleGpuUploadPlanUVE plan;
    plan.commandCount = commandCount;
    plan.byteCount = commandCount * ParticleGpuUploadPlanUVE::kParticleStrideBytesUVE;
    plan.truncated = recording.truncated;
    outPlan = plan;
    return true;
}

/// Copies renderer-owned particle queue values into a bounded command description. This v1 does
/// not allocate GPU buffers, bind pipelines, submit OpenGL calls, resolve assets, or mutate the queue.
class ParticleDrawRecorderUVE final {
public:
    [[nodiscard]] static ParticleDrawRecordingUVE RecordUVE(
        const RenderQueueUVE& queue,
        std::size_t maximumCommands = kMaximumParticleDrawCommandsUVE);
};

} // namespace UVE::Render
