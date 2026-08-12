// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <algorithm>
#include <filesystem>
#include <limits>
#include <numbers>
#include <string>
#include <vector>

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

struct UnregisteredEditorLifecycleComponentUVE final {
    int value = 0;
};

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

TEST(EditorUVETest, EditorHistoryUVE_TransformUndoRedoRestoresSelectionAndDirtyState) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_history_transform.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE root = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, root, Scene::TransformComponentUVE{});
        editor.SelectEntityUVE(root);

        Scene::TransformComponentUVE moved{};
        moved.localPosition = Math::Vector3UVE{4.0F, -3.0F, 2.0F};
        ASSERT_TRUE(editor.SetSelectedLocalTransformUVE(moved));
        EXPECT_TRUE(editor.CanUndoUVE());
        EXPECT_FALSE(editor.CanRedoUVE());
        EXPECT_TRUE(editor.IsSceneDirtyUVE());

        ASSERT_TRUE(editor.UndoUVE());
        EXPECT_EQ(editor.GetSelectedEntityUVE(), root);
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(root).localPosition,
                  Math::Vector3UVE{});
        EXPECT_FALSE(editor.IsSceneDirtyUVE());
        EXPECT_FALSE(editor.CanUndoUVE());
        EXPECT_TRUE(editor.CanRedoUVE());

        ASSERT_TRUE(editor.RedoUVE());
        EXPECT_EQ(editor.GetSelectedEntityUVE(), root);
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(root).localPosition,
                  moved.localPosition);
        EXPECT_TRUE(editor.IsSceneDirtyUVE());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, EditorHistoryUVE_NameUndoRedoRestoresOptionalComponentState) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_history_name.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE root = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, root, Scene::TransformComponentUVE{});
        editor.SelectEntityUVE(root);

        ASSERT_TRUE(editor.SetSelectedEntityNameUVE("Level Root"));
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::NameComponentUVE>(root));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::NameComponentUVE>(root).name, "Level Root");
        ASSERT_TRUE(editor.UndoUVE());
        EXPECT_FALSE(entityManager.HasComponentUVE<Scene::NameComponentUVE>(root));
        EXPECT_FALSE(editor.IsSceneDirtyUVE());
        ASSERT_TRUE(editor.RedoUVE());
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::NameComponentUVE>(root));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::NameComponentUVE>(root).name, "Level Root");
        EXPECT_TRUE(editor.IsSceneDirtyUVE());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, EditorHistoryUVE_CreationUndoRedoRecreatesArchetypeAndName) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_history_create.uvescene");
        editor.InitUVE();
        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();

        const Scene::EntityUVE created = editor.CreateDocumentEntityUVE(EditorEntityKindUVE::CollisionBox);
        ASSERT_TRUE(entityManager.IsAliveUVE(created));
        ASSERT_TRUE(editor.CanUndoUVE());
        ASSERT_TRUE(editor.UndoUVE());
        EXPECT_FALSE(entityManager.IsAliveUVE(created));
        EXPECT_TRUE(editor.GetDocumentRootsUVE().empty());
        EXPECT_EQ(editor.GetSelectedEntityUVE(), Scene::kInvalidEntityUVE);
        EXPECT_FALSE(editor.IsSceneDirtyUVE());

        ASSERT_TRUE(editor.RedoUVE());
        const Scene::EntityUVE recreated = editor.GetSelectedEntityUVE();
        ASSERT_TRUE(entityManager.IsAliveUVE(recreated));
        EXPECT_TRUE(entityManager.HasComponentUVE<Scene::TransformComponentUVE>(recreated));
        EXPECT_TRUE(entityManager.HasComponentUVE<Scene::ColliderComponentUVE>(recreated));
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::NameComponentUVE>(recreated));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::NameComponentUVE>(recreated).name, "Collision Box");
        EXPECT_TRUE(editor.IsSceneDirtyUVE());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, EditorHistoryUVE_NewMutationClearsRedoAndCapacityDiscardsOldestCommand) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_history_capacity.uvescene", 1U);
        editor.InitUVE();
        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
        const Scene::EntityUVE root = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, root, Scene::TransformComponentUVE{});
        editor.SelectEntityUVE(root);

        Scene::TransformComponentUVE first{};
        first.localPosition = Math::Vector3UVE{1.0F, 0.0F, 0.0F};
        ASSERT_TRUE(editor.SetSelectedLocalTransformUVE(first));
        Scene::TransformComponentUVE second = first;
        second.localPosition = Math::Vector3UVE{2.0F, 0.0F, 0.0F};
        ASSERT_TRUE(editor.SetSelectedLocalTransformUVE(second));
        ASSERT_TRUE(editor.UndoUVE());
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(root).localPosition,
                  first.localPosition);
        EXPECT_FALSE(editor.CanUndoUVE());
        EXPECT_TRUE(editor.CanRedoUVE());

        Scene::TransformComponentUVE third = first;
        third.localPosition = Math::Vector3UVE{3.0F, 0.0F, 0.0F};
        ASSERT_TRUE(editor.SetSelectedLocalTransformUVE(third));
        EXPECT_FALSE(editor.CanRedoUVE());
        EXPECT_FALSE(editor.SetSelectedLocalTransformUVE(third));

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, EditorHistoryUVE_StaleTargetsAndNonRunningStateFailWithoutMutation) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_history_stale.uvescene");
        EXPECT_FALSE(editor.UndoUVE());
        EXPECT_FALSE(editor.RedoUVE());
        editor.InitUVE();
        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
        const Scene::EntityUVE root = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, root, Scene::TransformComponentUVE{});
        editor.SelectEntityUVE(root);

        Scene::TransformComponentUVE moved{};
        moved.localPosition = Math::Vector3UVE{1.0F, 2.0F, 3.0F};
        ASSERT_TRUE(editor.SetSelectedLocalTransformUVE(moved));
        entityManager.DestroyEntityUVE(root);
        editor.TickUVE();
        EXPECT_FALSE(editor.UndoUVE());
        EXPECT_FALSE(editor.CanUndoUVE());
        EXPECT_FALSE(editor.CanRedoUVE());

        editor.ShutdownUVE();
        EXPECT_FALSE(editor.UndoUVE());
        EXPECT_FALSE(editor.RedoUVE());
    }

    engine.Shutdown();
}

