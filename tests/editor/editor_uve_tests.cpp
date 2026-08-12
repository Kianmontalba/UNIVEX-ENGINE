//                                UniVex Engine
//
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.

#include <algorithm>
#include <filesystem>
#include <limits>
#include <string>

#include <gtest/gtest.h>

#include "uve/core/engine_core_uve.h"
#include "uve/editor/editor_uve.h"
#include "uve/scene/components/camera_component_uve.h"
#include "uve/scene/components/collider_component_uve.h"
#include "uve/scene/components/light_component_uve.h"
#include "uve/scene/components/name_component_uve.h"
#include "uve/scene/components/transform_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"

namespace UVE::Editor::Tests {
namespace {

[[nodiscard]] Core::EngineConfigUVE MakeEditorTestConfigUVE() {
    Core::EngineConfigUVE config{};
    config.headlessUVE = true;
    config.logFilePath = "uve_editor_tests.log";
    config.settingsFilePath = "uve_editor_tests_settings.json";
    config.assetDatabaseFilePath = "uve_editor_tests_assets.json";
    config.saveDirectoryPath = "uve_editor_tests_saves";
    config.shaderCachePath = "uve_editor_tests_shader_cache";
    config.shaderSourceRealDirectoryUVE = "engine/render/shader/built_in";
    config.shaderSourceMountPrefixUVE = "shaders";
    return config;
}

void AttachRootUVE(Core::EngineCoreUVE& engine, const Scene::EntityUVE entity,
                   const Scene::TransformComponentUVE& transform) {
    Core::EngineServicesUVE& services = engine.GetServicesUVE();
    services.GetSceneGraphUVE().AttachTransformUVE(services.GetEntityManagerUVE(), entity, transform);
}

TEST(EditorUVETest, InitUVE_CreatesCameraOutsideDocumentRootsAndSupportsHeadlessLifecycle) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_lifecycle.uvescene");
        editor.InitUVE();

        EXPECT_EQ(editor.GetStateUVE(), EditorStateUVE::Running);
        EXPECT_TRUE(engine.GetServicesUVE().GetEntityManagerUVE().IsAliveUVE(editor.GetViewportCameraUVE()));
        EXPECT_TRUE(editor.GetDocumentRootsUVE().empty());

        editor.ShutdownUVE();
        EXPECT_EQ(editor.GetStateUVE(), EditorStateUVE::Shutdown);
    }

    engine.Shutdown();
}

