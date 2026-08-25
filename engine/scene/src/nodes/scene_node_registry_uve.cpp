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
constexpr std::array<std::string_view, 1U> kAreaContracts{"AreaComponentUVE"};
constexpr std::array<std::string_view, 1U> kRayCastContracts{"RayCast3DNodeComponentUVE"};
constexpr std::array<std::string_view, 1U> kAnimatableBodyContracts{"AnimatableBody3DNodeComponentUVE"};
constexpr std::array<std::string_view, 1U> kNavigationRegionContracts{"NavigationRegion3DNodeComponentUVE"};
constexpr std::array<std::string_view, 1U> kNavigationAgentContracts{"NavigationAgent3DNodeComponentUVE"};
constexpr std::array<std::string_view, 1U> kSkeletonContracts{"Skeleton3DNodeComponentUVE"};
constexpr std::array<std::string_view, 1U> kBoneAttachmentContracts{"BoneAttachment3DNodeComponentUVE"};
constexpr std::array<std::string_view, 1U> kSpringArmContracts{"SpringArm3DNodeComponentUVE"};
constexpr std::array<std::string_view, 1U> kMarkerContracts{"Marker3DNodeComponentUVE"};
constexpr std::array<std::string_view, 1U> kHitboxContracts{"Hitbox3DNodeComponentUVE"};
constexpr std::array<std::string_view, 1U> kHurtboxContracts{"Hurtbox3DNodeComponentUVE"};
constexpr std::array<std::string_view, 1U> kProjectileContracts{"Projectile3DNodeComponentUVE"};
constexpr std::array<std::string_view, 1U> kInteractionAreaContracts{"InteractionArea3DNodeComponentUVE"};
constexpr std::array<std::string_view, 1U> kEnvironmentContracts{"WorldEnvironment3DNodeComponentUVE"};
constexpr std::array<std::string_view, 1U> kReflectionProbeContracts{"ReflectionProbe3DNodeComponentUVE"};
constexpr std::array<std::string_view, 1U> kDecalContracts{"Decal3DNodeComponentUVE"};
constexpr std::array<std::string_view, 1U> kLodContracts{"LodGroup3DNodeComponentUVE"};
constexpr std::array<std::string_view, 1U> kOccluderContracts{"Occluder3DNodeComponentUVE"};
constexpr std::array<std::string_view, 1U> kVisibilityContracts{"VisibilityRegion3DNodeComponentUVE"};
constexpr std::array<std::string_view, 1U> kSpawnContracts{"SpawnPoint3DNodeComponentUVE"};
constexpr std::array<std::string_view, 1U> kStreamerContracts{"LevelStreamer3DNodeComponentUVE"};
constexpr std::array<std::string_view, 1U> kPartitionContracts{"WorldPartition3DNodeComponentUVE"};
constexpr std::array<std::string_view, 1U> kRigidBodyContracts{"RigidBodyComponentUVE"};
constexpr std::array<std::string_view, 1U> kAudioContracts{"AudioSourceComponentUVE"};
constexpr std::array<std::string_view, 1U> kParticleContracts{"ParticleEmitterComponentUVE"};
constexpr std::array<std::string_view, 1U> kScriptContracts{"ScriptComponentUVE"};

constexpr std::array<SceneNodeDescriptorUVE, 38U> kDescriptors{
    SceneNodeDescriptorUVE{SceneNodeKindUVE::Empty, "empty", "Empty", "Scene", "Scene/ECS", kNoContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::Area3D, "area_3d", "Area3D", "Physics", "Physics/AreaOverlapSystemUVE", kAreaContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::RayCast3D, "ray_cast_3d", "RayCast3D", "Physics", "Physics/RaycastSystemUVE", kRayCastContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::StaticBody3D, "static_body_3d", "StaticBody3D", "Physics", "Physics/CollisionSystemUVE", kColliderContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::AnimatableBody3D, "animatable_body_3d", "AnimatableBody3D", "Physics", "Scene/AnimatableBody3DNodeComponentUVE", kAnimatableBodyContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::NavigationRegion3D, "navigation_region_3d", "NavigationRegion3D", "Navigation", "Scene/NavigationRegion3DNodeComponentUVE", kNavigationRegionContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::NavigationAgent3D, "navigation_agent_3d", "NavigationAgent3D", "Navigation", "Scene/NavigationAgent3DNodeComponentUVE", kNavigationAgentContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::Skeleton3D, "skeleton_3d", "Skeleton3D", "Animation", "Scene/Skeleton3DNodeComponentUVE", kSkeletonContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::BoneAttachment3D, "bone_attachment_3d", "BoneAttachment3D", "Animation", "Scene/BoneAttachment3DNodeComponentUVE", kBoneAttachmentContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::SpringArm3D, "spring_arm_3d", "SpringArm3D", "Camera", "Physics/RaycastSystemUVE", kSpringArmContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::Marker3D, "marker_3d", "Marker3D", "Scene", "Scene/Marker3DNodeComponentUVE", kMarkerContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::Hitbox3D, "hitbox_3d", "Hitbox3D", "Combat", "Physics/Hitbox3DNodeComponentUVE", kHitboxContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::Hurtbox3D, "hurtbox_3d", "Hurtbox3D", "Combat", "Physics/Hurtbox3DNodeComponentUVE", kHurtboxContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::Projectile3D, "projectile_3d", "Projectile3D", "Combat", "Physics/Projectile3DNodeComponentUVE", kProjectileContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::InteractionArea3D, "interaction_area_3d", "InteractionArea3D", "Gameplay", "Physics/AreaOverlapSystemUVE", kInteractionAreaContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::WorldEnvironment3D, "world_environment_3d", "WorldEnvironment3D", "Rendering", "Render/WorldEnvironment3DNodeComponentUVE", kEnvironmentContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::ReflectionProbe3D, "reflection_probe_3d", "ReflectionProbe3D", "Rendering", "Render/ReflectionProbe3DNodeComponentUVE", kReflectionProbeContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::Decal3D, "decal_3d", "Decal3D", "Rendering", "Render/Decal3DNodeComponentUVE", kDecalContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::LODGroup3D, "lod_group_3d", "LODGroup3D", "Optimization", "Render/LODGroup3DNodeComponentUVE", kLodContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::Occluder3D, "occluder_3d", "Occluder3D", "Optimization", "Render/Occluder3DNodeComponentUVE", kOccluderContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::VisibilityRegion3D, "visibility_region_3d", "VisibilityRegion3D", "Optimization", "Render/VisibilityRegion3DNodeComponentUVE", kVisibilityContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::SpawnPoint3D, "spawn_point_3d", "SpawnPoint3D", "Gameplay", "Scene/SpawnPoint3DNodeComponentUVE", kSpawnContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::LevelStreamer3D, "level_streamer_3d", "LevelStreamer3D", "World", "Scene/LevelStreamer3DNodeComponentUVE", kStreamerContracts, true},
    SceneNodeDescriptorUVE{SceneNodeKindUVE::WorldPartition3D, "world_partition_3d", "WorldPartition3D", "World", "Scene/WorldPartition3DNodeComponentUVE", kPartitionContracts, true},
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
