// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>
#include <utility>

#include "uve/math/quaternion_uve.h"
#include "uve/math/vector3_uve.h"

namespace UVE::Scene {

inline constexpr std::size_t kMaximum3DNodeStringLengthUVE = 256U;
inline constexpr std::size_t kMaximumRayCastExclusionsUVE = 8U;
inline constexpr std::size_t kMaximumSkeletonBonesUVE = 256U;
inline constexpr std::size_t kMaximumLodLevelsUVE = 8U;
inline constexpr std::size_t kMaximumStreamedCellsUVE = 4096U;

[[nodiscard]] inline bool IsBounded3DNodeStringUVE(const std::string& value, const bool allowEmpty = true) noexcept {
    return (allowEmpty || !value.empty()) && value.size() <= kMaximum3DNodeStringLengthUVE &&
           value.find('\0') == std::string::npos;
}

[[nodiscard]] inline bool IsFinite3DNodeVectorUVE(const Math::Vector3UVE& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] inline bool IsFinite3DNodeQuaternionUVE(const Math::QuaternionUVE& value) noexcept {
    return Math::IsFiniteUVE(value) && std::isfinite(Math::LengthSquaredUVE(value));
}

struct RayCast3DNodeComponentUVE final {
    Math::Vector3UVE direction{0.0F, -1.0F, 0.0F};
    float length = 100.0F;
    std::uint32_t collisionMask = 0xFFFFFFFFU;
    bool enabled = true;
    std::array<std::uint32_t, kMaximumRayCastExclusionsUVE> exclusions{};
    std::uint8_t exclusionCount = 0U;
    bool hit = false;
    Math::Vector3UVE hitPosition{};
    Math::Vector3UVE hitNormal{};
    std::uint32_t colliderLocalId = std::numeric_limits<std::uint32_t>::max();
};

[[nodiscard]] inline bool IsRayCast3DNodeComponentValidUVE(
    const RayCast3DNodeComponentUVE& value) noexcept {
    const float directionLengthSquared = Math::LengthSquaredUVE(value.direction);
    if (!IsFinite3DNodeVectorUVE(value.direction) || !std::isfinite(directionLengthSquared) ||
        directionLengthSquared <= 1.0e-8F || !std::isfinite(value.length) || value.length <= 0.0F ||
        value.exclusionCount > kMaximumRayCastExclusionsUVE) {
        return false;
    }
    for (std::size_t index = 0U; index < value.exclusionCount; ++index) {
        if (value.exclusions[index] == std::numeric_limits<std::uint32_t>::max()) {
            return false;
        }
        for (std::size_t previous = 0U; previous < index; ++previous) {
            if (value.exclusions[previous] == value.exclusions[index]) {
                return false;
            }
        }
    }
    return !value.hit || (value.colliderLocalId != std::numeric_limits<std::uint32_t>::max() &&
                           IsFinite3DNodeVectorUVE(value.hitPosition) && IsFinite3DNodeVectorUVE(value.hitNormal));
}

struct AnimatableBody3DNodeComponentUVE final {
    Math::Vector3UVE targetVelocity{};
    float interpolation = 1.0F;
    bool active = true;
};

[[nodiscard]] inline bool IsAnimatableBody3DNodeComponentValidUVE(
    const AnimatableBody3DNodeComponentUVE& value) noexcept {
    return IsFinite3DNodeVectorUVE(value.targetVelocity) && std::isfinite(value.interpolation) &&
           value.interpolation >= 0.0F && value.interpolation <= 1.0F;
}

struct NavigationRegion3DNodeComponentUVE final {
    Math::Vector3UVE boundsHalfExtents{10.0F, 2.0F, 10.0F};
    std::string navigationMeshAssetPath;
    std::uint32_t navigationLayers = 1U;
    bool enabled = true;
    bool rebuildRequested = false;
};

[[nodiscard]] inline bool IsNavigationRegion3DNodeComponentValidUVE(
    const NavigationRegion3DNodeComponentUVE& value) noexcept {
    return IsFinite3DNodeVectorUVE(value.boundsHalfExtents) && value.boundsHalfExtents.x > 0.0F &&
           value.boundsHalfExtents.y > 0.0F && value.boundsHalfExtents.z > 0.0F &&
           value.navigationLayers != 0U && IsBounded3DNodeStringUVE(value.navigationMeshAssetPath);
}

enum class NavigationAgentPathStatusUVE : std::uint8_t {
    Idle = 0,
    Searching,
    Following,
    Finished,
    Failed,
};

struct NavigationAgent3DNodeComponentUVE final {
    Math::Vector3UVE targetPosition{};
    Math::Vector3UVE nextPathPosition{};
    Math::Vector3UVE desiredVelocity{};
    float radius = 0.5F;
    float height = 1.8F;
    float maxSpeed = 4.0F;
    float pathUpdateInterval = 0.1F;
    std::uint32_t navigationLayers = 1U;
    NavigationAgentPathStatusUVE pathStatus = NavigationAgentPathStatusUVE::Idle;
    bool avoidanceEnabled = true;
    bool enabled = true;
    bool pathChanged = false;
    bool targetReached = false;
};

[[nodiscard]] inline bool IsNavigationAgent3DNodeComponentValidUVE(
    const NavigationAgent3DNodeComponentUVE& value) noexcept {
    return IsFinite3DNodeVectorUVE(value.targetPosition) && IsFinite3DNodeVectorUVE(value.nextPathPosition) &&
           IsFinite3DNodeVectorUVE(value.desiredVelocity) && std::isfinite(value.radius) && value.radius > 0.0F &&
           std::isfinite(value.height) && value.height >= value.radius * 2.0F && std::isfinite(value.maxSpeed) &&
           value.maxSpeed > 0.0F && std::isfinite(value.pathUpdateInterval) && value.pathUpdateInterval > 0.0F &&
           value.pathUpdateInterval <= 10.0F && value.navigationLayers != 0U &&
           value.pathStatus <= NavigationAgentPathStatusUVE::Failed;
}

struct SkeletonBoneUVE final {
    std::string name;
    std::int32_t parentIndex = -1;
    Math::Vector3UVE localPosition{};
    Math::QuaternionUVE localRotation{};
    Math::Vector3UVE localScale{1.0F, 1.0F, 1.0F};
};

struct Skeleton3DNodeComponentUVE final {
    std::string skeletonAssetPath;
    std::vector<SkeletonBoneUVE> bones;
    bool enabled = true;
};

[[nodiscard]] inline bool IsSkeleton3DNodeComponentValidUVE(
    const Skeleton3DNodeComponentUVE& value) noexcept {
    if (!IsBounded3DNodeStringUVE(value.skeletonAssetPath) || value.bones.size() > kMaximumSkeletonBonesUVE) {
        return false;
    }
    for (std::size_t index = 0U; index < value.bones.size(); ++index) {
        const SkeletonBoneUVE& bone = value.bones[index];
        if (!IsBounded3DNodeStringUVE(bone.name, false) || !IsFinite3DNodeVectorUVE(bone.localPosition) ||
            !IsFinite3DNodeQuaternionUVE(bone.localRotation) || !IsFinite3DNodeVectorUVE(bone.localScale) ||
            bone.localScale.x <= 0.0F || bone.localScale.y <= 0.0F || bone.localScale.z <= 0.0F ||
            (bone.parentIndex >= 0 && static_cast<std::size_t>(bone.parentIndex) >= index)) {
            return false;
        }
        for (std::size_t previous = 0U; previous < index; ++previous) {
            if (value.bones[previous].name == bone.name) {
                return false;
            }
        }
    }
    return true;
}

/// Applies only an explicit authored/imported skeleton asset payload. This function deliberately
/// rejects an empty asset or empty hierarchy so retarget metadata cannot hydrate a Skeleton3D node.
[[nodiscard]] inline bool TryBindExplicitSkeleton3DAssetUVE(
    Skeleton3DNodeComponentUVE& target, std::string assetPath, std::vector<SkeletonBoneUVE> bones) {
    Skeleton3DNodeComponentUVE candidate;
    candidate.skeletonAssetPath = std::move(assetPath);
    candidate.bones = std::move(bones);
    candidate.enabled = target.enabled;
    if (candidate.skeletonAssetPath.empty() || candidate.bones.empty() ||
        !IsSkeleton3DNodeComponentValidUVE(candidate)) {
        return false;
    }
    target = std::move(candidate);
    return true;
}

struct BoneAttachment3DNodeComponentUVE final {

