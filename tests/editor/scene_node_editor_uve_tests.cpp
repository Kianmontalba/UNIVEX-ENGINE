// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <gtest/gtest.h>

#include "uve/core/engine_core_uve.h"
#include "uve/editor/editor_uve.h"
#include "uve/scene/components/animation_player_component_uve.h"
#include "uve/scene/components/audio_source_component_uve.h"
#include "uve/scene/components/camera_component_uve.h"
#include "uve/scene/components/collider_component_uve.h"
#include "uve/scene/components/light_component_uve.h"
#include "uve/scene/components/mesh_component_uve.h"
#include "uve/scene/components/particle_emitter_component_uve.h"
#include "uve/scene/components/rigid_body_component_uve.h"
#include "uve/scene/components/script_component_uve.h"
#include "uve/scene/nodes/scene_node_registry_uve.h"

namespace UVE::Editor::Tests {
namespace {

[[nodiscard]] Core::EngineConfigUVE MakeSceneNodeEditorTestConfigUVE() {
    Core::EngineConfigUVE config{};
    config.headlessUVE = true;
    config.logFilePath = "uve_scene_node_editor_tests.log";
    config.settingsFilePath = "uve_scene_node_editor_tests_settings.json";
    config.assetDatabaseFilePath = "uve_scene_node_editor_tests_assets.json";
    config.saveDirectoryPath = "uve_scene_node_editor_tests_saves";
    config.shaderCachePath = "uve_scene_node_editor_tests_shader_cache";
    config.shaderSourceRealDirectoryUVE = "engine/render/shader/built_in";
    config.shaderSourceMountPrefixUVE = "shaders";
    return config;
}

TEST(SceneNodeEditorUVETest, CentralizedRegistryCreationUVE_AttachesExpectedAuthoredComponents) {
    Core::EngineCoreUVE engine(MakeSceneNodeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_scene_node_editor_tests.uvescene");
        editor.InitUVE();
        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();

        const Scene::EntityUVE camera =
            editor.CreateDocumentSceneNodeUVE(Scene::Nodes::SceneNodeKindUVE::Camera3D);
        ASSERT_NE(camera, Scene::kInvalidEntityUVE);
        EXPECT_TRUE(entityManager.HasComponentUVE<Scene::CameraComponentUVE>(camera));

        const Scene::EntityUVE mesh =
            editor.CreateDocumentSceneNodeUVE(Scene::Nodes::SceneNodeKindUVE::MeshInstance3D);
        ASSERT_NE(mesh, Scene::kInvalidEntityUVE);
        EXPECT_TRUE(entityManager.HasComponentUVE<Scene::MeshComponentUVE>(mesh));

        const Scene::EntityUVE character =
            editor.CreateDocumentSceneNodeUVE(Scene::Nodes::SceneNodeKindUVE::CharacterBody3D);
        ASSERT_NE(character, Scene::kInvalidEntityUVE);
        EXPECT_TRUE(entityManager.HasComponentUVE<Scene::ColliderComponentUVE>(character));
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::RigidBodyComponentUVE>(character));
        EXPECT_TRUE(entityManager.GetComponentUVE<Scene::RigidBodyComponentUVE>(character).isKinematic);

        const Scene::EntityUVE animationPlayer =
            editor.CreateDocumentSceneNodeUVE(Scene::Nodes::SceneNodeKindUVE::AnimationPlayer);
        ASSERT_NE(animationPlayer, Scene::kInvalidEntityUVE);
        EXPECT_TRUE(entityManager.HasComponentUVE<Scene::AnimationPlayerComponentUVE>(animationPlayer));

        const Scene::EntityUVE audio =
            editor.CreateDocumentSceneNodeUVE(Scene::Nodes::SceneNodeKindUVE::AudioSource3D);
        ASSERT_NE(audio, Scene::kInvalidEntityUVE);
        EXPECT_TRUE(entityManager.HasComponentUVE<Scene::AudioSourceComponentUVE>(audio));

        const Scene::EntityUVE particles =
            editor.CreateDocumentSceneNodeUVE(Scene::Nodes::SceneNodeKindUVE::ParticleEmitter3D);
        ASSERT_NE(particles, Scene::kInvalidEntityUVE);
        EXPECT_TRUE(entityManager.HasComponentUVE<Scene::ParticleEmitterComponentUVE>(particles));

        const Scene::EntityUVE script =
            editor.CreateDocumentSceneNodeUVE(Scene::Nodes::SceneNodeKindUVE::Script);
        ASSERT_NE(script, Scene::kInvalidEntityUVE);
        EXPECT_TRUE(entityManager.HasComponentUVE<Scene::ScriptComponentUVE>(script));

        const Scene::EntityUVE rigidBody =
            editor.CreateDocumentSceneNodeUVE(Scene::Nodes::SceneNodeKindUVE::RigidBody3D);
        ASSERT_NE(rigidBody, Scene::kInvalidEntityUVE);
        EXPECT_TRUE(entityManager.HasComponentUVE<Scene::RigidBodyComponentUVE>(rigidBody));

        EXPECT_EQ(editor.CreateDocumentSceneNodeUVE(Scene::Nodes::SceneNodeKindUVE::AnimationTree),
                  Scene::kInvalidEntityUVE);
        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

TEST(SceneNodeEditorUVETest, CharacterBodyCreationUVE_IsOneAtomicUndoRedoTransaction) {
    Core::EngineCoreUVE engine(MakeSceneNodeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_scene_node_editor_history_tests.uvescene");
        editor.InitUVE();
        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();

        const Scene::EntityUVE created =
            editor.CreateDocumentSceneNodeUVE(Scene::Nodes::SceneNodeKindUVE::CharacterBody3D);
        ASSERT_NE(created, Scene::kInvalidEntityUVE);
        ASSERT_TRUE(editor.CanUndoUVE());
        EXPECT_TRUE(editor.IsSceneDirtyUVE());

        ASSERT_TRUE(editor.UndoUVE());
        EXPECT_FALSE(entityManager.IsAliveUVE(created));
        EXPECT_EQ(editor.GetSelectedEntityUVE(), Scene::kInvalidEntityUVE);
        EXPECT_FALSE(editor.IsSceneDirtyUVE());
        EXPECT_TRUE(editor.CanRedoUVE());

        ASSERT_TRUE(editor.RedoUVE());
        const Scene::EntityUVE restored = editor.GetSelectedEntityUVE();
        ASSERT_NE(restored, Scene::kInvalidEntityUVE);
        EXPECT_NE(restored, created);
        EXPECT_TRUE(entityManager.HasComponentUVE<Scene::ColliderComponentUVE>(restored));
        ASSERT_TRUE(entityManager.HasComponentUVE<Scene::RigidBodyComponentUVE>(restored));
        EXPECT_TRUE(entityManager.GetComponentUVE<Scene::RigidBodyComponentUVE>(restored).isKinematic);
        EXPECT_TRUE(editor.IsSceneDirtyUVE());
        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

TEST(SceneNodeEditorUVETest, CentralizedCreationUVE_RejectsMultiSelectionAndPlayModeWithoutMutation) {
    Core::EngineCoreUVE engine(MakeSceneNodeEditorTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());
    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_scene_node_editor_safety_tests.uvescene", 100U, &engine);
        editor.InitUVE();
        const Scene::EntityUVE first =
            editor.CreateDocumentSceneNodeUVE(Scene::Nodes::SceneNodeKindUVE::Empty);
        const Scene::EntityUVE second =
            editor.CreateDocumentSceneNodeUVE(Scene::Nodes::SceneNodeKindUVE::Empty);
        ASSERT_NE(first, Scene::kInvalidEntityUVE);
        ASSERT_NE(second, Scene::kInvalidEntityUVE);
        editor.SelectEntityUVE(first);
        editor.ToggleEntitySelectionUVE(second);
        ASSERT_FALSE(editor.HasSingleDocumentSelectionUVE());
        const std::vector<Scene::EntityUVE> rootsBefore = editor.GetDocumentRootsUVE();
        EXPECT_EQ(editor.CreateDocumentSceneNodeUVE(Scene::Nodes::SceneNodeKindUVE::Camera3D),
                  Scene::kInvalidEntityUVE);
        EXPECT_EQ(editor.GetDocumentRootsUVE(), rootsBefore);

        ASSERT_TRUE(editor.EnterPlayModeUVE());
        EXPECT_EQ(editor.CreateDocumentSceneNodeUVE(Scene::Nodes::SceneNodeKindUVE::Light3D),
                  Scene::kInvalidEntityUVE);
        ASSERT_TRUE(editor.StopPlayModeUVE());
        EXPECT_EQ(editor.GetDocumentRootsUVE().size(), rootsBefore.size());
        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

} // namespace
} // namespace UVE::Editor::Tests
