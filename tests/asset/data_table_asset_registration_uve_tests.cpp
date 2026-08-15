// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <cstddef>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "uve/asset/asset_database_uve.h"
#include "uve/asset/data_table_asset_registration_uve.h"
#include "uve/asset/data_table_asset_uve.h"
#include "uve/asset/uve_file_envelope_uve.h"

namespace UVE::Asset::Tests {
namespace {

[[nodiscard]] DataTableUVE MakeTableUVE() {
    DataTableUVE table("weapons");
    static_cast<void>(table.DefineColumnUVE("damage", DataTableColumnTypeUVE::Integer));
    static_cast<void>(table.AddRowUVE("pistol", {std::int64_t{25}}));
    return table;
}

class TemporaryRegistrationFilesUVE final {
public:
    TemporaryRegistrationFilesUVE() {
        for (const std::string name : {"valid.uvetable", "wrong_kind.uvetable", "wrong_extension.txt"}) {
            m_paths.emplace_back(std::filesystem::temp_directory_path() / ("uve_data_table_registration_" + name));
            static_cast<void>(std::filesystem::remove(m_paths.back()));
        }
    }
    TemporaryRegistrationFilesUVE(const TemporaryRegistrationFilesUVE&) = delete;
    TemporaryRegistrationFilesUVE& operator=(const TemporaryRegistrationFilesUVE&) = delete;
    ~TemporaryRegistrationFilesUVE() {
        for (const std::filesystem::path& path : m_paths) {
            static_cast<void>(std::filesystem::remove(path));
        }
    }

    [[nodiscard]] const std::filesystem::path& ValidUVE() const noexcept { return m_paths[0]; }
    [[nodiscard]] const std::filesystem::path& WrongKindUVE() const noexcept { return m_paths[1]; }
    [[nodiscard]] const std::filesystem::path& WrongExtensionUVE() const noexcept { return m_paths[2]; }

private:
    std::vector<std::filesystem::path> m_paths;
};

TEST(DataTableAssetRegistrationUVE, ValidatesAndRegistersIdempotentlyWithLexicalResolution) {
    const TemporaryRegistrationFilesUVE files;
    ASSERT_TRUE(SaveDataTableAssetUVE(MakeTableUVE(), files.ValidUVE()));
    AssetDatabaseUVE database;

    const std::optional<AssetGuidUVE> first = RegisterDataTableAssetUVE(database, files.ValidUVE());
    ASSERT_TRUE(first.has_value());
    EXPECT_NE(*first, kInvalidAssetGuidUVE);
    EXPECT_TRUE(database.HasGuidUVE(*first));
    EXPECT_EQ(database.ResolveUVE(*first), files.ValidUVE().lexically_normal());
    ASSERT_EQ(database.GetRegisteredAssetsUVE().size(), 1U);

    const std::optional<AssetGuidUVE> second = RegisterDataTableAssetUVE(database, files.ValidUVE());
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(*second, *first);
    EXPECT_EQ(database.GetRegisteredAssetsUVE().size(), 1U);
}

TEST(DataTableAssetRegistrationUVE, RejectsInvalidFilesBeforeChangingDatabase) {
    const TemporaryRegistrationFilesUVE files;
    ASSERT_TRUE(WriteUveFileUVE(files.WrongKindUVE(), AssetKindUVE::Blob,
                                std::vector<std::byte>{std::byte{'x'}}));
    ASSERT_TRUE(SaveDataTableAssetUVE(MakeTableUVE(), files.WrongExtensionUVE()));
    AssetDatabaseUVE database;

    EXPECT_FALSE(RegisterDataTableAssetUVE(database, files.WrongKindUVE()).has_value());
    EXPECT_FALSE(RegisterDataTableAssetUVE(database, files.WrongExtensionUVE()).has_value());
    EXPECT_FALSE(RegisterDataTableAssetUVE(database,
                                           std::filesystem::temp_directory_path() /
                                               "uve_data_table_registration_missing.uvetable").has_value());
    EXPECT_TRUE(database.GetRegisteredAssetsUVE().empty());
}

} // namespace
} // namespace UVE::Asset::Tests