TEST(EditorUVETest, DuplicateSelectedEntityUVE_RootCreatesNamedSiblingWithCopiedComponents) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_duplicate_root.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE source = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE sourceTransform{};
        sourceTransform.localPosition = Math::Vector3UVE{2.0F, 4.0F, 6.0F};
        AttachRootUVE(engine, source, sourceTransform);
        entityManager.AddComponentUVE<Scene::ColliderComponentUVE>(source);
        entityManager.AddComponentUVE<Scene::NameComponentUVE>(source, Scene::NameComponentUVE{"Lamp"});
        editor.SelectEntityUVE(source);

        const Scene::EntityUVE duplicate = editor.DuplicateSelectedEntityUVE();
        ASSERT_TRUE(entityManager.IsAliveUVE(duplicate));
        EXPECT_NE(duplicate, source);
        EXPECT_EQ(editor.GetSelectedEntityUVE(), duplicate);
        EXPECT_TRUE(editor.IsSceneDirtyUVE());
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::NameComponentUVE>(duplicate).name, "Lamp 2");
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(duplicate).localPosition,
                  sourceTransform.localPosition);
        EXPECT_TRUE(entityManager.HasComponentUVE<Scene::ColliderComponentUVE>(duplicate));

        const std::vector<Scene::EntityUVE> roots = editor.GetDocumentRootsUVE();
        ASSERT_EQ(roots.size(), 2U);
        EXPECT_NE(std::find(roots.begin(), roots.end(), source), roots.end());
        EXPECT_NE(std::find(roots.begin(), roots.end(), duplicate), roots.end());
        EXPECT_EQ(std::find(roots.begin(), roots.end(), editor.GetViewportCameraUVE()), roots.end());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, DuplicateSelectedEntityUVE_ChildRestoresAsSiblingUnderSameParent) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_duplicate_child.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE parent = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, parent, Scene::TransformComponentUVE{});
        const Scene::EntityUVE child = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, child, Scene::TransformComponentUVE{});
        services.GetSceneGraphUVE().SetParentUVE(entityManager, child, parent);
        entityManager.AddComponentUVE<Scene::NameComponentUVE>(child, Scene::NameComponentUVE{"Child"});
        editor.SelectEntityUVE(child);

        const Scene::EntityUVE duplicate = editor.DuplicateSelectedEntityUVE();
        ASSERT_TRUE(entityManager.IsAliveUVE(duplicate));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::NameComponentUVE>(duplicate).name, "Child 2");
        const std::vector<Scene::EntityUVE> children =
            services.GetSceneGraphUVE().GetChildrenUVE(entityManager, parent);
        ASSERT_EQ(children.size(), 2U);
        EXPECT_NE(std::find(children.begin(), children.end(), child), children.end());
        EXPECT_NE(std::find(children.begin(), children.end(), duplicate), children.end());
        EXPECT_EQ(editor.GetSelectedEntityUVE(), duplicate);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, DeleteSelectedEntityUVE_DeletesSubtreeAndSelectsLiveParent) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_delete_subtree.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE parent = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, parent, Scene::TransformComponentUVE{});
        const Scene::EntityUVE child = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, child, Scene::TransformComponentUVE{});
        services.GetSceneGraphUVE().SetParentUVE(entityManager, child, parent);
        const Scene::EntityUVE grandchild = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, grandchild, Scene::TransformComponentUVE{});
        services.GetSceneGraphUVE().SetParentUVE(entityManager, grandchild, child);
        editor.SelectEntityUVE(child);

        ASSERT_TRUE(editor.DeleteSelectedEntityUVE());
        EXPECT_FALSE(entityManager.IsAliveUVE(child));
        EXPECT_FALSE(entityManager.IsAliveUVE(grandchild));
        EXPECT_TRUE(entityManager.IsAliveUVE(parent));
        EXPECT_EQ(editor.GetSelectedEntityUVE(), parent);
        EXPECT_TRUE(editor.IsSceneDirtyUVE());
        EXPECT_TRUE(editor.CanUndoUVE());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, DeleteSelectedEntityUVE_RootClearsSelection) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_delete_root.uvescene");
        editor.InitUVE();
        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
        const Scene::EntityUVE root = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, root, Scene::TransformComponentUVE{});
        editor.SelectEntityUVE(root);

        ASSERT_TRUE(editor.DeleteSelectedEntityUVE());
        EXPECT_FALSE(entityManager.IsAliveUVE(root));
        EXPECT_EQ(editor.GetSelectedEntityUVE(), Scene::kInvalidEntityUVE);
        EXPECT_TRUE(editor.IsSceneDirtyUVE());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, EditorHistoryUVE_DuplicateUndoRedoUsesFreshHandlesAndRestoresDirtySelection) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_duplicate_history.uvescene");
        editor.InitUVE();
        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
        const Scene::EntityUVE source = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, source, Scene::TransformComponentUVE{});
        entityManager.AddComponentUVE<Scene::NameComponentUVE>(source, Scene::NameComponentUVE{"Actor"});
        editor.SelectEntityUVE(source);

        const Scene::EntityUVE duplicate = editor.DuplicateSelectedEntityUVE();
        ASSERT_TRUE(entityManager.IsAliveUVE(duplicate));
        ASSERT_TRUE(editor.UndoUVE());
        EXPECT_FALSE(entityManager.IsAliveUVE(duplicate));
        EXPECT_EQ(editor.GetSelectedEntityUVE(), source);
        EXPECT_FALSE(editor.IsSceneDirtyUVE());
        ASSERT_TRUE(editor.RedoUVE());
        const Scene::EntityUVE recreated = editor.GetSelectedEntityUVE();
        EXPECT_TRUE(entityManager.IsAliveUVE(recreated));
        EXPECT_NE(recreated, duplicate);
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::NameComponentUVE>(recreated).name, "Actor 2");
        EXPECT_TRUE(editor.IsSceneDirtyUVE());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, EditorHistoryUVE_DeleteUndoRedoRestoresSubtreeUnderOriginalParentWithFreshHandles) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_delete_history.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE parent = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, parent, Scene::TransformComponentUVE{});
        const Scene::EntityUVE child = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, child, Scene::TransformComponentUVE{});
        services.GetSceneGraphUVE().SetParentUVE(entityManager, child, parent);
        entityManager.AddComponentUVE<Scene::NameComponentUVE>(child, Scene::NameComponentUVE{"Deleted Child"});
        editor.SelectEntityUVE(child);

        ASSERT_TRUE(editor.DeleteSelectedEntityUVE());
        EXPECT_FALSE(entityManager.IsAliveUVE(child));
        EXPECT_EQ(editor.GetSelectedEntityUVE(), parent);
        ASSERT_TRUE(editor.UndoUVE());
        const Scene::EntityUVE restored = editor.GetSelectedEntityUVE();
        EXPECT_TRUE(entityManager.IsAliveUVE(restored));
        EXPECT_NE(restored, child);
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::NameComponentUVE>(restored).name, "Deleted Child");
        const std::vector<Scene::EntityUVE> children = services.GetSceneGraphUVE().GetChildrenUVE(entityManager, parent);
        EXPECT_NE(std::find(children.begin(), children.end(), restored), children.end());
        EXPECT_FALSE(editor.IsSceneDirtyUVE());
        ASSERT_TRUE(editor.RedoUVE());
        EXPECT_FALSE(entityManager.IsAliveUVE(restored));
        EXPECT_EQ(editor.GetSelectedEntityUVE(), parent);
        EXPECT_TRUE(editor.IsSceneDirtyUVE());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, EditorHistoryUVE_DeleteUndoRejectsStaleParentAndClearsTimeline) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_delete_stale_parent.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE parent = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, parent, Scene::TransformComponentUVE{});
        const Scene::EntityUVE child = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, child, Scene::TransformComponentUVE{});
        services.GetSceneGraphUVE().SetParentUVE(entityManager, child, parent);
        editor.SelectEntityUVE(child);

        ASSERT_TRUE(editor.DeleteSelectedEntityUVE());
        entityManager.DestroyEntityUVE(parent);
        EXPECT_FALSE(editor.UndoUVE());
        EXPECT_FALSE(editor.CanUndoUVE());
        EXPECT_FALSE(editor.CanRedoUVE());
        EXPECT_EQ(editor.GetDocumentRootsUVE().size(), 0U);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, EditorHistoryUVE_NewMutationAfterDuplicateUndoInvalidatesRedo) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_duplicate_redo_invalidation.uvescene");
        editor.InitUVE();
        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
        const Scene::EntityUVE source = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, source, Scene::TransformComponentUVE{});
        entityManager.AddComponentUVE<Scene::NameComponentUVE>(source, Scene::NameComponentUVE{"Source"});
        editor.SelectEntityUVE(source);

        ASSERT_NE(editor.DuplicateSelectedEntityUVE(), Scene::kInvalidEntityUVE);
        ASSERT_TRUE(editor.UndoUVE());
        ASSERT_TRUE(editor.CanRedoUVE());
        ASSERT_TRUE(editor.SetSelectedEntityNameUVE("Source Revised"));
        EXPECT_FALSE(editor.CanRedoUVE());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, EntityLifecycleUVE_RejectsUnselectedCameraStaleNonRunningAndUnsupportedCapture) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_lifecycle_safety.uvescene");
        EXPECT_EQ(editor.DuplicateSelectedEntityUVE(), Scene::kInvalidEntityUVE);
        EXPECT_FALSE(editor.DeleteSelectedEntityUVE());
        editor.InitUVE();
        EXPECT_EQ(editor.DuplicateSelectedEntityUVE(), Scene::kInvalidEntityUVE);
        EXPECT_FALSE(editor.DeleteSelectedEntityUVE());
        editor.SelectEntityUVE(editor.GetViewportCameraUVE());
        EXPECT_EQ(editor.GetSelectedEntityUVE(), Scene::kInvalidEntityUVE);
        EXPECT_EQ(editor.DuplicateSelectedEntityUVE(), Scene::kInvalidEntityUVE);
        EXPECT_FALSE(editor.DeleteSelectedEntityUVE());

        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
        const Scene::EntityUVE unsupported = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, unsupported, Scene::TransformComponentUVE{});
        entityManager.AddComponentUVE<UnregisteredEditorLifecycleComponentUVE>(
            unsupported, UnregisteredEditorLifecycleComponentUVE{7});
        editor.SelectEntityUVE(unsupported);
        const std::size_t entityCountBefore = entityManager.GetEntityCountUVE();
        EXPECT_EQ(editor.DuplicateSelectedEntityUVE(), Scene::kInvalidEntityUVE);
        EXPECT_TRUE(entityManager.IsAliveUVE(unsupported));
        EXPECT_EQ(entityManager.GetEntityCountUVE(), entityCountBefore);
        EXPECT_FALSE(editor.DeleteSelectedEntityUVE());
        EXPECT_TRUE(entityManager.IsAliveUVE(unsupported));

        entityManager.DestroyEntityUVE(unsupported);
        editor.TickUVE();
        EXPECT_EQ(editor.DuplicateSelectedEntityUVE(), Scene::kInvalidEntityUVE);
        EXPECT_FALSE(editor.DeleteSelectedEntityUVE());
        editor.ShutdownUVE();
        EXPECT_EQ(editor.DuplicateSelectedEntityUVE(), Scene::kInvalidEntityUVE);
        EXPECT_FALSE(editor.DeleteSelectedEntityUVE());
    }

    engine.Shutdown();
}

