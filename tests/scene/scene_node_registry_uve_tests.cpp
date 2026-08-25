// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include <algorithm>
#include <string_view>
#include <type_traits>
#include <unordered_set>

#include <gtest/gtest.h>

#include "uve/scene/nodes/scene_node_uve.h"

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
    static_assert(std::is_same_v<AnimationTreeNodeFacadeUVE, Core::AnimationTreeUVE>);
    static_assert(std::is_same_v<AnimationPlayerNodeFacadeUVE, AnimationPlayerComponentUVE>);
    static_assert(std::is_same_v<Camera3DNodeFacadeUVE, CameraComponentUVE>);
    static_assert(std::is_same_v<CharacterBody3DNodeFacadeUVE, Physics::CharacterControllerInputUVE>);
    static_assert(std::is_same_v<RigidBody3DNodeFacadeUVE, RigidBodyComponentUVE>);
    SUCCEED();
}

TEST(SceneNodeRegistryUVETest, UnknownLookupUVE_ReturnsEmptyOrNull) {
    EXPECT_EQ(FindSceneNodeDescriptorUVE(std::string_view{"missing_node"}), nullptr);
    EXPECT_EQ(GetSceneNodeTypeIdUVE(static_cast<SceneNodeKindUVE>(255U)), std::string_view{});
}

} // namespace
} // namespace UVE::Scene::Nodes::Tests
