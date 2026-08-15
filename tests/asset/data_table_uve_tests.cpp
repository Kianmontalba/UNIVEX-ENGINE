// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/asset/data_table_uve.h"

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

namespace UVE::Asset::Tests {

TEST(DataTableUVE, SchemaAndRowsExposeDeterministicTypedSnapshot) {
    DataTableUVE table("weapons");
    ASSERT_TRUE(table.DefineColumnUVE("damage", DataTableColumnTypeUVE::Integer));
    ASSERT_TRUE(table.DefineColumnUVE("rate", DataTableColumnTypeUVE::Number));
    ASSERT_TRUE(table.DefineColumnUVE("enabled", DataTableColumnTypeUVE::Boolean));
    ASSERT_TRUE(table.DefineColumnUVE("display", DataTableColumnTypeUVE::String));
    ASSERT_TRUE(table.AddRowUVE("pistol", {std::int64_t{25}, 0.3, true, std::string{"Pistol"}}));

    const DataTableSnapshotUVE snapshot = table.GetSnapshotUVE();
    EXPECT_EQ(snapshot.name, "weapons");
    EXPECT_EQ(snapshot.generation, 6U);
    ASSERT_EQ(snapshot.columns.size(), 4U);
    ASSERT_EQ(snapshot.rows.size(), 1U);
    ASSERT_EQ(snapshot.rows.front().values.size(), 4U);
    EXPECT_EQ(std::get<std::int64_t>(snapshot.rows.front().values[0]), 25);
    EXPECT_DOUBLE_EQ(std::get<double>(snapshot.rows.front().values[1]), 0.3);
    EXPECT_TRUE(std::get<bool>(snapshot.rows.front().values[2]));
    EXPECT_EQ(std::get<std::string>(snapshot.rows.front().values[3]), "Pistol");
    ASSERT_NE(table.FindRowUVE("pistol"), nullptr);
    EXPECT_EQ(table.FindRowUVE("missing"), nullptr);
}

TEST(DataTableUVE, InvalidMutationsAreFailureAtomic) {
    DataTableUVE table("stable");
    ASSERT_TRUE(table.DefineColumnUVE("value", DataTableColumnTypeUVE::Integer));
    ASSERT_TRUE(table.AddRowUVE("one", {std::int64_t{1}}));
    const DataTableSnapshotUVE before = table.GetSnapshotUVE();

    EXPECT_FALSE(table.DefineColumnUVE("value", DataTableColumnTypeUVE::String));
    EXPECT_FALSE(table.AddRowUVE("one", {std::int64_t{2}}));
    EXPECT_FALSE(table.AddRowUVE("two", {std::string{"wrong type"}}));
    EXPECT_FALSE(table.AddRowUVE("bad id!", {std::int64_t{3}}));

    const DataTableSnapshotUVE after = table.GetSnapshotUVE();
    EXPECT_EQ(after, before);
}

TEST(DataTableUVE, CsvImportSupportsQuotedFieldsAndCrLf) {
    DataTableUVE table("dialogue");
    ASSERT_TRUE(table.DefineColumnUVE("speaker", DataTableColumnTypeUVE::String));
    ASSERT_TRUE(table.DefineColumnUVE("line", DataTableColumnTypeUVE::String));
    ASSERT_TRUE(table.DefineColumnUVE("next", DataTableColumnTypeUVE::Integer));
    ASSERT_TRUE(table.DefineColumnUVE("enabled", DataTableColumnTypeUVE::Boolean));

    const std::string csv = "id,speaker,line,next,enabled\r\n"
                            "intro,NPC,\"Hello, \"\"traveler\"\"?\",2,true\r\n";
    ASSERT_TRUE(table.ImportCsvUVE(csv));
    const DataTableSnapshotUVE snapshot = table.GetSnapshotUVE();
    ASSERT_EQ(snapshot.rows.size(), 1U);
    EXPECT_EQ(std::get<std::string>(snapshot.rows[0].values[1]), "Hello, \"traveler\"?");
    EXPECT_EQ(std::get<std::int64_t>(snapshot.rows[0].values[2]), 2);
    EXPECT_TRUE(std::get<bool>(snapshot.rows[0].values[3]));
    EXPECT_TRUE(snapshot.diagnostics.empty());
}

TEST(DataTableUVE, CsvImportRejectsTypedErrorsWithoutReplacingRows) {
    DataTableUVE table("numbers");
    ASSERT_TRUE(table.DefineColumnUVE("value", DataTableColumnTypeUVE::Integer));
    ASSERT_TRUE(table.ImportCsvUVE("id,value\nvalid,7\n"));
    const DataTableSnapshotUVE before = table.GetSnapshotUVE();

    EXPECT_FALSE(table.ImportCsvUVE("id,value\ninvalid,not-a-number\n"));
    const DataTableSnapshotUVE after = table.GetSnapshotUVE();
    EXPECT_EQ(after.rows, before.rows);
    ASSERT_EQ(after.diagnostics.size(), 1U);
    EXPECT_EQ(after.diagnostics.front().code, DataTableDiagnosticCodeUVE::InvalidValue);
    EXPECT_EQ(after.diagnostics.front().line, 2U);
    EXPECT_EQ(after.diagnostics.front().column, 9U);
}

TEST(DataTableUVE, CsvImportRejectsHeaderAndDuplicateRowsDeterministically) {
    DataTableUVE table("items");
    ASSERT_TRUE(table.DefineColumnUVE("label", DataTableColumnTypeUVE::String));

    EXPECT_FALSE(table.ImportCsvUVE("id,wrong\na,one\n"));
    ASSERT_EQ(table.GetSnapshotUVE().diagnostics.size(), 1U);
    EXPECT_EQ(table.GetSnapshotUVE().diagnostics.front().code, DataTableDiagnosticCodeUVE::HeaderMismatch);

    EXPECT_FALSE(table.ImportCsvUVE("id,label\na,one\na,two\n"));
    const DataTableSnapshotUVE snapshot = table.GetSnapshotUVE();
    ASSERT_EQ(snapshot.diagnostics.size(), 1U);
    EXPECT_EQ(snapshot.diagnostics.front().code, DataTableDiagnosticCodeUVE::DuplicateRow);
    EXPECT_TRUE(snapshot.rows.empty());
}

TEST(DataTableUVE, TsvImportSupportsQuotedFieldsAndTabs) {
    DataTableUVE table("dialogue");
    ASSERT_TRUE(table.DefineColumnUVE("speaker", DataTableColumnTypeUVE::String));
    ASSERT_TRUE(table.DefineColumnUVE("line", DataTableColumnTypeUVE::String));
    ASSERT_TRUE(table.DefineColumnUVE("next", DataTableColumnTypeUVE::Integer));

    ASSERT_TRUE(table.ImportTsvUVE("id\tspeaker\tline\tnext\r\n"
                                   "intro\tNPC\t\"Hello\ttraveler\"\t2\r\n"));
    const DataTableSnapshotUVE snapshot = table.GetSnapshotUVE();
    ASSERT_EQ(snapshot.rows.size(), 1U);
    EXPECT_EQ(std::get<std::string>(snapshot.rows[0].values[1]), "Hello\ttraveler");
    EXPECT_EQ(std::get<std::int64_t>(snapshot.rows[0].values[2]), 2);
}

TEST(DataTableUVE, JsonImportUsesSchemaTypesAndIgnoresObjectMemberOrder) {
    DataTableUVE table("items");
    ASSERT_TRUE(table.DefineColumnUVE("label", DataTableColumnTypeUVE::String));
    ASSERT_TRUE(table.DefineColumnUVE("count", DataTableColumnTypeUVE::Integer));
    ASSERT_TRUE(table.DefineColumnUVE("enabled", DataTableColumnTypeUVE::Boolean));
    ASSERT_TRUE(table.ImportJsonUVE(R"([{"enabled":true,"id":"item_a","count":7,"label":"Potion"}])"));

    const DataTableSnapshotUVE snapshot = table.GetSnapshotUVE();
    ASSERT_EQ(snapshot.rows.size(), 1U);
    EXPECT_EQ(std::get<std::string>(snapshot.rows[0].values[0]), "Potion");
    EXPECT_EQ(std::get<std::int64_t>(snapshot.rows[0].values[1]), 7);
    EXPECT_TRUE(std::get<bool>(snapshot.rows[0].values[2]));
}

TEST(DataTableUVE, JsonImportRejectsWrongTypesAndDuplicateRowsAtomically) {
    DataTableUVE table("items");
    ASSERT_TRUE(table.DefineColumnUVE("count", DataTableColumnTypeUVE::Integer));
    ASSERT_TRUE(table.ImportJsonUVE(R"([{"id":"stable","count":3}])"));
    const DataTableSnapshotUVE before = table.GetSnapshotUVE();

    EXPECT_FALSE(table.ImportJsonUVE(R"([{"id":"new","count":"wrong"}])"));
    EXPECT_EQ(table.GetSnapshotUVE().rows, before.rows);
    ASSERT_EQ(table.GetSnapshotUVE().diagnostics.size(), 1U);
    EXPECT_EQ(table.GetSnapshotUVE().diagnostics.front().code, DataTableDiagnosticCodeUVE::InvalidValue);

    EXPECT_FALSE(table.ImportJsonUVE(R"([{"id":"duplicate","count":1},{"id":"duplicate","count":2}])"));
    EXPECT_TRUE(table.GetSnapshotUVE().rows == before.rows);
    ASSERT_EQ(table.GetSnapshotUVE().diagnostics.size(), 1U);
    EXPECT_EQ(table.GetSnapshotUVE().diagnostics.front().code, DataTableDiagnosticCodeUVE::DuplicateRow);
}

TEST(DataTableUVE, BoundsAndDiagnosticGenerationAreExplicit) {
    DataTableUVE table("bounded");
    for (std::size_t index = 0U; index < DataTableUVE::kMaximumColumnsUVE; ++index) {
        ASSERT_TRUE(table.DefineColumnUVE("column_" + std::to_string(index), DataTableColumnTypeUVE::String));
    }
    EXPECT_FALSE(table.DefineColumnUVE("overflow", DataTableColumnTypeUVE::String));
    const std::uint64_t before = table.GetSnapshotUVE().generation;
    EXPECT_FALSE(table.ImportCsvUVE("id,column_0\nrow,\"unterminated\n"));
    const DataTableSnapshotUVE after = table.GetSnapshotUVE();
    EXPECT_GT(after.generation, before);
    EXPECT_FALSE(after.diagnostics.empty());
    EXPECT_TRUE(after.rows.empty());
}

TEST(DataTableUVE, CsvImportRejectsInvalidBooleanAndNonFiniteNumber) {
    DataTableUVE table("flags");
    ASSERT_TRUE(table.DefineColumnUVE("enabled", DataTableColumnTypeUVE::Boolean));
    ASSERT_TRUE(table.DefineColumnUVE("weight", DataTableColumnTypeUVE::Number));

    EXPECT_FALSE(table.ImportCsvUVE("id,enabled,weight\na,TRUE,1\n"));
    ASSERT_EQ(table.GetSnapshotUVE().diagnostics.size(), 1U);
    EXPECT_EQ(table.GetSnapshotUVE().diagnostics.front().code, DataTableDiagnosticCodeUVE::InvalidValue);
    EXPECT_FALSE(table.ImportCsvUVE("id,enabled,weight\nb,true,nan\n"));
    ASSERT_EQ(table.GetSnapshotUVE().diagnostics.size(), 1U);
    EXPECT_EQ(table.GetSnapshotUVE().diagnostics.front().code, DataTableDiagnosticCodeUVE::InvalidValue);
}

TEST(DataTableUVE, AssetEnvelopeRoundTripIsDeterministicAndTyped) {
    DataTableUVE source("weapons");
    ASSERT_TRUE(source.DefineColumnUVE("damage", DataTableColumnTypeUVE::Integer));
    ASSERT_TRUE(source.DefineColumnUVE("weight", DataTableColumnTypeUVE::Number));
    ASSERT_TRUE(source.DefineColumnUVE("name", DataTableColumnTypeUVE::String));
    ASSERT_TRUE(source.AddRowUVE("pistol", {std::int64_t{25}, 1.5, std::string{"Pistol"}}));

    std::string first;
    std::string second;
    ASSERT_TRUE(DataTableAssetSerializerUVE::SerializeUVE(source, first));
    ASSERT_TRUE(DataTableAssetSerializerUVE::SerializeUVE(source, second));
    EXPECT_EQ(first, second);

    DataTableUVE restored("placeholder");
    ASSERT_TRUE(DataTableAssetSerializerUVE::DeserializeUVE(first, restored));
    EXPECT_EQ(restored.GetSnapshotUVE().columns, source.GetSnapshotUVE().columns);
    EXPECT_EQ(restored.GetSnapshotUVE().rows, source.GetSnapshotUVE().rows);
}

TEST(DataTableUVE, AssetEnvelopeRejectsInvalidInputWithoutReplacingDestination) {
    DataTableUVE table("stable");
    ASSERT_TRUE(table.DefineColumnUVE("value", DataTableColumnTypeUVE::Integer));
    ASSERT_TRUE(table.AddRowUVE("one", {std::int64_t{1}}));
    const DataTableSnapshotUVE before = table.GetSnapshotUVE();

    EXPECT_FALSE(DataTableAssetSerializerUVE::DeserializeUVE(
        R"({"format":"uve.data_table","version":2,"name":"new","columns":[],"rows":[]})", table));
    EXPECT_EQ(table.GetSnapshotUVE().columns, before.columns);
    EXPECT_EQ(table.GetSnapshotUVE().rows, before.rows);
}

TEST(DataTableUVE, CatalogSnapshotIsSortedCopiedAndGenerationCounted) {
    DataTableUVE zebra("zebra");
    ASSERT_TRUE(zebra.DefineColumnUVE("value", DataTableColumnTypeUVE::Integer));
    ASSERT_TRUE(zebra.AddRowUVE("row", {std::int64_t{1}}));
    DataTableUVE alpha("alpha");
    ASSERT_TRUE(alpha.DefineColumnUVE("value", DataTableColumnTypeUVE::Integer));

    DataTableCatalogUVE catalog;
    ASSERT_TRUE(catalog.UpsertUVE(zebra.GetSnapshotUVE()));
    ASSERT_TRUE(catalog.UpsertUVE(alpha.GetSnapshotUVE()));
    const DataTableCatalogSnapshotUVE snapshot = catalog.GetSnapshotUVE();
    ASSERT_EQ(snapshot.entries.size(), 2U);
    EXPECT_EQ(snapshot.entries[0].name, "alpha");
    EXPECT_EQ(snapshot.entries[1].name, "zebra");
    EXPECT_TRUE(snapshot.entries[1].valid);
    EXPECT_EQ(snapshot.entries[0].rowCount, 0U);
    EXPECT_TRUE(catalog.ContainsUVE("alpha"));

    EXPECT_FALSE(catalog.UpsertUVE(alpha.GetSnapshotUVE()));
    ASSERT_TRUE(catalog.RemoveUVE("alpha"));
    EXPECT_FALSE(catalog.ContainsUVE("alpha"));
    EXPECT_FALSE(catalog.RemoveUVE("alpha"));
}

} // namespace UVE::Asset::Tests
