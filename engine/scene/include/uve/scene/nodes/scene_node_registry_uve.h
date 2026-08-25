// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace UVE::Scene::Nodes {

enum class SceneNodeKindUVE : std::uint8_t {
    Empty = 0,
    AnimationTree,
    AnimationPlayer,
    CharacterBody3D,
    Camera3D,
    MeshInstance3D,
    BoxMesh3D,
    SphereMesh3D,
    PlaneMesh3D,
    Light3D,
    Collider3D,
    RigidBody3D,
    AudioSource3D,
    ParticleEmitter3D,
    Script,
};

struct SceneNodeDescriptorUVE final {
    SceneNodeKindUVE kind = SceneNodeKindUVE::Empty;
    std::string_view typeId;
    std::string_view displayName;
    std::string_view category;
    std::string_view runtimeOwner;
    std::span<const std::string_view> authoredContracts;
    bool libraryCreatable = false;
};

inline constexpr std::size_t kMaximumSceneNodeDescriptorsUVE = 32U;

[[nodiscard]] std::span<const SceneNodeDescriptorUVE> GetSceneNodeDescriptorsUVE() noexcept;
[[nodiscard]] const SceneNodeDescriptorUVE* FindSceneNodeDescriptorUVE(
    SceneNodeKindUVE kind) noexcept;
[[nodiscard]] const SceneNodeDescriptorUVE* FindSceneNodeDescriptorUVE(
    std::string_view typeId) noexcept;
[[nodiscard]] std::string_view GetSceneNodeTypeIdUVE(SceneNodeKindUVE kind) noexcept;

} // namespace UVE::Scene::Nodes
