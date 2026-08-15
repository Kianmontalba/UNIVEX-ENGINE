// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/data_table_importer_uve.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>

#include "uve/asset/data_table_asset_uve.h"
#include "uve/debug/logging_macros_uve.h"

namespace UVE::Asset {
namespace {

enum class DataTableSourceFormatUVE : std::uint8_t {
    Csv,
    Tsv,
    Json,
};

[[nodiscard]] bool ReadSourceDocumentUVE(const std::filesystem::path& sourcePath, std::string& document) {
    std::ifstream input(sourcePath, std::ios::binary);
    if (!input.is_open()) {
        return false;
    }
    document.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    return document.size() <= DataTableUVE::kMaximumDocumentBytesUVE;
}

[[nodiscard]] bool ImportDataTableSourceUVE(const std::filesystem::path& sourcePath,
                                             const std::filesystem::path& destinationPath,
                                             const AssetImportSettingsUVE& baseSettings,
                                             const DataTableSourceFormatUVE format) {
    const auto* const settings = dynamic_cast<const DataTableImportSettingsUVE*>(&baseSettings);
    if (settings == nullptr || settings->tableName.empty() || settings->columns.empty() ||
        destinationPath.extension() != ".uvetable") {
        UVE_ERROR("DataTableImporterUVE: missing schema settings or invalid .uvetable destination");
        return false;
    }

    std::string document;
    if (!ReadSourceDocumentUVE(sourcePath, document)) {
        UVE_ERROR("DataTableImporterUVE: failed to read bounded source document {}", sourcePath.string());
        return false;
    }

    DataTableUVE table(settings->tableName);
    for (const DataTableColumnUVE& column : settings->columns) {
        if (!table.DefineColumnUVE(column.name, column.type)) {
            UVE_ERROR("DataTableImporterUVE: invalid schema settings for {}", sourcePath.string());
            return false;
        }
    }

    bool imported = false;
    switch (format) {
        case DataTableSourceFormatUVE::Csv:
            imported = table.ImportCsvUVE(document);
            break;
        case DataTableSourceFormatUVE::Tsv:
            imported = table.ImportTsvUVE(document);
            break;
        case DataTableSourceFormatUVE::Json:
            imported = table.ImportJsonUVE(document);
            break;
    }
    return imported && SaveDataTableAssetUVE(table, destinationPath);
}

} // namespace

std::string DataTableImportSettingsUVE::GetCacheVersionUVE() const {
    constexpr std::uint64_t kFnvOffsetBasis = 14695981039346656037ULL;
    constexpr std::uint64_t kFnvPrime = 1099511628211ULL;
    std::uint64_t hash = kFnvOffsetBasis;
    const auto appendBytes = [&hash](const std::string_view value) {
        for (const char character : value) {
            const auto byte = static_cast<std::uint64_t>(static_cast<unsigned char>(character));
            hash ^= byte;
            hash *= kFnvPrime;
        }
    };
    const auto appendLengthDelimited = [&appendBytes](const std::string_view value) {
        appendBytes(std::to_string(value.size()));
        appendBytes(":");
        appendBytes(value);
    };

    appendLengthDelimited(tableName);
    appendLengthDelimited(std::to_string(columns.size()));
    for (const DataTableColumnUVE& column : columns) {
        appendLengthDelimited(column.name);
        appendLengthDelimited(std::to_string(static_cast<unsigned int>(column.type)));
    }

    std::ostringstream version;
    version << "data-table-import-v2-" << std::hex << std::setw(16) << std::setfill('0') << hash;
    return version.str();
}

void RegisterDataTableImportersUVE(IAssetImporterUVE& importer) {
    importer.RegisterImporterUVE(".csv", [](const std::filesystem::path& sourcePath,
                                            const std::filesystem::path& destinationPath,
                                            const AssetImportSettingsUVE& settings) {
        return ImportDataTableSourceUVE(sourcePath, destinationPath, settings, DataTableSourceFormatUVE::Csv);
    });
    importer.RegisterImporterUVE(".tsv", [](const std::filesystem::path& sourcePath,
                                            const std::filesystem::path& destinationPath,
                                            const AssetImportSettingsUVE& settings) {
        return ImportDataTableSourceUVE(sourcePath, destinationPath, settings, DataTableSourceFormatUVE::Tsv);
    });
    importer.RegisterImporterUVE(".json", [](const std::filesystem::path& sourcePath,
                                             const std::filesystem::path& destinationPath,
                                             const AssetImportSettingsUVE& settings) {
        return ImportDataTableSourceUVE(sourcePath, destinationPath, settings, DataTableSourceFormatUVE::Json);
    });
}

} // namespace UVE::Asset
