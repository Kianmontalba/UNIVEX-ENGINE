// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/core/retarget_map_uve.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

namespace UVE::Core {
namespace {

constexpr std::size_t kMaximumIdentifierBytesUVE = kMaximumAnimationIdentifierBytesUVE;

[[nodiscard]] bool IsIdentifierUVE(const std::string& value, const bool allowEmpty = false) noexcept {
    return (allowEmpty || !value.empty()) && value.size() <= kMaximumIdentifierBytesUVE &&
           value.find('\0') == std::string::npos;
}

[[nodiscard]] bool IsFiniteVectorUVE(const Math::Vector3UVE& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] bool IsValidCategoryUVE(const RetargetMapBoneCategoryUVE category) noexcept {
    return category <= RetargetMapBoneCategoryUVE::Utility;
}

[[nodiscard]] const RetargetMapBoneUVE* FindEntryUVE(
    const RetargetMapUVE& map, const std::string& boneId) noexcept {
    const auto iterator = std::find_if(map.entries.cbegin(), map.entries.cend(), [&boneId](const auto& entry) {
        return entry.boneId == boneId;
    });
    return iterator == map.entries.cend() ? nullptr : &*iterator;
}

[[nodiscard]] const SkeletonJointUVE* FindJointUVE(
    const SkeletonDefinitionUVE& skeleton, const std::string& jointId) noexcept {
    const auto iterator = std::find_if(skeleton.joints.cbegin(), skeleton.joints.cend(), [&jointId](const auto& joint) {
        return joint.jointId == jointId;
    });
    return iterator == skeleton.joints.cend() ? nullptr : &*iterator;
}

[[nodiscard]] RetargetMapValidationResultUVE ValidResultUVE() {
    return {RetargetMapValidationCodeUVE::Valid, 0U, {}, "Retarget map is valid."};
}

} // namespace

RetargetMapValidationResultUVE ValidateRetargetMapUVE(const RetargetMapUVE& map) noexcept {
    if (map.schemaVersion != RetargetMapUVE::kCurrentSchemaVersionUVE) {
        return {RetargetMapValidationCodeUVE::InvalidSchema, 0U, {},
                "Retarget map schema version is unsupported."};
    }
    if (!IsIdentifierUVE(map.mapId) || !IsIdentifierUVE(map.sourceSkeletonId) ||
        !IsIdentifierUVE(map.coordinateSystem) || !IsIdentifierUVE(map.units) ||
        !IsIdentifierUVE(map.rootBoneId)) {
        return {RetargetMapValidationCodeUVE::InvalidMetadata, 0U, {},
                "Retarget map requires bounded map, skeleton, coordinate, units, and root identifiers."};
    }
    if (map.entries.empty()) {
        return {RetargetMapValidationCodeUVE::EmptyMap, 0U, {},
                "Retarget map requires at least one bone entry."};
    }
    if (map.entries.size() > RetargetMapUVE::kMaximumEntriesUVE) {
        return {RetargetMapValidationCodeUVE::CapacityExceeded, map.entries.size(), {},
                "Retarget map entry count exceeds the bounded contract."};
    }

    std::size_t rootCount = 0U;
    std::size_t rootIndex = 0U;
    for (std::size_t index = 0U; index < map.entries.size(); ++index) {
        const RetargetMapBoneUVE& entry = map.entries[index];
        if (!IsIdentifierUVE(entry.boneId) || !IsIdentifierUVE(entry.parentBoneId, true) ||
            !IsIdentifierUVE(entry.genericName, true) || !IsValidCategoryUVE(entry.category) ||
            !IsFiniteVectorUVE(entry.bindPoseGlobalPosition)) {
            return {RetargetMapValidationCodeUVE::InvalidEntry, index, entry.boneId,
                    "Retarget map entry identity, category, or bind position is invalid."};
        }
        if (entry.category == RetargetMapBoneCategoryUVE::Core && entry.genericName.empty()) {
            return {RetargetMapValidationCodeUVE::MissingGenericName, index, entry.boneId,
                    "Core retarget map entries require a generic role name."};
        }
        if (FindEntryUVE(map, entry.boneId) != &entry) {
            return {RetargetMapValidationCodeUVE::DuplicateBone, index, entry.boneId,
                    "Retarget map bone identifiers must be unique."};
        }
        if (entry.parentBoneId.empty()) {
            ++rootCount;
            rootIndex = index;
        } else if (FindEntryUVE(map, entry.parentBoneId) == nullptr) {
            return {RetargetMapValidationCodeUVE::UnknownParent, index, entry.boneId,
                    "Retarget map entry references an unknown parent bone."};
        }
    }
    if (rootCount != 1U) {
        return {RetargetMapValidationCodeUVE::MultipleRoots, rootCount, map.rootBoneId,
                "Retarget map must contain exactly one root entry."};
    }
    if (map.entries[rootIndex].boneId != map.rootBoneId) {
        return {RetargetMapValidationCodeUVE::RootMismatch, rootIndex, map.rootBoneId,
                "Retarget map root identifier does not match the hierarchy root."};
    }

    for (std::size_t startIndex = 0U; startIndex < map.entries.size(); ++startIndex) {
        std::vector<std::size_t> visited;
        std::size_t currentIndex = startIndex;
        while (true) {
            if (std::find(visited.cbegin(), visited.cend(), currentIndex) != visited.cend()) {
                return {RetargetMapValidationCodeUVE::CyclicHierarchy, startIndex,
                        map.entries[startIndex].boneId, "Retarget map hierarchy contains a cycle."};
            }
            visited.push_back(currentIndex);
            const std::string& parentId = map.entries[currentIndex].parentBoneId;
            if (parentId.empty()) {
                break;
            }
            const RetargetMapBoneUVE* parent = FindEntryUVE(map, parentId);
            if (parent == nullptr) {
                return {RetargetMapValidationCodeUVE::UnknownParent, currentIndex,
                        map.entries[currentIndex].boneId, "Retarget map parent resolution failed."};
            }
            currentIndex = static_cast<std::size_t>(parent - map.entries.data());
        }
    }
    return ValidResultUVE();
}

RetargetMapValidationResultUVE ValidateRetargetMapAgainstSkeletonUVE(
    const RetargetMapUVE& map, const SkeletonDefinitionUVE& sourceSkeleton) noexcept {
    const RetargetMapValidationResultUVE mapValidation = ValidateRetargetMapUVE(map);
    if (!mapValidation.IsValidUVE()) {
        return mapValidation;
    }
    const AnimationContractValidationResultUVE skeletonValidation =
        ValidateSkeletonDefinitionUVE(sourceSkeleton);
    if (!skeletonValidation.IsValidUVE()) {
        return {RetargetMapValidationCodeUVE::SkeletonMismatch, skeletonValidation.index,
                sourceSkeleton.skeletonId, "Source skeleton definition is invalid."};
    }
    if (map.sourceSkeletonId != sourceSkeleton.skeletonId) {
        return {RetargetMapValidationCodeUVE::SkeletonMismatch, 0U, map.sourceSkeletonId,
                "Retarget map source skeleton does not match the supplied skeleton definition."};
    }
    for (std::size_t index = 0U; index < map.entries.size(); ++index) {
        const RetargetMapBoneUVE& entry = map.entries[index];
        const SkeletonJointUVE* joint = FindJointUVE(sourceSkeleton, entry.boneId);
        if (joint == nullptr || joint->parentJointId != entry.parentBoneId) {
            return {RetargetMapValidationCodeUVE::SkeletonMismatch, index, entry.boneId,
                    "Retarget map hierarchy does not match the supplied skeleton definition."};
        }
    }
    return ValidResultUVE();
}

} // namespace UVE::Core
