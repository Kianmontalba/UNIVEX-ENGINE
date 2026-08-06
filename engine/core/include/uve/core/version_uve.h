//                                      UVE
//                                UniVex Engine
//
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.


#pragma once

#include <cstdint>
#include <string>

namespace UVE::Core {

/// A four-component engine version number (major.minor.patch+build).
/// EngineCoreUVE::GetEngineVersionUVE() is the single source of truth for
/// the engine's current version; future systems (assets, plugins, project
/// files, crash reports, Hub compatibility checks) are expected to reuse
/// this type rather than tracking their own version fields.
/// Thread-safety: value type, safe to copy/move/compare freely.
struct VersionUVE {
    std::uint32_t major = 0;
    std::uint32_t minor = 0;
    std::uint32_t patch = 0;
    std::uint32_t build = 0;

    /// Formats as "MAJOR.MINOR.PATCH+build.BUILD", e.g. "0.1.0+build.1".
    [[nodiscard]] std::string ToStringUVE() const;
};

[[nodiscard]] constexpr bool operator==(const VersionUVE& lhs, const VersionUVE& rhs) noexcept {
    return lhs.major == rhs.major && lhs.minor == rhs.minor && lhs.patch == rhs.patch &&
           lhs.build == rhs.build;
}

[[nodiscard]] constexpr bool operator!=(const VersionUVE& lhs, const VersionUVE& rhs) noexcept {
    return !(lhs == rhs);
}

} // namespace UVE::Core
