// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <thread>

#include <gtest/gtest.h>

#include "uve/asset/asset_database_uve.h"
#include "uve/asset/asset_importer_uve.h"
#include "uve/asset/asset_handle_uve.h"
#include "uve/asset/asset_manager_uve.h"
#include "uve/asset/data_table_importer_uve.h"
#include "uve/asset/data_table_pipeline_uve.h"
#include "uve/asset/data_table_uve.h"
#include "uve/events/event_system_uve.h"
#include "uve/threading/thread_pool_uve.h"

namespace UVE::Asset::Tests {
namespace {

[[nodiscard]] bool WaitForTerminalStateUVE(const AssetHandleUVE<DataTableUVE>& handle,
                                           const int maxIterations = 200000) {
    for (int iteration = 0; iteration < maxIterations; ++iteration) {
        if (handle.IsReadyUVE() || handle.HasFailedUVE()) {
            return true;
        }
        std::this_thread::yield();
    }
    return false;
}

TEST(DataTablePipelineUVE, BootstrapComposesImportAndTypedLoadServices) {
    const std::filesystem::path source = std::filesystem::temp_directory_path() / "uve_data_table_pipeline.csv";
    const std::filesystem::path destination =
        std::filesystem::temp_directory_path() / "uve_data_table_pipeline.uvetable";
    static_cast<void>(std::filesystem::remove(source));
    static_cast<void>(std::filesystem::remove(destination));
    struct CleanupUVE final {
        std::filesystem::path source;
        std::filesystem::path destination;
        ~CleanupUVE() {
            static_cast<void>(std::filesystem::remove(source));
            static_cast<void>(std::filesystem::remove(destination));
        }
    } cleanup{source, destination};

    {
        std::ofstream output(source, std::ios::binary | std::ios::trunc);
        ASSERT_TRUE(output.is_open());
        output << "id,damage\npistol,25\n";
    }

    AssetImporterUVE importer;
    Threading::ThreadPoolUVE threadPool(2);
    Events::EventSystemUVE eventSystem;
    AssetManagerUVE assetManager(threadPool, eventSystem);
    AssetDatabaseUVE assetDatabase;
    RegisterDataTablePipelineUVE(importer, assetManager);

    DataTableImportSettingsUVE settings;
    settings.tableName = "weapons";
    settings.columns = {DataTableColumnUVE{"damage", DataTableColumnTypeUVE::Integer}};
    const AssetGuidUVE guid = importer.ImportUVE(source, destination, assetDatabase, settings);
    ASSERT_NE(guid, kInvalidAssetGuidUVE);

    const AssetHandleUVE<DataTableUVE> handle = assetManager.LoadUVE<DataTableUVE>(guid, assetDatabase);
    ASSERT_TRUE(WaitForTerminalStateUVE(handle));
    ASSERT_TRUE(handle.IsReadyUVE());
    const DataTableUVE* const table = handle.TryGetUVE();
    ASSERT_NE(table, nullptr);
    const DataTableSnapshotUVE snapshot = table->GetSnapshotUVE();
    ASSERT_EQ(snapshot.name, "weapons");
    ASSERT_EQ(snapshot.rows.size(), 1U);
    ASSERT_EQ(snapshot.rows.front().values.size(), 1U);
    EXPECT_EQ(std::get<std::int64_t>(snapshot.rows.front().values.front()), 25);
}

} // namespace
} // namespace UVE::Asset::Tests
