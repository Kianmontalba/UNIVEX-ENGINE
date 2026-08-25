// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <filesystem>

#include <gtest/gtest.h>

#include "uve/core/engine_core_uve.h"
#include "uve/editor/editor_bridge_uve.h"
#include "uve/editor/editor_uve.h"
#include "uve/scene/components/prefab_instance_component_uve.h"
#include "uve/scene/components/transform_component_uve.h"

namespace UVE::Editor::Tests {
namespace {

[[nodiscard]] Core::EngineConfigUVE MakePrefabEditorTestConfigUVE() {
    Core::EngineConfigUVE config{};
    config.headlessUVE = true;
    config.logFilePath = "uve_prefab_editor_tests.log";
    config.settingsFilePath = "uve_prefab_editor_tests_settings.json";
    config.assetDatabaseFilePath = "uve_prefab_editor_tests_assets.json";
    config.saveDirectoryPath = "uve_prefab_editor_tests_saves";
    config.shaderCachePath = "uve_prefab_editor_tests_shader_cache";
    config.shaderSourceRealDirectoryUVE = "engine/render/shader/built_in";
    config.shaderSourceMountPrefixUVE = "shaders";
    return config;
}

TEST(PrefabMaturityEditorUVETest, SaveRefreshAndBridgeFactsUVE_AreRevisionAwareAndPlayGuarded) {
    const std::filesystem::path prefabPath = "uve_prefab_editor_maturity.uveprefab";
    std::filesystem::remove(prefabPath);

    Core::EngineCoreUVE engine(MakePrefabEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_prefab_editor_maturity.uvescene", 100U, &engine);
        editor.InitUVE();
        EditorBridgeUVE bridge(editor);
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();

        const Scene::EntityUVE source = editor.CreateDocumentEntityUVE(EditorEntityKindUVE::Empty);
        ASSERT_NE(source, Scene::kInvalidEntityUVE);
        ASSERT_TRUE(editor.SaveSelectedPrefabUVE(prefabPath));
        const Asset::AssetGuidUVE guid = entityManager.HasComponentUVE<Scene::PrefabInstanceComponentUVE>(source)
                                             ? Asset::kInvalidAssetGuidUVE
                                             : services.GetAssetDatabaseUVE().RegisterUVE(prefabPath);
        ASSERT_NE(guid, Asset::kInvalidAssetGuidUVE);

        const Scene::EntityUVE instance = services.GetPrefabSystemUVE().InstantiateUVE(
            entityManager, services.GetSceneGraphUVE(), services.GetAssetDatabaseUVE(), guid,
            Scene::kInvalidEntityUVE);
        ASSERT_NE(instance, Scene::kInvalidEntityUVE);
        editor.SelectEntityUVE(instance);

        const EditorBridgeSnapshotUVE currentSnapshot = bridge.GetSnapshotUVE();
        ASSERT_TRUE(currentSnapshot.inspector.prefab.has_value());
        EXPECT_EQ(currentSnapshot.inspector.prefab->status, EditorBridgePrefabRevisionStatusUVE::Current);
        EXPECT_FALSE(currentSnapshot.inspector.prefab->canRefresh);

        editor.SelectEntityUVE(source);
        Scene::TransformComponentUVE changed = entityManager.GetComponentUVE<Scene::TransformComponentUVE>(source);
        changed.localPosition.x = 3.0F;
        ASSERT_TRUE(editor.SetSelectedLocalTransformUVE(changed));
        ASSERT_TRUE(editor.SaveSelectedPrefabUVE(prefabPath));

        editor.SelectEntityUVE(instance);
        const EditorBridgeSnapshotUVE staleSnapshot = bridge.GetSnapshotUVE();
        ASSERT_TRUE(staleSnapshot.inspector.prefab.has_value());
        EXPECT_EQ(staleSnapshot.inspector.prefab->status, EditorBridgePrefabRevisionStatusUVE::Stale);
        EXPECT_TRUE(staleSnapshot.inspector.prefab->canRefresh);

        ASSERT_TRUE(editor.RefreshSelectedPrefabUVE());
        const Scene::EntityUVE refreshed = editor.GetSelectedEntityUVE();
        EXPECT_NE(refreshed, instance);
        EXPECT_FALSE(entityManager.IsAliveUVE(instance));
        ASSERT_TRUE(entityManager.IsAliveUVE(refreshed));
        EXPECT_FLOAT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(refreshed).localPosition.x, 3.0F);

        ASSERT_TRUE(editor.EnterPlayModeUVE());
        EXPECT_FALSE(editor.RefreshSelectedPrefabUVE());
        ASSERT_TRUE(editor.StopPlayModeUVE());

        editor.ShutdownUVE();
    }
    engine.Shutdown();
    std::filesystem::remove(prefabPath);
}

} // namespace
} // namespace UVE::Editor::Tests
