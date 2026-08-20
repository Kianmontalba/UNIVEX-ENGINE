// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once
#include <cstdint>
namespace UVE::Window {
inline constexpr std::uint32_t kMaximumDisplayModeAxisUVE = 16384U;
inline constexpr std::uint32_t kMaximumDisplayModeRefreshRateUVE = 1000U;
struct DisplayModeDescUVE final {
    std::uint32_t width = 1920U;
    std::uint32_t height = 1080U;
    std::uint32_t refreshRateHz = 0U;
};
/// Validates one caller-owned display-mode descriptor; zero refresh requests backend/default mode.
[[nodiscard]] bool ValidateDisplayModeDescUVE(const DisplayModeDescUVE& desc) noexcept;
} // namespace UVE::Window
