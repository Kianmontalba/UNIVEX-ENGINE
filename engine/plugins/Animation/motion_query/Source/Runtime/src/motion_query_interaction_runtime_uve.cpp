// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/motion_query_interaction_runtime_uve.h"

#include <algorithm>
#include <utility>

namespace UVE::Plugins {
namespace {
[[nodiscard]] bool IsValidHandleUVE(const UVE::Asset::ResourceHandleUVE handle) noexcept {
    return handle.guid.value != 0U && handle.generation != 0U;
}

[[nodiscard]] bool ContainsHandleUVE(
    const std::vector<UVE::Asset::ResourceHandleUVE>& handles,
    const UVE::Asset::ResourceHandleUVE handle) noexcept {
    return std::find(handles.cbegin(), handles.cend(), handle) != handles.cend();
}

[[nodiscard]] MotionQueryInteractionValidationResultUVE MakeValidationResultUVE(
    MotionQueryInteractionValidationCodeUVE code, std::size_t index, const char* message) noexcept {
    return MotionQueryInteractionValidationResultUVE{code, index, message};
}

[[nodiscard]] MotionQueryInteractionSelectionResultUVE MakeSelectionResultUVE(
    MotionQueryInteractionSelectionCodeUVE code, const char* message) noexcept {
    return MotionQueryInteractionSelectionResultUVE{code, {}, {}, message};
}

[[nodiscard]] bool IsAssetBeforeUVE(const MotionQueryInteractionAssetUVE& lhs,
                                    const MotionQueryInteractionAssetUVE& rhs) noexcept {
    if (lhs.interactionId != rhs.interactionId) {
        return lhs.interactionId < rhs.interactionId;
    }
    if (lhs.handle.guid.value != rhs.handle.guid.value) {
        return lhs.handle.guid.value < rhs.handle.guid.value;
    }
    return lhs.handle.generation < rhs.handle.generation;
}
} // namespace

MotionQueryInteractionValidationResultUVE ValidateMotionQueryInteractionAssetUVE(
    const MotionQueryInteractionAssetUVE& asset) noexcept {
    if (!IsValidHandleUVE(asset.handle)) {
        return MakeValidationResultUVE(MotionQueryInteractionValidationCodeUVE::InvalidHandle, 0U,
                                       "motion query interaction asset handle is invalid");
    }
    if (asset.interactionId.empty() ||
        asset.interactionId.size() > MotionQueryInteractionAssetUVE::kMaximumIdBytesUVE) {
        return MakeValidationResultUVE(MotionQueryInteractionValidationCodeUVE::InvalidIdentifier,
                                       0U, "motion query interaction asset identifier is invalid");
    }
    if (asset.participants.empty() ||
        asset.participants.size() > MotionQueryInteractionAssetUVE::kMaximumParticipantsUVE) {
        return MakeValidationResultUVE(MotionQueryInteractionValidationCodeUVE::EmptyParticipants,
                                       0U, "motion query interaction asset participants are invalid");
    }
    for (std::size_t index = 0U; index < asset.participants.size(); ++index) {
        const MotionQueryInteractionParticipantUVE& participant = asset.participants[index];
        if (participant.role.empty() ||
            participant.role.size() > MotionQueryInteractionParticipantUVE::kMaximumRoleBytesUVE ||
            !IsValidHandleUVE(participant.resource)) {
            return MakeValidationResultUVE(MotionQueryInteractionValidationCodeUVE::InvalidParticipant,
                                           index, "motion query interaction participant is invalid");
        }
        for (std::size_t previous = 0U; previous < index; ++previous) {
            if (asset.participants[previous].role == participant.role) {
                return MakeValidationResultUVE(
                    MotionQueryInteractionValidationCodeUVE::DuplicateParticipantRole, index,
                    "motion query interaction participant roles must be unique");
            }
            if (asset.participants[previous].resource == participant.resource) {
                return MakeValidationResultUVE(MotionQueryInteractionValidationCodeUVE::DuplicateResource,
                                               index, "motion query interaction resources must be unique");
            }
        }
    }
    for (std::size_t index = 0U; index < asset.requiredResources.size(); ++index) {
        if (!IsValidHandleUVE(asset.requiredResources[index])) {
            return MakeValidationResultUVE(MotionQueryInteractionValidationCodeUVE::InvalidHandle,
                                           index, "motion query interaction required resource is invalid");
        }
        for (std::size_t previous = 0U; previous < index; ++previous) {
            if (asset.requiredResources[previous] == asset.requiredResources[index]) {
                return MakeValidationResultUVE(MotionQueryInteractionValidationCodeUVE::DuplicateResource,
                                               index, "motion query interaction required resources are duplicated");
            }
        }
    }
    return MakeValidationResultUVE(MotionQueryInteractionValidationCodeUVE::Valid, 0U,
                                   "motion query interaction asset is valid");
}

MotionQueryInteractionValidationResultUVE ValidateMotionQueryInteractionIslandUVE(
    const MotionQueryInteractionIslandUVE& island) noexcept {
    if (!IsValidHandleUVE(island.handle)) {
        return MakeValidationResultUVE(MotionQueryInteractionValidationCodeUVE::InvalidHandle, 0U,
                                       "motion query interaction island handle is invalid");
    }
    if (island.islandId.empty() ||
        island.islandId.size() > MotionQueryInteractionIslandUVE::kMaximumIdBytesUVE) {
        return MakeValidationResultUVE(MotionQueryInteractionValidationCodeUVE::InvalidIdentifier,
                                       0U, "motion query interaction island identifier is invalid");
    }
    if (island.interactionAssets.empty()) {
        return MakeValidationResultUVE(MotionQueryInteractionValidationCodeUVE::EmptyIsland, 0U,
                                       "motion query interaction island must contain an asset");
    }
    if (island.interactionAssets.size() > MotionQueryInteractionIslandUVE::kMaximumAssetsUVE) {
        return MakeValidationResultUVE(MotionQueryInteractionValidationCodeUVE::CapacityExceeded, 0U,
                                       "motion query interaction island exceeds asset capacity");
    }
    for (std::size_t index = 0U; index < island.interactionAssets.size(); ++index) {
        if (!IsValidHandleUVE(island.interactionAssets[index])) {
            return MakeValidationResultUVE(MotionQueryInteractionValidationCodeUVE::InvalidHandle,
                                           index, "motion query interaction island asset handle is invalid");
        }
        if (std::find(island.interactionAssets.cbegin(), island.interactionAssets.cbegin() +
                          static_cast<std::ptrdiff_t>(index),
                      island.interactionAssets[index]) !=
            island.interactionAssets.cbegin() + static_cast<std::ptrdiff_t>(index)) {
            return MakeValidationResultUVE(MotionQueryInteractionValidationCodeUVE::DuplicateResource,
                                           index, "motion query interaction island assets must be unique");
        }
    }
    return MakeValidationResultUVE(MotionQueryInteractionValidationCodeUVE::Valid, 0U,
                                   "motion query interaction island is valid");
}

MotionQueryInteractionValidationResultUVE MotionQueryInteractionLibraryUVE::RegisterAssetUVE(
    MotionQueryInteractionAssetUVE asset) noexcept {
    const MotionQueryInteractionValidationResultUVE validation =
        ValidateMotionQueryInteractionAssetUVE(asset);
    if (!validation.IsValidUVE()) {
        return validation;
    }
    if (assets_.size() >= kMaximumAssetsUVE) {
        return MakeValidationResultUVE(MotionQueryInteractionValidationCodeUVE::CapacityExceeded,
                                       assets_.size(), "motion query interaction asset library is full");
    }
    for (const MotionQueryInteractionAssetUVE& existing : assets_) {
        if (existing.handle == asset.handle) {
            return MakeValidationResultUVE(MotionQueryInteractionValidationCodeUVE::DuplicateResource,
                                           0U, "motion query interaction asset handle is already registered");
        }
        if (existing.interactionId == asset.interactionId) {
            return MakeValidationResultUVE(MotionQueryInteractionValidationCodeUVE::DuplicateInteractionId,
                                           0U, "motion query interaction identifier is already registered");
        }
    }
    assets_.push_back(std::move(asset));
    return MakeValidationResultUVE(MotionQueryInteractionValidationCodeUVE::Valid, 0U,
                                   "motion query interaction asset registered");
}

MotionQueryInteractionValidationResultUVE MotionQueryInteractionLibraryUVE::RegisterIslandUVE(
    MotionQueryInteractionIslandUVE island) noexcept {
    const MotionQueryInteractionValidationResultUVE validation =
        ValidateMotionQueryInteractionIslandUVE(island);
    if (!validation.IsValidUVE()) {
        return validation;
    }
    if (islands_.size() >= kMaximumIslandsUVE) {
        return MakeValidationResultUVE(MotionQueryInteractionValidationCodeUVE::CapacityExceeded,
                                       islands_.size(), "motion query interaction island library is full");
    }
    for (const MotionQueryInteractionIslandUVE& existing : islands_) {
        if (existing.handle == island.handle) {
            return MakeValidationResultUVE(MotionQueryInteractionValidationCodeUVE::DuplicateResource,
                                           0U, "motion query interaction island handle is already registered");
        }
        if (existing.islandId == island.islandId) {
            return MakeValidationResultUVE(MotionQueryInteractionValidationCodeUVE::DuplicateIslandId,
                                           0U, "motion query interaction island identifier is already registered");
        }
    }
    for (std::size_t index = 0U; index < island.interactionAssets.size(); ++index) {
        if (std::find_if(assets_.cbegin(), assets_.cend(), [&](const auto& asset) {
                return asset.handle == island.interactionAssets[index];
            }) == assets_.cend()) {
            return MakeValidationResultUVE(MotionQueryInteractionValidationCodeUVE::UnknownInteractionAsset,
                                           index, "motion query interaction island references an unknown asset");
        }
    }
    islands_.push_back(std::move(island));
    return MakeValidationResultUVE(MotionQueryInteractionValidationCodeUVE::Valid, 0U,
                                   "motion query interaction island registered");
}

void MotionQueryInteractionLibraryUVE::ClearUVE() noexcept {
    assets_.clear();
    islands_.clear();
}

MotionQueryInteractionSelectionResultUVE MotionQueryInteractionLibraryUVE::SelectUVE(
    const MotionQueryInteractionSelectionRequestUVE& request) const noexcept {
    if (request.availableResources.size() >
        MotionQueryInteractionSelectionRequestUVE::kMaximumAvailableResourcesUVE) {
        return MakeSelectionResultUVE(MotionQueryInteractionSelectionCodeUVE::InvalidRequest,
                                      "motion query interaction selection resource list exceeds capacity");
    }
    if (request.islandId.has_value() &&
        (request.islandId->empty() || request.islandId->size() > MotionQueryInteractionIslandUVE::kMaximumIdBytesUVE)) {
        return MakeSelectionResultUVE(MotionQueryInteractionSelectionCodeUVE::InvalidRequest,
                                      "motion query interaction selection island identifier is invalid");
    }
    if (request.interactionId.has_value() &&
        (request.interactionId->empty() || request.interactionId->size() > MotionQueryInteractionAssetUVE::kMaximumIdBytesUVE)) {
        return MakeSelectionResultUVE(MotionQueryInteractionSelectionCodeUVE::InvalidRequest,
                                      "motion query interaction selection identifier is invalid");
    }
    for (std::size_t index = 0U; index < request.availableResources.size(); ++index) {
        if (!IsValidHandleUVE(request.availableResources[index])) {
            return MakeSelectionResultUVE(MotionQueryInteractionSelectionCodeUVE::InvalidRequest,
                                          "motion query interaction available resource handle is invalid");
        }
        if (std::find(request.availableResources.cbegin(),
                      request.availableResources.cbegin() + static_cast<std::ptrdiff_t>(index),
                      request.availableResources[index]) !=
            request.availableResources.cbegin() + static_cast<std::ptrdiff_t>(index)) {
            return MakeSelectionResultUVE(MotionQueryInteractionSelectionCodeUVE::InvalidRequest,
                                          "motion query interaction available resources must be unique");
        }
    }

    std::vector<UVE::Asset::ResourceHandleUVE> allowedAssets;
    if (request.islandId.has_value()) {
        const auto island = std::find_if(islands_.cbegin(), islands_.cend(), [&](const auto& value) {
            return value.islandId == *request.islandId;
        });
        if (island == islands_.cend()) {
            return MakeSelectionResultUVE(MotionQueryInteractionSelectionCodeUVE::UnknownIsland,
                                          "motion query interaction selection island is unknown");
        }
        allowedAssets = island->interactionAssets;
    }

    std::vector<const MotionQueryInteractionAssetUVE*> candidates;
    for (const MotionQueryInteractionAssetUVE& asset : assets_) {
        if (!allowedAssets.empty() && !ContainsHandleUVE(allowedAssets, asset.handle)) {
            continue;
        }
        if (request.interactionId.has_value() && asset.interactionId != *request.interactionId) {
            continue;
        }
        bool compatible = true;
        for (const UVE::Asset::ResourceHandleUVE required : asset.requiredResources) {
            if (!ContainsHandleUVE(request.availableResources, required)) {
                compatible = false;
                break;
            }
        }
        if (compatible) {
            candidates.push_back(&asset);
        }
    }
    std::sort(candidates.begin(), candidates.end(), [](const auto* lhs, const auto* rhs) noexcept {
        return IsAssetBeforeUVE(*lhs, *rhs);
    });
    if (candidates.empty()) {
        return MakeSelectionResultUVE(MotionQueryInteractionSelectionCodeUVE::NoCompatibleInteraction,
                                      "motion query interaction selection found no compatible asset");
    }
    const MotionQueryInteractionAssetUVE& selected = *candidates.front();
    MotionQueryInteractionSelectionResultUVE result;
    result.code = MotionQueryInteractionSelectionCodeUVE::Accepted;
    result.interactionAsset = selected.handle;
    result.interactionId = selected.interactionId;
    result.message = "motion query interaction selected";
    return result;
}

MotionQueryInteractionSelectionResultUVE MotionQueryInteractionSubsystemUVE::SelectUVE(
    const MotionQueryInteractionLibraryUVE& library,
    const MotionQueryInteractionSelectionRequestUVE& request) noexcept {
    MotionQueryInteractionSelectionResultUVE result = library.SelectUVE(request);
    if (result.IsAcceptedUVE()) {
        selection_ = result;
    } else {
        selection_.reset();
    }
    return result;
}

void MotionQueryInteractionSubsystemUVE::ClearUVE() noexcept {
    selection_.reset();
}

} // namespace UVE::Plugins
