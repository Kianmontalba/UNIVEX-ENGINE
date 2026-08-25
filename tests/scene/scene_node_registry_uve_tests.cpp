// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <algorithm>
#include <string_view>
#include <type_traits>
#include <unordered_set>

#include <gtest/gtest.h>

#include "uve/scene/nodes/animation_player_node_uve.h"
#include "uve/scene/nodes/animation_tree_node_uve.h"
#include "uve/scene/nodes/audio_source_3d_node_uve.h"
#include "uve/scene/nodes/box_mesh_3d_node_uve.h"
#include "uve/scene/nodes/camera_3d_node_uve.h"
#include "uve/scene/nodes/character_body_3d_node_uve.h"
#include "uve/scene/nodes/collision_shape_3d_node_uve.h"
#include "uve/scene/nodes/empty_node_uve.h"
#include "uve/scene/nodes/light_3d_node_uve.h"
#include "uve/scene/nodes/mesh_instance_3d_node_uve.h"
#include "uve/scene/nodes/particle_emitter_3d_node_uve.h"
#include "uve/scene/nodes/plane_mesh_3d_node_uve.h"
#include "uve/scene/nodes/rigid_body_3d_node_uve.h"
#include "uve/scene/nodes/scene_node_uve.h"
#include "uve/scene/nodes/sphere_mesh_3d_node_uve.h"
#include "uve/scene/nodes/script_node_uve.h"
#include "uve/scene/nodes/transform_node_uve.h"

namespace UVE::Scene::Nodes::Tests {
namespace {

TEST(SceneNodeRegistryUVETest, BuiltInDescriptorsUVE_AreStableUniqueAndRuntimeBound) {
    const std::span<const SceneNodeDescriptorUVE> descriptors = GetSceneNodeDescriptorsUVE();
    ASSERT_EQ(descriptors.size(), 15U);

    std::unordered_set<std::string_view> ids;
    for (const SceneNodeDescriptorUVE& descriptor : descriptors) {
        EXPECT_TRUE(ids.insert(descriptor.typeId).second);
        EXPECT_FALSE(descriptor.typeId.empty());
        EXPECT_FALSE(descriptor.displayName.empty());
        EXPECT_FALSE(descriptor.category.empty());
        EXPECT_FALSE(descriptor.runtimeOwner.empty());
        EXPECT_LE(descriptor.authoredContracts.size(), 8U);
        if (descriptor.kind == SceneNodeKindUVE::AnimationTree) {
            EXPECT_FALSE(descriptor.libraryCreatable);
        } else {
            EXPECT_TRUE(descriptor.libraryCreatable);
        }
        EXPECT_EQ(FindSceneNodeDescriptorUVE(descriptor.kind), &descriptor);
        EXPECT_EQ(FindSceneNodeDescriptorUVE(descriptor.typeId), &descriptor);
        EXPECT_EQ(GetSceneNodeTypeIdUVE(descriptor.kind), descriptor.typeId);
    }
}

TEST(SceneNodeRegistryUVETest, NodeFacadeAliasesUVE_ExposeExistingRuntimeContracts) {
    static_assert(std::is_same_v<AnimationTreeNodeUVE, Core::AnimationTreeUVE>);
    static_assert(std::is_same_v<AnimationTreeNodeFacadeUVE, Core::AnimationTreeUVE>);
    static_assert(std::is_same_v<AnimationPlayerNodeUVE, AnimationPlayerComponentUVE>);
    static_assert(std::is_same_v<AnimationPlayerNodeFacadeUVE, AnimationPlayerComponentUVE>);
    static_assert(std::is_same_v<Camera3DNodeUVE, CameraComponentUVE>);
    static_assert(std::is_same_v<Camera3DNodeFacadeUVE, CameraComponentUVE>);
    static_assert(std::is_same_v<CharacterBody3DNodeUVE, Physics::CharacterControllerInputUVE>);
    static_assert(std::is_same_v<CharacterBody3DNodeFacadeUVE, Physics::CharacterControllerInputUVE>);
    static_assert(std::is_same_v<CollisionShape3DNodeUVE, ColliderComponentUVE>);
    static_assert(std::is_same_v<Collider3DNodeFacadeUVE, ColliderComponentUVE>);
    static_assert(std::is_same_v<MeshInstance3DNodeUVE, MeshComponentUVE>);
    static_assert(std::is_same_v<BoxMesh3DNodeUVE, PrimitiveMeshComponentUVE>);
    static_assert(std::is_same_v<SphereMesh3DNodeUVE, PrimitiveMeshComponentUVE>);
    static_assert(std::is_same_v<PlaneMesh3DNodeUVE, PrimitiveMeshComponentUVE>);
    static_assert(std::is_same_v<Light3DNodeUVE, LightComponentUVE>);
    static_assert(std::is_same_v<RigidBody3DNodeUVE, RigidBodyComponentUVE>);
    static_assert(std::is_same_v<AudioSource3DNodeUVE, AudioSourceComponentUVE>);
    static_assert(std::is_same_v<ParticleEmitter3DNodeUVE, ParticleEmitterComponentUVE>);
    static_assert(std::is_same_v<ScriptNodeUVE, ScriptComponentUVE>);
    static_assert(std::is_same_v<TransformNodeUVE, TransformComponentUVE>);
    static_assert(std::is_same_v<EmptyNodeUVE, EntityUVE>);
    SUCCEED();
}

TEST(SceneNodeRegistryUVETest, UnknownLookupUVE_ReturnsEmptyOrNull) {
    EXPECT_EQ(FindSceneNodeDescriptorUVE(std::string_view{"missing_node"}), nullptr);
    EXPECT_EQ(GetSceneNodeTypeIdUVE(static_cast<SceneNodeKindUVE>(255U)), std::string_view{});
}

} // namespace
} // namespace UVE::Scene::Nodes::Tests
