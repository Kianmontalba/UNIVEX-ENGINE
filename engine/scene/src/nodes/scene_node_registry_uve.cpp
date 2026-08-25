// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/scene/nodes/scene_node_registry_uve.h"

namespace UVE::Scene::Nodes {
namespace {

constexpr std::array<std::string_view, 0U> kNoContracts{};
constexpr std::array<std::string_view, 1U> kAnimationTreeContracts{"Core::AnimationTreeUVE"};
constexpr std::array<std::string_view, 1U> kAnimationPlayerContracts{"AnimationPlayerComponentUVE"};
constexpr std::array<std::string_view, 2U> kCharacterContracts{
    "TransformComponentUVE", "ColliderComponentUVE"};
constexpr std::array<std::string_view, 1U> kCameraContracts{"CameraComponentUVE"};
constexpr std::array<std::string_view, 1U> kMeshContracts{"MeshComponentUVE"};
constexpr std::array<std::string_view, 1U> kLightContracts{"LightComponentUVE"};
constexpr std::array<std::string_view, 1U> kColliderContracts{"ColliderComponentUVE"};
constexpr std::array<std::string_view, 1U> kRigidBodyContracts{"RigidBodyComponentUVE"};
constexpr std::array<std::string_view, 1U> kAudioContracts{"AudioSourceComponentUVE"};
constexpr std::array<std::string_view, 1U> kParticleContracts{"ParticleEmitterComponentUVE"};
constexpr std::array<std::string_view, 1U> kScriptContracts{"ScriptComponentUVE"};

constexpr std::array<SceneNodeDescriptorUVE, 15U> kDescriptors{
    SceneNodeDescriptorUVE{SceneNodeKindUVE::Empty, "empty", "Empty", "Scene", "Scene/ECS", kNoContracts, true},
    // AnimationTree is a runtime core contract only; no authored scene component/persistence boundary exists yet.
    SceneNodeDescriptorUVE{SceneNodeKindUVE::AnimationTree, "animation_tree", "AnimationTree", "Animation", "Core/AnimationTreeUVE", kAnimationTreeContracts, false},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::AnimationPlayer, "animation_player", "AnimationPlayer", "Animation", "Scene/AnimationPlayerComponentUVE", kAnimationPlayerContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::CharacterBody3D, "character_body_3d", "CharacterBody3D", "Physics", "Physics/CharacterControllerUVE", kCharacterContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::Camera3D, "camera_3d", "Camera3D", "Rendering", "Render/CameraSystemUVE", kCameraContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::MeshInstance3D, "mesh_instance_3d", "MeshInstance3D", "Rendering", "Render/MeshRendererUVE", kMeshContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::BoxMesh3D, "box_mesh_3d", "BoxMesh3D", "Rendering", "Render/PrimitiveMesh", kNoContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::SphereMesh3D, "sphere_mesh_3d", "SphereMesh3D", "Rendering", "Render/PrimitiveMesh", kNoContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::PlaneMesh3D, "plane_mesh_3d", "PlaneMesh3D", "Rendering", "Render/PrimitiveMesh", kNoContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::Light3D, "light_3d", "Light3D", "Rendering", "Render/LightSystemUVE", kLightContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::Collider3D, "collider_3d", "Collider3D", "Physics", "Physics/CollisionSystemUVE", kColliderContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::RigidBody3D, "rigid_body_3d", "RigidBody3D", "Physics", "Physics/PhysicsSystemUVE", kRigidBodyContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::AudioSource3D, "audio_source_3d", "AudioSource3D", "Audio", "Audio/AudioSourceSystemUVE", kAudioContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::ParticleEmitter3D, "particle_emitter_3d", "ParticleEmitter3D", "VFX", "Scene/ParticleRuntimeUVE", kParticleContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::Script, "script", "Script", "Logic", "Scripting/ScriptRuntimeUVE", kScriptContracts, true},
};

} // namespace

std::span<const SceneNodeDescriptorUVE> GetSceneNodeDescriptorsUVE() noexcept {
    return kDescriptors;
}

const SceneNodeDescriptorUVE* FindSceneNodeDescriptorUVE(const SceneNodeKindUVE kind) noexcept {
    for (const SceneNodeDescriptorUVE& descriptor : kDescriptors) {
        if (descriptor.kind == kind) {
            return &descriptor;
        }
    }
    return nullptr;
}

const SceneNodeDescriptorUVE* FindSceneNodeDescriptorUVE(const std::string_view typeId) noexcept {
    for (const SceneNodeDescriptorUVE& descriptor : kDescriptors) {
        if (descriptor.typeId == typeId) {
            return &descriptor;
        }
    }
    return nullptr;
}

std::string_view GetSceneNodeTypeIdUVE(const SceneNodeKindUVE kind) noexcept {
    const SceneNodeDescriptorUVE* descriptor = FindSceneNodeDescriptorUVE(kind);
    return descriptor == nullptr ? std::string_view{} : descriptor->typeId;
}

} // namespace UVE::Scene::Nodes
