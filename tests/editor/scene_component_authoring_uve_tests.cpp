// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <limits>
#include <string>

#include <gtest/gtest.h>

#include "uve/asset/asset_guid_uve.h"
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

namespace UVE::Editor::Tests {
namespace {

[[nodiscard]] Core::EngineConfigUVE MakeSceneComponentAuthoringTestConfigUVE() {
    Core::EngineConfigUVE config{};
    config.headlessUVE = true;
    config.logFilePath = "uve_scene_component_authoring_tests.log";
    config.settingsFilePath = "uve_scene_component_authoring_tests_settings.json";
    config.assetDatabaseFilePath = "uve_scene_component_authoring_tests_assets.json";
    config.saveDirectoryPath = "uve_scene_component_authoring_tests_saves";
    config.shaderCachePath = "uve_scene_component_authoring_tests_shader_cache";
    config.shaderSourceRealDirectoryUVE = "engine/render/shader/built_in";
    config.shaderSourceMountPrefixUVE = "shaders";
    return config;
}

TEST(SceneComponentAuthoringUVETest, SetSelectedSceneComponentUVE_AddsAllSupportedComponentKindsAndReplaysHistory) {
    Core::EngineCoreUVE engine(MakeSceneComponentAuthoringTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_scene_component_authoring.uvescene");
        editor.InitUVE();
        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
        const Scene::EntityUVE entity = editor.CreateDocumentEntityUVE(EditorEntityKindUVE::Empty);
        ASSERT_TRUE(entityManager.IsAliveUVE(entity));

        const Scene::CameraComponentUVE camera{75.0F, 0.05F, 500.0F};
        ASSERT_TRUE(editor.SetSelectedSceneComponentUVE(EditorSceneComponentKindUVE::Camera, camera));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::CameraComponentUVE>(entity).fieldOfViewDegrees, 75.0F);

        const Scene::MeshComponentUVE mesh{Asset::AssetGuidUVE{0x1010U}, Asset::AssetGuidUVE{0x2020U}};
        ASSERT_TRUE(editor.SetSelectedSceneComponentUVE(EditorSceneComponentKindUVE::Mesh, mesh));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::MeshComponentUVE>(entity).meshGuid.value, 0x1010U);

        Scene::LightComponentUVE light{};
        light.type = Scene::LightTypeUVE::Spot;
        light.intensity = 4.0F;
        ASSERT_TRUE(editor.SetSelectedSceneComponentUVE(EditorSceneComponentKindUVE::Light, light));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::LightComponentUVE>(entity).type, Scene::LightTypeUVE::Spot);

        Scene::ColliderComponentUVE collider{};
        collider.shapeType = Scene::ColliderShapeTypeUVE::Capsule;
        collider.radius = 0.35F;
        collider.height = 1.8F;
        ASSERT_TRUE(editor.SetSelectedSceneComponentUVE(EditorSceneComponentKindUVE::Collider, collider));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::ColliderComponentUVE>(entity).shapeType,
                  Scene::ColliderShapeTypeUVE::Capsule);

        Scene::RigidBodyComponentUVE rigidBody{};
        rigidBody.isKinematic = true;
        rigidBody.mass = 2.0F;
        rigidBody.gravityScale = 0.0F;
        ASSERT_TRUE(editor.SetSelectedSceneComponentUVE(EditorSceneComponentKindUVE::RigidBody, rigidBody));
        EXPECT_TRUE(entityManager.GetComponentUVE<Scene::RigidBodyComponentUVE>(entity).isKinematic);

        Scene::AudioSourceComponentUVE audio{};
        audio.audioAssetPath = "audio/impact.wav";
        audio.mixerGroup = "SFX";
        audio.playOnAwake = false;
        ASSERT_TRUE(editor.SetSelectedSceneComponentUVE(EditorSceneComponentKindUVE::AudioSource, audio));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::AudioSourceComponentUVE>(entity).audioAssetPath,
                  "audio/impact.wav");

        const Scene::ParticleEmitterComponentUVE particles{2048U};
        ASSERT_TRUE(editor.SetSelectedSceneComponentUVE(EditorSceneComponentKindUVE::ParticleEmitter, particles));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::ParticleEmitterComponentUVE>(entity).maxParticles, 2048U);

        const Scene::ScriptComponentUVE script{"scripts/player.uvescript"};
        ASSERT_TRUE(editor.SetSelectedSceneComponentUVE(EditorSceneComponentKindUVE::Script, script));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::ScriptComponentUVE>(entity).scriptAssetPath,
                  "scripts/player.uvescript");

        const Scene::AnimationPlayerComponentUVE animation{"animations/run.uveclip", 1.25F, true, true, true};
        ASSERT_TRUE(editor.SetSelectedSceneComponentUVE(EditorSceneComponentKindUVE::AnimationPlayer, animation));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::AnimationPlayerComponentUVE>(entity).clipAssetPath,
                  "animations/run.uveclip");

        ASSERT_TRUE(editor.RemoveSelectedSceneComponentUVE(EditorSceneComponentKindUVE::AnimationPlayer));
        EXPECT_FALSE(entityManager.HasComponentUVE<Scene::AnimationPlayerComponentUVE>(entity));
        ASSERT_TRUE(editor.UndoUVE());
        EXPECT_TRUE(entityManager.HasComponentUVE<Scene::AnimationPlayerComponentUVE>(entity));
        EXPECT_EQ(entityManager.GetComponentUVE<Scene::AnimationPlayerComponentUVE>(entity).playbackSpeed, 1.25F);
        ASSERT_TRUE(editor.RedoUVE());
        EXPECT_FALSE(entityManager.HasComponentUVE<Scene::AnimationPlayerComponentUVE>(entity));

        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

