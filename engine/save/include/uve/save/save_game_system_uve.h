// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <filesystem>

#include "uve/save/i_save_game_system_uve.h"
#include "uve/scene/i_scene_serializer_uve.h"

namespace UVE::Save {

/// SaveGameSystemUVE is the concrete, engine-standard implementation of ISaveGameSystemUVE.
/// Composes a Scene::ISceneSerializerUVE& (dependency injection, matching Physics::PhysicsSystemUVE's
/// ICollisionSystemUVE& precedent) rather than duplicating any of its private per-component JSON
/// (de)serialization table: "world state" is bounced through a scratch `.uvescene`-shaped file on
/// disk — SaveUVE() calls sceneSerializer.SaveUVE() to a scratch path, reads the raw JSON payload
/// bytes back via Asset::ReadUveFileUVE(), deletes the scratch file, and embeds those bytes
/// verbatim into its own two-section `.uvesave` payload (metadata JSON, then the embedded world
/// JSON); LoadUVE() does the reverse. This is the only way to reuse SceneSerializerUVE's
/// already-tested component table without exposing it (deliberately private to
/// scene_serializer_uve.cpp) or changing ISceneSerializerUVE's signature.
/// A `.uvesave` file is its own dedicated `.uve*` envelope payload (Asset::AssetKindUVE::Save)
/// rather than an Asset::AssetBundleUVE-packed bundle: AssetBundleUVE::PackUVE() only reads
/// entries from real source files on disk and pays for a variable-length name-indexed entry
/// table a save file (always exactly two fixed sections) has no use for.
/// Writes are atomic: SaveUVE() writes to a temporary path in the save directory, then
/// std::filesystem::rename()s it over the real slot path only after a fully successful write, so
/// no half-written `.uvesave` file is ever visible at a slot's real path.
class SaveGameSystemUVE final : public ISaveGameSystemUVE {
public:
    /// `sceneSerializer` must outlive this SaveGameSystemUVE. `saveDirectory` is created lazily
    /// on first SaveUVE() (never at construction, so a SaveGameSystemUVE that never saves never
    /// touches disk).
    SaveGameSystemUVE(Scene::ISceneSerializerUVE& sceneSerializer, std::filesystem::path saveDirectory);

    [[nodiscard]] bool SaveUVE(int slotIndex, Scene::IEntityManagerUVE& entityManager,
                                const std::vector<Scene::EntityUVE>& rootEntities,
                                const GameStateMetadataUVE& metadata) override;
    [[nodiscard]] SaveMigrationRegistrationResultUVE RegisterMigrationUVE(
        std::uint32_t sourceSchemaVersion, std::uint32_t targetSchemaVersion,
        SavePayloadMigrationTransformUVE transform) override;
    [[nodiscard]] std::vector<Scene::EntityUVE> LoadUVE(int slotIndex,
                                                         Scene::IEntityManagerUVE& entityManager) override;
    [[nodiscard]] bool DeleteSaveUVE(int slotIndex) override;
    [[nodiscard]] bool HasSaveUVE(int slotIndex) const override;
    [[nodiscard]] std::optional<GameStateMetadataUVE> GetSaveMetadataUVE(int slotIndex) const override;
    [[nodiscard]] std::vector<int> ListUsedSlotsUVE() const override;
    [[nodiscard]] SaveMigrationDiagnosticsUVE GetLastMigrationDiagnosticsUVE() const override;

private:
    Scene::ISceneSerializerUVE* m_sceneSerializer;
    std::filesystem::path m_saveDirectory;
    SavePayloadMigrationRegistryUVE m_migrationRegistry;
    mutable SaveMigrationDiagnosticsUVE m_lastMigrationDiagnostics;
};

} // namespace UVE::Save
