// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/asset/resource_dependency_graph_uve.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace UVE::Plugins {

struct MotionQueryInteractionParticipantUVE final {
    static constexpr std::size_t kMaximumRoleBytesUVE = 64U;

    std::string role;
    UVE::Asset::ResourceHandleUVE resource;

    [[nodiscard]] bool operator==(const MotionQueryInteractionParticipantUVE&) const = default;
};

struct MotionQueryInteractionAssetUVE final {
    static constexpr std::size_t kMaximumIdBytesUVE = 128U;
    static constexpr std::size_t kMaximumParticipantsUVE = 8U;

    UVE::Asset::ResourceHandleUVE handle;
    std::string interactionId;
    std::vector<MotionQueryInteractionParticipantUVE> participants;
    std::vector<UVE::Asset::ResourceHandleUVE> requiredResources;

    [[nodiscard]] bool operator==(const MotionQueryInteractionAssetUVE&) const = default;
};

struct MotionQueryInteractionIslandUVE final {
    static constexpr std::size_t kMaximumIdBytesUVE = 128U;
    static constexpr std::size_t kMaximumAssetsUVE = 256U;

    UVE::Asset::ResourceHandleUVE handle;
    std::string islandId;
    std::vector<UVE::Asset::ResourceHandleUVE> interactionAssets;

    [[nodiscard]] bool operator==(const MotionQueryInteractionIslandUVE&) const = default;
};

enum class MotionQueryInteractionValidationCodeUVE : std::uint8_t {
    Valid = 0,
    InvalidHandle,
    InvalidIdentifier,
    CapacityExceeded,
    EmptyParticipants,
    InvalidParticipant,
    DuplicateParticipantRole,
    DuplicateResource,
    UnknownInteractionAsset,
    DuplicateInteractionId,
    DuplicateIslandId,
    EmptyIsland,
};

struct MotionQueryInteractionValidationResultUVE final {
    MotionQueryInteractionValidationCodeUVE code =
        MotionQueryInteractionValidationCodeUVE::InvalidHandle;
    std::size_t index = 0U;
    std::string message;

    [[nodiscard]] bool IsValidUVE() const noexcept {
        return code == MotionQueryInteractionValidationCodeUVE::Valid;
    }
};

[[nodiscard]] MotionQueryInteractionValidationResultUVE ValidateMotionQueryInteractionAssetUVE(
    const MotionQueryInteractionAssetUVE& asset) noexcept;

[[nodiscard]] MotionQueryInteractionValidationResultUVE ValidateMotionQueryInteractionIslandUVE(
    const MotionQueryInteractionIslandUVE& island) noexcept;

struct MotionQueryInteractionSelectionRequestUVE final {
    static constexpr std::size_t kMaximumAvailableResourcesUVE = 256U;

    std::optional<std::string> islandId;
    std::optional<std::string> interactionId;
    std::vector<UVE::Asset::ResourceHandleUVE> availableResources;
};

enum class MotionQueryInteractionSelectionCodeUVE : std::uint8_t {
    Accepted = 0,
    InvalidRequest,
    UnknownIsland,
    NoCompatibleInteraction,
};

struct MotionQueryInteractionSelectionResultUVE final {
    MotionQueryInteractionSelectionCodeUVE code =
        MotionQueryInteractionSelectionCodeUVE::InvalidRequest;
    UVE::Asset::ResourceHandleUVE interactionAsset;
    std::string interactionId;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == MotionQueryInteractionSelectionCodeUVE::Accepted;
    }
};

class MotionQueryInteractionLibraryUVE final {
public:
    static constexpr std::size_t kMaximumAssetsUVE = 4096U;
    static constexpr std::size_t kMaximumIslandsUVE = 256U;

    [[nodiscard]] MotionQueryInteractionValidationResultUVE RegisterAssetUVE(
        MotionQueryInteractionAssetUVE asset) noexcept;
    [[nodiscard]] MotionQueryInteractionValidationResultUVE RegisterIslandUVE(
        MotionQueryInteractionIslandUVE island) noexcept;
    void ClearUVE() noexcept;

    [[nodiscard]] MotionQueryInteractionSelectionResultUVE SelectUVE(
        const MotionQueryInteractionSelectionRequestUVE& request) const noexcept;

    [[nodiscard]] const std::vector<MotionQueryInteractionAssetUVE>& GetAssetsUVE() const noexcept {
        return assets_;
    }

    [[nodiscard]] const std::vector<MotionQueryInteractionIslandUVE>& GetIslandsUVE() const noexcept {
        return islands_;
    }

private:
    std::vector<MotionQueryInteractionAssetUVE> assets_;
    std::vector<MotionQueryInteractionIslandUVE> islands_;
};

class MotionQueryInteractionSubsystemUVE final {
public:
    [[nodiscard]] MotionQueryInteractionSelectionResultUVE SelectUVE(
        const MotionQueryInteractionLibraryUVE& library,
        const MotionQueryInteractionSelectionRequestUVE& request) noexcept;
    void ClearUVE() noexcept;

    [[nodiscard]] bool HasSelectionUVE() const noexcept {
        return selection_.has_value();
    }

    [[nodiscard]] const std::optional<MotionQueryInteractionSelectionResultUVE>&
    GetSelectionUVE() const noexcept {
        return selection_;
    }

private:
    std::optional<MotionQueryInteractionSelectionResultUVE> selection_;
};

} // namespace UVE::Plugins
