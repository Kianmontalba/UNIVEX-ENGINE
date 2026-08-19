// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/render/particle_draw_command_uve.h"

#include <cmath>

namespace UVE::Render {

bool IsValidParticleDrawCommandUVE(const ParticleDrawCommandUVE& command) noexcept {
    return std::isfinite(command.position.x) && std::isfinite(command.position.y) &&
           std::isfinite(command.position.z) && std::isfinite(command.remainingLifetimeSeconds) &&
           command.remainingLifetimeSeconds >= 0.0F && std::isfinite(command.sortDepth) &&
           command.sequence != 0U;
}

ParticleDrawRecordingUVE ParticleDrawRecorderUVE::RecordUVE(const RenderQueueUVE& queue,
                                                             const std::size_t maximumCommands) {
    ParticleDrawRecordingUVE recording;
    recording.sourceItemCount = queue.particleItems.size();
    if (maximumCommands == 0U) {
        recording.truncated = recording.sourceItemCount != 0U;
        return recording;
    }

    recording.commands.reserve(std::min(queue.particleItems.size(), maximumCommands));
    for (const ParticleRenderItemUVE& item : queue.particleItems) {
        if (recording.commands.size() >= maximumCommands) {
            recording.truncated = true;
            break;
        }
        recording.commands.push_back(
            {item.entity, item.position, item.remainingLifetimeSeconds, item.sortDepth, item.sequence});
    }
    recording.truncated = recording.truncated || queue.particleItemsTruncated ||
                          recording.commands.size() < recording.sourceItemCount;
    return recording;
}

} // namespace UVE::Render
