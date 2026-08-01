//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <memory>

#include "uve/audio/i_audio_source_system_uve.h"

namespace UVE::Audio {

/// AudioSourceSystemUVE is the concrete, engine-standard implementation of
/// IAudioSourceSystemUVE. Unlike Render::MeshRendererUVE/CameraSystemUVE (stateless), this system
/// must be stateful — it owns the entity->VoiceHandleUVE registry needed to detect "this entity's
/// source already exists" vs. "this entity is new" vs. "this entity's source should be destroyed"
/// across successive SyncUVE() calls — closer in shape to a pimpl'd system like
/// Physics::PhysicsSystemUVE than to MeshRendererUVE's no-members design.
class AudioSourceSystemUVE final : public IAudioSourceSystemUVE {
public:
    AudioSourceSystemUVE();
    ~AudioSourceSystemUVE() override;

    AudioSourceSystemUVE(const AudioSourceSystemUVE&) = delete;
    AudioSourceSystemUVE& operator=(const AudioSourceSystemUVE&) = delete;

    void SyncUVE(Scene::IEntityManagerUVE& entityManager, IAudioSystemUVE& audioSystem) override;

private:
    struct ImplUVE;
    std::unique_ptr<ImplUVE> m_impl;
};

} // namespace UVE::Audio