TEST(EditorUVETest, ReparentSelectedEntityUVE_RootMovesBelowTargetAndPreservesLocalTransform) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_reparent_root.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE target = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, target, Scene::TransformComponentUVE{});
        const Scene::EntityUVE moved = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE localTransform{};
        localTransform.localPosition = Math::Vector3UVE{2.0F, -3.0F, 7.0F};
        AttachRootUVE(engine, moved, localTransform);
        editor.SelectEntityUVE(moved);

        ASSERT_TRUE(editor.ReparentSelectedEntityUVE(target));
        const std::vector<Scene::EntityUVE> targetChildren =
            services.GetSceneGraphUVE().GetChildrenUVE(entityManager, target);
        EXPECT_NE(std::find(targetChildren.begin(), targetChildren.end(), moved), targetChildren.end());
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(moved).localPosition,
                  localTransform.localPosition);
        EXPECT_EQ(editor.GetSelectedEntityUVE(), moved);
        EXPECT_TRUE(editor.IsSceneDirtyUVE());
        EXPECT_TRUE(editor.CanUndoUVE());
        const std::vector<Scene::EntityUVE> roots = editor.GetDocumentRootsUVE();
        ASSERT_EQ(roots.size(), 1U);
        EXPECT_EQ(roots.front(), target);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, ReparentSelectedEntityUVE_ChildCanReturnToRootWithoutDetachingDescendants) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_reparent_root_detach.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE parent = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, parent, Scene::TransformComponentUVE{});
        const Scene::EntityUVE child = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, child, Scene::TransformComponentUVE{});
        services.GetSceneGraphUVE().SetParentUVE(entityManager, child, parent);
        const Scene::EntityUVE grandchild = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, grandchild, Scene::TransformComponentUVE{});
        services.GetSceneGraphUVE().SetParentUVE(entityManager, grandchild, child);
        editor.SelectEntityUVE(child);

        ASSERT_TRUE(editor.ReparentSelectedEntityUVE(Scene::kInvalidEntityUVE));
        const std::vector<Scene::EntityUVE> roots = editor.GetDocumentRootsUVE();
        ASSERT_EQ(roots.size(), 2U);
        EXPECT_NE(std::find(roots.begin(), roots.end(), parent), roots.end());
        EXPECT_NE(std::find(roots.begin(), roots.end(), child), roots.end());
        EXPECT_TRUE(services.GetSceneGraphUVE().GetChildrenUVE(entityManager, parent).empty());
        const std::vector<Scene::EntityUVE> childChildren =
            services.GetSceneGraphUVE().GetChildrenUVE(entityManager, child);
        EXPECT_NE(std::find(childChildren.begin(), childChildren.end(), grandchild), childChildren.end());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, EditorHistoryUVE_ReparentUndoRedoRestoresParentsSelectionAndDirtyState) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_reparent_history.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE oldParent = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, oldParent, Scene::TransformComponentUVE{});
        const Scene::EntityUVE newParent = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, newParent, Scene::TransformComponentUVE{});
        const Scene::EntityUVE moved = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, moved, Scene::TransformComponentUVE{});
        services.GetSceneGraphUVE().SetParentUVE(entityManager, moved, oldParent);
        entityManager.AddComponentUVE<Scene::NameComponentUVE>(moved, Scene::NameComponentUVE{"Moved"});
        editor.SelectEntityUVE(moved);

        ASSERT_TRUE(editor.ReparentSelectedEntityUVE(newParent));
        const std::vector<Scene::EntityUVE> childrenAfterReparent =
            services.GetSceneGraphUVE().GetChildrenUVE(entityManager, newParent);
        EXPECT_NE(std::find(childrenAfterReparent.begin(), childrenAfterReparent.end(), moved),
                  childrenAfterReparent.end());
        ASSERT_TRUE(editor.UndoUVE());
        const std::vector<Scene::EntityUVE> childrenAfterUndo =
            services.GetSceneGraphUVE().GetChildrenUVE(entityManager, oldParent);
        EXPECT_NE(std::find(childrenAfterUndo.begin(), childrenAfterUndo.end(), moved), childrenAfterUndo.end());
        EXPECT_EQ(editor.GetSelectedEntityUVE(), moved);
        EXPECT_FALSE(editor.IsSceneDirtyUVE());
        ASSERT_TRUE(editor.RedoUVE());
        const std::vector<Scene::EntityUVE> childrenAfterRedo =
            services.GetSceneGraphUVE().GetChildrenUVE(entityManager, newParent);
        EXPECT_NE(std::find(childrenAfterRedo.begin(), childrenAfterRedo.end(), moved), childrenAfterRedo.end());
        EXPECT_TRUE(editor.IsSceneDirtyUVE());
        ASSERT_TRUE(editor.UndoUVE());
        ASSERT_TRUE(editor.SetSelectedEntityNameUVE("Moved Again"));
        EXPECT_FALSE(editor.CanRedoUVE());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, ReparentSelectedEntityUVE_RejectsCyclesNoOpCameraStaleAndNonRunningStates) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_reparent_safety.uvescene");
        EXPECT_FALSE(editor.ReparentSelectedEntityUVE(Scene::kInvalidEntityUVE));
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE root = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, root, Scene::TransformComponentUVE{});
        const Scene::EntityUVE child = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, child, Scene::TransformComponentUVE{});
        services.GetSceneGraphUVE().SetParentUVE(entityManager, child, root);
        editor.SelectEntityUVE(root);
        EXPECT_FALSE(editor.ReparentSelectedEntityUVE(root));
        EXPECT_FALSE(editor.ReparentSelectedEntityUVE(child));
        EXPECT_FALSE(editor.ReparentSelectedEntityUVE(Scene::kInvalidEntityUVE));
        editor.SelectEntityUVE(child);
        EXPECT_FALSE(editor.ReparentSelectedEntityUVE(root));
        editor.SelectEntityUVE(editor.GetViewportCameraUVE());
        EXPECT_FALSE(editor.ReparentSelectedEntityUVE(root));

        const Scene::EntityUVE staleTarget = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, staleTarget, Scene::TransformComponentUVE{});
        entityManager.DestroyEntityUVE(staleTarget);
        editor.SelectEntityUVE(root);
        EXPECT_FALSE(editor.ReparentSelectedEntityUVE(staleTarget));

        const Scene::EntityUVE malformed = entityManager.CreateEntityUVE();
        editor.SelectEntityUVE(malformed);
        EXPECT_FALSE(editor.ReparentSelectedEntityUVE(Scene::kInvalidEntityUVE));
        editor.ShutdownUVE();
        EXPECT_FALSE(editor.ReparentSelectedEntityUVE(Scene::kInvalidEntityUVE));
    }

    engine.Shutdown();
}

