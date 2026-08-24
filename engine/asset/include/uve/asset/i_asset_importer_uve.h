// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>

#include "uve/asset/asset_guid_uve.h"
#include "uve/asset/i_asset_database_uve.h"

namespace UVE::Asset {

/// The source-kind vocabulary is intentionally explicit even while raw format parsers remain
/// deferred; future format-specific importers extend this classification rather than hiding parser
/// authority inside generic copy behavior.
enum class AssetImportSourceKindUVE {
    Unknown,
    PlainText,
    SceneEnvelope,
    PrefabEnvelope,
    MeshEnvelope,
    TextureEnvelope,
    ShaderEnvelope,
    MaterialEnvelope,
    RawModel,
    RawTexture,
    RawMaterial,
    RawAudio,
};

/// Value-only classification of an import source. It describes the source format and current
/// importer/parser authority; it does not perform conversion or transfer filesystem ownership.
struct AssetImportSourceClassificationUVE final {
    AssetImportSourceKindUVE kind = AssetImportSourceKindUVE::Unknown;
    std::string normalizedExtension;
    bool importerRegistered = false;
    bool requiresFormatSpecificParser = false;
    std::string diagnostic;
};

/// Base for per-asset-type import settings. Format-specific importers derive from this and add
/// only settings that affect conversion; the bounded text parser below is the first such importer.
/// ImportUVE() therefore keeps one stable settings parameter as future formats add their own policy.
struct AssetImportSettingsUVE {
    virtual ~AssetImportSettingsUVE() = default;

    /// Stable cache discriminator for all import behavior represented by this settings object.
    /// Future format-specific settings override this when a conversion option changes derived
    /// output. The default keeps the current generic copy-and-register importer deterministic.
    [[nodiscard]] virtual std::string GetCacheVersionUVE() const { return "generic-v1"; }
};

enum class TextImportLineEndingUVE : std::uint8_t {
    Preserve,
    LineFeed,
    CarriageReturnLineFeed,
};

/// Settings for the bounded text-source parser. The parser rejects NUL bytes, normalizes CR/CRLF
/// according to this policy, and can add one terminal line ending. It does not infer encodings,
/// rewrite arbitrary Unicode, or become a general-purpose text-processing runtime.
struct TextImportSettingsUVE final : AssetImportSettingsUVE {
    TextImportLineEndingUVE lineEnding = TextImportLineEndingUVE::Preserve;
    bool ensureTrailingLineEnding = false;

    [[nodiscard]] std::string GetCacheVersionUVE() const override;
};

/// IAssetImporterUVE is the spec's "import pipeline with settings per asset type" (Part 7.4):
/// register an import function per source file extension, then ImportUVE() any source file
/// through whichever one matches. Built-in registrations include the bounded text parser for `.txt`,
/// deterministic generic envelope copying for UVE-owned formats, and bounded PNG/BMP/TGA-to-`.uvetex`,
/// OBJ-to-`.uvemodel`, MTL-to-`.uvemat`, glTF/GLB-to-`.uvemodel`, and JPEG-to-`.uvetex` bridges. FBX/WAV
/// and broader glTF scene/material/image conversion remains independently registered through RegisterImporterUVE().
/// Thread-safety: thread-safe. Every method is guarded by an internal mutex, matching
/// ConfigManagerUVE's/AssetDatabaseUVE's contract.
class IAssetImporterUVE {
public:
    virtual ~IAssetImporterUVE() = default;

    /// Registers `importFunc` for `sourceExtension` (matched case-insensitively, with or without
    /// a leading dot — ".png" and "png" register the same key): `importFunc(source, destination,
    /// settings)` should copy/convert `source` to `destination` and return true on success.
    /// Replaces any importer already registered for that extension, including a built-in one. Empty
    /// normalized extensions and empty callbacks are rejected without changing an existing importer.
    virtual void RegisterImporterUVE(
        std::string sourceExtension,
        std::function<bool(const std::filesystem::path&, const std::filesystem::path&,
                            const AssetImportSettingsUVE&)>
            importFunc) = 0;

    /// Classifies a source path without touching the filesystem. Raw model/texture/material/audio kinds
    /// explicitly report that a format-specific parser is required; current built-in UVE envelope
    /// kinds report deterministic generic copy authority, while PlainText reports the
    /// bounded text parser authority.
    [[nodiscard]] virtual AssetImportSourceClassificationUVE ClassifySourceUVE(
        const std::filesystem::path& sourcePath) const {
        static_cast<void>(sourcePath);
        return {};
    }

    /// Imports `sourcePath` to `destinationPath` using whichever importer is registered for
    /// `sourcePath`'s extension, registers `destinationPath` in `assetDatabase`, and returns its
    /// GUID. Returns `kInvalidAssetGuidUVE` (logging the reason) if no importer is registered for
    /// that extension, or if the registered importer itself reports failure.
    [[nodiscard]] virtual AssetGuidUVE ImportUVE(const std::filesystem::path& sourcePath,
                                                  const std::filesystem::path& destinationPath,
                                                  IAssetDatabaseUVE& assetDatabase,
                                                  const AssetImportSettingsUVE& settings = {}) = 0;
};

} // namespace UVE::Asset
