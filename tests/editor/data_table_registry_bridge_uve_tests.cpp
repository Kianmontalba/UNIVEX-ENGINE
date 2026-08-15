// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "uve/asset/data_table_registry_uve.h"
#include "uve/core/engine_core_uve.h"
#include "uve/editor/editor_bridge_uve.h"

namespace UVE::Editor::Tests {
namespace {

[[nodiscard]] Core::EngineConfigUVE MakeRegistryBridgeConfigUVE() {
    Core::EngineConfigUVE config{};
    config.headlessUVE = true;
    config.logFilePath = "uve_data_table_registry_bridge_tests.log";
    config.settingsFilePath = "uve_data_table_registry_bridge_tests_settings.json";
    config.assetDatabaseFilePath = "uve_data_table_registry_bridge_tests_assets.json";
    config.saveDirectoryPath = "uve_data_table_registry_bridge_tests_saves";
    config.shaderCachePath = "uve_data_table_registry_bridge_tests_shader_cache";
    config.shaderSourceRealDirectoryUVE = "engine/render/shader/built_in";
    config.shaderSourceMountPrefixUVE = "shaders";
    return config;
}

[[nodiscard]] Asset::DataTableUVE MakeTableUVE(std::string name, std::string identifier,
                                               std::int64_t value) {
    Asset::DataTableUVE table(std::move(name));
    static_cast<void>(table.DefineColumnUVE("value", Asset::DataTableColumnTypeUVE::Integer));
    static_cast<void>(table.AddRowUVE(std::move(identifier), {value}));
    return table;
}

TEST(EditorDataTableRegistryBridgeUVE, RegistryIsAuthoritativeForCatalogAndPreview) {
    Core::EngineCoreUVE engine(MakeRegistryBridgeConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_data_table_registry_authority.uvescene");
        editor.InitUVE();
        Asset::DataTableRegistryUVE registry;
        EditorBridgeUVE bridge(editor, &registry);

        const EditorBridgeSnapshotUVE initial = bridge.GetSnapshotUVE();
        ASSERT_TRUE(initial.dataTableCatalog.entries.empty());
        EXPECT_FALSE(initial.dataTablePreview.available);

        ASSERT_TRUE(registry.RegisterUVE(MakeTableUVE("zeta", "last", 9)));
        ASSERT_TRUE(registry.RegisterUVE(MakeTableUVE("alpha", "first", 1)));
        const EditorBridgeSnapshotUVE catalog = bridge.GetSnapshotUVE();
        ASSERT_GT(catalog.revision, initial.revision);
        ASSERT_EQ(catalog.dataTableCatalog.entries.size(), 2U);
        EXPECT_EQ(catalog.dataTableCatalog.entries[0].name, "alpha");
        EXPECT_EQ(catalog.dataTableCatalog.entries[1].name, "zeta");
        EXPECT_EQ(catalog.dataTableCatalog.generation, registry.GetCatalogSnapshotUVE().generation);

        Asset::DataTableUVE legacyTable = MakeTableUVE("legacy", "ignored", 77);
        Asset::DataTableCatalogUVE legacyCatalog;
        ASSERT_TRUE(legacyCatalog.UpsertUVE(legacyTable.GetSnapshotUVE()));
        bridge.SetDataTableCatalogSnapshotUVE(legacyCatalog.GetSnapshotUVE());
        const EditorBridgeSnapshotUVE registryStillWins = bridge.GetSnapshotUVE();
        ASSERT_EQ(registryStillWins.dataTableCatalog.entries.size(), 2U);
        EXPECT_EQ(registryStillWins.dataTableCatalog.entries.front().name, "alpha");

        ASSERT_TRUE(bridge.SetPreviewTableUVE("alpha"));
        const EditorBridgeSnapshotUVE preview = bridge.GetSnapshotUVE();
        ASSERT_TRUE(preview.dataTablePreview.available);
        EXPECT_EQ(preview.dataTablePreview.name, "alpha");
        ASSERT_EQ(preview.dataTablePreview.rows.size(), 1U);
        EXPECT_EQ(preview.dataTablePreview.rows.front().identifier, "first");
        EXPECT_EQ(preview.dataTablePreview.rows.front().values, (std::vector<std::string>{"1"}));

        Asset::DataTableSnapshotUVE legacyPreview = MakeTableUVE("legacy-preview", "wrong", 88).GetSnapshotUVE();
        bridge.SetDataTablePreviewSnapshotUVE(std::move(legacyPreview));
        const EditorBridgeSnapshotUVE previewStillWins = bridge.GetSnapshotUVE();
        EXPECT_EQ(previewStillWins.dataTablePreview.name, "alpha");

        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

TEST(EditorDataTableRegistryBridgeUVE, PreviewSelectionRejectsUnknownAndReflectsRemoval) {
    Core::EngineCoreUVE engine(MakeRegistryBridgeConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_data_table_registry_selection.uvescene");
        editor.InitUVE();
        Asset::DataTableRegistryUVE registry;
        EditorBridgeUVE bridge(editor, &registry);
        ASSERT_TRUE(registry.RegisterUVE(MakeTableUVE("weapons", "pistol", 25)));

        const EditorBridgeSnapshotUVE before = bridge.GetSnapshotUVE();
        EXPECT_FALSE(bridge.SetPreviewTableUVE("missing"));
        EXPECT_EQ(bridge.GetSnapshotUVE().revision, before.revision);
        EXPECT_FALSE(bridge.GetSnapshotUVE().dataTablePreview.available);

        ASSERT_TRUE(bridge.SetPreviewTableUVE("weapons"));
        const EditorBridgeSnapshotUVE selected = bridge.GetSnapshotUVE();
        ASSERT_TRUE(selected.dataTablePreview.available);
        EXPECT_EQ(selected.dataTablePreview.name, "weapons");

        ASSERT_TRUE(registry.RemoveUVE("weapons"));
        const EditorBridgeSnapshotUVE removed = bridge.GetSnapshotUVE();
        EXPECT_TRUE(removed.dataTableCatalog.entries.empty());
        EXPECT_FALSE(removed.dataTablePreview.available);
        EXPECT_EQ(removed.dataTablePreview.reason,
                  "The selected native data-table preview is no longer available.");

        ASSERT_TRUE(bridge.SetPreviewTableUVE(""));
        const EditorBridgeSnapshotUVE cleared = bridge.GetSnapshotUVE();
        EXPECT_FALSE(cleared.dataTablePreview.available);
        EXPECT_EQ(cleared.dataTablePreview.reason,
                  "No native data-table preview is selected in this bridge session.");

        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

TEST(EditorDataTableRegistryBridgeUVE, RegistryMutationAdvancesBridgeRevisionOnlyWhenObservable) {
    Core::EngineCoreUVE engine(MakeRegistryBridgeConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_data_table_registry_revision.uvescene");
        editor.InitUVE();
        Asset::DataTableRegistryUVE registry;
        EditorBridgeUVE bridge(editor, &registry);
        const EditorBridgeSnapshotUVE initial = bridge.GetSnapshotUVE();

        ASSERT_TRUE(registry.RegisterUVE(MakeTableUVE("weapons", "pistol", 25)));
        const EditorBridgeSnapshotUVE registered = bridge.GetSnapshotUVE();
        EXPECT_GT(registered.revision, initial.revision);
        EXPECT_EQ(bridge.GetSnapshotUVE().revision, registered.revision);

        ASSERT_TRUE(registry.RemoveUVE("weapons"));
        const EditorBridgeSnapshotUVE removed = bridge.GetSnapshotUVE();
        EXPECT_GT(removed.revision, registered.revision);

        EXPECT_FALSE(registry.RemoveUVE("weapons"));
        EXPECT_EQ(bridge.GetSnapshotUVE().revision, removed.revision);

        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

} // namespace
} // namespace UVE::Editor::Tests
