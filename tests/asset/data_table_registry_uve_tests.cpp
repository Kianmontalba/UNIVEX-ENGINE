#include "uve/asset/data_table_registry_uve.h"

#include <cstdint>
#include <string>

#include <gtest/gtest.h>

namespace UVE::Asset::Tests {
namespace {

DataTableUVE MakeTableUVE(const std::string& name, const std::int64_t value) {
    DataTableUVE table(name);
    EXPECT_TRUE(table.DefineColumnUVE("value", DataTableColumnTypeUVE::Integer));
    EXPECT_TRUE(table.AddRowUVE("row", {value}));
    return table;
}

TEST(DataTableRegistryUVE, RegistersOwnedTablesAndRejectsDuplicatesAtomically) {
    DataTableRegistryUVE registry;
    EXPECT_EQ(registry.SizeUVE(), 0U);
    EXPECT_EQ(registry.GetCatalogSnapshotUVE().generation, 1U);

    ASSERT_TRUE(registry.RegisterUVE(MakeTableUVE("alpha", 1)));
    EXPECT_EQ(registry.SizeUVE(), 1U);
    EXPECT_EQ(registry.GetCatalogSnapshotUVE().generation, 2U);
    EXPECT_FALSE(registry.RegisterUVE(MakeTableUVE("alpha", 2)));
    EXPECT_EQ(registry.SizeUVE(), 1U);
    EXPECT_EQ(registry.GetCatalogSnapshotUVE().generation, 2U);
}

TEST(DataTableRegistryUVE, RejectsTablesWithDiagnosticsWithoutChangingRegistry) {
    DataTableRegistryUVE registry;
    DataTableUVE invalid("invalid");
    ASSERT_TRUE(invalid.DefineColumnUVE("value", DataTableColumnTypeUVE::Integer));
    EXPECT_FALSE(invalid.ImportCsvUVE("id,value\nrow,not-an-integer\n"));
    ASSERT_FALSE(invalid.GetSnapshotUVE().diagnostics.empty());

    EXPECT_FALSE(registry.RegisterUVE(std::move(invalid)));
    EXPECT_EQ(registry.SizeUVE(), 0U);
    EXPECT_EQ(registry.GetCatalogSnapshotUVE().generation, 1U);
}

TEST(DataTableRegistryUVE, ReturnsCopiedSnapshotsAndSortedCatalogFacts) {
    DataTableRegistryUVE registry;
    ASSERT_TRUE(registry.RegisterUVE(MakeTableUVE("zeta", 2)));
    ASSERT_TRUE(registry.RegisterUVE(MakeTableUVE("alpha", 1)));

    DataTableSnapshotUVE copied;
    ASSERT_TRUE(registry.TryGetSnapshotUVE("alpha", copied));
    copied.rows.clear();
    EXPECT_TRUE(copied.rows.empty());

    DataTableSnapshotUVE stillOwned;
    ASSERT_TRUE(registry.TryGetSnapshotUVE("alpha", stillOwned));
    ASSERT_EQ(stillOwned.rows.size(), 1U);
    EXPECT_EQ(stillOwned.rows.front().values.front(), DataTableValueUVE{std::int64_t{1}});

    const DataTableCatalogSnapshotUVE catalog = registry.GetCatalogSnapshotUVE();
    ASSERT_EQ(catalog.entries.size(), 2U);
    EXPECT_EQ(catalog.entries[0].name, "alpha");
    EXPECT_EQ(catalog.entries[1].name, "zeta");
    EXPECT_TRUE(catalog.entries[0].valid);
    EXPECT_EQ(catalog.generation, 3U);
}

TEST(DataTableRegistryUVE, RemoveAndClearAdvanceGenerationOnlyOnMutation) {
    DataTableRegistryUVE registry;
    ASSERT_TRUE(registry.RegisterUVE(MakeTableUVE("alpha", 1)));
    ASSERT_TRUE(registry.RemoveUVE("alpha"));
    EXPECT_EQ(registry.GetCatalogSnapshotUVE().generation, 3U);
    EXPECT_FALSE(registry.RemoveUVE("alpha"));
    EXPECT_EQ(registry.GetCatalogSnapshotUVE().generation, 3U);

    ASSERT_TRUE(registry.RegisterUVE(MakeTableUVE("beta", 2)));
    ASSERT_TRUE(registry.ClearUVE());
    EXPECT_EQ(registry.SizeUVE(), 0U);
    EXPECT_EQ(registry.GetCatalogSnapshotUVE().generation, 5U);
    EXPECT_FALSE(registry.ClearUVE());
    EXPECT_EQ(registry.GetCatalogSnapshotUVE().generation, 5U);
}

TEST(DataTableRegistryUVE, EnforcesMaximumTableBound) {
    DataTableRegistryUVE registry;
    for (std::size_t index = 0U; index < DataTableRegistryUVE::kMaximumTablesUVE; ++index) {
        ASSERT_TRUE(registry.RegisterUVE(MakeTableUVE("table_" + std::to_string(index), static_cast<std::int64_t>(index))));
    }
    EXPECT_EQ(registry.SizeUVE(), DataTableRegistryUVE::kMaximumTablesUVE);
    EXPECT_FALSE(registry.RegisterUVE(MakeTableUVE("overflow", 0)));
    EXPECT_EQ(registry.SizeUVE(), DataTableRegistryUVE::kMaximumTablesUVE);
}

} // namespace
} // namespace UVE::Asset::Tests