TEST(EditorUVETest, SelectionAndInspectorTransformEdit_ValidateLifetimeAndFiniteValues) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_selection.uvescene");
        editor.InitUVE();

        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        const Scene::EntityUVE root = services.GetEntityManagerUVE().CreateEntityUVE();
        AttachRootUVE(engine, root, Scene::TransformComponentUVE{});

        editor.SelectEntityUVE(root);
        EXPECT_EQ(editor.GetSelectedEntityUVE(), root);

        Scene::TransformComponentUVE edited{};
        edited.localPosition = Math::Vector3UVE{3.0F, -2.0F, 7.0F};
        edited.localScale = Math::Vector3UVE{2.0F, 3.0F, 4.0F};
        ASSERT_TRUE(editor.SetSelectedLocalTransformUVE(edited));
        EXPECT_TRUE(editor.IsSceneDirtyUVE());
        EXPECT_EQ(services.GetEntityManagerUVE().GetComponentUVE<Scene::TransformComponentUVE>(root).localPosition,
                  edited.localPosition);

        edited.localScale.x = std::numeric_limits<float>::infinity();
        EXPECT_FALSE(editor.SetSelectedLocalTransformUVE(edited));

        services.GetEntityManagerUVE().DestroyEntityUVE(root);
        editor.TickUVE();
        EXPECT_EQ(editor.GetSelectedEntityUVE(), Scene::kInvalidEntityUVE);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, CreateDocumentEntityUVE_CreatesSelectedDirtyRootArchetypes) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_create_entities.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();

        const Scene::EntityUVE empty = editor.CreateDocumentEntityUVE(EditorEntityKindUVE::Empty);
        ASSERT_TRUE(entityManager.IsAliveUVE(empty));
        EXPECT_TRUE(entityManager.HasComponentUVE<Scene::TransformComponentUVE>(empty));
        EXPECT_FALSE(entityManager.HasComponentUVE<Scene::CameraComponentUVE>(empty));
        EXPECT_FALSE(entityManager.HasComponentUVE<Scene::LightComponentUVE>(empty));
        EXPECT_FALSE(entityManager.HasComponentUVE<Scene::ColliderComponentUVE>(empty));
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::NameComponentUVE>(empty));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::NameComponentUVE>(empty).name, "Empty");
        EXPECT_EQ(editor.GetSelectedEntityUVE(), empty);
        EXPECT_TRUE(editor.IsSceneDirtyUVE());

        const Scene::EntityUVE camera = editor.CreateDocumentEntityUVE(EditorEntityKindUVE::Camera);
        ASSERT_TRUE(entityManager.IsAliveUVE(camera));
        EXPECT_TRUE(entityManager.HasComponentUVE<Scene::TransformComponentUVE>(camera));
        EXPECT_TRUE(entityManager.HasComponentUVE<Scene::CameraComponentUVE>(camera));
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::NameComponentUVE>(camera));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::NameComponentUVE>(camera).name, "Camera");
        EXPECT_EQ(editor.GetSelectedEntityUVE(), camera);
        EXPECT_TRUE(editor.IsSceneDirtyUVE());

        const Scene::EntityUVE directionalLight =
            editor.CreateDocumentEntityUVE(EditorEntityKindUVE::DirectionalLight);
        ASSERT_TRUE(entityManager.IsAliveUVE(directionalLight));
        EXPECT_TRUE(entityManager.HasComponentUVE<Scene::TransformComponentUVE>(directionalLight));
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::LightComponentUVE>(directionalLight));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::LightComponentUVE>(directionalLight).type,
                  Scene::LightTypeUVE::Directional);
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::NameComponentUVE>(directionalLight));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::NameComponentUVE>(directionalLight).name, "Directional Light");
        EXPECT_EQ(editor.GetSelectedEntityUVE(), directionalLight);
        EXPECT_TRUE(editor.IsSceneDirtyUVE());

        const Scene::EntityUVE collisionBox = editor.CreateDocumentEntityUVE(EditorEntityKindUVE::CollisionBox);
        ASSERT_TRUE(entityManager.IsAliveUVE(collisionBox));
        EXPECT_TRUE(entityManager.HasComponentUVE<Scene::TransformComponentUVE>(collisionBox));
        EXPECT_TRUE(entityManager.HasComponentUVE<Scene::ColliderComponentUVE>(collisionBox));
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::NameComponentUVE>(collisionBox));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::NameComponentUVE>(collisionBox).name, "Collision Box");
        EXPECT_EQ(editor.GetSelectedEntityUVE(), collisionBox);
        EXPECT_TRUE(editor.IsSceneDirtyUVE());

        const std::vector<Scene::EntityUVE> roots = editor.GetDocumentRootsUVE();
        ASSERT_EQ(roots.size(), 4U);
        EXPECT_NE(std::find(roots.begin(), roots.end(), empty), roots.end());
        EXPECT_NE(std::find(roots.begin(), roots.end(), camera), roots.end());
        EXPECT_NE(std::find(roots.begin(), roots.end(), directionalLight), roots.end());
        EXPECT_NE(std::find(roots.begin(), roots.end(), collisionBox), roots.end());
        EXPECT_EQ(std::find(roots.begin(), roots.end(), editor.GetViewportCameraUVE()), roots.end());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, CreateDocumentEntityUVE_RejectsInvalidKindsAndNonRunningStates) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_create_invalid.uvescene");
        EXPECT_EQ(editor.CreateDocumentEntityUVE(EditorEntityKindUVE::Empty), Scene::kInvalidEntityUVE);

        editor.InitUVE();
        EXPECT_EQ(editor.CreateDocumentEntityUVE(static_cast<EditorEntityKindUVE>(999)),
                  Scene::kInvalidEntityUVE);
        EXPECT_TRUE(editor.GetDocumentRootsUVE().empty());
        EXPECT_FALSE(editor.IsSceneDirtyUVE());

        editor.ShutdownUVE();
        EXPECT_EQ(editor.CreateDocumentEntityUVE(EditorEntityKindUVE::Empty), Scene::kInvalidEntityUVE);
    }

    engine.Shutdown();
}

