// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstddef>
#include <cstdint>
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

enum class PrefabOverrideOperationCodeUVE : std::uint8_t {
    Applied = 0,
    InvalidInstance,
    InvalidBaseline,
    ReadFailed,
    WriteFailed,
    RollbackFailed,
};

struct PrefabOverrideOperationResultUVE final {
    PrefabOverrideOperationCodeUVE code = PrefabOverrideOperationCodeUVE::InvalidInstance;
    std::size_t affectedCount = 0U;
    std::string message;

    [[nodiscard]] bool IsAppliedUVE() const noexcept {
        return code == PrefabOverrideOperationCodeUVE::Applied;
    }
};

/// Main-thread value boundary for applying serialized override values to one live instance. The
/// target resolves property paths for a concrete component/property implementation; it never owns
/// the prefab asset or the instance entity lifetime.
[[nodiscard]] inline bool IsPrefabInstanceComponentValidUVE(
    const PrefabInstanceComponentUVE& component) noexcept;

class IPrefabOverrideTargetUVE {
public:
    virtual ~IPrefabOverrideTargetUVE() = default;

    [[nodiscard]] virtual bool ReadPropertyUVE(std::string_view propertyPath,
                                               std::string& serializedValue) const = 0;
    [[nodiscard]] virtual bool WritePropertyUVE(std::string_view propertyPath,
                                                std::string_view serializedValue) = 0;
};

/// Applies the instance's sorted override records with rollback if any target write fails. The
/// instance record and source prefab remain unchanged; only the caller-owned live target mutates.
[[nodiscard]] inline PrefabOverrideOperationResultUVE ApplyPrefabOverridesUVE(
    const PrefabInstanceComponentUVE& instance, IPrefabOverrideTargetUVE& target) {
    if (!IsPrefabInstanceComponentValidUVE(instance)) {
        return {PrefabOverrideOperationCodeUVE::InvalidInstance, 0U,
                "Prefab override apply rejected because the instance data is invalid."};
    }
    std::vector<PrefabPropertyOverrideUVE> previousValues;
    previousValues.reserve(instance.overrides.size());
    for (const PrefabPropertyOverrideUVE& override : instance.overrides) {
        std::string previousValue;
        if (!target.ReadPropertyUVE(override.propertyPath, previousValue) || previousValue.empty() ||
            previousValue.size() > kMaximumPrefabOverrideValueBytesUVE ||
            previousValue.find('\0') != std::string::npos) {
            return {PrefabOverrideOperationCodeUVE::ReadFailed, previousValues.size(),
                    "Prefab override apply could not read the existing target property."};
        }
        if (!target.WritePropertyUVE(override.propertyPath, override.serializedValue)) {
            for (std::size_t index = previousValues.size(); index > 0U; --index) {
                const PrefabPropertyOverrideUVE& previous = previousValues[index - 1U];
                if (!target.WritePropertyUVE(previous.propertyPath, previous.serializedValue)) {
                    return {PrefabOverrideOperationCodeUVE::RollbackFailed, index - 1U,
                            "Prefab override apply failed and rollback could not restore the target."};
                }
            }
            return {PrefabOverrideOperationCodeUVE::WriteFailed, previousValues.size(),
                    "Prefab override apply failed; target writes were rolled back."};
        }
        previousValues.push_back({override.propertyPath, std::move(previousValue)});
    }
    return {PrefabOverrideOperationCodeUVE::Applied, instance.overrides.size(),
            "Prefab overrides applied to the live target."};
}

/// Restores a caller-supplied sorted baseline and clears the instance override records only after
/// the target is fully restored. The baseline is explicit because persistence intentionally does
/// not invent or silently fetch source-prefab state.
[[nodiscard]] inline PrefabOverrideOperationResultUVE RevertPrefabOverridesUVE(
    PrefabInstanceComponentUVE& instance, const std::vector<PrefabPropertyOverrideUVE>& baseline,
    IPrefabOverrideTargetUVE& target) {
    if (!IsPrefabInstanceComponentValidUVE(instance)) {
        return {PrefabOverrideOperationCodeUVE::InvalidInstance, 0U,
                "Prefab override revert rejected because the instance data is invalid."};
    }
    PrefabInstanceComponentUVE baselineInstance{instance.sourcePrefabGuid, baseline};
    if (!IsPrefabInstanceComponentValidUVE(baselineInstance) || baseline.size() != instance.overrides.size() ||
        !std::equal(baseline.begin(), baseline.end(), instance.overrides.begin(),
                    [](const PrefabPropertyOverrideUVE& lhs, const PrefabPropertyOverrideUVE& rhs) {
                        return lhs.propertyPath == rhs.propertyPath;
                    })) {
        return {PrefabOverrideOperationCodeUVE::InvalidBaseline, 0U,
                "Prefab override revert rejected because the baseline paths do not match the instance overrides."};
    }
    const PrefabOverrideOperationResultUVE applied = ApplyPrefabOverridesUVE(baselineInstance, target);
    if (!applied.IsAppliedUVE()) {
        return applied;
    }
    const std::size_t revertedCount = instance.overrides.size();
    instance.overrides.clear();
    return {PrefabOverrideOperationCodeUVE::Applied, revertedCount,
            "Prefab overrides reverted to the supplied baseline."};
}

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
