//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "uve/asset/asset_database_uve.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <mutex>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "uve/debug/logging_macros_uve.h"

namespace UVE::Asset {

namespace {

[[nodiscard]] std::string ToHexStringUVE(std::uint64_t value) {
    char buffer[17];
    std::snprintf(buffer, sizeof(buffer), "%016llx", static_cast<unsigned long long>(value));
    return std::string(buffer);
}

[[nodiscard]] std::uint64_t FromHexStringUVE(const std::string& hex) {
    return std::strtoull(hex.c_str(), nullptr, 16);
}

/// Writes `document` to `path`, logging a detailed Error (path + reason) on failure — the same
/// contract ConfigManagerUVE's save path uses.
[[nodiscard]] bool SaveDocumentToFileUVE(const nlohmann::json& document, const std::filesystem::path& path) {
    std::ofstream file(path);
    if (!file.is_open()) {
        UVE_ERROR("AssetDatabaseUVE: failed to open \"{}\" for writing: {}", path.string(),
                   std::strerror(errno));
        return false;
    }
    file << document.dump(4);
    if (!file.good()) {
        UVE_ERROR("AssetDatabaseUVE: failed to write registry to \"{}\": stream error after write",
                   path.string());
        return false;
    }
    return true;
}

} // namespace

struct AssetDatabaseUVE::ImplUVE {
    mutable std::mutex mutex;
    std::unordered_map<AssetGuidUVE, std::filesystem::path> guidToPath;
    std::unordered_map<std::string, AssetGuidUVE> pathToGuid;
    std::filesystem::path loadedPath;
};

AssetDatabaseUVE::AssetDatabaseUVE() : m_impl(std::make_unique<ImplUVE>()) {}

AssetDatabaseUVE::~AssetDatabaseUVE() = default;

bool AssetDatabaseUVE::LoadUVE(const std::filesystem::path& path) {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->loadedPath = path;

    std::ifstream file(path);
    if (!file.is_open()) {
        UVE_WARNING("AssetDatabaseUVE: registry file not found at \"{}\" - starting with an "
                    "empty registry",
                    path.string());
        m_impl->guidToPath.clear();
        m_impl->pathToGuid.clear();
        return false;
    }

    try {
        nlohmann::json parsed;
        file >> parsed;

        std::unordered_map<AssetGuidUVE, std::filesystem::path> guidToPath;
        std::unordered_map<std::string, AssetGuidUVE> pathToGuid;
        for (const auto& [guidHex, pathValue] : parsed.items()) {
            const AssetGuidUVE guid{FromHexStringUVE(guidHex)};
            const std::string pathString = pathValue.get<std::string>();
            guidToPath.emplace(guid, std::filesystem::path(pathString));
            pathToGuid.emplace(pathString, guid);
        }
        m_impl->guidToPath = std::move(guidToPath);
        m_impl->pathToGuid = std::move(pathToGuid);
        return true;
    } catch (const nlohmann::json::exception& parseError) {
        UVE_ERROR("AssetDatabaseUVE: failed to parse registry file \"{}\": {}", path.string(),
                   parseError.what());
        return false;
    }
}

bool AssetDatabaseUVE::SaveUVE() {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    if (m_impl->loadedPath.empty()) {
        UVE_ERROR("AssetDatabaseUVE: SaveUVE() called with no known target path - call LoadUVE() "
                  "or SaveUVE(path) first");
        return false;
    }

    nlohmann::json document = nlohmann::json::object();
    for (const auto& [guid, path] : m_impl->guidToPath) {
        document[ToHexStringUVE(guid.value)] = path.string();
    }
    return SaveDocumentToFileUVE(document, m_impl->loadedPath);
}

bool AssetDatabaseUVE::SaveUVE(const std::filesystem::path& path) {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->loadedPath = path;

    nlohmann::json document = nlohmann::json::object();
    for (const auto& [guid, storedPath] : m_impl->guidToPath) {
        document[ToHexStringUVE(guid.value)] = storedPath.string();
    }
    return SaveDocumentToFileUVE(document, path);
}

AssetGuidUVE AssetDatabaseUVE::RegisterUVE(const std::filesystem::path& assetPath) {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    const std::string pathKey = assetPath.string();

    const auto existingIt = m_impl->pathToGuid.find(pathKey);
    if (existingIt != m_impl->pathToGuid.end()) {
        return existingIt->second;
    }

    AssetGuidUVE guid = GenerateAssetGuidUVE();
    while (m_impl->guidToPath.find(guid) != m_impl->guidToPath.end()) {
        guid = GenerateAssetGuidUVE(); // vanishingly unlikely, guarded anyway
    }

    m_impl->guidToPath.emplace(guid, assetPath);
    m_impl->pathToGuid.emplace(pathKey, guid);
    return guid;
}

std::filesystem::path AssetDatabaseUVE::ResolveUVE(AssetGuidUVE guid) const {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    const auto it = m_impl->guidToPath.find(guid);
    if (it == m_impl->guidToPath.end()) {
        return {};
    }
    return it->second;
}

bool AssetDatabaseUVE::HasGuidUVE(AssetGuidUVE guid) const {
    const std::lock_guard<std::mutex> lock(m_impl->mutex);
    return m_impl->guidToPath.find(guid) != m_impl->guidToPath.end();
}

} // namespace UVE::Asset
