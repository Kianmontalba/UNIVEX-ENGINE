// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once
#include <cstdint>
namespace UVE::Scene {
enum class PrefabRevisionStatusUVE : std::uint8_t { Invalid = 0, Current, Stale };
/// Compares a nonzero loaded source revision with an instance-recorded revision.
/// Value-only contract; it does not read assets, merge overrides, or mutate prefab instances.
[[nodiscard]] PrefabRevisionStatusUVE EvaluatePrefabRevisionUVE(
    std::uint64_t sourceRevision, std::uint64_t instanceRevision) noexcept;
} // namespace UVE::Scene
