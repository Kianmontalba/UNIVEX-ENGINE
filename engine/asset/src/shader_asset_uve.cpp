//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/asset/shader_asset_uve.h"

#include <nlohmann/json.hpp>

#include "uve/asset/uve_file_envelope_uve.h"
#include "uve/debug/logging_macros_uve.h"

namespace UVE::Asset {

bool LoadShaderAssetUVE(const std::filesystem::path& path, ShaderAssetUVE& outShader) {
    const std::optional<std::pair<UveFileHeaderUVE, std::vector<std::byte>>> file = ReadUveFileUVE(path);
    if (!file.has_value()) {
        return false; // ReadUveFileUVE already logged the specific reason.
    }
    if (file->first.assetType != AssetKindUVE::Shader) {
        UVE_ERROR("ShaderAssetUVE: \"{}\" is not a shader file (asset type {})", path.string(),
                   static_cast<std::uint32_t>(file->first.assetType));
        return false;
    }

    const std::vector<std::byte>& payloadBuffer = file->second;
    const std::string payloadText(reinterpret_cast<const char*>(payloadBuffer.data()), payloadBuffer.size());

    nlohmann::json payload;
    try {
        payload = nlohmann::json::parse(payloadText);
    } catch (const nlohmann::json::parse_error& parseError) {
        UVE_ERROR("ShaderAssetUVE: failed to parse \"{}\": {}", path.string(), parseError.what());
        return false;
    }

    ShaderAssetUVE shader;
    try {
        shader.stage = static_cast<ShaderStageKindUVE>(payload.at("stage").get<std::uint8_t>());
        shader.sourceCode = payload.at("sourceCode").get<std::string>();
        shader.entryPointName = payload.value("entryPointName", std::string("main"));
    } catch (const nlohmann::json::exception& fieldError) {
        UVE_ERROR("ShaderAssetUVE: \"{}\" is missing an expected field: {}", path.string(), fieldError.what());
        return false;
    }

    if (shader.sourceCode.empty()) {
        UVE_ERROR("ShaderAssetUVE: \"{}\" has empty source code", path.string());
        return false;
    }

    outShader = std::move(shader);
    return true;
}

bool SaveShaderAssetUVE(const ShaderAssetUVE& shader, const std::filesystem::path& path) {
    nlohmann::json payload;
    payload["stage"] = static_cast<std::uint8_t>(shader.stage);
    payload["sourceCode"] = shader.sourceCode;
    payload["entryPointName"] = shader.entryPointName;

    const std::string payloadText = payload.dump();
    const auto* const payloadBytes = reinterpret_cast<const std::byte*>(payloadText.data());
    const std::vector<std::byte> payloadBuffer(payloadBytes, payloadBytes + payloadText.size());

    return WriteUveFileUVE(path, AssetKindUVE::Shader, payloadBuffer);
}

} // namespace UVE::Asset
