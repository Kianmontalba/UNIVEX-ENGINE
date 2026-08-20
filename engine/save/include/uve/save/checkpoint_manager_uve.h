// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include "uve/save/i_checkpoint_manager_uve.h"

namespace UVE::Save {

/// CheckpointManagerUVE is the concrete, engine-standard implementation of
/// ICheckpointManagerUVE. Composes an ISaveGameSystemUVE& (dependency injection, matching
/// Physics::PhysicsSystemUVE's ICollisionSystemUVE& precedent) and holds only the small pieces of
/// state its own doc comment promises (elapsed-since-last-save, total playtime, configured
/// interval) — no per-entity state.
class CheckpointManagerUVE final : public ICheckpointManagerUVE {
public:
    /// `saveGameSystem` must outlive this CheckpointManagerUVE. `autoSaveIntervalSeconds`
    /// defaults to 300.0 (the spec's "e.g., every 5 minutes" example).
    explicit CheckpointManagerUVE(ISaveGameSystemUVE& saveGameSystem, double autoSaveIntervalSeconds = 300.0);

    void UpdateUVE(double deltaTimeSeconds, Scene::IEntityManagerUVE& entityManager,
                    const std::vector<Scene::EntityUVE>& rootEntities) override;
    [[nodiscard]] bool CheckpointUVE(Scene::IEntityManagerUVE& entityManager,
                                      const std::vector<Scene::EntityUVE>& rootEntities) override;
    void SetAutoSaveIntervalSecondsUVE(double intervalSeconds) noexcept override;
    [[nodiscard]] double GetAutoSaveIntervalSecondsUVE() const noexcept override;
    [[nodiscard]] double GetElapsedSinceLastSaveSecondsUVE() const noexcept override;
    [[nodiscard]] double GetTotalPlaytimeSecondsUVE() const noexcept override;

private:
    /// Builds this checkpoint's GameStateMetadataUVE (playtimeSeconds = m_totalPlaytimeSeconds,
    /// every other field default) and calls m_saveGameSystem->SaveUVE() for the caller-selected
    /// reserved slot, then resets
    /// m_elapsedSinceLastSaveSeconds to 0 regardless of the result. Shared by both UpdateUVE()'s
    /// interval-triggered path and CheckpointUVE()'s immediate path.
    [[nodiscard]] bool SaveCheckpointUVE(int slotIndex, Scene::IEntityManagerUVE& entityManager,
                                          const std::vector<Scene::EntityUVE>& rootEntities);

    ISaveGameSystemUVE* m_saveGameSystem;
    double m_autoSaveIntervalSeconds;
    double m_elapsedSinceLastSaveSeconds = 0.0;
    double m_totalPlaytimeSeconds = 0.0;
};

} // namespace UVE::Save
