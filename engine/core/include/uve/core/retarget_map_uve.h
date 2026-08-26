// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/core/time_pose_contract_uve.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace UVE::Core {

enum class RetargetMapBoneCategoryUVE : std::uint8_t {
    Core = 0,
    CorrectiveHelper,
    IK,
    Utility,
    Root,
};

struct RetargetMapBoneUVE final {
    std::string boneId;
    std::string parentBoneId;
    std::string genericName;
    RetargetMapBoneCategoryUVE category = RetargetMapBoneCategoryUVE::Core;
    Math::Vector3UVE bindPoseGlobalPosition{};
};

enum class RetargetPoseClassificationUVE : std::uint8_t {
    Unknown = 0,
    APose,
    TPOSE,
    RelaxedArmsDown,
};

struct RetargetMapPoseAnalysisUVE final {
    RetargetPoseClassificationUVE detectedPose = RetargetPoseClassificationUVE::Unknown;
    std::string detectedPoseLabel;
    float leftArmAngleFromDownDegrees = 0.0F;
    float rightArmAngleFromDownDegrees = 0.0F;
    float aPoseMinimumDegrees = 25.0F;
    float aPoseMaximumDegrees = 55.0F;
    float relaxedArmsDownMinimumDegrees = 0.0F;
    float relaxedArmsDownMaximumDegrees = 20.0F;
    float tPoseMinimumDegrees = 75.0F;
    float tPoseMaximumDegrees = 95.0F;
    std::string method;
    std::string aPoseThresholdText;
    std::string relaxedArmsDownThresholdText;
    std::string tPoseThresholdText;
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
    RetargetMapPoseAnalysisUVE poseAnalysis;
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

/// Loads the owner-authored retarget map JSON into native UVE data. This parser never creates
/// entities, skeleton nodes, runtime components, or serialized scene content.
[[nodiscard]] RetargetMapValidationResultUVE LoadRetargetMapJsonUVE(
    const std::string& jsonText, RetargetMapUVE& output) noexcept;

/// Loads and parses a retarget map JSON file through the same bounded native contract.
[[nodiscard]] RetargetMapValidationResultUVE LoadRetargetMapFileUVE(
    const std::filesystem::path& path, RetargetMapUVE& output) noexcept;

/// Recomputes the arm angles and pose class from the map's authored bind positions. The stored JSON
/// analysis remains preserved in poseAnalysis; callers can compare both values for import QA.
[[nodiscard]] RetargetMapPoseAnalysisUVE AnalyzeRetargetMapPoseUVE(
    const RetargetMapUVE& map) noexcept;

} // namespace UVE::Core
