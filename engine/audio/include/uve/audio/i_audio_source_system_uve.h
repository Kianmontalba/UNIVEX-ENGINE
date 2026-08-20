// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include "uve/audio/i_audio_system_uve.h"
#include "uve/scene/i_entity_manager_uve.h"

namespace UVE::Audio {

/// IAudioSourceSystemUVE is the ECS-to-AudioSystemUVE sync step (mirrors Render::IMeshRendererUVE's
/// "ECS -> render queue extraction" shape, adapted for a resource with a create/destroy lifecycle
/// rather than a per-frame-rebuilt queue): walks every entity with WorldTransformComponentUVE +
/// AudioSourceComponentUVE, creating/updating/destroying the corresponding IAudioSystemUVE source
/// so per-entity spatial audio needs no hand-written boilerplate at every call site.
/// Thread-safety: not thread-safe; SyncUVE() is intended to be called once per frame from the main
/// engine thread, after SceneGraphUVE::UpdateUVE() has produced this frame's world transforms —
/// the same ordering contract Render::IMeshRendererUVE::ExtractRenderQueueUVE() documents.
class IAudioSourceSystemUVE {
public:
    virtual ~IAudioSourceSystemUVE() = default;

    /// For every entity with both WorldTransformComponentUVE and AudioSourceComponentUVE: creates
    /// a source (via `audioSystem.CreateSourceUVE()`) the first time this entity is seen —
    /// auto-playing it if `AudioSourceComponentUVE::playOnAwake` is true — caching entity->voice in
    /// this system's own internal registry; updates the source's position/volume/pitch every call
    /// to track the entity's current WorldTransformComponentUVE/AudioSourceComponentUVE values;
    /// and destroys the source for any previously-seen entity no longer matching (removed
    /// entirely, or its AudioSourceComponentUVE was removed) since the last call.
    /// Descriptor-affecting edits to audioAssetPath, looping, spatial, minDistance, maxDistance, or
    /// attenuationCurve are applied by creating a replacement voice first. The replacement inherits
    /// the previous voice's Stopped/Playing state; if creation or playback restart fails, the old
    /// voice and its cached descriptor remain live. Volume, pitch, and position continue updating in
    /// place, while playOnAwake is only honored for the first source creation.
    virtual void SyncUVE(Scene::IEntityManagerUVE& entityManager, IAudioSystemUVE& audioSystem) = 0;
};

} // namespace UVE::Audio
