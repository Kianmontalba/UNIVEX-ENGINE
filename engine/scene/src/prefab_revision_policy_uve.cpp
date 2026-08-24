// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/scene/prefab_revision_policy_uve.h"
namespace UVE::Scene {
PrefabRevisionStatusUVE EvaluatePrefabRevisionUVE(const std::uint64_t sourceRevision,
                                                  const std::uint64_t instanceRevision) noexcept {
    if (sourceRevision == 0U || instanceRevision == 0U) {
        return PrefabRevisionStatusUVE::Invalid;
    }
    return sourceRevision == instanceRevision ? PrefabRevisionStatusUVE::Current
                                               : PrefabRevisionStatusUVE::Stale;
}

bool RefreshPrefabInstanceRevisionUVE(PrefabInstanceComponentUVE& instance,
                                      const std::uint64_t observedSourceRevision) noexcept {
    if (!IsPrefabInstanceComponentValidUVE(instance) || observedSourceRevision == 0U ||
        observedSourceRevision < instance.sourceRevision || !instance.overrides.empty()) {
        return false;
    }
    instance.sourceRevision = observedSourceRevision;
    instance.instanceRevision = observedSourceRevision;
    return true;
}

PrefabRevisionRefreshDecisionUVE EvaluatePrefabRevisionRefreshDecisionUVE(
    const std::uint64_t observedSourceRevision, const std::uint64_t instanceRevision,
    const bool hasLocalOverrides) noexcept {
    if (observedSourceRevision == 0U || instanceRevision == 0U ||
        observedSourceRevision < instanceRevision) {
        return PrefabRevisionRefreshDecisionUVE::Invalid;
    }
    if (observedSourceRevision == instanceRevision) {
        return PrefabRevisionRefreshDecisionUVE::NoOp;
    }
    return hasLocalOverrides ? PrefabRevisionRefreshDecisionUVE::MergeRequired
                             : PrefabRevisionRefreshDecisionUVE::Refresh;
}
} // namespace UVE::Scene