TEST(EditorUVETest, EditorHistoryUVE_ReparentUndoRejectsStalePriorParentAndClearsTimeline) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_reparent_stale_parent.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE oldParent = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, oldParent, Scene::TransformComponentUVE{});
        const Scene::EntityUVE newParent = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, newParent, Scene::TransformComponentUVE{});
        const Scene::EntityUVE moved = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, moved, Scene::TransformComponentUVE{});
        services.GetSceneGraphUVE().SetParentUVE(entityManager, moved, oldParent);
        editor.SelectEntityUVE(moved);

        ASSERT_TRUE(editor.ReparentSelectedEntityUVE(newParent));
        entityManager.DestroyEntityUVE(oldParent);
        EXPECT_FALSE(editor.UndoUVE());
        EXPECT_FALSE(editor.CanUndoUVE());
        EXPECT_FALSE(editor.CanRedoUVE());

        editor.ShutdownUVE();
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
        AttachRootUVE(engine, root, Scene::TransformComponentUVE{});

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
        EXPECT_FALSE(editor.CanUndoUVE());
        EXPECT_FALSE(editor.CanRedoUVE());

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

TEST(EditorUVETest, ViewportNavigationUVE_FocusOrbitPanZoomPreserveDocumentAndHistoryState) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_navigation.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE root = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE rootTransform{};
        rootTransform.localPosition = Math::Vector3UVE{3.0F, 2.0F, -4.0F};
        AttachRootUVE(engine, root, rootTransform);
        services.GetSceneGraphUVE().UpdateUVE(entityManager);
        editor.SelectEntityUVE(root);

        const Scene::TransformComponentUVE documentBefore =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(root);
        const Scene::TransformComponentUVE cameraBefore =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(editor.GetViewportCameraUVE());
        ASSERT_TRUE(editor.FocusSelectedEntityUVE());
        EXPECT_EQ(editor.GetViewportFocusPointUVE(), rootTransform.localPosition);
        EXPECT_EQ(editor.GetSelectedEntityUVE(), root);
        EXPECT_FALSE(editor.IsSceneDirtyUVE());
        EXPECT_FALSE(editor.CanUndoUVE());
        EXPECT_FALSE(editor.CanRedoUVE());

        ASSERT_TRUE(editor.OrbitViewportUVE(0.5F, 0.25F));
        const Scene::TransformComponentUVE cameraAfterOrbit =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(editor.GetViewportCameraUVE());
        EXPECT_NE(cameraAfterOrbit.localPosition, cameraBefore.localPosition);
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(root).localPosition,
                  documentBefore.localPosition);
        EXPECT_FALSE(editor.IsSceneDirtyUVE());
        EXPECT_FALSE(editor.CanUndoUVE());

        const EditorViewportRectUVE viewportRect{
            Math::Vector2UVE{0.0F, 0.0F}, Math::Vector2UVE{800.0F, 600.0F}};
        const Math::Vector3UVE focusBeforePan = editor.GetViewportFocusPointUVE();
        ASSERT_TRUE(editor.PanViewportUVE(Math::Vector2UVE{120.0F, -40.0F}, viewportRect));
        EXPECT_NE(editor.GetViewportFocusPointUVE(), focusBeforePan);
        const float distanceBeforeZoom = editor.GetViewportDistanceUVE();
        ASSERT_TRUE(editor.ZoomViewportUVE(2.0F));
        EXPECT_LT(editor.GetViewportDistanceUVE(), distanceBeforeZoom);
        EXPECT_FALSE(editor.IsSceneDirtyUVE());
        EXPECT_FALSE(editor.CanUndoUVE());
        EXPECT_EQ(editor.GetViewportNavigationModeUVE(), EditorViewportNavigationModeUVE::None);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, ViewportNavigationUVE_ValidatesSelectionInputAndDistanceLimits) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_navigation_invalid.uvescene");
        EXPECT_FALSE(editor.FocusSelectedEntityUVE());
        EXPECT_FALSE(editor.OrbitViewportUVE(0.0F, 0.0F));
        editor.InitUVE();
        const EditorViewportRectUVE validViewport{
            Math::Vector2UVE{0.0F, 0.0F}, Math::Vector2UVE{800.0F, 600.0F}};
        EXPECT_FALSE(editor.FocusSelectedEntityUVE());
        EXPECT_FALSE(editor.OrbitViewportUVE(std::numeric_limits<float>::infinity(), 0.0F));
        EXPECT_FALSE(editor.PanViewportUVE(Math::Vector2UVE{0.0F, 1.0F},
                                           EditorViewportRectUVE{Math::Vector2UVE{}, Math::Vector2UVE{1.0F, 1.0F}}));
        EXPECT_FALSE(editor.ZoomViewportUVE(std::numeric_limits<float>::quiet_NaN()));

        ASSERT_TRUE(editor.ZoomViewportUVE(100.0F));
        EXPECT_GE(editor.GetViewportDistanceUVE(), 0.5F);
        EXPECT_LE(editor.GetViewportDistanceUVE(), 500.0F);
        EXPECT_FALSE(editor.ZoomViewportUVE(100.0F));
        ASSERT_TRUE(editor.ZoomViewportUVE(-100.0F));
        EXPECT_GE(editor.GetViewportDistanceUVE(), 0.5F);
        EXPECT_LE(editor.GetViewportDistanceUVE(), 500.0F);
        EXPECT_FALSE(editor.ZoomViewportUVE(-100.0F));
        EXPECT_TRUE(editor.PanViewportUVE(Math::Vector2UVE{0.0F, 0.0F}, validViewport));

        editor.ShutdownUVE();
        EXPECT_FALSE(editor.OrbitViewportUVE(0.1F, 0.1F));
        EXPECT_FALSE(editor.ZoomViewportUVE(1.0F));
    }

    engine.Shutdown();
}

