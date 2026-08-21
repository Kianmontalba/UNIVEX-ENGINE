// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "uve/asset/asset_database_uve.h"
#include "uve/asset/asset_guid_uve.h"
#include "uve/asset/asset_importer_uve.h"
#include "uve/asset/data_table_asset_uve.h"
#include "uve/asset/data_table_importer_uve.h"

namespace UVE::Asset::Tests {
namespace {

void WriteTextUVE(const std::filesystem::path& path, const std::string& document) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(output.is_open());
    output << document;
}

class TemporaryImporterFilesUVE final {
public:
    TemporaryImporterFilesUVE() {
        m_paths = {
            std::filesystem::temp_directory_path() / "uve_data_table_importer.csv",
            std::filesystem::temp_directory_path() / "uve_data_table_importer.tsv",
            std::filesystem::temp_directory_path() / "uve_data_table_importer.json",
            std::filesystem::temp_directory_path() / "uve_data_table_importer_csv.uvetable",
            std::filesystem::temp_directory_path() / "uve_data_table_importer_tsv.uvetable",
            std::filesystem::temp_directory_path() / "uve_data_table_importer_json.uvetable",
            std::filesystem::temp_directory_path() / "uve_data_table_importer_bad.uvetable",
            std::filesystem::temp_directory_path() / "uve_data_table_importer_wrong.txt",
        };
        for (const std::filesystem::path& path : m_paths) {
            static_cast<void>(std::filesystem::remove(path));
        }
    }
    TemporaryImporterFilesUVE(const TemporaryImporterFilesUVE&) = delete;
    TemporaryImporterFilesUVE& operator=(const TemporaryImporterFilesUVE&) = delete;
    ~TemporaryImporterFilesUVE() {
        for (const std::filesystem::path& path : m_paths) {
            static_cast<void>(std::filesystem::remove(path));
        }
    }