TEST(EditorUVETest, CreateDocumentEntityUVE_AllocatesUniqueNamesAndKeepsEditorCameraOutOfNamespace) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_create_names.uvescene");
        editor.InitUVE();
        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();

        const Scene::EntityUVE firstCamera = editor.CreateDocumentEntityUVE(EditorEntityKindUVE::Camera);
        const Scene::EntityUVE secondCamera = editor.CreateDocumentEntityUVE(EditorEntityKindUVE::Camera);
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::NameComponentUVE>(firstCamera));
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::NameComponentUVE>(secondCamera));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::NameComponentUVE>(firstCamera).name, "Camera");
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::NameComponentUVE>(secondCamera).name, "Camera 2");
        EXPECT_FALSE(entityManager.HasComponentUVE<Scene::NameComponentUVE>(editor.GetViewportCameraUVE()));

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, SetSelectedEntityNameUVE_ValidatesInputAndMarksDocumentDirty) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_rename.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE root = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, root, Scene::TransformComponentUVE{});

        EXPECT_FALSE(editor.SetSelectedEntityNameUVE("Unselected"));
        editor.SelectEntityUVE(root);
        ASSERT_TRUE(editor.SetSelectedEntityNameUVE("Gameplay Root"));
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::NameComponentUVE>(root));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::NameComponentUVE>(root).name, "Gameplay Root");
        EXPECT_EQ(editor.GetSelectedEntityUVE(), root);
        EXPECT_TRUE(editor.IsSceneDirtyUVE());
        EXPECT_FALSE(editor.SetSelectedEntityNameUVE("Gameplay Root"));
        EXPECT_FALSE(editor.SetSelectedEntityNameUVE(""));
        EXPECT_FALSE(editor.SetSelectedEntityNameUVE("   \t"));
        EXPECT_FALSE(editor.SetSelectedEntityNameUVE(std::string(97U, 'n')));

        entityManager.DestroyEntityUVE(root);
        editor.TickUVE();
        EXPECT_FALSE(editor.SetSelectedEntityNameUVE("Destroyed"));
        editor.ShutdownUVE();
        EXPECT_FALSE(editor.SetSelectedEntityNameUVE("Shutdown"));
    }

    engine.Shutdown();
}

TEST(EditorUVETest, SaveThenLoadScene_RoundTripsDocumentRootsWithoutSerializingEditorCamera) {
    const std::filesystem::path scenePath = "uve_editor_tests_round_trip.uvescene";
    std::filesystem::remove(scenePath);
    std::filesystem::remove(scenePath.string() + ".editor-recovery");

    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), scenePath);
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();

        const Scene::EntityUVE root = services.GetEntityManagerUVE().CreateEntityUVE();
        Scene::TransformComponentUVE rootTransform{};
        rootTransform.localPosition = Math::Vector3UVE{1.0F, 2.0F, 3.0F};
        AttachRootUVE(engine, root, rootTransform);

        const Scene::EntityUVE child = services.GetEntityManagerUVE().CreateEntityUVE();
        Scene::TransformComponentUVE childTransform{};
        childTransform.localPosition = Math::Vector3UVE{4.0F, 5.0F, 6.0F};
        AttachRootUVE(engine, child, childTransform);
        services.GetSceneGraphUVE().SetParentUVE(services.GetEntityManagerUVE(), child, root);

        editor.SelectEntityUVE(root);
        ASSERT_TRUE(editor.SetSelectedLocalTransformUVE(rootTransform));
        ASSERT_TRUE(editor.SaveSceneUVE());
        EXPECT_FALSE(editor.IsSceneDirtyUVE());
        ASSERT_TRUE(std::filesystem::exists(scenePath));

        Scene::TransformComponentUVE modified = rootTransform;
        modified.localPosition = Math::Vector3UVE{9.0F, 9.0F, 9.0F};
        ASSERT_TRUE(editor.SetSelectedLocalTransformUVE(modified));
        ASSERT_TRUE(editor.LoadSceneUVE());

        const std::vector<Scene::EntityUVE> loadedRoots = editor.GetDocumentRootsUVE();
        ASSERT_EQ(loadedRoots.size(), 1U);
        EXPECT_NE(loadedRoots.front(), editor.GetViewportCameraUVE());
        const Scene::TransformComponentUVE& loadedTransform =
            services.GetEntityManagerUVE().GetComponentUVE<Scene::TransformComponentUVE>(loadedRoots.front());
        EXPECT_EQ(loadedTransform.localPosition, rootTransform.localPosition);
        EXPECT_EQ(services.GetSceneGraphUVE().GetChildrenUVE(services.GetEntityManagerUVE(), loadedRoots.front()).size(),
                  1U);
        EXPECT_FALSE(editor.IsSceneDirtyUVE());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
    std::filesystem::remove(scenePath);
    std::filesystem::remove(scenePath.string() + ".editor-recovery");
}

