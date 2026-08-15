// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <cstddef>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "uve/asset/data_table_asset_uve.h"
#include "uve/asset/uve_file_envelope_uve.h"

namespace UVE::Asset::Tests {
namespace {

[[nodiscard]] std::filesystem::path TestPathUVE(const std::string& suffix) {
    return std::filesystem::temp_directory_path() / ("uve_data_table_asset_" + suffix + ".uvetable");
}

[[nodiscard]] DataTableUVE MakeTableUVE(const std::string& name, const std::string& rowId,
                                        const std::int64_t value) {
    DataTableUVE table(name);
    static_cast<void>(table.DefineColumnUVE("damage", DataTableColumnTypeUVE::Integer));
    static_cast<void>(table.DefineColumnUVE("label", DataTableColumnTypeUVE::String));
    static_cast<void>(table.AddRowUVE(rowId, {value, std::string{"pistol"}}));
    return table;
}

class TemporaryAssetPathUVE final {
public:
    explicit TemporaryAssetPathUVE(std::filesystem::path path) : m_path(std::move(path)) {
        static_cast<void>(std::filesystem::remove(m_path));
    }
    TemporaryAssetPathUVE(const TemporaryAssetPathUVE&) = delete;
    TemporaryAssetPathUVE& operator=(const TemporaryAssetPathUVE&) = delete;
    ~TemporaryAssetPathUVE() { static_cast<void>(std::filesystem::remove(m_path)); }

    [[nodiscard]] const std::filesystem::path& GetUVE() const noexcept { return m_path; }

private:
    std::filesystem::path m_path;
};

TEST(DataTableAssetUVE, SaveAndLoadRoundTripUsesUniversalDataTableAssetKind) {
    const TemporaryAssetPathUVE path(TestPathUVE("round_trip"));
    const DataTableUVE source = MakeTableUVE("weapons", "pistol", 25);
    ASSERT_TRUE(SaveDataTableAssetUVE(source, path.GetUVE()));

    const auto file = ReadUveFileUVE(path.GetUVE());
    ASSERT_TRUE(file.has_value());
    EXPECT_EQ(file->first.assetType, AssetKindUVE::DataTable);

    DataTableUVE loaded("replacement");
    ASSERT_TRUE(loaded.DefineColumnUVE("old", DataTableColumnTypeUVE::String));
    ASSERT_TRUE(LoadDataTableAssetUVE(path.GetUVE(), loaded));
    EXPECT_EQ(loaded.GetSnapshotUVE(), source.GetSnapshotUVE());
}

TEST(DataTableAssetUVE, RejectsWrongKindAndMalformedPayloadWithoutChangingDestination) {
    const TemporaryAssetPathUVE wrongKindPath(TestPathUVE("wrong_kind"));
    const TemporaryAssetPathUVE malformedPath(TestPathUVE("malformed"));
    const DataTableUVE original = MakeTableUVE("original", "row", 1);
    DataTableUVE destination = MakeTableUVE("destination", "keep", 7);
    const DataTableSnapshotUVE before = destination.GetSnapshotUVE();

    const std::vector<std::byte> wrongKindPayload{std::byte{'x'}};
    ASSERT_TRUE(WriteUveFileUVE(wrongKindPath.GetUVE(), AssetKindUVE::Blob, wrongKindPayload));
    EXPECT_FALSE(LoadDataTableAssetUVE(wrongKindPath.GetUVE(), destination));
    EXPECT_EQ(destination.GetSnapshotUVE(), before);

    ASSERT_TRUE(WriteUveFileUVE(malformedPath.GetUVE(), AssetKindUVE::DataTable,
                                std::vector<std::byte>{std::byte{'n'}, std::byte{'o'}, std::byte{'t'}}));
    EXPECT_FALSE(LoadDataTableAssetUVE(malformedPath.GetUVE(), destination));
    EXPECT_EQ(destination.GetSnapshotUVE(), before);
    EXPECT_NE(destination.GetSnapshotUVE(), original.GetSnapshotUVE());
}

TEST(DataTableAssetUVE, RejectsInvalidSourceAndMissingFile) {
    const TemporaryAssetPathUVE invalidPath(TestPathUVE("invalid_source"));
    DataTableUVE invalid("contains space");
    ASSERT_TRUE(invalid.DefineColumnUVE("value", DataTableColumnTypeUVE::Integer));
    ASSERT_TRUE(invalid.AddRowUVE("row", {std::int64_t{1}}));
    EXPECT_FALSE(SaveDataTableAssetUVE(invalid, invalidPath.GetUVE()));
    EXPECT_FALSE(std::filesystem::exists(invalidPath.GetUVE()));

    DataTableUVE unchanged = MakeTableUVE("unchanged", "row", 3);
    const DataTableSnapshotUVE before = unchanged.GetSnapshotUVE();
    EXPECT_FALSE(LoadDataTableAssetUVE(TestPathUVE("does_not_exist"), unchanged));
    EXPECT_EQ(unchanged.GetSnapshotUVE(), before);
}

} // namespace
} // namespace UVE::Asset::Tests
