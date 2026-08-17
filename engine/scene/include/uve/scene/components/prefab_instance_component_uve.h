// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "uve/asset/asset_guid_uve.h"

namespace UVE::Scene {

inline constexpr std::size_t kMaximumPrefabOverridesUVE = 256U;
inline constexpr std::size_t kMaximumPrefabOverridePathBytesUVE = 256U;
inline constexpr std::size_t kMaximumPrefabOverrideValueBytesUVE = 4096U;

/// A copied property-path/value pair persisted on a prefab instance. The value is serialized data;
/// this foundation deliberately does not interpret or apply it.
struct PrefabPropertyOverrideUVE final {
    std::string propertyPath;
    std::string serializedValue;
};

/// Records which prefab asset an entity was instantiated from — internal infrastructure
/// PrefabSystemUVE owns, not one of the master spec's named built-in components (like
/// HierarchyComponentUVE/WorldTransformComponentUVE are SceneGraphUVE-owned infrastructure).
/// Added by PrefabSystemUVE::InstantiateUVE() to the instantiated root entity; preserved
/// through ordinary save/load like any other component, including its bounded override records
/// (it never drives special re-instantiation behavior during a plain SceneSerializerUVE load — only
/// an explicit InstantiateUVE() call actually instantiates a prefab).
struct PrefabInstanceComponentUVE final {
    Asset::AssetGuidUVE sourcePrefabGuid;
    std::vector<PrefabPropertyOverrideUVE> overrides;
};

/// Validates the persisted source reference without resolving the prefab or reading the asset
/// database. A prefab-instance tag must identify a real source asset; nested instances preserve
/// this value and are still loaded as data rather than recursively re-instantiated.
[[nodiscard]] inline bool IsPrefabInstanceComponentValidUVE(
    const PrefabInstanceComponentUVE& component) noexcept {
    if (component.sourcePrefabGuid == Asset::kInvalidAssetGuidUVE ||
        component.overrides.size() > kMaximumPrefabOverridesUVE) {
        return false;
    }

    std::string_view previousPath;
    for (const PrefabPropertyOverrideUVE& override : component.overrides) {
        if (override.propertyPath.empty() || override.propertyPath.size() > kMaximumPrefabOverridePathBytesUVE ||
            override.serializedValue.empty() || override.serializedValue.size() > kMaximumPrefabOverrideValueBytesUVE ||
            override.propertyPath.find('\0') != std::string_view::npos ||
            override.serializedValue.find('\0') != std::string_view::npos ||
            (!previousPath.empty() && override.propertyPath <= previousPath)) {
            return false;
        }
        previousPath = override.propertyPath;
    }
    return true;
}

} // namespace UVE::Scene
