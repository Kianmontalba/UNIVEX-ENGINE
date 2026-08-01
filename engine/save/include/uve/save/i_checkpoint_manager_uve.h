//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <vector>

#include "uve/save/i_save_game_system_uve.h"
#include "uve/scene/entity_uve.h"
#include "uve/scene/i_entity_manager_uve.h"

namespace UVE::Save {

/// ICheckpointManagerUVE is the auto-save/manual-checkpoint system (Part 17's
/// CheckpointManagerUVE): accumulates elapsed time via UpdateUVE(), writing to
/// ISaveGameSystemUVE's reserved kAutoSaveSlotIndexUVE whenever the configured interval elapses;
/// CheckpointUVE() writes to that same reserved slot immediately, on demand (both share one
/// reserved slot for this "Foundations" increment).
///
/// TODO(Increment 19+): Future versions may separate autosave and manual checkpoint into
/// independent reserved slots. Sharing one slot means a manual checkpoint is silently overwritten
/// by the next auto-save tick, and vice versa — acceptable for this foundation increment, not a
/// long-term design.
///
/// A checkpoint — automatic or manual — resets the elapsed-time accumulator, so a manual
/// CheckpointUVE() call defers the next automatic one rather than being immediately followed by a
/// redundant one.
/// Thread-safety: not thread-safe, matching every other per-frame-driven system in this engine
/// (PhysicsSystemUVE, AudioSourceSystemUVE) — UpdateUVE() is intended to be called once per frame
/// from the main engine thread.
class ICheckpointManagerUVE {
public:
    virtual ~ICheckpointManagerUVE() = default;

    /// Accumulates `deltaTimeSeconds` into both the total-playtime counter and the
    /// since-last-save counter; once the latter reaches GetAutoSaveIntervalSecondsUVE(), saves
    /// `rootEntities` (from `entityManager`) to kAutoSaveSlotIndexUVE via the composed
    /// ISaveGameSystemUVE and resets the since-last-save counter to 0, whether or not the save
    /// succeeded (a save that fails once — e.g. a full disk — is retried on the next elapsed
    /// interval, not spammed every subsequent frame).
    virtual void UpdateUVE(double deltaTimeSeconds, Scene::IEntityManagerUVE& entityManager,
                            const std::vector<Scene::EntityUVE>& rootEntities) = 0;

    /// Immediately saves `rootEntities` to kAutoSaveSlotIndexUVE (the spec's "manual save points
    /// - e.g. before boss fight"), regardless of how much time has elapsed since the last
    /// auto-save, then resets the since-last-save counter to 0. Returns whatever the underlying
    /// ISaveGameSystemUVE::SaveUVE() returned.
    [[nodiscard]] virtual bool CheckpointUVE(Scene::IEntityManagerUVE& entityManager,
                                              const std::vector<Scene::EntityUVE>& rootEntities) = 0;

    /// Changes the auto-save interval, in seconds, without resetting the current elapsed
    /// counter — shortening the interval below the already-elapsed time triggers an auto-save on
    /// the very next UpdateUVE() call, rather than silently skipping one.
    virtual void SetAutoSaveIntervalSecondsUVE(double intervalSeconds) noexcept = 0;
    [[nodiscard]] virtual double GetAutoSaveIntervalSecondsUVE() const noexcept = 0;

    /// Seconds accumulated since the last successful-or-attempted auto-save/checkpoint. Exposed
    /// so tests can assert "just under the interval, no save fired" / "at/over the interval, one
    /// fired" without a real wall-clock wait.
    [[nodiscard]] virtual double GetElapsedSinceLastSaveSecondsUVE() const noexcept = 0;

    /// Total gameplay seconds accumulated across every UpdateUVE() call since construction —
    /// copied into GameStateMetadataUVE::playtimeSeconds for every auto-save/checkpoint this
    /// CheckpointManagerUVE writes.
    [[nodiscard]] virtual double GetTotalPlaytimeSecondsUVE() const noexcept = 0;
};

} // namespace UVE::Save
