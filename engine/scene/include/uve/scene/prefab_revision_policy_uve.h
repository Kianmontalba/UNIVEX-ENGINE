// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once
#include <cstdint>
#include "uve/scene/components/prefab_instance_component_uve.h"
namespace UVE::Scene {
enum class PrefabRevisionStatusUVE : std::uint8_t { Invalid = 0, Current, Stale };
enum class PrefabRevisionRefreshDecisionUVE : std::uint8_t { Invalid = 0, NoOp, Refresh, MergeRequired };
/// Compares a nonzero loaded source revision with an instance-recorded revision.
/// Value-only contract; it does not read assets, merge overrides, or mutate prefab instances.
[[nodiscard]] PrefabRevisionStatusUVE EvaluatePrefabRevisionUVE(
    std::uint64_t sourceRevision, std::uint64_t instanceRevision) noexcept;
/// Refreshes both persisted revision fields from a caller-observed source revision only when the
/// instance is valid and has no local overrides. It does not resolve assets or merge state.
[[nodiscard]] bool RefreshPrefabInstanceRevisionUVE(
    PrefabInstanceComponentUVE& instance, std::uint64_t observedSourceRevision) noexcept;
/// Classifies a caller-observed source revision before refresh or merge. A newer source requires
/// merge when local overrides exist; invalid or regressed observations fail closed.
[[nodiscard]] PrefabRevisionRefreshDecisionUVE EvaluatePrefabRevisionRefreshDecisionUVE(
    std::uint64_t observedSourceRevision, std::uint64_t instanceRevision,
    bool hasLocalOverrides) noexcept;
} // namespace UVE::Scene