TEST(EditorUVETest, ViewportRayAndColliderPicking_SelectClosestDocumentEntityAndClearOnMiss) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_picking.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();

        const Scene::EntityUVE nearEntity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE nearTransform{};
        nearTransform.localPosition = Math::Vector3UVE{0.0F, 1.5F, 2.0F};
        AttachRootUVE(engine, nearEntity, nearTransform);
        entityManager.AddComponentUVE<Scene::ColliderComponentUVE>(nearEntity);

        const Scene::EntityUVE farEntity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE farTransform{};
        farTransform.localPosition = Math::Vector3UVE{0.0F, 1.5F, -2.0F};
        AttachRootUVE(engine, farEntity, farTransform);
        entityManager.AddComponentUVE<Scene::ColliderComponentUVE>(farEntity);
        services.GetSceneGraphUVE().UpdateUVE(entityManager);

        const EditorViewportRectUVE viewportRect{
            Math::Vector2UVE{0.0F, 0.0F}, Math::Vector2UVE{800.0F, 600.0F}};
        const std::optional<Math::RayUVE> centerRay =
            editor.MakeViewportRayUVE(viewportRect, Math::Vector2UVE{400.0F, 300.0F});
        ASSERT_TRUE(centerRay.has_value());
        EXPECT_NEAR(centerRay->origin.x, 0.0F, 0.0001F);
        EXPECT_NEAR(centerRay->origin.y, 1.5F, 0.0001F);
        EXPECT_NEAR(centerRay->origin.z, 6.0F, 0.0001F);
        EXPECT_NEAR(centerRay->direction.x, 0.0F, 0.0001F);
        EXPECT_NEAR(centerRay->direction.y, 0.0F, 0.0001F);
        EXPECT_NEAR(centerRay->direction.z, -1.0F, 0.0001F);
        EXPECT_FALSE(editor.MakeViewportRayUVE(viewportRect, Math::Vector2UVE{-1.0F, 300.0F}).has_value());

        EXPECT_TRUE(editor.PickViewportUVE(viewportRect, Math::Vector2UVE{400.0F, 300.0F}));
        EXPECT_EQ(editor.GetSelectedEntityUVE(), nearEntity);
        EXPECT_FALSE(editor.PickViewportUVE(viewportRect, Math::Vector2UVE{0.0F, 0.0F}));
        EXPECT_EQ(editor.GetSelectedEntityUVE(), Scene::kInvalidEntityUVE);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, TranslateSelectedAlongAxis_UpdatesLocalTransformAndConvertsParentScale) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_gizmo.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();

        const Scene::EntityUVE parent = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE parentTransform{};
        parentTransform.localScale = Math::Vector3UVE{2.0F, 3.0F, 4.0F};
        AttachRootUVE(engine, parent, parentTransform);

        const Scene::EntityUVE child = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE childTransform{};
        childTransform.localPosition = Math::Vector3UVE{1.0F, 2.0F, 3.0F};
        AttachRootUVE(engine, child, childTransform);
        services.GetSceneGraphUVE().SetParentUVE(entityManager, child, parent);
        services.GetSceneGraphUVE().UpdateUVE(entityManager);

        editor.SelectEntityUVE(child);
        EXPECT_TRUE(editor.TranslateSelectedAlongAxisUVE(EditorTranslateAxisUVE::X, 2.0F));
        const Scene::TransformComponentUVE& translated =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(child);
        EXPECT_NEAR(translated.localPosition.x, 2.0F, 0.0001F);
        EXPECT_NEAR(translated.localPosition.y, 2.0F, 0.0001F);
        EXPECT_NEAR(translated.localPosition.z, 3.0F, 0.0001F);
        EXPECT_TRUE(editor.IsSceneDirtyUVE());
        EXPECT_FALSE(editor.TranslateSelectedAlongAxisUVE(EditorTranslateAxisUVE::None, 1.0F));
        EXPECT_FALSE(editor.TranslateSelectedAlongAxisUVE(
            EditorTranslateAxisUVE::Y, std::numeric_limits<float>::infinity()));

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, LoadMissingScene_FailsWithoutDestroyingCurrentDocument) {
    const std::filesystem::path missingScenePath = "uve_editor_tests_missing.uvescene";
    std::filesystem::remove(missingScenePath);

    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), missingScenePath);
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        const Scene::EntityUVE root = services.GetEntityManagerUVE().CreateEntityUVE();
        AttachRootUVE(engine, root, Scene::TransformComponentUVE{});

        EXPECT_FALSE(editor.LoadSceneUVE());
        const std::vector<Scene::EntityUVE> roots = editor.GetDocumentRootsUVE();
        ASSERT_EQ(roots.size(), 1U);
        EXPECT_EQ(roots.front(), root);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

} // namespace
} // namespace UVE::Editor::Tests