TEST(SceneComponentAuthoringUVETest, SetSelectedSceneComponentUVE_RejectsInvalidValuesWithoutMutation) {
    Core::EngineCoreUVE engine(MakeSceneComponentAuthoringTestConfigUVE());
    engine.Init();
    ASSERT_TRUE(engine.Load());

    {
        EditorUVE editor(engine.GetServicesUVE(), "uve_scene_component_authoring_invalid.uvescene");
        editor.InitUVE();
        Scene::IEntityManagerUVE& entityManager = engine.GetServicesUVE().GetEntityManagerUVE();
        const Scene::EntityUVE entity = editor.CreateDocumentEntityUVE(EditorEntityKindUVE::Empty);
        ASSERT_TRUE(entityManager.IsAliveUVE(entity));

        Scene::CameraComponentUVE invalidCamera{180.0F, 0.1F, 100.0F};
        EXPECT_FALSE(editor.SetSelectedSceneComponentUVE(EditorSceneComponentKindUVE::Camera, invalidCamera));
        EXPECT_FALSE(entityManager.HasComponentUVE<Scene::CameraComponentUVE>(entity));

        Scene::ColliderComponentUVE invalidCollider{};
        invalidCollider.shapeType = Scene::ColliderShapeTypeUVE::Capsule;
        invalidCollider.radius = 1.0F;
        invalidCollider.height = 1.0F;
        EXPECT_FALSE(editor.SetSelectedSceneComponentUVE(EditorSceneComponentKindUVE::Collider, invalidCollider));
        EXPECT_FALSE(entityManager.HasComponentUVE<Scene::ColliderComponentUVE>(entity));

        Scene::AudioSourceComponentUVE invalidAudio{};
        invalidAudio.spatial = true;
        invalidAudio.minDistance = 5.0F;
        invalidAudio.maxDistance = 2.0F;
        EXPECT_FALSE(editor.SetSelectedSceneComponentUVE(EditorSceneComponentKindUVE::AudioSource, invalidAudio));
        EXPECT_FALSE(entityManager.HasComponentUVE<Scene::AudioSourceComponentUVE>(entity));

        const Scene::ParticleEmitterComponentUVE invalidParticles{0U};
        EXPECT_FALSE(editor.SetSelectedSceneComponentUVE(EditorSceneComponentKindUVE::ParticleEmitter, invalidParticles));
        EXPECT_FALSE(entityManager.HasComponentUVE<Scene::ParticleEmitterComponentUVE>(entity));

        const Scene::ScriptComponentUVE invalidScript{"../player.uvescript"};
        EXPECT_FALSE(editor.SetSelectedSceneComponentUVE(EditorSceneComponentKindUVE::Script, invalidScript));
        EXPECT_FALSE(entityManager.HasComponentUVE<Scene::ScriptComponentUVE>(entity));

        const Scene::AnimationPlayerComponentUVE invalidAnimation{
            "animations/run.uveclip", std::numeric_limits<float>::quiet_NaN(), true, true, true};
        EXPECT_FALSE(editor.SetSelectedSceneComponentUVE(EditorSceneComponentKindUVE::AnimationPlayer, invalidAnimation));
        EXPECT_FALSE(entityManager.HasComponentUVE<Scene::AnimationPlayerComponentUVE>(entity));

        editor.ShutdownUVE();
    }
    engine.Shutdown();
}

} // namespace
} // namespace UVE::Editor::Tests
