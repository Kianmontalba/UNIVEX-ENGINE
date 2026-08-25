// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cmath>
#include <cstddef>
#include <string>
#include <string_view>

namespace UVE::Scene {

inline constexpr std::size_t kMaximumAnimationClipAssetPathBytesUVE = 1024U;

/// Authored scene animation-player state. The clip path is a project-relative identity; clip
/// decoding and pose evaluation remain owned by the core animation contracts. Runtime playback
/// state is intentionally not stored here so scene serialization remains deterministic.
struct AnimationPlayerComponentUVE final {
    std::string clipAssetPath;
    float playbackSpeed = 1.0F;
    bool looping = true;
    bool playOnAwake = true;
    bool enabled = true;
};

[[nodiscard]] inline bool IsAnimationClipAssetPathValidUVE(const std::string_view path) noexcept {
    if (path.empty() || path.size() > kMaximumAnimationClipAssetPathBytesUVE ||
        path.find('\0') != std::string_view::npos || path.find('\\') != std::string_view::npos ||
        path.find(':') != std::string_view::npos || path.front() == '/') {
        return path.empty();
    }

    std::size_t segmentStart = 0U;
    for (std::size_t index = 0U; index <= path.size(); ++index) {
        if (index != path.size() && path[index] != '/') {
            continue;
        }
        const std::string_view segment = path.substr(segmentStart, index - segmentStart);
        if (segment.empty() || segment == "." || segment == "..") {
            return false;
        }
        segmentStart = index + 1U;
    }
    return true;
}

[[nodiscard]] inline bool IsAnimationPlayerComponentValidUVE(
    const AnimationPlayerComponentUVE& component) noexcept {
    return IsAnimationClipAssetPathValidUVE(component.clipAssetPath) &&
           std::isfinite(component.playbackSpeed) && component.playbackSpeed > 0.0F;
}

} // namespace UVE::Scene
