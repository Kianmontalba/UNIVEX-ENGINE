// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <vector>

#include "uve/save/i_save_game_system_uve.h"
#include "uve/scene/entity_uve.h"
#include "uve/scene/i_entity_manager_uve.h"

namespace UVE::Save {

/// ICheckpointManagerUVE is the auto-save/manual-checkpoint system (Part 17's
/// CheckpointManagerUVE): accumulates elapsed time via UpdateUVE(), writing to
/// ISaveGameSystemUVE's reserved kAutoSaveSlotIndexUVE whenever the configured interval elapses;
/// CheckpointUVE() writes to the distinct kManualCheckpointSlotIndexUVE immediately, on demand.
///
/// A successfully written checkpoint — automatic or manual — resets the elapsed-time accumulator,
/// so a successful manual CheckpointUVE() call defers the next automatic one rather than being
/// immediately followed by a redundant one. A failed save preserves the accumulated time for a
/// later retry.
/// Thread-safety: not thread-safe, matching every other per-frame-driven system in this engine
/// (PhysicsSystemUVE, AudioSourceSystemUVE) — UpdateUVE() is intended to be called once per frame
/// from the main engine thread.
class ICheckpointManagerUVE {
public:
    virtual ~ICheckpointManagerUVE() = default;

    /// Accumulates a finite nonnegative `deltaTimeSeconds` into both the total-playtime counter
    /// and the since-last-save counter; invalid negative or non-finite deltas are ignored without
    /// counter or save-state mutation. Once the latter reaches GetAutoSaveIntervalSecondsUVE(), saves
    /// `rootEntities` (from `entityManager`) to kAutoSaveSlotIndexUVE via the composed
    /// ISaveGameSystemUVE and resets the since-last-save counter to 0 only when the save succeeds;
    /// a failed save preserves accumulated time so it can be retried on a later update rather than
    /// losing the elapsed interval.
    virtual void UpdateUVE(double deltaTimeSeconds, Scene::IEntityManagerUVE& entityManager,
                            const std::vector<Scene::EntityUVE>& rootEntities) = 0;

    /// Immediately saves `rootEntities` to kManualCheckpointSlotIndexUVE (the spec's "manual save points
    /// - e.g. before boss fight"), regardless of how much time has elapsed since the last auto-save,
    /// then resets the since-last-save counter to 0 only when the save succeeds. Returns whatever the
    /// underlying ISaveGameSystemUVE::SaveUVE() returned.
    [[nodiscard]] virtual bool CheckpointUVE(Scene::IEntityManagerUVE& entityManager,
                                              const std::vector<Scene::EntityUVE>& rootEntities) = 0;

    /// Changes the auto-save interval, in seconds, without resetting the current elapsed
    /// counter — shortening the interval below the already-elapsed time triggers an auto-save on
    /// the very next UpdateUVE() call, rather than silently skipping one.
    virtual void SetAutoSaveIntervalSecondsUVE(double intervalSeconds) noexcept = 0;
    [[nodiscard]] virtual double GetAutoSaveIntervalSecondsUVE() const noexcept = 0;

    /// Seconds accumulated since the last successful auto-save. Successful manual checkpoints
    /// reset this accumulator because they defer the next automatic save; failed saves preserve it
    /// for retry. Exposed
    /// so tests can assert "just under the interval, no save fired" / "at/over the interval, one
    /// fired" without a real wall-clock wait.
    [[nodiscard]] virtual double GetElapsedSinceLastSaveSecondsUVE() const noexcept = 0;

    /// Total gameplay seconds accumulated across every UpdateUVE() call since construction —
    /// copied into GameStateMetadataUVE::playtimeSeconds for every auto-save/checkpoint this
    /// CheckpointManagerUVE writes.
    [[nodiscard]] virtual double GetTotalPlaytimeSecondsUVE() const noexcept = 0;
};

} // namespace UVE::Save
