// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include "uve/plugins/plugin_manifest_validation_uve.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace UVE::Plugins {

inline constexpr char kMotionQueryPluginIdUVE[] = "uve.motion_query";
inline constexpr char kMotionQueryPluginDisplayNameUVE[] = "UniVex Motion Query";
inline constexpr char kMotionQueryCapabilityRuntimeUVE[] = "motion_query.runtime";
inline constexpr char kMotionQueryCapabilityAssetSamplingUVE[] = "motion_query.asset_sampling";
inline constexpr char kMotionQueryCapabilityEditorUVE[] = "motion_query.editor";
inline constexpr char kMotionQueryCapabilityDiagnosticsUVE[] = "motion_query.diagnostics";

inline constexpr std::size_t kMotionQueryPluginConfigurationMaximumCopyrightBytesUVE = 256U;
inline constexpr std::size_t kMotionQueryPluginConfigurationMaximumLayoutPathBytesUVE = 128U;

struct MotionQueryPluginConfigurationUVE final {
    std::string copyright;
    NativePluginManifestUVE manifest;
    std::string runtimePath;
    std::string editorPath;
    std::string sharedPath;
    std::string testsPath;

    [[nodiscard]] bool operator==(const MotionQueryPluginConfigurationUVE&) const = default;
};

enum class MotionQueryPluginConfigurationCodeUVE : std::uint8_t {
    Valid = 0,
    InvalidJson,
    InvalidRoot,
    InvalidField,
    InvalidVersion,
    InvalidLayout,
    ManifestMismatch,
};

struct MotionQueryPluginConfigurationResultUVE final {
    MotionQueryPluginConfigurationCodeUVE code = MotionQueryPluginConfigurationCodeUVE::InvalidRoot;
    std::size_t index = 0U;
    std::string message;
    std::optional<MotionQueryPluginConfigurationUVE> configuration;

    [[nodiscard]] bool IsValidUVE() const noexcept {
        return code == MotionQueryPluginConfigurationCodeUVE::Valid;
    }
};

struct MotionQueryPluginDescriptorUVE final {
    NativePluginManifestUVE manifest;
    std::vector<std::string> featureIds;

    [[nodiscard]] bool operator==(const MotionQueryPluginDescriptorUVE&) const = default;
};

[[nodiscard]] MotionQueryPluginDescriptorUVE MakeMotionQueryPluginDescriptorUVE();

[[nodiscard]] MotionQueryPluginConfigurationResultUVE ParseMotionQueryPluginConfigurationUVE(
    std::string_view document);

[[nodiscard]] MotionQueryPluginConfigurationResultUVE ValidateMotionQueryPluginConfigurationUVE(
    const MotionQueryPluginConfigurationUVE& configuration) noexcept;

[[nodiscard]] MotionQueryPluginConfigurationResultUVE ValidateMotionQueryPluginConfigurationParityUVE(
    const MotionQueryPluginConfigurationUVE& configuration,
    const MotionQueryPluginDescriptorUVE& descriptor) noexcept;

[[nodiscard]] NativePluginRegistryResultUVE RegisterMotionQueryPluginUVE(
    NativePluginRegistryUVE& registry);

[[nodiscard]] NativePluginRegistryResultUVE RegisterMotionQueryPluginUVE(
    NativePluginRegistryUVE& registry, const NativePluginCapabilityPolicyUVE& policy);

} // namespace UVE::Plugins
