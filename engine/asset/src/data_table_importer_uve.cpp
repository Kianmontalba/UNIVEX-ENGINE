// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/data_table_importer_uve.h"

#include <filesystem>
#include <fstream>
#include <iterator>
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
