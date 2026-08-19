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
} // namespace UVE::Scene