TEST(EditorUVETest, ViewportNavigationUVE_DoesNotInterfereWithDocumentHistory) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_navigation_history.uvescene");
        editor.InitUVE();
        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
        const Scene::EntityUVE root = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, root, Scene::TransformComponentUVE{});
        editor.SelectEntityUVE(root);

        Scene::TransformComponentUVE edited{};
        edited.localPosition = Math::Vector3UVE{2.0F, 0.0F, 0.0F};
        ASSERT_TRUE(editor.SetSelectedLocalTransformUVE(edited));
        ASSERT_TRUE(editor.CanUndoUVE());
        ASSERT_TRUE(editor.OrbitViewportUVE(0.3F, -0.2F));
        ASSERT_TRUE(editor.ZoomViewportUVE(1.0F));
        EXPECT_TRUE(editor.CanUndoUVE());
        EXPECT_FALSE(editor.CanRedoUVE());
        ASSERT_TRUE(editor.UndoUVE());
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(root).localPosition,
                  Math::Vector3UVE{});
        EXPECT_FALSE(editor.IsSceneDirtyUVE());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
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
        ASSERT_TRUE(editor.UndoUVE());
        EXPECT_NEAR(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(child).localPosition.x,
                    1.0F, 0.0001F);
        ASSERT_TRUE(editor.RedoUVE());
        EXPECT_NEAR(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(child).localPosition.x,
                    2.0F, 0.0001F);
        EXPECT_FALSE(editor.TranslateSelectedAlongAxisUVE(EditorTranslateAxisUVE::None, 1.0F));
        EXPECT_FALSE(editor.TranslateSelectedAlongAxisUVE(
            EditorTranslateAxisUVE::Y, std::numeric_limits<float>::infinity()));

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, RotateSelectedAroundWorldAxis_RotatesRootPreservesOtherLocalFieldsAndReplaysHistory) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_rotate_root.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE initial{};
        initial.localPosition = Math::Vector3UVE{2.0F, 3.0F, 4.0F};
        initial.localScale = Math::Vector3UVE{2.0F, 3.0F, 4.0F};
        AttachRootUVE(engine, entity, initial);

        editor.SelectEntityUVE(entity);
        EXPECT_EQ(editor.GetGizmoModeUVE(), EditorGizmoModeUVE::Translate);
        editor.SetGizmoModeUVE(EditorGizmoModeUVE::Rotate);
        EXPECT_EQ(editor.GetGizmoModeUVE(), EditorGizmoModeUVE::Rotate);
        ASSERT_TRUE(editor.RotateSelectedAroundWorldAxisUVE(EditorTranslateAxisUVE::Z,
                                                            std::numbers::pi_v<float> * 0.5F));
        const Scene::TransformComponentUVE& rotated =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity);
        const Math::Vector3UVE localXAxis =
            Math::RotateVectorUVE(rotated.localRotation, Math::Vector3UVE{1.0F, 0.0F, 0.0F});
        EXPECT_NEAR(localXAxis.x, 0.0F, 0.0001F);
        EXPECT_NEAR(localXAxis.y, 1.0F, 0.0001F);
        EXPECT_NEAR(localXAxis.z, 0.0F, 0.0001F);
        EXPECT_EQ(rotated.localPosition, initial.localPosition);
        EXPECT_EQ(rotated.localScale, initial.localScale);
        EXPECT_TRUE(editor.IsSceneDirtyUVE());

        ASSERT_TRUE(editor.UndoUVE());
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity).localRotation,
                  initial.localRotation);
        ASSERT_TRUE(editor.RedoUVE());
        const Math::Vector3UVE replayedXAxis = Math::RotateVectorUVE(
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity).localRotation,
            Math::Vector3UVE{1.0F, 0.0F, 0.0F});
        EXPECT_NEAR(replayedXAxis.x, 0.0F, 0.0001F);
        EXPECT_NEAR(replayedXAxis.y, 1.0F, 0.0001F);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, RotateSelectedAroundWorldAxis_ConvertsParentWorldRotationToLocalRotation) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_rotate_parented.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();

        Math::QuaternionUVE parentRotation{};
        ASSERT_TRUE(Math::TryMakeAxisAngleUVE(Math::Vector3UVE{0.0F, 0.0F, 1.0F},
                                              std::numbers::pi_v<float> * 0.5F, parentRotation));
        const Scene::EntityUVE parent = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE parentTransform{};
        parentTransform.localRotation = parentRotation;
        AttachRootUVE(engine, parent, parentTransform);

        const Scene::EntityUVE child = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE childTransform{};
        childTransform.localPosition = Math::Vector3UVE{1.0F, 2.0F, 3.0F};
        childTransform.localScale = Math::Vector3UVE{2.0F, 2.0F, 2.0F};
        AttachRootUVE(engine, child, childTransform);
        services.GetSceneGraphUVE().SetParentUVE(entityManager, child, parent);
        services.GetSceneGraphUVE().UpdateUVE(entityManager);

        editor.SelectEntityUVE(child);
        ASSERT_TRUE(editor.RotateSelectedAroundWorldAxisUVE(EditorTranslateAxisUVE::X,
                                                            std::numbers::pi_v<float> * 0.5F));
        services.GetSceneGraphUVE().UpdateUVE(entityManager);

        Math::QuaternionUVE worldDelta{};
        ASSERT_TRUE(Math::TryMakeAxisAngleUVE(Math::Vector3UVE{1.0F, 0.0F, 0.0F},
                                              std::numbers::pi_v<float> * 0.5F, worldDelta));
        const Math::QuaternionUVE expectedWorld = Math::MultiplyUVE(worldDelta, parentRotation);
        const Scene::WorldTransformComponentUVE& childWorld =
            entityManager.GetComponentUVE<Scene::WorldTransformComponentUVE>(child);
        const Math::Vector3UVE expectedProbe =
            Math::RotateVectorUVE(expectedWorld, Math::Vector3UVE{0.0F, 1.0F, 0.0F});
        const Math::Vector3UVE actualProbe =
            Math::RotateVectorUVE(childWorld.worldRotation, Math::Vector3UVE{0.0F, 1.0F, 0.0F});
        EXPECT_NEAR(actualProbe.x, expectedProbe.x, 0.0001F);
        EXPECT_NEAR(actualProbe.y, expectedProbe.y, 0.0001F);
        EXPECT_NEAR(actualProbe.z, expectedProbe.z, 0.0001F);
        const Scene::TransformComponentUVE& rotated =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(child);
        EXPECT_EQ(rotated.localPosition, childTransform.localPosition);
        EXPECT_EQ(rotated.localScale, childTransform.localScale);

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, RotateSelectedAroundWorldAxis_RejectsInvalidOrUnsafeStateWithoutMutation) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_rotate_safety.uvescene");
        editor.InitUVE();
        EXPECT_FALSE(editor.RotateSelectedAroundWorldAxisUVE(EditorTranslateAxisUVE::Z, 1.0F));

        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, entity, Scene::TransformComponentUVE{});
        editor.SelectEntityUVE(entity);
        const Scene::TransformComponentUVE before =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity);
        EXPECT_FALSE(editor.RotateSelectedAroundWorldAxisUVE(EditorTranslateAxisUVE::None, 1.0F));
        EXPECT_FALSE(editor.RotateSelectedAroundWorldAxisUVE(EditorTranslateAxisUVE::Y,
                                                             std::numeric_limits<float>::infinity()));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity).localRotation,
                  before.localRotation);
        EXPECT_FALSE(editor.IsSceneDirtyUVE());

        editor.ShutdownUVE();
    }

    engine.Shutdown();
}

