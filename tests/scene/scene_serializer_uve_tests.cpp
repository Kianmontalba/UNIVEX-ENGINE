// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/scene/scene_serializer_uve.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "uve/asset/asset_guid_uve.h"
#include "uve/asset/uve_file_envelope_uve.h"
#include "uve/debug/log_sink_uve.h"
#include "uve/debug/logger_uve.h"
#include "uve/events/event_system_uve.h"
#include "uve/math/vector3_uve.h"
#include "uve/memory/memory_manager_uve.h"
#include "uve/scene/components/audio_source_component_uve.h"
#include "uve/scene/components/camera_component_uve.h"
#include "uve/scene/components/collider_component_uve.h"
#include "uve/scene/components/hierarchy_component_uve.h"
#include "uve/scene/components/light_component_uve.h"
#include "uve/scene/components/mesh_component_uve.h"
#include "uve/scene/components/name_component_uve.h"
#include "uve/scene/components/particle_emitter_component_uve.h"
#include "uve/scene/components/primitive_mesh_component_uve.h"
#include "uve/scene/components/prefab_instance_component_uve.h"
#include "uve/scene/components/rigid_body_component_uve.h"
#include "uve/scene/components/script_component_uve.h"
#include "uve/scene/components/transform_component_uve.h"
#include "uve/scene/components/world_transform_component_uve.h"
#include "uve/scene/entity_manager_uve.h"
#include "uve/scene/scene_graph_uve.h"