    [[nodiscard]] const std::filesystem::path& CsvUVE() const noexcept { return m_paths[0]; }
    [[nodiscard]] const std::filesystem::path& TsvUVE() const noexcept { return m_paths[1]; }
    [[nodiscard]] const std::filesystem::path& JsonUVE() const noexcept { return m_paths[2]; }
    [[nodiscard]] const std::filesystem::path& CsvDestinationUVE() const noexcept { return m_paths[3]; }
    [[nodiscard]] const std::filesystem::path& TsvDestinationUVE() const noexcept { return m_paths[4]; }
    [[nodiscard]] const std::filesystem::path& JsonDestinationUVE() const noexcept { return m_paths[5]; }
    [[nodiscard]] const std::filesystem::path& BadDestinationUVE() const noexcept { return m_paths[6]; }
    [[nodiscard]] const std::filesystem::path& WrongDestinationUVE() const noexcept { return m_paths[7]; }

private:
    std::vector<std::filesystem::path> m_paths;
};

[[nodiscard]] DataTableImportSettingsUVE MakeSettingsUVE() {
    DataTableImportSettingsUVE settings;
    settings.tableName = "weapons";
    settings.columns = {
        DataTableColumnUVE{"damage", DataTableColumnTypeUVE::Integer},
        DataTableColumnUVE{"enabled", DataTableColumnTypeUVE::Boolean},
        DataTableColumnUVE{"label", DataTableColumnTypeUVE::String},
    };
    return settings;
}

TEST(DataTableImporterUVE, RegistersCsvTsvAndJsonImportersWithTypedDestinations) {
    const TemporaryImporterFilesUVE files;
    WriteTextUVE(files.CsvUVE(), "id,damage,enabled,label\npistol,25,true,Pistol\n");
    WriteTextUVE(files.TsvUVE(), "id\tdamage\tenabled\tlabel\npistol\t25\ttrue\tPistol\n");
    WriteTextUVE(files.JsonUVE(), R"([{"label":"Pistol","enabled":true,"damage":25,"id":"pistol"}])");

    AssetImporterUVE importer;
    RegisterDataTableImportersUVE(importer);
    AssetDatabaseUVE database;
    const DataTableImportSettingsUVE settings = MakeSettingsUVE();

    const AssetGuidUVE csvGuid = importer.ImportUVE(files.CsvUVE(), files.CsvDestinationUVE(), database, settings);
    const AssetGuidUVE tsvGuid = importer.ImportUVE(files.TsvUVE(), files.TsvDestinationUVE(), database, settings);
    const AssetGuidUVE jsonGuid = importer.ImportUVE(files.JsonUVE(), files.JsonDestinationUVE(), database, settings);
    ASSERT_NE(csvGuid, kInvalidAssetGuidUVE);
    ASSERT_NE(tsvGuid, kInvalidAssetGuidUVE);
    ASSERT_NE(jsonGuid, kInvalidAssetGuidUVE);

    DataTableUVE csv;
    DataTableUVE tsv;
    DataTableUVE json;
    ASSERT_TRUE(LoadDataTableAssetUVE(files.CsvDestinationUVE(), csv));
    ASSERT_TRUE(LoadDataTableAssetUVE(files.TsvDestinationUVE(), tsv));
    ASSERT_TRUE(LoadDataTableAssetUVE(files.JsonDestinationUVE(), json));
    EXPECT_EQ(csv.GetSnapshotUVE().rows.front().values, tsv.GetSnapshotUVE().rows.front().values);
    EXPECT_EQ(tsv.GetSnapshotUVE().rows.front().values, json.GetSnapshotUVE().rows.front().values);
    EXPECT_EQ(database.ResolveUVE(csvGuid), files.CsvDestinationUVE().lexically_normal());
    EXPECT_EQ(database.GetRegisteredAssetsUVE().size(), 3U);
}

TEST(DataTableImporterUVE, RejectsOverCapSourceBeforeTypedParseAndPublish) {
    const TemporaryImporterFilesUVE files;
    const std::string oversized(DataTableUVE::kMaximumDocumentBytesUVE + 1U, 'x');
    WriteTextUVE(files.CsvUVE(), oversized);

    AssetImporterUVE importer;
    RegisterDataTableImportersUVE(importer);
    AssetDatabaseUVE database;

    EXPECT_EQ(importer.ImportUVE(files.CsvUVE(), files.CsvDestinationUVE(), database, MakeSettingsUVE()),
              kInvalidAssetGuidUVE);
    EXPECT_FALSE(std::filesystem::exists(files.CsvDestinationUVE()));
    EXPECT_TRUE(database.GetRegisteredAssetsUVE().empty());
}

TEST(DataTableImporterUVE, RejectsMalformedSourceMissingSchemaAndWrongDestinationWithoutRegistration) {
    const TemporaryImporterFilesUVE files;
    WriteTextUVE(files.CsvUVE(), "id,damage,enabled,label\npistol,not-an-integer,true,Pistol\n");
    AssetImporterUVE importer;
    RegisterDataTableImportersUVE(importer);
    AssetDatabaseUVE database;
    const AssetImportSettingsUVE genericSettings;
    const DataTableImportSettingsUVE settings = MakeSettingsUVE();

    EXPECT_EQ(importer.ImportUVE(files.CsvUVE(), files.BadDestinationUVE(), database, settings), kInvalidAssetGuidUVE);
    EXPECT_EQ(importer.ImportUVE(files.CsvUVE(), files.CsvDestinationUVE(), database, genericSettings),
              kInvalidAssetGuidUVE);
    EXPECT_EQ(importer.ImportUVE(files.CsvUVE(), files.WrongDestinationUVE(), database, settings),
              kInvalidAssetGuidUVE);
    EXPECT_FALSE(std::filesystem::exists(files.BadDestinationUVE()));
    EXPECT_FALSE(std::filesystem::exists(files.CsvDestinationUVE()));
    EXPECT_TRUE(database.GetRegisteredAssetsUVE().empty());
}

} // namespace
} // namespace UVE::Asset::Tests


namespace UVE::Asset::Tests {

TEST(DataTableImportSettingsUVE, CacheVersionIsDeterministicAndSchemaSensitive) {
    DataTableImportSettingsUVE first;
    first.tableName = "items";
    first.columns = {
        DataTableColumnUVE{"label", DataTableColumnTypeUVE::String},
        DataTableColumnUVE{"count", DataTableColumnTypeUVE::Integer},
    };
    const DataTableImportSettingsUVE same = first;
    DataTableImportSettingsUVE reordered = first;
    std::swap(reordered.columns[0], reordered.columns[1]);
    DataTableImportSettingsUVE renamed = first;
    renamed.tableName = "other_items";
    DataTableImportSettingsUVE retyped = first;
    retyped.columns[1].type = DataTableColumnTypeUVE::Number;

    EXPECT_EQ(first.GetCacheVersionUVE(), same.GetCacheVersionUVE());
    EXPECT_NE(first.GetCacheVersionUVE(), reordered.GetCacheVersionUVE());
    EXPECT_NE(first.GetCacheVersionUVE(), renamed.GetCacheVersionUVE());
    EXPECT_NE(first.GetCacheVersionUVE(), retyped.GetCacheVersionUVE());
    EXPECT_EQ(first.GetCacheVersionUVE().size(), std::string{"data-table-import-v2-"}.size() + 16U);
}

} // namespace UVE::Asset::Tests