    std::uint32_t skeletonLocalId = std::numeric_limits<std::uint32_t>::max();
    std::uint32_t boneIndex = std::numeric_limits<std::uint32_t>::max();
    std::string boneName;
    Math::Vector3UVE localPosition{};
    Math::QuaternionUVE localRotation{};
    Math::Vector3UVE localScale{1.0F, 1.0F, 1.0F};
    bool enabled = true;
};

[[nodiscard]] inline bool IsBoneAttachment3DNodeComponentValidUVE(
    const BoneAttachment3DNodeComponentUVE& value) noexcept {
    return IsBounded3DNodeStringUVE(value.boneName) && IsFinite3DNodeVectorUVE(value.localPosition) &&
           IsFinite3DNodeQuaternionUVE(value.localRotation) && IsFinite3DNodeVectorUVE(value.localScale) &&
           value.localScale.x > 0.0F && value.localScale.y > 0.0F && value.localScale.z > 0.0F;
}

/// Returns whether an attachment has enough explicit references to participate in runtime
/// binding. A default attachment is valid scene data but remains inert until this is true.
[[nodiscard]] inline bool IsBoneAttachment3DNodeComponentResolvableUVE(
    const BoneAttachment3DNodeComponentUVE& value) noexcept {
    const bool hasSkeletonReference = value.skeletonLocalId != std::numeric_limits<std::uint32_t>::max();
    const bool hasBoneReference = value.boneIndex != std::numeric_limits<std::uint32_t>::max() ||
                                  !value.boneName.empty();
    return value.enabled && hasSkeletonReference && hasBoneReference;
}

struct SpringArm3DNodeComponentUVE final {

