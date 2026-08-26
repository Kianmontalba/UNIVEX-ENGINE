// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/core/retarget_map_uve.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <nlohmann/json.hpp>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
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
    return category <= RetargetMapBoneCategoryUVE::Root;
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

[[nodiscard]] std::optional<RetargetMapBoneCategoryUVE> ParseCategoryUVE(
    const std::string_view value) noexcept {
    if (value == "core") {
        return RetargetMapBoneCategoryUVE::Core;
    }
    if (value == "corrective_helper") {
        return RetargetMapBoneCategoryUVE::CorrectiveHelper;
    }
    if (value == "ik") {
        return RetargetMapBoneCategoryUVE::IK;
    }
    if (value == "utility") {
        return RetargetMapBoneCategoryUVE::Utility;
    }
    if (value == "root") {
        return RetargetMapBoneCategoryUVE::Root;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<RetargetPoseClassificationUVE> ParsePoseClassUVE(
    const std::string_view value) noexcept {
    if (value == "A-pose") {
        return RetargetPoseClassificationUVE::APose;
    }
    if (value == "T-pose") {
        return RetargetPoseClassificationUVE::TPOSE;
    }
    if (value == "relaxed_arms_down") {
        return RetargetPoseClassificationUVE::RelaxedArmsDown;
    }
    return RetargetPoseClassificationUVE::Unknown;
}

[[nodiscard]] bool ParseRangeUVE(const std::string& value, float& minimum, float& maximum) noexcept {
    char* end = nullptr;
    const float first = std::strtof(value.c_str(), &end);
    if (end == value.c_str() || !std::isfinite(first)) {
        return false;
    }
    while (*end != '\0' && *end != '-' && (*end < '0' || *end > '9')) {
        ++end;
    }
    if (*end == '-') {
        ++end;
    }
    char* secondEnd = nullptr;
    const float second = std::strtof(end, &secondEnd);
    if (secondEnd == end || !std::isfinite(second)) {
        return false;
    }
    minimum = first;
    maximum = second;
    return minimum <= maximum;
}

[[nodiscard]] RetargetMapValidationResultUVE ParsePoseAnalysisUVE(
    const nlohmann::json& json, RetargetMapPoseAnalysisUVE& output) noexcept {
    try {
        if (!json.is_object()) {
            return {RetargetMapValidationCodeUVE::InvalidMetadata, 0U, {},
                    "Retarget map pose analysis must be an object."};
        }
        const std::string detectedPose = json.value("detected_pose", std::string{});
        const std::optional<RetargetPoseClassificationUVE> classification = ParsePoseClassUVE(detectedPose);
        if (!classification.has_value()) {
            return {RetargetMapValidationCodeUVE::InvalidMetadata, 0U, detectedPose,
                    "Retarget map pose classification is unsupported."};
        }
        output.detectedPose = *classification;
        output.detectedPoseLabel = detectedPose;
        output.leftArmAngleFromDownDegrees = json.value("left_arm_angle_from_down_deg", 0.0F);
        output.rightArmAngleFromDownDegrees = json.value("right_arm_angle_from_down_deg", 0.0F);
        output.method = json.value("method", std::string{});
        const nlohmann::json thresholds = json.value("thresholds", nlohmann::json::object());
        if (!thresholds.is_object()) {
            return {RetargetMapValidationCodeUVE::InvalidMetadata, 0U, {},
                    "Retarget map pose thresholds must be an object."};
        }
        const auto parseThreshold = [&thresholds](const char* const key, std::string& sourceText,
                                                   float& minimum, float& maximum) {
            sourceText = thresholds.value(key, std::string{});
            return ParseRangeUVE(sourceText, minimum, maximum);
        };
        if (!parseThreshold("a_pose", output.aPoseThresholdText, output.aPoseMinimumDegrees,
                             output.aPoseMaximumDegrees) ||
            !parseThreshold("relaxed_arms_down", output.relaxedArmsDownThresholdText,
                            output.relaxedArmsDownMinimumDegrees, output.relaxedArmsDownMaximumDegrees) ||
            !parseThreshold("t_pose", output.tPoseThresholdText, output.tPoseMinimumDegrees,
                            output.tPoseMaximumDegrees) ||
            !std::isfinite(output.leftArmAngleFromDownDegrees) ||
            !std::isfinite(output.rightArmAngleFromDownDegrees)) {
            return {RetargetMapValidationCodeUVE::InvalidMetadata, 0U, {},
                    "Retarget map pose analysis contains invalid numeric data."};
        }
        return ValidResultUVE();
    } catch (const std::exception&) {
        return {RetargetMapValidationCodeUVE::InvalidMetadata, 0U, {},
                "Retarget map pose analysis could not be parsed."};
    }
}

[[nodiscard]] RetargetMapValidationResultUVE InvalidJsonResultUVE(const std::string& message) {
    return {RetargetMapValidationCodeUVE::InvalidMetadata, 0U, {}, message};
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

RetargetMapValidationResultUVE LoadRetargetMapJsonUVE(
    const std::string& jsonText, RetargetMapUVE& output) noexcept {
    try {
        const nlohmann::json json = nlohmann::json::parse(jsonText);
        if (!json.is_object()) {
            return InvalidJsonResultUVE("Retarget map JSON root must be an object.");
        }

        RetargetMapUVE candidate;
        // The owner JSON is a bone payload rather than a UVE envelope. These bounded defaults
        // satisfy the existing map contract without altering the supplied bone or pose fields.
        // Z-up is explicit in the supplied -Z pose-analysis method; units remain source-authored.
        candidate.mapId = json.value("mapId", std::string{"UNIVEX_bone_retarget_map"});
        candidate.sourceSkeletonId = json.value("sourceSkeletonId", candidate.mapId);
        candidate.coordinateSystem = json.value("coordinateSystem", std::string{"Z-up"});
        candidate.units = json.value("units", std::string{"source-authored"});
        candidate.rootBoneId = json.value("rootBoneId", std::string{"root"});
        if (json.contains("schemaVersion")) {
            candidate.schemaVersion = json.at("schemaVersion").get<std::uint32_t>();
        }

        if (json.contains("_pose_analysis")) {
            const RetargetMapValidationResultUVE poseResult =
                ParsePoseAnalysisUVE(json.at("_pose_analysis"), candidate.poseAnalysis);
            if (!poseResult.IsValidUVE()) {
                return poseResult;
            }
        }
        candidate.isAPose = candidate.poseAnalysis.detectedPose == RetargetPoseClassificationUVE::APose;

        for (const auto& [boneId, boneJson] : json.items()) {
            if (boneId == "_pose_analysis") {
                continue;
            }
            if (!boneJson.is_object() || !boneJson.contains("bind_pose_global_position") ||
                !boneJson.contains("category") || !boneJson.contains("generic_name") ||
                !boneJson.contains("parent")) {
                return InvalidJsonResultUVE("Retarget map bone entry is missing a required field.");
            }
            const std::optional<RetargetMapBoneCategoryUVE> category =
                ParseCategoryUVE(boneJson.at("category").get<std::string>());
            if (!category.has_value()) {
                return {RetargetMapValidationCodeUVE::InvalidCategory, candidate.entries.size(), boneId,
                        "Retarget map bone category is unsupported."};
            }
            const std::vector<float> position = boneJson.at("bind_pose_global_position").get<std::vector<float>>();
            if (position.size() != 3U) {
                return {RetargetMapValidationCodeUVE::InvalidEntry, candidate.entries.size(), boneId,
                        "Retarget map bind position must contain three values."};
            }
            RetargetMapBoneUVE entry;
            entry.boneId = boneId;
            entry.parentBoneId = boneJson.at("parent").is_null() ? std::string{} :
                                 boneJson.at("parent").get<std::string>();
            entry.genericName = boneJson.at("generic_name").is_null() ? std::string{} :
                                boneJson.at("generic_name").get<std::string>();
            entry.category = *category;
            entry.bindPoseGlobalPosition = Math::Vector3UVE{position[0], position[1], position[2]};
            candidate.entries.push_back(std::move(entry));
        }

        const RetargetMapValidationResultUVE validation = ValidateRetargetMapUVE(candidate);
        if (!validation.IsValidUVE()) {
            return validation;
        }
        output = std::move(candidate);
        return validation;
    } catch (const std::exception&) {
        return InvalidJsonResultUVE("Retarget map JSON could not be parsed.");
    }
}

RetargetMapValidationResultUVE LoadRetargetMapFileUVE(
    const std::filesystem::path& path, RetargetMapUVE& output) noexcept {
    try {
        std::ifstream input(path);
        if (!input.is_open()) {
            return InvalidJsonResultUVE("Retarget map file could not be opened.");
        }
        std::ostringstream content;
        content << input.rdbuf();
        return LoadRetargetMapJsonUVE(content.str(), output);
    } catch (const std::exception&) {
        return InvalidJsonResultUVE("Retarget map file could not be read.");
    }
}

RetargetMapPoseAnalysisUVE AnalyzeRetargetMapPoseUVE(const RetargetMapUVE& map) noexcept {
    RetargetMapPoseAnalysisUVE analysis = map.poseAnalysis;
    const auto findPosition = [&map](const std::string_view boneId) -> const Math::Vector3UVE* {
        const auto iterator = std::find_if(map.entries.cbegin(), map.entries.cend(), [boneId](const auto& entry) {
            return entry.boneId == boneId;
        });
        return iterator == map.entries.cend() ? nullptr : &iterator->bindPoseGlobalPosition;
    };
    const auto calculateAngle = [&findPosition](const std::string_view upperArmId,
                                                 const std::string_view lowerArmId, float& angle) {
        const Math::Vector3UVE* upperArm = findPosition(upperArmId);
        const Math::Vector3UVE* lowerArm = findPosition(lowerArmId);
        if (upperArm == nullptr || lowerArm == nullptr) {
            return false;
        }
        const Math::Vector3UVE delta = *lowerArm - *upperArm;
        if (!std::isfinite(delta.x) || !std::isfinite(delta.z) ||
            (std::abs(delta.x) <= 0.0001F && std::abs(delta.z) <= 0.0001F)) {
            return false;
        }
        angle = std::atan2(std::abs(delta.x), -delta.z) * 180.0F / 3.14159265358979323846F;
        return std::isfinite(angle);
    };

    if (!calculateAngle("upperarm_l", "lowerarm_l", analysis.leftArmAngleFromDownDegrees) ||
        !calculateAngle("upperarm_r", "lowerarm_r", analysis.rightArmAngleFromDownDegrees)) {
        analysis.detectedPose = RetargetPoseClassificationUVE::Unknown;
        analysis.detectedPoseLabel.clear();
        return analysis;
    }
    const auto inRange = [](const float value, const float minimum, const float maximum) {
        return value >= minimum && value <= maximum;
    };
    const bool leftAPose = inRange(analysis.leftArmAngleFromDownDegrees, analysis.aPoseMinimumDegrees,
                                   analysis.aPoseMaximumDegrees);
    const bool rightAPose = inRange(analysis.rightArmAngleFromDownDegrees, analysis.aPoseMinimumDegrees,
                                    analysis.aPoseMaximumDegrees);
    const bool leftTpose = inRange(analysis.leftArmAngleFromDownDegrees, analysis.tPoseMinimumDegrees,
                                   analysis.tPoseMaximumDegrees);
    const bool rightTpose = inRange(analysis.rightArmAngleFromDownDegrees, analysis.tPoseMinimumDegrees,
                                    analysis.tPoseMaximumDegrees);
    const bool leftRelaxed = inRange(analysis.leftArmAngleFromDownDegrees,
                                     analysis.relaxedArmsDownMinimumDegrees,
                                     analysis.relaxedArmsDownMaximumDegrees);
    const bool rightRelaxed = inRange(analysis.rightArmAngleFromDownDegrees,
                                      analysis.relaxedArmsDownMinimumDegrees,
                                      analysis.relaxedArmsDownMaximumDegrees);
    if (leftAPose && rightAPose) {
        analysis.detectedPose = RetargetPoseClassificationUVE::APose;
    } else if (leftTpose && rightTpose) {
        analysis.detectedPose = RetargetPoseClassificationUVE::TPOSE;
    } else if (leftRelaxed && rightRelaxed) {
        analysis.detectedPose = RetargetPoseClassificationUVE::RelaxedArmsDown;
    } else {
        analysis.detectedPose = RetargetPoseClassificationUVE::Unknown;
    }
    return analysis;
}

} // namespace UVE::Core