namespace UVE::Scene::Tests {
namespace {

class SceneSerializerUVETest : public ::testing::Test {
protected:
    Memory::MemoryManagerUVE memoryManager;
    Events::EventSystemUVE eventSystem;
    EntityManagerUVE entityManager{memoryManager.GetDefaultAllocatorUVE(), eventSystem};
    SceneSerializerUVE serializer;
};

struct UnregisteredSnapshotComponentUVE final {
    int value = 0;
};

TEST_F(SceneSerializerUVETest, CaptureThenRestore_EmptyRootList_ReturnsValidEmptySnapshotWithoutMutation) {
    const std::size_t entityCountBefore = entityManager.GetEntityCountUVE();

    const std::optional<SceneSnapshotUVE> snapshot =
        serializer.CaptureUVE(entityManager, {}, SceneAssetTypeUVE::Scene);
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_FALSE(snapshot->bytes.empty());
    EXPECT_EQ(snapshot->assetType, SceneAssetTypeUVE::Scene);

    const std::vector<EntityUVE> roots = serializer.RestoreUVE(entityManager, *snapshot);
    EXPECT_TRUE(roots.empty());
    EXPECT_EQ(entityManager.GetEntityCountUVE(), entityCountBefore);
}

TEST_F(SceneSerializerUVETest, CaptureThenRestore_NestedHierarchy_RecreatesFreshHandlesAndRelationships) {
    const EntityUVE sourceRoot = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<TransformComponentUVE>(sourceRoot, TransformComponentUVE{});
    entityManager.AddComponentUVE<HierarchyComponentUVE>(sourceRoot, HierarchyComponentUVE{kInvalidEntityUVE});
    entityManager.AddComponentUVE<NameComponentUVE>(sourceRoot, NameComponentUVE{"Source Root"});

    const EntityUVE sourceChild = entityManager.CreateEntityUVE();
    TransformComponentUVE childTransform{};
    childTransform.localPosition = Math::Vector3UVE{2.0F, 3.0F, 4.0F};
    entityManager.AddComponentUVE<TransformComponentUVE>(sourceChild, childTransform);
    entityManager.AddComponentUVE<HierarchyComponentUVE>(sourceChild, HierarchyComponentUVE{sourceRoot});
    entityManager.AddComponentUVE<NameComponentUVE>(sourceChild, NameComponentUVE{"Source Child"});

    const std::optional<SceneSnapshotUVE> snapshot =
        serializer.CaptureUVE(entityManager, {sourceRoot}, SceneAssetTypeUVE::Scene);
    ASSERT_TRUE(snapshot.has_value());

    const std::vector<EntityUVE> restoredRoots = serializer.RestoreUVE(entityManager, *snapshot);
    ASSERT_EQ(restoredRoots.size(), 1U);
    const EntityUVE restoredRoot = restoredRoots.front();
    EXPECT_NE(restoredRoot, sourceRoot);
    EXPECT_EQ(entityManager.GetComponentUVE<NameComponentUVE>(restoredRoot).name, "Source Root");

    EntityUVE restoredChild = kInvalidEntityUVE;
    entityManager.ForEachUVE<HierarchyComponentUVE>(
        [&restoredChild, restoredRoot](const EntityUVE entity, HierarchyComponentUVE& hierarchy) {
            if (hierarchy.parent == restoredRoot) {
                restoredChild = entity;
            }
        });
    ASSERT_NE(restoredChild, kInvalidEntityUVE);
    EXPECT_NE(restoredChild, sourceChild);
    EXPECT_EQ(entityManager.GetComponentUVE<NameComponentUVE>(restoredChild).name, "Source Child");
    EXPECT_TRUE(entityManager.GetComponentUVE<TransformComponentUVE>(restoredChild).localPosition ==
                childTransform.localPosition);
    EXPECT_TRUE(entityManager.HasComponentUVE<WorldTransformComponentUVE>(restoredRoot));
    EXPECT_TRUE(entityManager.HasComponentUVE<WorldTransformComponentUVE>(restoredChild));
}

TEST_F(SceneSerializerUVETest, CaptureThenRestore_AllRegisteredComponentTypes_RoundTrip) {
    const EntityUVE source = entityManager.CreateEntityUVE();
    TransformComponentUVE transform{};
    transform.localPosition = Math::Vector3UVE{1.0F, 2.0F, 3.0F};
    entityManager.AddComponentUVE<TransformComponentUVE>(source, transform);
    entityManager.AddComponentUVE<MeshComponentUVE>(source,
                                                     MeshComponentUVE{Asset::AssetGuidUVE{11}, Asset::AssetGuidUVE{22}});
    entityManager.AddComponentUVE<PrimitiveMeshComponentUVE>(
        source, PrimitiveMeshComponentUVE{PrimitiveMeshKindUVE::UVSphere, Math::Vector3UVE{0.2F, 0.5F, 0.8F}});
    LightComponentUVE light{};
    light.type = LightTypeUVE::Spot;
    light.intensity = 4.0F;
    entityManager.AddComponentUVE<LightComponentUVE>(source, light);
    entityManager.AddComponentUVE<CameraComponentUVE>(source, CameraComponentUVE{75.0F, 0.2F, 250.0F});
    entityManager.AddComponentUVE<NameComponentUVE>(source, NameComponentUVE{"Complete Snapshot"});
    ColliderComponentUVE collider{};
    collider.friction = 0.25F;
    entityManager.AddComponentUVE<ColliderComponentUVE>(source, collider);
    RigidBodyComponentUVE body{};
    body.mass = 9.5F;
    body.isKinematic = true;
    entityManager.AddComponentUVE<RigidBodyComponentUVE>(source, body);
    AudioSourceComponentUVE audio{};
    audio.audioAssetPath = "sounds/lifecycle.wav";
    audio.looping = true;
    entityManager.AddComponentUVE<AudioSourceComponentUVE>(source, audio);
    entityManager.AddComponentUVE<ScriptComponentUVE>(source, ScriptComponentUVE{"scripts/lifecycle.lua"});
    entityManager.AddComponentUVE<ParticleEmitterComponentUVE>(source, ParticleEmitterComponentUVE{128U});
    entityManager.AddComponentUVE<PrefabInstanceComponentUVE>(source,
                                                               PrefabInstanceComponentUVE{Asset::AssetGuidUVE{9001}});

    const std::optional<SceneSnapshotUVE> snapshot =
        serializer.CaptureUVE(entityManager, {source}, SceneAssetTypeUVE::Scene);
    ASSERT_TRUE(snapshot.has_value());
    const std::vector<EntityUVE> restoredRoots = serializer.RestoreUVE(entityManager, *snapshot);
    ASSERT_EQ(restoredRoots.size(), 1U);
    const EntityUVE restored = restoredRoots.front();

    EXPECT_NE(restored, source);
    EXPECT_TRUE(entityManager.GetComponentUVE<TransformComponentUVE>(restored).localPosition == transform.localPosition);
    EXPECT_EQ(entityManager.GetComponentUVE<MeshComponentUVE>(restored).meshGuid, Asset::AssetGuidUVE{11});
    ASSERT_TRUE(entityManager.HasComponentUVE<PrimitiveMeshComponentUVE>(restored));
    EXPECT_EQ(entityManager.GetComponentUVE<PrimitiveMeshComponentUVE>(restored).kind,
              PrimitiveMeshKindUVE::UVSphere);
    EXPECT_EQ(entityManager.GetComponentUVE<PrimitiveMeshComponentUVE>(restored).baseColor,
              (Math::Vector3UVE{0.2F, 0.5F, 0.8F}));
    EXPECT_EQ(entityManager.GetComponentUVE<LightComponentUVE>(restored).type, LightTypeUVE::Spot);
    EXPECT_FLOAT_EQ(entityManager.GetComponentUVE<CameraComponentUVE>(restored).farPlane, 250.0F);
    EXPECT_EQ(entityManager.GetComponentUVE<NameComponentUVE>(restored).name, "Complete Snapshot");
    EXPECT_FLOAT_EQ(entityManager.GetComponentUVE<ColliderComponentUVE>(restored).friction, 0.25F);
    EXPECT_FLOAT_EQ(entityManager.GetComponentUVE<RigidBodyComponentUVE>(restored).mass, 9.5F);
    EXPECT_TRUE(entityManager.GetComponentUVE<RigidBodyComponentUVE>(restored).isKinematic);
    EXPECT_EQ(entityManager.GetComponentUVE<AudioSourceComponentUVE>(restored).audioAssetPath,
              "sounds/lifecycle.wav");
    EXPECT_TRUE(entityManager.GetComponentUVE<AudioSourceComponentUVE>(restored).looping);
    EXPECT_EQ(entityManager.GetComponentUVE<ScriptComponentUVE>(restored).scriptAssetPath, "scripts/lifecycle.lua");
    EXPECT_EQ(entityManager.GetComponentUVE<ParticleEmitterComponentUVE>(restored).maxParticles, 128U);
    EXPECT_EQ(entityManager.GetComponentUVE<PrefabInstanceComponentUVE>(restored).sourcePrefabGuid,
              Asset::AssetGuidUVE{9001});
    EXPECT_TRUE(entityManager.HasComponentUVE<WorldTransformComponentUVE>(restored));
}

TEST_F(SceneSerializerUVETest, CaptureUVE_UnregisteredComponent_ReturnsNulloptWithoutMutation) {
    const EntityUVE entity = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<UnregisteredSnapshotComponentUVE>(entity, UnregisteredSnapshotComponentUVE{42});
    const std::size_t entityCountBefore = entityManager.GetEntityCountUVE();

    const std::optional<SceneSnapshotUVE> snapshot =
        serializer.CaptureUVE(entityManager, {entity}, SceneAssetTypeUVE::Scene);

    EXPECT_FALSE(snapshot.has_value());
    EXPECT_TRUE(entityManager.IsAliveUVE(entity));
    EXPECT_EQ(entityManager.GetEntityCountUVE(), entityCountBefore);
}

TEST_F(SceneSerializerUVETest, RestoreUVE_MalformedComponentData_RollsBackCreatedEntities) {
    const EntityUVE existing = entityManager.CreateEntityUVE();
    const std::size_t entityCountBefore = entityManager.GetEntityCountUVE();
    const std::string payloadText =
        R"({"entities":[{"localId":0,"components":{"NameComponentUVE":{"name":"Valid"}}},{"localId":1,"components":{"MeshComponentUVE":{"meshGuid":5}}}]})";
    const auto* const payloadBytes = reinterpret_cast<const std::byte*>(payloadText.data());
    const SceneSnapshotUVE snapshot{
        Asset::EncodeUveFileEnvelopeUVE(SceneAssetTypeUVE::Scene,
                                        std::vector<std::byte>{payloadBytes, payloadBytes + payloadText.size()}),
        SceneAssetTypeUVE::Scene};

    const std::vector<EntityUVE> roots = serializer.RestoreUVE(entityManager, snapshot);

    EXPECT_TRUE(roots.empty());
    EXPECT_TRUE(entityManager.IsAliveUVE(existing));
    EXPECT_EQ(entityManager.GetEntityCountUVE(), entityCountBefore);
}

TEST_F(SceneSerializerUVETest, RestoreUVE_InvalidPrimitivePayload_RollsBackCreatedEntities) {
    const EntityUVE existing = entityManager.CreateEntityUVE();
    const std::size_t entityCountBefore = entityManager.GetEntityCountUVE();
    const std::string payloadText =
        R"({"entities":[{"localId":0,"components":{"PrimitiveMeshComponentUVE":{"kind":99,"baseColor":[0.5,0.5,0.5]}}}]})";
    const auto* const payloadBytes = reinterpret_cast<const std::byte*>(payloadText.data());
    const SceneSnapshotUVE snapshot{
        Asset::EncodeUveFileEnvelopeUVE(SceneAssetTypeUVE::Scene,
                                        std::vector<std::byte>{payloadBytes, payloadBytes + payloadText.size()}),
        SceneAssetTypeUVE::Scene};

    const std::vector<EntityUVE> roots = serializer.RestoreUVE(entityManager, snapshot);

    EXPECT_TRUE(roots.empty());
    EXPECT_TRUE(entityManager.IsAliveUVE(existing));
    EXPECT_EQ(entityManager.GetEntityCountUVE(), entityCountBefore);
}

TEST_F(SceneSerializerUVETest, RestoreUVE_InvalidCameraPayload_RollsBackCreatedEntities) {
    const EntityUVE existing = entityManager.CreateEntityUVE();
    const std::size_t entityCountBefore = entityManager.GetEntityCountUVE();
    const std::string payloadText =
        R"({"entities":[{"localId":0,"components":{"CameraComponentUVE":{"fieldOfViewDegrees":180.0,"nearPlane":0.1,"farPlane":100.0}}}]})";
    const auto* const payloadBytes = reinterpret_cast<const std::byte*>(payloadText.data());
    const SceneSnapshotUVE snapshot{
        Asset::EncodeUveFileEnvelopeUVE(SceneAssetTypeUVE::Scene,
                                        std::vector<std::byte>{payloadBytes, payloadBytes + payloadText.size()}),
        SceneAssetTypeUVE::Scene};

    const std::vector<EntityUVE> roots = serializer.RestoreUVE(entityManager, snapshot);

    EXPECT_TRUE(roots.empty());
    EXPECT_TRUE(entityManager.IsAliveUVE(existing));
    EXPECT_EQ(entityManager.GetEntityCountUVE(), entityCountBefore);
}

TEST_F(SceneSerializerUVETest, RestoreUVE_InvalidLightPayload_RollsBackCreatedEntities) {
    const EntityUVE existing = entityManager.CreateEntityUVE();
    const std::size_t entityCountBefore = entityManager.GetEntityCountUVE();
    const std::string payloadText =
        R"({"entities":[{"localId":0,"components":{"LightComponentUVE":{"color":[1.0,1.0,-0.1],"intensity":2.0,"type":0,"range":10.0,"spotAngleDegrees":45.0}}}]})";
    const auto* const payloadBytes = reinterpret_cast<const std::byte*>(payloadText.data());
    const SceneSnapshotUVE snapshot{
        Asset::EncodeUveFileEnvelopeUVE(SceneAssetTypeUVE::Scene,
                                        std::vector<std::byte>{payloadBytes, payloadBytes + payloadText.size()}),
        SceneAssetTypeUVE::Scene};

    const std::vector<EntityUVE> roots = serializer.RestoreUVE(entityManager, snapshot);

    EXPECT_TRUE(roots.empty());
    EXPECT_TRUE(entityManager.IsAliveUVE(existing));
    EXPECT_EQ(entityManager.GetEntityCountUVE(), entityCountBefore);
}

TEST_F(SceneSerializerUVETest, RestoreUVE_InvalidColliderPayload_RollsBackCreatedEntities) {
    const EntityUVE existing = entityManager.CreateEntityUVE();
    const std::size_t entityCountBefore = entityManager.GetEntityCountUVE();
    const std::string payloadText =
        R"({"entities":[{"localId":0,"components":{"ColliderComponentUVE":{"halfExtents":[0.5,0.0,0.5],"collisionLayer":1,"collisionMask":4294967295,"friction":0.0,"restitution":0.0,"density":1.0}}}]})";
    const auto* const payloadBytes = reinterpret_cast<const std::byte*>(payloadText.data());
    const SceneSnapshotUVE snapshot{
        Asset::EncodeUveFileEnvelopeUVE(SceneAssetTypeUVE::Scene,
                                        std::vector<std::byte>{payloadBytes, payloadBytes + payloadText.size()}),
        SceneAssetTypeUVE::Scene};

    const std::vector<EntityUVE> roots = serializer.RestoreUVE(entityManager, snapshot);

    EXPECT_TRUE(roots.empty());
    EXPECT_TRUE(entityManager.IsAliveUVE(existing));
    EXPECT_EQ(entityManager.GetEntityCountUVE(), entityCountBefore);
}

TEST_F(SceneSerializerUVETest, SaveThenLoad_SingleEntityWithMultipleComponents_RoundTripsExactly) {
    const EntityUVE entity = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<MeshComponentUVE>(
        entity, MeshComponentUVE{Asset::AssetGuidUVE{111}, Asset::AssetGuidUVE{222}});
    entityManager.AddComponentUVE<LightComponentUVE>(
        entity, LightComponentUVE{Math::Vector3UVE{0.2F, 0.4F, 0.6F}, 2.5F});
    entityManager.AddComponentUVE<RigidBodyComponentUVE>(entity, RigidBodyComponentUVE{5.0F, true});

    const std::filesystem::path path = "uve_scene_serializer_tests_single.uvescene";
    std::filesystem::remove(path);
    ASSERT_TRUE(serializer.SaveUVE(entityManager, {entity}, path, SceneAssetTypeUVE::Scene));

    EntityManagerUVE loadedManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
    const std::vector<EntityUVE> roots = serializer.LoadUVE(loadedManager, path);
    ASSERT_EQ(roots.size(), 1U);
    const EntityUVE loaded = roots[0];

    EXPECT_EQ(loadedManager.GetComponentUVE<MeshComponentUVE>(loaded).meshGuid, Asset::AssetGuidUVE{111});
    EXPECT_EQ(loadedManager.GetComponentUVE<MeshComponentUVE>(loaded).materialGuid, Asset::AssetGuidUVE{222});
    EXPECT_FLOAT_EQ(loadedManager.GetComponentUVE<LightComponentUVE>(loaded).intensity, 2.5F);
    const Math::Vector3UVE expectedColor{0.2F, 0.4F, 0.6F};
    EXPECT_TRUE(loadedManager.GetComponentUVE<LightComponentUVE>(loaded).color == expectedColor);
    EXPECT_FLOAT_EQ(loadedManager.GetComponentUVE<RigidBodyComponentUVE>(loaded).mass, 5.0F);
    EXPECT_TRUE(loadedManager.GetComponentUVE<RigidBodyComponentUVE>(loaded).isKinematic);

    std::filesystem::remove(path);
}

TEST_F(SceneSerializerUVETest, SaveThenLoad_NameComponentUVE_RoundTripsExactly) {
    const EntityUVE entity = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<NameComponentUVE>(entity, NameComponentUVE{"Gameplay Root"});

    const std::filesystem::path path = "uve_scene_serializer_tests_name.uvescene";
    std::filesystem::remove(path);
    ASSERT_TRUE(serializer.SaveUVE(entityManager, {entity}, path, SceneAssetTypeUVE::Scene));

    EntityManagerUVE loadedManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
    const std::vector<EntityUVE> roots = serializer.LoadUVE(loadedManager, path);
    ASSERT_EQ(roots.size(), 1U);
    ASSERT_TRUE(loadedManager.HasComponentUVE<NameComponentUVE>(roots[0]));
    EXPECT_EQ(loadedManager.GetComponentUVE<NameComponentUVE>(roots[0]).name, "Gameplay Root");

    std::filesystem::remove(path);
}

TEST_F(SceneSerializerUVETest, LoadUVE_LegacyDocumentWithoutNameComponent_RemainsValid) {
    const std::string payloadText =
        R"({"entities":[{"localId":0,"components":{"TransformComponentUVE":{"localPosition":[0.0,0.0,0.0],"localRotation":[0.0,0.0,0.0,1.0],"localScale":[1.0,1.0,1.0]}}}]})";
    const auto* const payloadBytesPtr = reinterpret_cast<const std::byte*>(payloadText.data());
    const std::vector<std::byte> payloadBytes(payloadBytesPtr, payloadBytesPtr + payloadText.size());

    const std::filesystem::path path = "uve_scene_serializer_tests_name_legacy.uvescene";
    std::filesystem::remove(path);
    ASSERT_TRUE(Asset::WriteUveFileUVE(path, SceneAssetTypeUVE::Scene, payloadBytes));

    const std::vector<EntityUVE> roots = serializer.LoadUVE(entityManager, path);
    ASSERT_EQ(roots.size(), 1U);
    EXPECT_FALSE(entityManager.HasComponentUVE<NameComponentUVE>(roots[0]));

    std::filesystem::remove(path);
}

TEST_F(SceneSerializerUVETest, SaveThenLoad_ColliderComponentUVE_RoundTripsFrictionRestitutionDensity) {
    const EntityUVE entity = entityManager.CreateEntityUVE();
    ColliderComponentUVE collider{Math::Vector3UVE{0.5F, 0.5F, 0.5F}};
    collider.collisionLayer = 2;
    collider.collisionMask = 0x0000FFFFU;
    collider.friction = 0.4F;
    collider.restitution = 0.9F;
    collider.density = 2.5F;
    entityManager.AddComponentUVE<ColliderComponentUVE>(entity, collider);

    const std::filesystem::path path = "uve_scene_serializer_tests_collider.uvescene";
    std::filesystem::remove(path);
    ASSERT_TRUE(serializer.SaveUVE(entityManager, {entity}, path, SceneAssetTypeUVE::Scene));

    EntityManagerUVE loadedManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
    const std::vector<EntityUVE> roots = serializer.LoadUVE(loadedManager, path);
    ASSERT_EQ(roots.size(), 1U);
    const ColliderComponentUVE& loaded = loadedManager.GetComponentUVE<ColliderComponentUVE>(roots[0]);

    EXPECT_EQ(loaded.collisionLayer, 2U);
    EXPECT_EQ(loaded.collisionMask, 0x0000FFFFU);
    EXPECT_FLOAT_EQ(loaded.friction, 0.4F);
    EXPECT_FLOAT_EQ(loaded.restitution, 0.9F);
    EXPECT_FLOAT_EQ(loaded.density, 2.5F);

    std::filesystem::remove(path);
}

TEST_F(SceneSerializerUVETest, SaveThenLoad_LightComponentUVE_RoundTripsTypeRangeSpotAngle) {
    const EntityUVE entity = entityManager.CreateEntityUVE();
    LightComponentUVE light;
    light.color = Math::Vector3UVE{0.9F, 0.8F, 0.7F};
    light.intensity = 3.5F;
    light.type = LightTypeUVE::Spot;
    light.range = 15.0F;
    light.spotAngleDegrees = 30.0F;
    entityManager.AddComponentUVE<LightComponentUVE>(entity, light);

    const std::filesystem::path path = "uve_scene_serializer_tests_light.uvescene";
    std::filesystem::remove(path);
    ASSERT_TRUE(serializer.SaveUVE(entityManager, {entity}, path, SceneAssetTypeUVE::Scene));

    EntityManagerUVE loadedManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
    const std::vector<EntityUVE> roots = serializer.LoadUVE(loadedManager, path);
    ASSERT_EQ(roots.size(), 1U);
    const LightComponentUVE& loaded = loadedManager.GetComponentUVE<LightComponentUVE>(roots[0]);

    EXPECT_TRUE(loaded.color == light.color);
    EXPECT_FLOAT_EQ(loaded.intensity, 3.5F);
    EXPECT_EQ(loaded.type, LightTypeUVE::Spot);
    EXPECT_FLOAT_EQ(loaded.range, 15.0F);
    EXPECT_FLOAT_EQ(loaded.spotAngleDegrees, 30.0F);

    std::filesystem::remove(path);
}

TEST_F(SceneSerializerUVETest, LoadUVE_OldFormatLightComponentUVEMissingNewFields_FillsInDefaults) {
    // Hand-built payload matching the pre-Increment-25 LightComponentUVE JSON shape (only
    // "color"/"intensity" - no "type"/"range"/"spotAngleDegrees" keys), proving old saves still
    // load correctly via the fromJson lambda's json.value(...) backward-compat defaults. Written
    // as a raw string literal (not nlohmann::json) since nlohmann_json is deliberately linked
    // PRIVATE to uve_scene, confined to scene_serializer_uve.cpp - not available to test code.
    const std::string payloadText =
        R"({"entities":[{"localId":0,"components":{"LightComponentUVE":{"color":[0.1,0.2,0.3],"intensity":4.0}}}]})";
    const auto* const payloadBytesPtr = reinterpret_cast<const std::byte*>(payloadText.data());
    const std::vector<std::byte> payloadBytes(payloadBytesPtr, payloadBytesPtr + payloadText.size());

    const std::filesystem::path path = "uve_scene_serializer_tests_light_old_format.uvescene";
    std::filesystem::remove(path);
    ASSERT_TRUE(Asset::WriteUveFileUVE(path, SceneAssetTypeUVE::Scene, payloadBytes));

    const std::vector<EntityUVE> roots = serializer.LoadUVE(entityManager, path);
    ASSERT_EQ(roots.size(), 1U);
    const LightComponentUVE& loaded = entityManager.GetComponentUVE<LightComponentUVE>(roots[0]);

    EXPECT_TRUE(loaded.color == (Math::Vector3UVE{0.1F, 0.2F, 0.3F}));
    EXPECT_FLOAT_EQ(loaded.intensity, 4.0F);
    EXPECT_EQ(loaded.type, LightTypeUVE::Directional); // default
    EXPECT_FLOAT_EQ(loaded.range, 10.0F);               // default
    EXPECT_FLOAT_EQ(loaded.spotAngleDegrees, 45.0F);     // default

    std::filesystem::remove(path);
}

TEST_F(SceneSerializerUVETest, SaveThenLoad_AudioSourceComponentUVE_RoundTripsAllFields) {
    const EntityUVE entity = entityManager.CreateEntityUVE();
    AudioSourceComponentUVE audioSource;
    audioSource.audioAssetPath = "sounds/explosion.wav";
    audioSource.volume = 0.6F;
    audioSource.looping = true;
    audioSource.pitch = 1.5F;
    audioSource.spatial = false;
    audioSource.minDistance = 2.0F;
    audioSource.maxDistance = 50.0F;
    audioSource.attenuationCurve = AudioAttenuationCurveUVE::InverseSquare;
    audioSource.playOnAwake = false;
    entityManager.AddComponentUVE<AudioSourceComponentUVE>(entity, audioSource);

    const std::filesystem::path path = "uve_scene_serializer_tests_audio_source.uvescene";
    std::filesystem::remove(path);
    ASSERT_TRUE(serializer.SaveUVE(entityManager, {entity}, path, SceneAssetTypeUVE::Scene));

    EntityManagerUVE loadedManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
    const std::vector<EntityUVE> roots = serializer.LoadUVE(loadedManager, path);
    ASSERT_EQ(roots.size(), 1U);
    const AudioSourceComponentUVE& loaded = loadedManager.GetComponentUVE<AudioSourceComponentUVE>(roots[0]);

    EXPECT_EQ(loaded.audioAssetPath, "sounds/explosion.wav");
    EXPECT_FLOAT_EQ(loaded.volume, 0.6F);
    EXPECT_TRUE(loaded.looping);
    EXPECT_FLOAT_EQ(loaded.pitch, 1.5F);
    EXPECT_FALSE(loaded.spatial);
    EXPECT_FLOAT_EQ(loaded.minDistance, 2.0F);
    EXPECT_FLOAT_EQ(loaded.maxDistance, 50.0F);
    EXPECT_EQ(loaded.attenuationCurve, AudioAttenuationCurveUVE::InverseSquare);
    EXPECT_FALSE(loaded.playOnAwake);

    std::filesystem::remove(path);
}

TEST_F(SceneSerializerUVETest, SaveThenLoad_AudioSourceComponentUVE_DefaultsRoundTripCorrectly) {
    // Confirms every new field's default survives a save/load cycle, matching what a scene
    // serialized before this increment's fields existed would fall back to on load, via the
    // json.value(key, default) idiom in the fromJson registration.
    const EntityUVE entity = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<AudioSourceComponentUVE>(entity, AudioSourceComponentUVE{});

    const std::filesystem::path path = "uve_scene_serializer_tests_audio_source_defaults.uvescene";
    std::filesystem::remove(path);
    ASSERT_TRUE(serializer.SaveUVE(entityManager, {entity}, path, SceneAssetTypeUVE::Scene));

    EntityManagerUVE loadedManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
    const std::vector<EntityUVE> roots = serializer.LoadUVE(loadedManager, path);
    ASSERT_EQ(roots.size(), 1U);
    const AudioSourceComponentUVE& loaded = loadedManager.GetComponentUVE<AudioSourceComponentUVE>(roots[0]);

    EXPECT_FALSE(loaded.looping);
    EXPECT_FLOAT_EQ(loaded.pitch, 1.0F);
    EXPECT_TRUE(loaded.spatial);
    EXPECT_FLOAT_EQ(loaded.minDistance, 1.0F);
    EXPECT_FLOAT_EQ(loaded.maxDistance, 25.0F);
    EXPECT_EQ(loaded.attenuationCurve, AudioAttenuationCurveUVE::Linear);
    EXPECT_TRUE(loaded.playOnAwake);

    std::filesystem::remove(path);
}

TEST_F(SceneSerializerUVETest, SaveThenLoad_Hierarchy_RemapsParentCorrectly) {
    const EntityUVE parent = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<HierarchyComponentUVE>(parent, HierarchyComponentUVE{kInvalidEntityUVE});
    const EntityUVE child = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<HierarchyComponentUVE>(child, HierarchyComponentUVE{parent});

    const std::filesystem::path path = "uve_scene_serializer_tests_hierarchy.uvescene";
    std::filesystem::remove(path);
    ASSERT_TRUE(serializer.SaveUVE(entityManager, {parent}, path, SceneAssetTypeUVE::Scene));

    EntityManagerUVE loadedManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
    const std::vector<EntityUVE> roots = serializer.LoadUVE(loadedManager, path);
    ASSERT_EQ(roots.size(), 1U);
    const EntityUVE loadedParent = roots[0];

    EntityUVE loadedChild = kInvalidEntityUVE;
    loadedManager.ForEachUVE<HierarchyComponentUVE>(
        [&loadedChild, loadedParent](EntityUVE entity, HierarchyComponentUVE& hierarchy) {
            if (hierarchy.parent == loadedParent) {
                loadedChild = entity;
            }
        });
    EXPECT_NE(loadedChild, kInvalidEntityUVE);

    std::filesystem::remove(path);
}

TEST_F(SceneSerializerUVETest, SaveThenLoad_MultipleRoots_AllPresentInFileOrder) {
    const EntityUVE rootA = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<MeshComponentUVE>(rootA, MeshComponentUVE{Asset::AssetGuidUVE{1}, Asset::AssetGuidUVE{2}});
    const EntityUVE rootB = entityManager.CreateEntityUVE();
    entityManager.AddComponentUVE<MeshComponentUVE>(rootB, MeshComponentUVE{Asset::AssetGuidUVE{3}, Asset::AssetGuidUVE{4}});

    const std::filesystem::path path = "uve_scene_serializer_tests_multi_root.uvescene";
    std::filesystem::remove(path);
    ASSERT_TRUE(serializer.SaveUVE(entityManager, {rootA, rootB}, path, SceneAssetTypeUVE::Scene));

    EntityManagerUVE loadedManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
    const std::vector<EntityUVE> roots = serializer.LoadUVE(loadedManager, path);
    ASSERT_EQ(roots.size(), 2U);
    EXPECT_EQ(loadedManager.GetComponentUVE<MeshComponentUVE>(roots[0]).meshGuid, Asset::AssetGuidUVE{1});
    EXPECT_EQ(loadedManager.GetComponentUVE<MeshComponentUVE>(roots[1]).meshGuid, Asset::AssetGuidUVE{3});

    std::filesystem::remove(path);
}

TEST_F(SceneSerializerUVETest, SaveUVE_NeverSerializesWorldTransformComponent_AndSceneGraphRecomputesAfterLoad) {
    SceneGraphUVE sceneGraph;
    const EntityUVE entity = entityManager.CreateEntityUVE();
    TransformComponentUVE local;
    local.localPosition = Math::Vector3UVE{1.0F, 2.0F, 3.0F};
    sceneGraph.AttachTransformUVE(entityManager, entity, local);
    sceneGraph.UpdateUVE(entityManager);

    const std::filesystem::path path = "uve_scene_serializer_tests_no_world_transform.uvescene";
    std::filesystem::remove(path);
    ASSERT_TRUE(serializer.SaveUVE(entityManager, {entity}, path, SceneAssetTypeUVE::Scene));

    std::ifstream rawFile(path, std::ios::binary);
    const std::string contents((std::istreambuf_iterator<char>(rawFile)), std::istreambuf_iterator<char>());
    EXPECT_EQ(contents.find("WorldTransformComponentUVE"), std::string::npos);

    EntityManagerUVE loadedManager(memoryManager.GetDefaultAllocatorUVE(), eventSystem);
    const std::vector<EntityUVE> roots = serializer.LoadUVE(loadedManager, path);
    ASSERT_EQ(roots.size(), 1U);
    sceneGraph.UpdateUVE(loadedManager);

    const WorldTransformComponentUVE& world =
        loadedManager.GetComponentUVE<WorldTransformComponentUVE>(roots[0]);
    EXPECT_TRUE(world.worldPosition == local.localPosition);

    std::filesystem::remove(path);
}

TEST_F(SceneSerializerUVETest, LoadUVE_MissingFile_ReturnsEmptyVector) {
    const std::vector<EntityUVE> roots =
        serializer.LoadUVE(entityManager, "uve_scene_serializer_tests_nonexistent.uvescene");
    EXPECT_TRUE(roots.empty());
}

TEST_F(SceneSerializerUVETest, LoadUVE_BadMagic_ReturnsEmptyAndLogsError) {
    const std::filesystem::path path = "uve_scene_serializer_tests_bad_magic.uvescene";
    {
        std::ofstream file(path, std::ios::binary);
        file << "NOT A VALID UVE FILE AT ALL";
    }

    Debug::LoggerUVE logger;
    logger.Init(Debug::LogLevelUVE::Trace);
    auto memorySink = std::make_unique<Debug::MemorySinkUVE>();
    Debug::MemorySinkUVE* const memorySinkPtr = memorySink.get();
    logger.AddSink(std::move(memorySink));

    const std::vector<EntityUVE> roots = serializer.LoadUVE(entityManager, path);
    EXPECT_TRUE(roots.empty());

    const std::vector<Debug::LogMessageUVE> messages = memorySinkPtr->GetMessagesUVE();
    const bool foundError =
        std::any_of(messages.begin(), messages.end(), [](const Debug::LogMessageUVE& message) {
            return message.level == Debug::LogLevelUVE::Error &&
                   message.message.find("bad magic") != std::string::npos;
        });
    EXPECT_TRUE(foundError);

    logger.Shutdown();
    std::filesystem::remove(path);
}

} // namespace
} // namespace UVE::Scene::Tests
