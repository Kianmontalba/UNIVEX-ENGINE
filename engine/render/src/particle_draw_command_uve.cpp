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
    RecordIntoUVE(queue, maximumCommands, recording);
    return recording;
}

void ParticleDrawRecorderUVE::RecordIntoUVE(const RenderQueueUVE& queue, const std::size_t maximumCommands,
                                            ParticleDrawRecordingUVE& outRecording) {
    outRecording.sourceItemCount = queue.particleItems.size();
    outRecording.truncated = false;
    outRecording.commands.clear();
    if (maximumCommands == 0U) {
        outRecording.truncated = outRecording.sourceItemCount != 0U;
        return;
    }

    outRecording.commands.reserve(std::min(queue.particleItems.size(), maximumCommands));
    for (const ParticleRenderItemUVE& item : queue.particleItems) {
        if (outRecording.commands.size() >= maximumCommands) {
            outRecording.truncated = true;
            break;
        }
        const ParticleDrawCommandUVE command{
            item.entity, item.position, item.remainingLifetimeSeconds, item.sortDepth, item.sequence};
        if (!IsValidParticleDrawCommandUVE(command)) {
            // RenderQueueUVE is intentionally mutable/public, so defend the GPU boundary even when
            // the normal ParticleRenderBridgeUVE path has already validated its source snapshot.
            outRecording.truncated = true;
            continue;
        }
        outRecording.commands.push_back(command);
    }
    outRecording.truncated = outRecording.truncated || queue.particleItemsTruncated ||
                             outRecording.commands.size() < outRecording.sourceItemCount;
}

} // namespace UVE::Render