TEST(EditorUVETest, ScaleSelectedAlongAxis_UpdatesOnlyPositiveLocalScaleAndReplaysHistory) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_scale.uvescene");
        editor.InitUVE();
        Core::EngineServicesUVE& services = engine.GetServicesUVE();
        Scene::IEntityManagerUVE& entityManager = services.GetEntityManagerUVE();
        const Scene::EntityUVE parent = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, parent, Scene::TransformComponentUVE{});
        const Scene::EntityUVE child = entityManager.CreateEntityUVE();
        Scene::TransformComponentUVE initial{};
        initial.localPosition = Math::Vector3UVE{1.0F, 2.0F, 3.0F};
        initial.localScale = Math::Vector3UVE{1.0F, 2.0F, 3.0F};
        AttachRootUVE(engine, child, initial);
        services.GetSceneGraphUVE().SetParentUVE(entityManager, child, parent);
        services.GetSceneGraphUVE().UpdateUVE(entityManager);

        editor.SelectEntityUVE(child);
        EXPECT_EQ(editor.GetGizmoModeUVE(), EditorGizmoModeUVE::Translate);
        editor.SetGizmoModeUVE(EditorGizmoModeUVE::Scale);
        EXPECT_EQ(editor.GetGizmoModeUVE(), EditorGizmoModeUVE::Scale);
        ASSERT_TRUE(editor.ScaleSelectedAlongAxisUVE(EditorTranslateAxisUVE::Y, 1.5F));
        const Scene::TransformComponentUVE& scaled =
            entityManager.GetComponentUVE<Scene::TransformComponentUVE>(child);
        EXPECT_EQ(scaled.localPosition, initial.localPosition);
        EXPECT_EQ(scaled.localRotation, initial.localRotation);
        EXPECT_NEAR(scaled.localScale.x, 1.0F, 0.0001F);
        EXPECT_NEAR(scaled.localScale.y, 3.5F, 0.0001F);
        EXPECT_NEAR(scaled.localScale.z, 3.0F, 0.0001F);
        EXPECT_TRUE(editor.IsSceneDirtyUVE());
        ASSERT_TRUE(editor.UndoUVE());
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(child).localScale,
                  initial.localScale);
        ASSERT_TRUE(editor.RedoUVE());
        EXPECT_NEAR(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(child).localScale.y,
                    3.5F, 0.0001F);

        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

TEST(EditorUVETest, ScaleSelectedAlongAxis_RejectsUnsafeInputWithoutMutation) {
    Core::EngineCoreUVE engine(MakeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_editor_tests_scale_safety.uvescene");
        editor.InitUVE();
        EXPECT_FALSE(editor.ScaleSelectedAlongAxisUVE(EditorTranslateAxisUVE::X, 1.0F));
        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
        const Scene::EntityUVE entity = entityManager.CreateEntityUVE();
        AttachRootUVE(engine, entity, Scene::TransformComponentUVE{});
        editor.SelectEntityUVE(entity);
        const Scene::TransformComponentUVE before = entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity);
        EXPECT_FALSE(editor.ScaleSelectedAlongAxisUVE(EditorTranslateAxisUVE::None, 1.0F));
        EXPECT_FALSE(editor.ScaleSelectedAlongAxisUVE(EditorTranslateAxisUVE::X, -1.0F));
        EXPECT_FALSE(editor.ScaleSelectedAlongAxisUVE(EditorTranslateAxisUVE::Z,
                                                       std::numeric_limits<float>::infinity()));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::TransformComponentUVE>(entity).localScale,
                  before.localScale);
        EXPECT_FALSE(editor.IsSceneDirtyUVE());
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
