// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/plugins/plugin_manifest_validation_uve.h"

#include <string>
#include <vector>

namespace UVE::Plugins {

inline constexpr char kMotionQueryPluginIdUVE[] = "uve.motion_query";
inline constexpr char kMotionQueryPluginDisplayNameUVE[] = "UniVex Motion Query";
inline constexpr char kMotionQueryCapabilityRuntimeUVE[] = "motion_query.runtime";
inline constexpr char kMotionQueryCapabilityAssetSamplingUVE[] = "motion_query.asset_sampling";
inline constexpr char kMotionQueryCapabilityEditorUVE[] = "motion_query.editor";
inline constexpr char kMotionQueryCapabilityDiagnosticsUVE[] = "motion_query.diagnostics";

struct MotionQueryPluginDescriptorUVE final {
    NativePluginManifestUVE manifest;
    std::vector<std::string> featureIds;

    [[nodiscard]] bool operator==(const MotionQueryPluginDescriptorUVE&) const = default;
};

[[nodiscard]] MotionQueryPluginDescriptorUVE MakeMotionQueryPluginDescriptorUVE();

[[nodiscard]] NativePluginRegistryResultUVE RegisterMotionQueryPluginUVE(
    NativePluginRegistryUVE& registry);

[[nodiscard]] NativePluginRegistryResultUVE RegisterMotionQueryPluginUVE(
    NativePluginRegistryUVE& registry, const NativePluginCapabilityPolicyUVE& policy);

} // namespace UVE::Plugins
