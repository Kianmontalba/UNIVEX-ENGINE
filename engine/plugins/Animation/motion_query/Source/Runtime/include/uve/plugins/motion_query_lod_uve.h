// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace UVE::Plugins {

enum class MotionQueryQualityTierUVE : std::uint8_t {
    Full = 0,
    Reduced,
    Minimal,
};

enum class MotionQuerySearchBudgetCodeUVE : std::uint8_t {
    Accepted = 0,
    Downgraded,
    InvalidResultCount,
    InvalidQualityTier,
};

struct MotionQuerySearchBudgetUVE final {
    static constexpr std::size_t kMaximumResultsUVE = 4096U;
    static constexpr std::size_t kReducedMaximumResultsUVE = 8U;
    static constexpr std::size_t kMinimalMaximumResultsUVE = 1U;

    MotionQuerySearchBudgetCodeUVE code = MotionQuerySearchBudgetCodeUVE::InvalidResultCount;
    std::size_t requestedResults = 0U;
    std::size_t effectiveResults = 0U;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == MotionQuerySearchBudgetCodeUVE::Accepted ||
               code == MotionQuerySearchBudgetCodeUVE::Downgraded;
    }

    [[nodiscard]] bool WasDowngradedUVE() const noexcept {
        return code == MotionQuerySearchBudgetCodeUVE::Downgraded;
    }
};

[[nodiscard]] MotionQuerySearchBudgetUVE ResolveMotionQuerySearchBudgetUVE(
    MotionQueryQualityTierUVE quality, std::size_t requestedResults) noexcept;

} // namespace UVE::Plugins
