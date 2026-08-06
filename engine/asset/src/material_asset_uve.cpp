// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/asset/material_asset_uve.h"

#include <nlohmann/json.hpp>

#include "uve/asset/uve_file_envelope_uve.h"
#include "uve/debug/logging_macros_uve.h"

namespace UVE::Asset {

namespace {

[[nodiscard]] nlohmann::json Vector3ToJsonUVE(const Math::Vector3UVE& value) {
    return nlohmann::json{{"x", value.x}, {"y", value.y}, {"z", value.z}};
}

[[nodiscard]] Math::Vector3UVE JsonToVector3UVE(const nlohmann::json& value) {
    return Math::Vector3UVE{value.at("x").get<float>(), value.at("y").get<float>(), value.at("z").get<float>()};
}

} // namespace

bool LoadMaterialAssetUVE(const std::filesystem::path& path, MaterialAssetUVE& outMaterial) {
    const std::optional<std::pair<UveFileHeaderUVE, std::vector<std::byte>>> file = ReadUveFileUVE(path);
    if (!file.has_value()) {
        return false; // ReadUveFileUVE already logged the specific reason.
    }
    if (file->first.assetType != AssetKindUVE::Material) {
        UVE_ERROR("MaterialAssetUVE: \"{}\" is not a material file (asset type {})", path.string(),
                   static_cast<std::uint32_t>(file->first.assetType));
        return false;
    }

    const std::vector<std::byte>& payloadBuffer = file->second;
    const std::string payloadText(reinterpret_cast<const char*>(payloadBuffer.data()), payloadBuffer.size());

    nlohmann::json payload;
    try {
        payload = nlohmann::json::parse(payloadText);
    } catch (const nlohmann::json::parse_error& parseError) {
        UVE_ERROR("MaterialAssetUVE: failed to parse \"{}\": {}", path.string(), parseError.what());
        return false;
    }

    MaterialAssetUVE material;
    try {
        material.albedoColor = JsonToVector3UVE(payload.at("albedoColor"));
        material.albedoTexture = AssetGuidUVE{payload.at("albedoTexture").get<std::uint64_t>()};
        material.normalTexture = AssetGuidUVE{payload.at("normalTexture").get<std::uint64_t>()};
        material.metallic = payload.at("metallic").get<float>();
        material.roughness = payload.at("roughness").get<float>();
        material.aoTexture = AssetGuidUVE{payload.at("aoTexture").get<std::uint64_t>()};
        material.emissiveColor = JsonToVector3UVE(payload.at("emissiveColor"));
        material.vertexShader = AssetGuidUVE{payload.at("vertexShader").get<std::uint64_t>()};
        material.fragmentShader = AssetGuidUVE{payload.at("fragmentShader").get<std::uint64_t>()};
        material.isTransparent = payload.at("isTransparent").get<bool>();
    } catch (const nlohmann::json::exception& fieldError) {
        UVE_ERROR("MaterialAssetUVE: \"{}\" is missing an expected field: {}", path.string(), fieldError.what());
        return false;
    }

    outMaterial = material;
    return true;
}

bool SaveMaterialAssetUVE(const MaterialAssetUVE& material, const std::filesystem::path& path) {
    nlohmann::json payload;
    payload["albedoColor"] = Vector3ToJsonUVE(material.albedoColor);
    payload["albedoTexture"] = material.albedoTexture.value;
    payload["normalTexture"] = material.normalTexture.value;
    payload["metallic"] = material.metallic;
    payload["roughness"] = material.roughness;
    payload["aoTexture"] = material.aoTexture.value;
    payload["emissiveColor"] = Vector3ToJsonUVE(material.emissiveColor);
    payload["vertexShader"] = material.vertexShader.value;
    payload["fragmentShader"] = material.fragmentShader.value;
    payload["isTransparent"] = material.isTransparent;

    const std::string payloadText = payload.dump();
    const auto* const payloadBytes = reinterpret_cast<const std::byte*>(payloadText.data());
    const std::vector<std::byte> payloadBuffer(payloadBytes, payloadBytes + payloadText.size());

    return WriteUveFileUVE(path, AssetKindUVE::Material, payloadBuffer);
}

} // namespace UVE::Asset
