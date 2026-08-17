// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "uve/asset/i_file_system_uve.h"
#include "uve/scene/components/script_component_uve.h"
#include "uve/scripting/script_graph_persistence_uve.h"

namespace UVE::Scripting {

enum class ScriptAssetLoadCodeUVE : std::uint8_t {
    Loaded = 0,
    NoScript,
    InvalidPath,
    MissingFile,
    TextTooLarge,
    EmbeddedNul,
    DecodeRejected,
};

struct ScriptAssetLoadResultUVE final {
    ScriptAssetLoadCodeUVE code = ScriptAssetLoadCodeUVE::InvalidPath;
    std::size_t sourceByteCount = 0U;
    std::optional<ScriptGraphSchemaUVE> schema;
    std::vector<ScriptPersistenceDiagnosticUVE> diagnostics;
    std::string message;

    [[nodiscard]] bool IsLoadedUVE() const noexcept {
        return code == ScriptAssetLoadCodeUVE::Loaded && schema.has_value();
    }

    [[nodiscard]] bool IsNoScriptUVE() const noexcept {
        return code == ScriptAssetLoadCodeUVE::NoScript;
    }
};

/// Stateless, main-thread script asset boundary. It validates the component's project-relative
/// virtual path, reads one bounded file through the borrowed VFS, decodes the existing graph-schema
/// format, and returns copied data. It never owns the VFS, file bytes, graph, bytecode, ECS entity,
/// runtime attachment, or hot-reload state. Calling LoadSchemaUVE again is the deterministic reload
/// operation; a caller decides whether to preserve the previous graph or runtime on failure.
class ScriptAssetLoaderUVE final {
public:
    [[nodiscard]] static ScriptAssetLoadResultUVE LoadSchemaUVE(
        const Scene::ScriptComponentUVE& component, const Asset::IFileSystemUVE& fileSystem,
        ScriptGraphPersistenceLimitsUVE limits = {});
};

} // namespace UVE::Scripting
