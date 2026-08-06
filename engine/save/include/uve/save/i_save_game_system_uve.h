// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <optional>
#include <vector>

#include "uve/save/game_state_metadata_uve.h"
#include "uve/scene/entity_uve.h"
#include "uve/scene/i_entity_manager_uve.h"

namespace UVE::Save {

/// Number of numbered player save slots (spec: "Up to 99 save slots per user"). Valid numbered
/// slotIndex values for every ISaveGameSystemUVE method below are [0, kSaveSlotCountUVE).
inline constexpr int kSaveSlotCountUVE = 99;

/// The reserved slot CheckpointManagerUVE writes both auto-saves and manual checkpoints to —
/// deliberately outside [0, kSaveSlotCountUVE) so it can never collide with, or be enumerated
/// alongside, a player's own numbered saves (ListUsedSlotsUVE() never returns it). Every
/// ISaveGameSystemUVE method that takes a slotIndex accepts this value too, so
/// CheckpointManagerUVE needs no separate API surface of its own on ISaveGameSystemUVE.
inline constexpr int kAutoSaveSlotIndexUVE = -1;

/// ISaveGameSystemUVE is the core save/load manager (Part 17's SaveGameSystemUVE): slot-based
/// persistence of "world state" — every entity reachable from an explicit root-entity list,
/// exactly like Scene::ISceneSerializerUVE::SaveUVE()'s own contract (a caller-supplied list, not
/// an implicit "the whole world") — plus a small metadata section (timestamp, engine version,
/// playtime). Deliberately takes the same "explicit roots" shape as ISceneSerializerUVE rather
/// than inventing a second, competing "capture everything" mechanism; a caller that wants "the
/// whole scene" passes every entity ISceneGraphUVE::GetChildrenUVE() reports for
/// Scene::kInvalidEntityUVE (i.e. every scene-graph root), exactly as EngineCoreUVE does when
/// driving CheckpointManagerUVE.
/// Thread-safety: not thread-safe, matching ISceneSerializerUVE's own contract — every method
/// does direct file I/O and/or mutates the entity manager passed to it.
class ISaveGameSystemUVE {
public:
    virtual ~ISaveGameSystemUVE() = default;

    /// Serializes every entity reachable from `rootEntities` (via SceneSerializerUVE, exactly
    /// like ISceneSerializerUVE::SaveUVE()'s own reachability rule) plus `metadata` to the file
    /// backing `slotIndex`, overwriting any existing save in that slot. `slotIndex` must be
    /// kAutoSaveSlotIndexUVE or in [0, kSaveSlotCountUVE) — any other value logs a UVE_ERROR and
    /// returns false without touching disk. `metadata.savedAtUnixSecondsUVE`/`slotIndex`/
    /// `payloadSchemaVersion` are overwritten internally; every other field is taken verbatim
    /// from the caller. Returns false (logging the reason) on an out-of-range slot, an
    /// unregistered component type, or any file-system write error — never leaves a corrupt or
    /// partially-written file visible at the slot's path (see SaveGameSystemUVE's atomic
    /// temp-file-then-rename write).
    [[nodiscard]] virtual bool SaveUVE(int slotIndex, Scene::IEntityManagerUVE& entityManager,
                                        const std::vector<Scene::EntityUVE>& rootEntities,
                                        const GameStateMetadataUVE& metadata) = 0;

    /// Deserializes `slotIndex`'s save into fresh live entities via `entityManager` (exactly
    /// like ISceneSerializerUVE::LoadUVE()), returning the newly-created root entities in file
    /// order. Returns an empty vector (logging the reason) on an out-of-range slot, a missing
    /// save, a corrupt/truncated file, or an unsupported envelope/compression/asset-type.
    [[nodiscard]] virtual std::vector<Scene::EntityUVE> LoadUVE(int slotIndex,
                                                                 Scene::IEntityManagerUVE& entityManager) = 0;

    /// Deletes `slotIndex`'s save file. Returns true iff a file was actually removed; returns
    /// false without logging if the slot was simply already empty (not an error), or false while
    /// logging if `slotIndex` itself is out of range.
    [[nodiscard]] virtual bool DeleteSaveUVE(int slotIndex) = 0;

    /// True iff `slotIndex` is in range and currently has a save file. Never logs — a false
    /// result for an empty slot is the expected common case, not an error.
    [[nodiscard]] virtual bool HasSaveUVE(int slotIndex) const = 0;

    /// Reads and returns just `slotIndex`'s metadata section, without deserializing any entity —
    /// cheap enough to call for every slot when populating a save/load picker UI, unlike
    /// LoadUVE(), which pays full ECS entity-creation cost. Returns std::nullopt (logging the
    /// reason) if `slotIndex` is out of range, empty, or the file is corrupt/truncated.
    [[nodiscard]] virtual std::optional<GameStateMetadataUVE> GetSaveMetadataUVE(int slotIndex) const = 0;

    /// Returns every numbered slot (never kAutoSaveSlotIndexUVE) currently holding a save file,
    /// ascending. Returns an empty vector (not an error) if the save directory doesn't exist yet
    /// (a first-run engine has no saves).
    [[nodiscard]] virtual std::vector<int> ListUsedSlotsUVE() const = 0;
};

} // namespace UVE::Save
