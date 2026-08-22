// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/save/save_game_system_uve.h"
#include "uve/save/save_payload_compression_uve.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <exception>
#include <system_error>
#include <utility>

#include <nlohmann/json.hpp>

#include "uve/debug/logging_macros_uve.h"

namespace UVE::Save {

namespace {

[[nodiscard]] bool IsValidSlotIndexUVE(int slotIndex) noexcept {
    return slotIndex == kAutoSaveSlotIndexUVE || slotIndex == kManualCheckpointSlotIndexUVE ||
           (slotIndex >= 0 && slotIndex < kSaveSlotCountUVE);
}

[[nodiscard]] bool IsValidSaveMetadataUVE(const GameStateMetadataUVE& metadata) noexcept {
    return metadata.savedAtUnixSecondsUVE >= 0 && std::isfinite(metadata.playtimeSeconds) &&
           metadata.playtimeSeconds >= 0.0 && metadata.saveName.size() <= kMaximumSaveNameBytesUVE &&
           metadata.saveName.find('\0') == std::string::npos;
}

[[nodiscard]] bool IsSaveMetadataForSlotUVE(const GameStateMetadataUVE& metadata, const int slotIndex) noexcept {
    return metadata.slotIndex == slotIndex;
}

/// The filename stem (no extension) for `slotIndex` — `"autosave"` and
/// `"manual_checkpoint"` for reserved slots, or `"slot_07"`-style for numbered slots.
[[nodiscard]] std::string SlotFileStemUVE(int slotIndex) {
    if (slotIndex == kAutoSaveSlotIndexUVE) {
        return "autosave";
    }
    if (slotIndex == kManualCheckpointSlotIndexUVE) {
        return "manual_checkpoint";
    }
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "slot_%02d", slotIndex);
    return std::string(buffer);
}

[[nodiscard]] std::filesystem::path SlotFilePathUVE(const std::filesystem::path& saveDirectory, int slotIndex) {
    return saveDirectory / (SlotFileStemUVE(slotIndex) + ".uvesave");
}

/// The atomic-write staging path SaveUVE() writes to before renaming over SlotFilePathUVE().
[[nodiscard]] std::filesystem::path TempSaveFilePathUVE(const std::filesystem::path& saveDirectory, int slotIndex) {
    return saveDirectory / (SlotFileStemUVE(slotIndex) + ".uvesave.tmp");
}

/// The scratch `.uvescene`-shaped file SaveUVE()/LoadUVE() bounce world-state JSON through, to
/// reuse Scene::ISceneSerializerUVE without exposing its private per-component JSON table.
[[nodiscard]] std::filesystem::path ScratchScenePathUVE(const std::filesystem::path& saveDirectory, int slotIndex) {
    return saveDirectory / ("." + SlotFileStemUVE(slotIndex) + "_scratch.uvescene");
}

class ScratchFileCleanupUVE final {
public:
    explicit ScratchFileCleanupUVE(std::filesystem::path path) : m_path(std::move(path)) {}
    ~ScratchFileCleanupUVE() noexcept {
        std::error_code errorCode;
        std::filesystem::remove(m_path, errorCode);
    }

