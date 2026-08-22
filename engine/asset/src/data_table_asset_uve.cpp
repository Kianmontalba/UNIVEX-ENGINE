// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/data_table_asset_uve.h"

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "uve/asset/uve_file_envelope_uve.h"
#include "uve/debug/logging_macros_uve.h"

namespace UVE::Asset {

bool SaveDataTableAssetUVE(const DataTableUVE& table, const std::filesystem::path& path) {
    std::string document;
    if (!DataTableAssetSerializerUVE::SerializeUVE(table, document)) {
        UVE_ERROR("DataTableAssetUVE: failed to serialize table at {}", path.string());
        return false;
    }
    const auto* const payloadBytes = reinterpret_cast<const std::byte*>(document.data());
    const std::vector<std::byte> payload(payloadBytes, payloadBytes + document.size());
    return WriteUveFileUVE(path, AssetKindUVE::DataTable, payload);
}

bool LoadDataTableAssetUVE(const std::filesystem::path& path, DataTableUVE& table) {
    const std::optional<std::pair<UveFileHeaderUVE, std::vector<std::byte>>> file = ReadUveFileUVE(path);
    if (!file.has_value()) {
        return false;
    }
    if (file->first.assetType != AssetKindUVE::DataTable) {
        UVE_ERROR("DataTableAssetUVE: {} is not a data-table file (asset type {})", path.string(),
                  static_cast<std::uint32_t>(file->first.assetType));
        return false;
    }

    const std::vector<std::byte>& payload = file->second;
    if (payload.empty() || payload.size() > DataTableUVE::kMaximumDocumentBytesUVE) {
        UVE_ERROR("DataTableAssetUVE: {} has an empty or oversized document payload", path.string());
        return false;
    }
    const std::string document(reinterpret_cast<const char*>(payload.data()), payload.size());
    DataTableUVE candidate;
    if (!DataTableAssetSerializerUVE::DeserializeUVE(document, candidate)) {
        UVE_ERROR("DataTableAssetUVE: failed to deserialize {}", path.string());
        return false;
    }
    table = std::move(candidate);
    return true;
}

} // namespace UVE::Asset
