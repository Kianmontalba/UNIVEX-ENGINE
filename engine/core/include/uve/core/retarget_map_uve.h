// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/core/time_pose_contract_uve.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace UVE::Core {

enum class RetargetMapBoneCategoryUVE : std::uint8_t {
    Core = 0,
    CorrectiveHelper,
    IK,
    Utility,
};

struct RetargetMapBoneUVE final {
    std::string boneId;
    std::string parentBoneId;
    std::string genericName;
    RetargetMapBoneCategoryUVE category = RetargetMapBoneCategoryUVE::Core;
    Math::Vector3UVE bindPoseGlobalPosition{};
};

/// Design-time source-rig data. This is intentionally separate from Skeleton3DNodeComponentUVE:
/// loading or validating a map never creates scene bones or hydrates a node.
struct RetargetMapUVE final {
    static constexpr std::uint32_t kCurrentSchemaVersionUVE = 1U;
    static constexpr std::size_t kMaximumEntriesUVE = kMaximumSkeletonJointsUVE;

    std::uint32_t schemaVersion = kCurrentSchemaVersionUVE;
    std::string mapId;
    std::string sourceSkeletonId;
    std::string coordinateSystem;
    std::string units;
    std::string rootBoneId;
    bool isAPose = false;
    std::vector<RetargetMapBoneUVE> entries;
};

enum class RetargetMapValidationCodeUVE : std::uint8_t {
    Valid = 0,
    InvalidSchema,
    InvalidMetadata,
    EmptyMap,
    CapacityExceeded,
    InvalidEntry,
    DuplicateBone,
    UnknownParent,
    MultipleRoots,
    MissingRoot,
    RootMismatch,
    CyclicHierarchy,
    MissingGenericName,
    InvalidCategory,
    SkeletonMismatch,
};

struct RetargetMapValidationResultUVE final {
    RetargetMapValidationCodeUVE code = RetargetMapValidationCodeUVE::EmptyMap;
    std::size_t index = 0U;
    std::string identifier;
    std::string message;

    [[nodiscard]] bool IsValidUVE() const noexcept {
        return code == RetargetMapValidationCodeUVE::Valid;
    }
};

[[nodiscard]] RetargetMapValidationResultUVE ValidateRetargetMapUVE(
    const RetargetMapUVE& map) noexcept;

/// Confirms that a validated design map describes an already-authored/imported source skeleton.
/// This function never creates or modifies the supplied skeleton.
[[nodiscard]] RetargetMapValidationResultUVE ValidateRetargetMapAgainstSkeletonUVE(
    const RetargetMapUVE& map, const SkeletonDefinitionUVE& sourceSkeleton) noexcept;

} // namespace UVE::Core