    ScratchFileCleanupUVE(const ScratchFileCleanupUVE&) = delete;
    ScratchFileCleanupUVE& operator=(const ScratchFileCleanupUVE&) = delete;

private:
    std::filesystem::path m_path;
};

void AppendBytesUVE(std::vector<std::byte>& buffer, const void* data, std::size_t size) {
    const auto* const bytes = static_cast<const std::byte*>(data);
    buffer.insert(buffer.end(), bytes, bytes + size);
}

void AppendUint32UVE(std::vector<std::byte>& buffer, std::uint32_t value) {
    AppendBytesUVE(buffer, &value, sizeof(value));
}

void AppendUint64UVE(std::vector<std::byte>& buffer, std::uint64_t value) {
    AppendBytesUVE(buffer, &value, sizeof(value));
}

[[nodiscard]] bool ReadUint32FromBufferUVE(const std::vector<std::byte>& buffer, std::size_t& offset,
                                            std::uint32_t& outValue) {
    if (offset + sizeof(outValue) > buffer.size()) {
        return false;
    }
    std::memcpy(&outValue, buffer.data() + offset, sizeof(outValue));
    offset += sizeof(outValue);
    return true;
}

[[nodiscard]] bool ReadUint64FromBufferUVE(const std::vector<std::byte>& buffer, std::size_t& offset,
                                            std::uint64_t& outValue) {
    if (offset + sizeof(outValue) > buffer.size()) {
        return false;
    }
    std::memcpy(&outValue, buffer.data() + offset, sizeof(outValue));
    offset += sizeof(outValue);
    return true;
}

[[nodiscard]] bool ReadBytesFromBufferUVE(const std::vector<std::byte>& buffer, std::size_t& offset,
                                           std::uint64_t length, std::vector<std::byte>& outBytes) {
    if (offset > buffer.size() || length > static_cast<std::uint64_t>(buffer.size() - offset)) {
        return false;
    }
    const std::size_t safeLength = static_cast<std::size_t>(length);
    outBytes.assign(buffer.begin() + static_cast<std::ptrdiff_t>(offset),
                     buffer.begin() + static_cast<std::ptrdiff_t>(offset + safeLength));
    offset += safeLength;
    return true;
}

[[nodiscard]] std::vector<std::byte> EncodeMetadataJsonUVE(const GameStateMetadataUVE& metadata) {
    nlohmann::json json;
    json["savedAtUnixSecondsUVE"] = metadata.savedAtUnixSecondsUVE;
    json["engineVersionMajor"] = metadata.engineVersionMajor;
    json["engineVersionMinor"] = metadata.engineVersionMinor;
    json["engineVersionPatch"] = metadata.engineVersionPatch;
    json["engineVersionBuild"] = metadata.engineVersionBuild;
    json["payloadSchemaVersion"] = metadata.payloadSchemaVersion;
    json["playtimeSeconds"] = metadata.playtimeSeconds;
    json["slotIndex"] = metadata.slotIndex;
    json["saveName"] = metadata.saveName;

    const std::string text = json.dump();
    const auto* const bytes = reinterpret_cast<const std::byte*>(text.data());
    return std::vector<std::byte>(bytes, bytes + text.size());
}

[[nodiscard]] std::optional<GameStateMetadataUVE> DecodeMetadataJsonUVE(const std::vector<std::byte>& bytes) {
    const std::string text(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    try {
        const nlohmann::json json = nlohmann::json::parse(text);
        GameStateMetadataUVE metadata;
        metadata.savedAtUnixSecondsUVE = json.at("savedAtUnixSecondsUVE").get<std::int64_t>();
        metadata.engineVersionMajor = json.at("engineVersionMajor").get<std::uint32_t>();
        metadata.engineVersionMinor = json.at("engineVersionMinor").get<std::uint32_t>();
        metadata.engineVersionPatch = json.at("engineVersionPatch").get<std::uint32_t>();
        metadata.engineVersionBuild = json.at("engineVersionBuild").get<std::uint32_t>();
        metadata.payloadSchemaVersion = json.value("payloadSchemaVersion", std::uint32_t{1});
        metadata.playtimeSeconds = json.at("playtimeSeconds").get<double>();
        metadata.slotIndex = json.at("slotIndex").get<int>();
        metadata.saveName = json.value("saveName", std::string{});
        return IsValidSaveMetadataUVE(metadata) ? std::optional<GameStateMetadataUVE>{metadata} : std::nullopt;
    } catch (const nlohmann::json::exception& jsonError) {
        UVE_ERROR("SaveGameSystemUVE: failed to parse save metadata: {}", jsonError.what());
        return std::nullopt;
    }
}

/// Reads only the schema discriminator needed to select a migration. Legacy metadata may not yet
/// satisfy DecodeMetadataJsonUVE(), so this intentionally performs no current-schema field access.
[[nodiscard]] std::optional<std::uint32_t> DecodeMetadataSchemaVersionUVE(
    const std::vector<std::byte>& bytes) {
    const std::string text(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    try {
        const nlohmann::json json = nlohmann::json::parse(text);
        return json.value("payloadSchemaVersion", std::uint32_t{1});
    } catch (const nlohmann::json::exception& jsonError) {
        UVE_ERROR("SaveGameSystemUVE: failed to parse save schema discriminator: {}", jsonError.what());
        return std::nullopt;
    }
}

/// Builds a `.uvesave` payload: `metadataJsonLength uint32`, `metadataJsonBytes`,
/// `worldJsonLength uint64`, `worldJsonBytes` — the *only* function that touches this fixed
/// layout, so a future increment can wrap this function's output (and SplitSavePayloadUVE's
/// input) through a compressor/encryptor without touching anything else in this file.
[[nodiscard]] std::vector<std::byte> BuildSavePayloadUVE(const std::vector<std::byte>& metadataJsonBytes,
                                                          const std::vector<std::byte>& worldJsonBytes) {
    std::vector<std::byte> payload;
    AppendUint32UVE(payload, static_cast<std::uint32_t>(metadataJsonBytes.size()));
    AppendBytesUVE(payload, metadataJsonBytes.data(), metadataJsonBytes.size());
    AppendUint64UVE(payload, worldJsonBytes.size());
    AppendBytesUVE(payload, worldJsonBytes.data(), worldJsonBytes.size());
    return CompressSavePayloadUVE(payload);
}

/// Splits a `.uvesave` payload back into its metadata and world JSON byte sections. Returns
/// false (no logging — callers attach path/slot context) on any truncation/bounds failure.
[[nodiscard]] bool SplitSavePayloadUVE(const std::vector<std::byte>& payload,
                                        std::vector<std::byte>& outMetadataJsonBytes,
                                        std::vector<std::byte>& outWorldJsonBytes) {
    std::size_t offset = 0;
    std::uint32_t metadataLength = 0;
    if (!ReadUint32FromBufferUVE(payload, offset, metadataLength) ||
        !ReadBytesFromBufferUVE(payload, offset, metadataLength, outMetadataJsonBytes)) {
        return false;
    }
    std::uint64_t worldLength = 0;
    if (!ReadUint64FromBufferUVE(payload, offset, worldLength) ||
        !ReadBytesFromBufferUVE(payload, offset, worldLength, outWorldJsonBytes)) {
        return false;
    }
    return true;
}

/// Reads only the metadata section of a `.uvesave` payload — never copies the (potentially much
/// larger) world JSON bytes, keeping GetSaveMetadataUVE() cheap relative to a full LoadUVE().
[[nodiscard]] bool SplitSaveMetadataOnlyUVE(const std::vector<std::byte>& payload,
                                             std::vector<std::byte>& outMetadataJsonBytes) {
    std::size_t offset = 0;
    std::uint32_t metadataLength = 0;
    if (!ReadUint32FromBufferUVE(payload, offset, metadataLength) ||
        !ReadBytesFromBufferUVE(payload, offset, metadataLength, outMetadataJsonBytes)) {
        return false;
    }
    std::uint64_t worldLength = 0U;
    if (!ReadUint64FromBufferUVE(payload, offset, worldLength)) {
        return false;
    }
    const std::size_t remainingBytes = payload.size() - offset;
    return worldLength == static_cast<std::uint64_t>(remainingBytes);
}

} // namespace

SaveGameSystemUVE::SaveGameSystemUVE(Scene::ISceneSerializerUVE& sceneSerializer, std::filesystem::path saveDirectory)
    : m_sceneSerializer(&sceneSerializer), m_saveDirectory(std::move(saveDirectory)) {}

SaveMigrationRegistrationResultUVE SaveGameSystemUVE::RegisterMigrationUVE(
    const std::uint32_t sourceSchemaVersion, const std::uint32_t targetSchemaVersion,
    SavePayloadMigrationTransformUVE transform) {
    return m_migrationRegistry.RegisterUVE(sourceSchemaVersion, targetSchemaVersion, std::move(transform));
}

bool SaveGameSystemUVE::SaveUVE(int slotIndex, Scene::IEntityManagerUVE& entityManager,
                                 const std::vector<Scene::EntityUVE>& rootEntities,
                                 const GameStateMetadataUVE& metadata) {
    if (!IsValidSlotIndexUVE(slotIndex)) {
        UVE_ERROR("SaveGameSystemUVE: SaveUVE called with an out-of-range slot index ({})", slotIndex);
        return false;
    }
    if (!IsValidSaveMetadataUVE(metadata)) {
        UVE_ERROR("SaveGameSystemUVE: SaveUVE rejected invalid playtime or save-name metadata for slot {}", slotIndex);
        return false;
    }

    std::error_code errorCode;
    std::filesystem::create_directories(m_saveDirectory, errorCode);
    if (errorCode) {
        UVE_ERROR("SaveGameSystemUVE: SaveUVE could not prepare save directory \"{}\": {}",
                  m_saveDirectory.string(), errorCode.message());
        return false;
    }

    const std::filesystem::path scratchPath = ScratchScenePathUVE(m_saveDirectory, slotIndex);
    if (!m_sceneSerializer->SaveUVE(entityManager, rootEntities, scratchPath, Scene::SceneAssetTypeUVE::Scene)) {
        UVE_ERROR("SaveGameSystemUVE: SaveUVE failed to serialize world state for slot {}", slotIndex);
        return false;
    }

    std::optional<std::pair<Asset::UveFileHeaderUVE, std::vector<std::byte>>> scratchFile =
        Asset::ReadUveFileUVE(scratchPath);
    std::filesystem::remove(scratchPath, errorCode);
    if (!scratchFile.has_value()) {
        UVE_ERROR("SaveGameSystemUVE: SaveUVE failed to re-read scratch world file for slot {}", slotIndex);
        return false;
    }

    GameStateMetadataUVE finalMetadata = metadata;
    finalMetadata.savedAtUnixSecondsUVE =
        static_cast<std::int64_t>(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
    finalMetadata.slotIndex = slotIndex;
    finalMetadata.payloadSchemaVersion = kCurrentSavePayloadSchemaVersionUVE;

    const std::vector<std::byte> metadataJsonBytes = EncodeMetadataJsonUVE(finalMetadata);
    const std::vector<std::byte> payload = BuildSavePayloadUVE(metadataJsonBytes, scratchFile->second);

    const std::filesystem::path tempPath = TempSaveFilePathUVE(m_saveDirectory, slotIndex);
    if (!Asset::WriteUveFileUVE(tempPath, Asset::AssetKindUVE::Save, payload)) {
        UVE_ERROR("SaveGameSystemUVE: SaveUVE failed to write temporary save file for slot {}", slotIndex);
        std::filesystem::remove(tempPath, errorCode);
        return false;
    }

    const std::filesystem::path finalPath = SlotFilePathUVE(m_saveDirectory, slotIndex);
    std::filesystem::rename(tempPath, finalPath, errorCode);
    if (errorCode) {
        UVE_ERROR("SaveGameSystemUVE: SaveUVE failed to finalize save file for slot {}: {}", slotIndex,
                   errorCode.message());
        std::filesystem::remove(tempPath, errorCode);
        return false;
    }

    return true;
}

std::vector<Scene::EntityUVE> SaveGameSystemUVE::LoadUVE(int slotIndex, Scene::IEntityManagerUVE& entityManager) {
    if (!IsValidSlotIndexUVE(slotIndex)) {
        UVE_ERROR("SaveGameSystemUVE: LoadUVE called with an out-of-range slot index ({})", slotIndex);
        return {};
    }

    const std::filesystem::path finalPath = SlotFilePathUVE(m_saveDirectory, slotIndex);
    std::optional<std::pair<Asset::UveFileHeaderUVE, std::vector<std::byte>>> file = Asset::ReadUveFileUVE(finalPath);
    if (!file.has_value()) {
        return {}; // ReadUveFileUVE already logged the specific reason.
    }
    auto& [header, payload] = file.value();
    if (header.assetType != Asset::AssetKindUVE::Save) {
        UVE_ERROR("SaveGameSystemUVE: \"{}\" has unexpected asset type {}", finalPath.string(),
                   static_cast<std::uint32_t>(header.assetType));
        return {};
    }
    std::vector<std::byte> expandedPayload;
    if (!DecompressSavePayloadUVE(payload, expandedPayload)) {
        UVE_ERROR("SaveGameSystemUVE: \"{}\" has a corrupt or oversized compressed payload", finalPath.string());
        return {};
    }
    payload = std::move(expandedPayload);

    std::vector<std::byte> metadataJsonBytes;
    std::vector<std::byte> worldJsonBytes;
    if (!SplitSavePayloadUVE(payload, metadataJsonBytes, worldJsonBytes)) {
        std::vector<std::byte> emptyPayload;
        m_lastMigrationDiagnostics = m_migrationRegistry.MigrateUVE(kCurrentSavePayloadSchemaVersionUVE,
                                                            kCurrentSavePayloadSchemaVersionUVE, emptyPayload);
        UVE_ERROR("SaveGameSystemUVE: \"{}\" has a corrupt or truncated save payload", finalPath.string());
        return {};
    }

    const std::optional<std::uint32_t> sourceSchemaVersion =
        DecodeMetadataSchemaVersionUVE(metadataJsonBytes);
    if (!sourceSchemaVersion.has_value()) {
        std::vector<std::byte> emptyPayload;
        m_lastMigrationDiagnostics = m_migrationRegistry.MigrateUVE(kCurrentSavePayloadSchemaVersionUVE,
                                                            kCurrentSavePayloadSchemaVersionUVE, emptyPayload);
        return {};
    }
    m_lastMigrationDiagnostics = m_migrationRegistry.MigrateUVE(sourceSchemaVersion.value(),
                                                        kCurrentSavePayloadSchemaVersionUVE, payload);
    if (!m_lastMigrationDiagnostics.SucceededUVE()) {
        UVE_ERROR("SaveGameSystemUVE: \"{}\" cannot load schema v{}: {}", finalPath.string(),
                   sourceSchemaVersion.value(), m_lastMigrationDiagnostics.reason);
        return {};
    }
    if (m_lastMigrationDiagnostics.status == SaveMigrationStatusUVE::Migrated &&
        !SplitSavePayloadUVE(payload, metadataJsonBytes, worldJsonBytes)) {
        UVE_ERROR("SaveGameSystemUVE: \"{}\" migration produced an invalid payload", finalPath.string());
        return {};
    }
    const std::optional<GameStateMetadataUVE> metadata = DecodeMetadataJsonUVE(metadataJsonBytes);
    if (!metadata.has_value()) {
        std::vector<std::byte> emptyPayload;
        m_lastMigrationDiagnostics = m_migrationRegistry.MigrateUVE(kCurrentSavePayloadSchemaVersionUVE,
                                                            kCurrentSavePayloadSchemaVersionUVE, emptyPayload);
        return {};
    }
    if (!IsValidSaveMetadataUVE(metadata.value())) {
        UVE_ERROR("SaveGameSystemUVE: slot {} contains invalid playtime metadata", slotIndex);
        return {};
    }
    if (!IsSaveMetadataForSlotUVE(metadata.value(), slotIndex)) {
        UVE_ERROR("SaveGameSystemUVE: slot {} contains metadata for slot {}", slotIndex, metadata->slotIndex);
        return {};
    }

    const std::filesystem::path scratchPath = ScratchScenePathUVE(m_saveDirectory, slotIndex);
    const ScratchFileCleanupUVE scratchCleanup(scratchPath);
    if (!Asset::WriteUveFileUVE(scratchPath, Asset::AssetKindUVE::Scene, worldJsonBytes)) {
        UVE_ERROR("SaveGameSystemUVE: LoadUVE failed to write scratch world file for slot {}", slotIndex);
        return {};
    }

    try {
        return m_sceneSerializer->LoadUVE(entityManager, scratchPath);
    } catch (const std::exception& exception) {
        UVE_ERROR("SaveGameSystemUVE: LoadUVE scene serializer threw for slot {}: {}", slotIndex, exception.what());
    } catch (...) {
        UVE_ERROR("SaveGameSystemUVE: LoadUVE scene serializer threw an unknown exception for slot {}", slotIndex);
    }
    return {};
}

bool SaveGameSystemUVE::DeleteSaveUVE(int slotIndex) {
    if (!IsValidSlotIndexUVE(slotIndex)) {
        UVE_ERROR("SaveGameSystemUVE: DeleteSaveUVE called with an out-of-range slot index ({})", slotIndex);
        return false;
    }

    std::error_code errorCode;
    const bool removed = std::filesystem::remove(SlotFilePathUVE(m_saveDirectory, slotIndex), errorCode);
    if (errorCode) {
        UVE_ERROR("SaveGameSystemUVE: DeleteSaveUVE failed for slot {}: {}", slotIndex, errorCode.message());
        return false;
    }
    return removed;
}

bool SaveGameSystemUVE::HasSaveUVE(int slotIndex) const {
    if (!IsValidSlotIndexUVE(slotIndex)) {
        return false;
    }
    return GetSaveMetadataUVE(slotIndex).has_value();
}

std::optional<GameStateMetadataUVE> SaveGameSystemUVE::GetSaveMetadataUVE(int slotIndex) const {
    if (!IsValidSlotIndexUVE(slotIndex)) {
        UVE_ERROR("SaveGameSystemUVE: GetSaveMetadataUVE called with an out-of-range slot index ({})", slotIndex);
        return std::nullopt;
    }

    const std::filesystem::path finalPath = SlotFilePathUVE(m_saveDirectory, slotIndex);
    std::optional<std::pair<Asset::UveFileHeaderUVE, std::vector<std::byte>>> file = Asset::ReadUveFileUVE(finalPath);
    if (!file.has_value()) {
        return std::nullopt;
    }
    const auto& [header, payload] = file.value();
    if (header.assetType != Asset::AssetKindUVE::Save) {
        UVE_ERROR("SaveGameSystemUVE: \"{}\" has unexpected asset type {}", finalPath.string(),
                   static_cast<std::uint32_t>(header.assetType));
        return std::nullopt;
    }
    std::vector<std::byte> expandedPayload;
    if (!DecompressSavePayloadUVE(payload, expandedPayload)) {
        UVE_ERROR("SaveGameSystemUVE: \"{}\" has a corrupt or oversized compressed payload", finalPath.string());
        return std::nullopt;
    }

    std::vector<std::byte> metadataJsonBytes;
    if (!SplitSaveMetadataOnlyUVE(expandedPayload, metadataJsonBytes)) {
        UVE_ERROR("SaveGameSystemUVE: \"{}\" has a corrupt or truncated save payload", finalPath.string());
        return std::nullopt;
    }

    const std::optional<std::uint32_t> sourceSchemaVersion =
        DecodeMetadataSchemaVersionUVE(metadataJsonBytes);
    if (!sourceSchemaVersion.has_value()) {
        return std::nullopt;
    }
    m_lastMigrationDiagnostics = m_migrationRegistry.MigrateUVE(sourceSchemaVersion.value(),
                                                        kCurrentSavePayloadSchemaVersionUVE, expandedPayload);
    if (!m_lastMigrationDiagnostics.SucceededUVE()) {
        UVE_ERROR("SaveGameSystemUVE: \"{}\" metadata schema v{} is unsupported: {}", finalPath.string(),
                   sourceSchemaVersion.value(), m_lastMigrationDiagnostics.reason);
        return std::nullopt;
    }
    if (m_lastMigrationDiagnostics.status == SaveMigrationStatusUVE::Migrated &&
        !SplitSaveMetadataOnlyUVE(expandedPayload, metadataJsonBytes)) {
        UVE_ERROR("SaveGameSystemUVE: \"{}\" migration produced an invalid payload", finalPath.string());
        return std::nullopt;
    }
    const std::optional<GameStateMetadataUVE> metadata = DecodeMetadataJsonUVE(metadataJsonBytes);
    if (!metadata.has_value() || !IsValidSaveMetadataUVE(metadata.value())) {
        if (metadata.has_value()) {
            UVE_ERROR("SaveGameSystemUVE: slot {} contains invalid playtime metadata", slotIndex);
        }
        return std::nullopt;
    }
    if (!IsSaveMetadataForSlotUVE(metadata.value(), slotIndex)) {
        UVE_ERROR("SaveGameSystemUVE: slot {} contains metadata for slot {}", slotIndex, metadata->slotIndex);
        return std::nullopt;
    }
    return metadata;
}

std::vector<int> SaveGameSystemUVE::ListUsedSlotsUVE() const {
    std::vector<int> slots;
    std::error_code errorCode;
    if (!std::filesystem::exists(m_saveDirectory, errorCode) || errorCode) {
        return slots;
    }

    for (int slotIndex = 0; slotIndex < kSaveSlotCountUVE; ++slotIndex) {
        if (HasSaveUVE(slotIndex)) {
            slots.push_back(slotIndex);
        }
    }
    return slots;
}

SaveMigrationDiagnosticsUVE SaveGameSystemUVE::GetLastMigrationDiagnosticsUVE() const {
    return m_lastMigrationDiagnostics;
}

} // namespace UVE::Save
