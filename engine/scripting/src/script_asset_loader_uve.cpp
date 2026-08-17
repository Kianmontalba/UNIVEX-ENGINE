// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/scripting/script_asset_loader_uve.h"

#include <cstddef>
#include <string_view>
#include <utility>

namespace UVE::Scripting {

namespace {

[[nodiscard]] ScriptAssetLoadResultUVE FailureUVE(const ScriptAssetLoadCodeUVE code,
                                                   const std::size_t sourceByteCount,
                                                   std::string message) {
    ScriptAssetLoadResultUVE result;
    result.code = code;
    result.sourceByteCount = sourceByteCount;
    result.message = std::move(message);
    return result;
}

} // namespace

ScriptAssetLoadResultUVE ScriptAssetLoaderUVE::LoadSchemaUVE(
    const Scene::ScriptComponentUVE& component, const Asset::IFileSystemUVE& fileSystem,
    const ScriptGraphPersistenceLimitsUVE limits) {
    if (component.scriptAssetPath.empty()) {
        return {ScriptAssetLoadCodeUVE::NoScript, 0U, std::nullopt, {},
                "Script asset loading skipped because the component has no script path."};
    }
    if (!Scene::IsScriptComponentValidUVE(component)) {
        return FailureUVE(ScriptAssetLoadCodeUVE::InvalidPath, 0U,
                          "Script asset loading rejected an invalid project-relative virtual path.");
    }

    const std::optional<std::vector<std::byte>> bytes = fileSystem.ReadFileUVE(component.scriptAssetPath);
    if (!bytes.has_value()) {
        return FailureUVE(ScriptAssetLoadCodeUVE::MissingFile, 0U,
                          "Script asset loading could not resolve the virtual path through the VFS.");
    }
    if (bytes->size() > limits.maximumTextBytes) {
        return FailureUVE(ScriptAssetLoadCodeUVE::TextTooLarge, bytes->size(),
                          "Script asset loading rejected a file larger than the graph text limit.");
    }

    std::string text;
    text.reserve(bytes->size());
    for (const std::byte byte : *bytes) {
        if (byte == std::byte{0}) {
            return FailureUVE(ScriptAssetLoadCodeUVE::EmbeddedNul, bytes->size(),
                              "Script asset loading rejected embedded NUL data.");
        }
        text.push_back(static_cast<char>(std::to_integer<unsigned char>(byte)));
    }

    ScriptGraphSchemaDecodeResultUVE decoded = DecodeScriptGraphSchemaUVE(text, limits);
    if (!decoded.IsSuccessUVE()) {
        ScriptAssetLoadResultUVE result;
        result.code = ScriptAssetLoadCodeUVE::DecodeRejected;
        result.sourceByteCount = bytes->size();
        result.diagnostics = std::move(decoded.diagnostics);
        result.message = "Script asset loading rejected the graph-schema payload.";
        return result;
    }

    ScriptAssetLoadResultUVE result;
    result.code = ScriptAssetLoadCodeUVE::Loaded;
    result.sourceByteCount = bytes->size();
    result.schema = std::move(decoded.schema);
    result.message = "Script graph schema loaded from the virtual file system.";
    return result;
}

} // namespace UVE::Scripting