    float armLength = 4.0F;
    float margin = 0.1F;
    float smoothing = 8.0F;
    std::uint32_t collisionMask = 0xFFFFFFFFU;
    float currentLength = 4.0F;
    bool enabled = true;
};

[[nodiscard]] inline bool IsSpringArm3DNodeComponentValidUVE(
    const SpringArm3DNodeComponentUVE& value) noexcept {
    return std::isfinite(value.armLength) && value.armLength > 0.0F && std::isfinite(value.margin) &&
           value.margin >= 0.0F && std::isfinite(value.smoothing) && value.smoothing >= 0.0F &&
           std::isfinite(value.currentLength) && value.currentLength >= 0.0F && value.currentLength <= value.armLength;
}

struct Marker3DNodeComponentUVE final {
    std::string markerName = "Marker";
    Math::Vector3UVE localPosition{};
    Math::QuaternionUVE localRotation{};
    bool enabled = true;
};

[[nodiscard]] inline bool IsMarker3DNodeComponentValidUVE(const Marker3DNodeComponentUVE& value) noexcept {
    return IsBounded3DNodeStringUVE(value.markerName, false) && IsFinite3DNodeVectorUVE(value.localPosition) &&
           IsFinite3DNodeQuaternionUVE(value.localRotation);
}

struct Hitbox3DNodeComponentUVE final {
    Math::Vector3UVE halfExtents{0.5F, 0.5F, 0.5F};
    std::uint32_t collisionLayer = 1U;
    std::uint32_t collisionMask = 0xFFFFFFFFU;
    std::string damageChannel = "default";
    bool enabled = true;
};

[[nodiscard]] inline bool IsHitbox3DNodeComponentValidUVE(const Hitbox3DNodeComponentUVE& value) noexcept {
    return IsFinite3DNodeVectorUVE(value.halfExtents) && value.halfExtents.x > 0.0F &&
           value.halfExtents.y > 0.0F && value.halfExtents.z > 0.0F && value.collisionLayer != 0U &&
           IsBounded3DNodeStringUVE(value.damageChannel, false);
}

struct Hurtbox3DNodeComponentUVE final {
    Math::Vector3UVE halfExtents{0.5F, 0.5F, 0.5F};
    std::uint32_t collisionLayer = 1U;
    std::uint32_t collisionMask = 0xFFFFFFFFU;
    std::string damageChannel = "default";
    bool enabled = true;
};

[[nodiscard]] inline bool IsHurtbox3DNodeComponentValidUVE(const Hurtbox3DNodeComponentUVE& value) noexcept {
    const Hitbox3DNodeComponentUVE equivalent{value.halfExtents, value.collisionLayer, value.collisionMask,
                                               value.damageChannel, value.enabled};
    return IsHitbox3DNodeComponentValidUVE(equivalent);
}

struct Projectile3DNodeComponentUVE final {
    Math::Vector3UVE velocity{};
    Math::Vector3UVE acceleration{};
    float radius = 0.1F;
    float maxLifetime = 10.0F;
    float remainingLifetime = 10.0F;
    std::uint32_t collisionMask = 0xFFFFFFFFU;
    bool active = true;
};

[[nodiscard]] inline bool IsProjectile3DNodeComponentValidUVE(
    const Projectile3DNodeComponentUVE& value) noexcept {
    return IsFinite3DNodeVectorUVE(value.velocity) && IsFinite3DNodeVectorUVE(value.acceleration) &&
           std::isfinite(value.radius) && value.radius > 0.0F && std::isfinite(value.maxLifetime) &&
           value.maxLifetime > 0.0F && std::isfinite(value.remainingLifetime) && value.remainingLifetime >= 0.0F &&
           value.remainingLifetime <= value.maxLifetime;
}

struct InteractionArea3DNodeComponentUVE final {
    Math::Vector3UVE halfExtents{1.0F, 1.0F, 1.0F};
    std::uint32_t collisionLayer = 1U;
    std::uint32_t collisionMask = 0xFFFFFFFFU;
    std::string interactionTag = "interactable";
    std::uint32_t maximumCandidates = 16U;
    bool enabled = true;
};

[[nodiscard]] inline bool IsInteractionArea3DNodeComponentValidUVE(
    const InteractionArea3DNodeComponentUVE& value) noexcept {
    return IsFinite3DNodeVectorUVE(value.halfExtents) && value.halfExtents.x > 0.0F &&
           value.halfExtents.y > 0.0F && value.halfExtents.z > 0.0F && value.collisionLayer != 0U &&
           value.maximumCandidates > 0U && value.maximumCandidates <= 4096U &&
           IsBounded3DNodeStringUVE(value.interactionTag, false);
}

struct WorldEnvironment3DNodeComponentUVE final {
    std::string skyAssetPath;
    Math::Vector3UVE ambientColor{0.2F, 0.2F, 0.2F};
    Math::Vector3UVE fogColor{0.5F, 0.6F, 0.7F};
    float ambientEnergy = 1.0F;
    float exposure = 1.0F;
    float fogDensity = 0.0F;
    bool fogEnabled = false;
    bool postProcessingEnabled = false;
};

[[nodiscard]] inline bool IsWorldEnvironment3DNodeComponentValidUVE(
    const WorldEnvironment3DNodeComponentUVE& value) noexcept {
    return IsBounded3DNodeStringUVE(value.skyAssetPath) && IsFinite3DNodeVectorUVE(value.ambientColor) &&
           IsFinite3DNodeVectorUVE(value.fogColor) && value.ambientColor.x >= 0.0F && value.ambientColor.y >= 0.0F &&
           value.ambientColor.z >= 0.0F && std::isfinite(value.ambientEnergy) && value.ambientEnergy >= 0.0F &&
           std::isfinite(value.exposure) && value.exposure > 0.0F && std::isfinite(value.fogDensity) &&
           value.fogDensity >= 0.0F;
}

enum class ReflectionProbeUpdateModeUVE : std::uint8_t {
    Once = 0,
    EveryFrame,
    OnDemand,
};

struct ReflectionProbe3DNodeComponentUVE final {
    Math::Vector3UVE size{5.0F, 5.0F, 5.0F};
    std::uint32_t visibilityLayers = 0xFFFFFFFFU;
    ReflectionProbeUpdateModeUVE updateMode = ReflectionProbeUpdateModeUVE::Once;
    bool updateRequested = false;
    bool enabled = true;
};

[[nodiscard]] inline bool IsReflectionProbe3DNodeComponentValidUVE(
    const ReflectionProbe3DNodeComponentUVE& value) noexcept {
    return IsFinite3DNodeVectorUVE(value.size) && value.size.x > 0.0F && value.size.y > 0.0F && value.size.z > 0.0F &&
           value.updateMode <= ReflectionProbeUpdateModeUVE::OnDemand;
}

enum class DecalProjectionModeUVE : std::uint8_t {
    Box = 0,
    Cylinder,
};

struct Decal3DNodeComponentUVE final {
    std::string materialAssetPath;
    Math::Vector3UVE size{1.0F, 1.0F, 1.0F};
    DecalProjectionModeUVE projection = DecalProjectionModeUVE::Box;
    float lifetime = 0.0F;
    bool enabled = true;
};

[[nodiscard]] inline bool IsDecal3DNodeComponentValidUVE(const Decal3DNodeComponentUVE& value) noexcept {
    return IsBounded3DNodeStringUVE(value.materialAssetPath) && IsFinite3DNodeVectorUVE(value.size) &&
           value.size.x > 0.0F && value.size.y > 0.0F && value.size.z > 0.0F && std::isfinite(value.lifetime) &&
           value.lifetime >= 0.0F && value.projection <= DecalProjectionModeUVE::Cylinder;
}

struct LodGroup3DNodeComponentUVE final {
    std::array<float, kMaximumLodLevelsUVE> distanceThresholds{10.0F, 25.0F, 60.0F, 120.0F, 240.0F, 480.0F, 960.0F, 1920.0F};
    std::uint8_t levelCount = 4U;
    std::uint8_t currentLevel = 0U;
    bool enabled = true;
};

[[nodiscard]] inline bool IsLodGroup3DNodeComponentValidUVE(const LodGroup3DNodeComponentUVE& value) noexcept {
    if (value.levelCount == 0U || value.levelCount > kMaximumLodLevelsUVE || value.currentLevel >= value.levelCount) {
        return false;
    }
    for (std::size_t index = 0U; index < value.levelCount; ++index) {
        if (!std::isfinite(value.distanceThresholds[index]) || value.distanceThresholds[index] < 0.0F ||
            (index > 0U && value.distanceThresholds[index] <= value.distanceThresholds[index - 1U])) {
            return false;
        }
    }
    return true;
}

enum class Occluder3DNodeModeUVE : std::uint8_t {
    ConservativeBox = 0,
};

struct Occluder3DNodeComponentUVE final {
    Math::Vector3UVE halfExtents{2.0F, 2.0F, 2.0F};
    Occluder3DNodeModeUVE mode = Occluder3DNodeModeUVE::ConservativeBox;
    bool enabled = true;
};

[[nodiscard]] inline bool IsOccluder3DNodeComponentValidUVE(
    const Occluder3DNodeComponentUVE& value) noexcept {
    return IsFinite3DNodeVectorUVE(value.halfExtents) && value.halfExtents.x > 0.0F &&
           value.halfExtents.y > 0.0F && value.halfExtents.z > 0.0F && value.mode == Occluder3DNodeModeUVE::ConservativeBox;
}

struct VisibilityRegion3DNodeComponentUVE final {
    Math::Vector3UVE halfExtents{10.0F, 10.0F, 10.0F};
    std::uint32_t visibilityLayers = 0xFFFFFFFFU;
    bool enabled = true;
    bool active = true;
};

[[nodiscard]] inline bool IsVisibilityRegion3DNodeComponentValidUVE(
    const VisibilityRegion3DNodeComponentUVE& value) noexcept {
    return IsFinite3DNodeVectorUVE(value.halfExtents) && value.halfExtents.x > 0.0F &&
           value.halfExtents.y > 0.0F && value.halfExtents.z > 0.0F;
}

struct SpawnPoint3DNodeComponentUVE final {
    std::string spawnTag = "spawn";
    Math::Vector3UVE localPosition{};
    Math::QuaternionUVE localRotation{};
    bool enabled = true;
    bool oneShot = false;
};

[[nodiscard]] inline bool IsSpawnPoint3DNodeComponentValidUVE(const SpawnPoint3DNodeComponentUVE& value) noexcept {
    return IsBounded3DNodeStringUVE(value.spawnTag, false) && IsFinite3DNodeVectorUVE(value.localPosition) &&
           IsFinite3DNodeQuaternionUVE(value.localRotation);
}

struct LevelStreamer3DNodeComponentUVE final {
    std::string levelPath;
    float loadDistance = 250.0F;
    float unloadDistance = 300.0F;
    bool enabled = false;
    bool loaded = false;
    bool loadRequested = false;
};

[[nodiscard]] inline bool IsLevelStreamer3DNodeComponentValidUVE(
    const LevelStreamer3DNodeComponentUVE& value) noexcept {
    if (!IsBounded3DNodeStringUVE(value.levelPath) || !std::isfinite(value.loadDistance) ||
        value.loadDistance <= 0.0F || !std::isfinite(value.unloadDistance) ||
        value.unloadDistance <= value.loadDistance) {
        return false;
    }
    if (value.levelPath.empty()) {
        return !value.enabled && !value.loaded && !value.loadRequested;
    }
    return true;
}

struct WorldPartition3DNodeComponentUVE final {
    float cellSize = 128.0F;
    std::array<std::uint32_t, 3U> cellCounts{16U, 1U, 16U};
    std::uint32_t maximumLoadedCells = 64U;
    std::uint32_t loadedCellCount = 0U;
    bool enabled = true;
};

[[nodiscard]] inline bool IsWorldPartition3DNodeComponentValidUVE(
    const WorldPartition3DNodeComponentUVE& value) noexcept {
    if (!std::isfinite(value.cellSize) || value.cellSize <= 0.0F || value.maximumLoadedCells == 0U ||
        value.maximumLoadedCells > kMaximumStreamedCellsUVE || value.loadedCellCount > value.maximumLoadedCells) {
        return false;
    }
    for (const std::uint32_t count : value.cellCounts) {
        if (count == 0U || count > kMaximumStreamedCellsUVE) {
            return false;
        }
    }
    return true;
}

} // namespace UVE::Scene
