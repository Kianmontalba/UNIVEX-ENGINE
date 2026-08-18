// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/plugins/motion_query_plugin_module_uve.h"

#include <charconv>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string_view>
#include <system_error>
#include <utility>

namespace UVE::Plugins {
namespace {

[[nodiscard]] MotionQueryPluginConfigurationResultUVE MakeConfigurationResultUVE(
    const MotionQueryPluginConfigurationCodeUVE code, const std::size_t index,
    const char* message) {
    return MotionQueryPluginConfigurationResultUVE{code, index, message, std::nullopt};
}

[[nodiscard]] bool IsBoundedRelativePathUVE(const std::string& value) noexcept {
    return !value.empty() && value.size() <= kMotionQueryPluginConfigurationMaximumLayoutPathBytesUVE &&
           value.front() != '/' && value.find('\\') == std::string::npos && value.find("..") == std::string::npos;
}

[[nodiscard]] std::optional<std::uint16_t> ParseVersionComponentUVE(const std::string_view value) noexcept {
    if (value.empty()) {
        return std::nullopt;
    }
    std::uint32_t parsed = 0U;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (error != std::errc{} || end != value.data() + value.size() || parsed > UINT16_MAX) {
        return std::nullopt;
    }
    return static_cast<std::uint16_t>(parsed);
}

[[nodiscard]] std::optional<NativePluginVersionUVE> ParseVersionUVE(const std::string_view value) noexcept {
    const std::size_t firstSeparator = value.find('.');
    const std::size_t secondSeparator = firstSeparator == std::string_view::npos
        ? std::string_view::npos
        : value.find('.', firstSeparator + 1U);
    if (firstSeparator == std::string_view::npos || secondSeparator == std::string_view::npos ||
        value.find('.', secondSeparator + 1U) != std::string_view::npos) {
        return std::nullopt;
    }
    const std::optional<std::uint16_t> major =
        ParseVersionComponentUVE(value.substr(0U, firstSeparator));
    const std::optional<std::uint16_t> minor =
        ParseVersionComponentUVE(value.substr(firstSeparator + 1U, secondSeparator - firstSeparator - 1U));
    const std::optional<std::uint16_t> patch =
        ParseVersionComponentUVE(value.substr(secondSeparator + 1U));
    if (!major.has_value() || !minor.has_value() || !patch.has_value()) {
        return std::nullopt;
    }
    return NativePluginVersionUVE{*major, *minor, *patch};
}

[[nodiscard]] bool IsStringArrayUVE(const nlohmann::json& value) {
    if (!value.is_array() || value.size() > NativePluginRegistryUVE::kMaximumCapabilitiesPerPluginUVE) {
        return false;
    }
    for (const nlohmann::json& item : value) {
        if (!item.is_string()) {
            return false;
        }
    }
    return true;
}

} // namespace

MotionQueryPluginDescriptorUVE MakeMotionQueryPluginDescriptorUVE() {
    MotionQueryPluginDescriptorUVE descriptor;
    descriptor.manifest.pluginId = kMotionQueryPluginIdUVE;
    descriptor.manifest.displayName = kMotionQueryPluginDisplayNameUVE;
    descriptor.manifest.version = NativePluginVersionUVE{1U, 0U, 0U};
    descriptor.manifest.requiredEngineProtocol = kNativePluginProtocolVersionUVE;
    descriptor.manifest.capabilityIds = {
        kMotionQueryCapabilityRuntimeUVE,
        kMotionQueryCapabilityAssetSamplingUVE,
        kMotionQueryCapabilityEditorUVE,
        kMotionQueryCapabilityDiagnosticsUVE,
    };
    descriptor.featureIds = {
        "motion_query.features",
        "motion_query.trajectory",
        "motion_query.history",
        "motion_query.database",
        "motion_query.matching",
    };
    return descriptor;
}

MotionQueryPluginConfigurationResultUVE ParseMotionQueryPluginConfigurationUVE(
    const std::string_view document) {
    try {
        const nlohmann::json root = nlohmann::json::parse(document);
        if (!root.is_object() || root.size() != 7U) {
            return MakeConfigurationResultUVE(MotionQueryPluginConfigurationCodeUVE::InvalidRoot, 0U,
                                              "Motion Query plugin configuration root is invalid.");
        }
        const nlohmann::json& layout = root.at("layout");
        const nlohmann::json& capabilities = root.at("capabilities");
        if (!root.at("copyright").is_string() || !root.at("pluginId").is_string() ||
            !root.at("displayName").is_string() || !root.at("version").is_string() ||
            !root.at("requiredEngineProtocol").is_number_unsigned() || !IsStringArrayUVE(capabilities) ||
            !layout.is_object() || layout.size() != 4U) {
            return MakeConfigurationResultUVE(MotionQueryPluginConfigurationCodeUVE::InvalidField, 0U,
                                              "Motion Query plugin configuration contains an invalid field.");
        }
        const std::optional<NativePluginVersionUVE> version =
            ParseVersionUVE(root.at("version").get<std::string>());
        if (!version.has_value()) {
            return MakeConfigurationResultUVE(MotionQueryPluginConfigurationCodeUVE::InvalidVersion, 0U,
                                              "Motion Query plugin configuration version must be major.minor.patch.");
        }
        if (!layout.contains("runtime") || !layout.contains("editor") || !layout.contains("shared") ||
            !layout.contains("tests") || !layout.at("runtime").is_string() ||
            !layout.at("editor").is_string() || !layout.at("shared").is_string() ||
            !layout.at("tests").is_string()) {
            return MakeConfigurationResultUVE(MotionQueryPluginConfigurationCodeUVE::InvalidLayout, 0U,
                                              "Motion Query plugin configuration layout is invalid.");
        }
        MotionQueryPluginConfigurationUVE configuration;
        configuration.copyright = root.at("copyright").get<std::string>();
        configuration.manifest.pluginId = root.at("pluginId").get<std::string>();
        configuration.manifest.displayName = root.at("displayName").get<std::string>();
        configuration.manifest.version = *version;
        configuration.manifest.requiredEngineProtocol = root.at("requiredEngineProtocol").get<std::uint32_t>();
        configuration.manifest.capabilityIds = capabilities.get<std::vector<std::string>>();
        configuration.runtimePath = layout.at("runtime").get<std::string>();
        configuration.editorPath = layout.at("editor").get<std::string>();
        configuration.sharedPath = layout.at("shared").get<std::string>();
        configuration.testsPath = layout.at("tests").get<std::string>();
        MotionQueryPluginConfigurationResultUVE result =
            ValidateMotionQueryPluginConfigurationUVE(configuration);
        if (result.IsValidUVE()) {
            result.configuration = std::move(configuration);
        }
        return result;
    } catch (const nlohmann::json::exception&) {
        return MakeConfigurationResultUVE(MotionQueryPluginConfigurationCodeUVE::InvalidJson, 0U,
                                          "Motion Query plugin configuration JSON is malformed.");
    }
}

MotionQueryPluginConfigurationResultUVE ValidateMotionQueryPluginConfigurationUVE(
    const MotionQueryPluginConfigurationUVE& configuration) noexcept {
    if (configuration.copyright.empty() ||
        configuration.copyright.size() > kMotionQueryPluginConfigurationMaximumCopyrightBytesUVE) {
        return MakeConfigurationResultUVE(MotionQueryPluginConfigurationCodeUVE::InvalidField, 0U,
                                          "Motion Query plugin configuration copyright is invalid.");
    }
    const NativePluginManifestValidationResultUVE manifestValidation =
        ValidateNativePluginManifestUVE(configuration.manifest);
    if (!manifestValidation.IsValidUVE()) {
        return MakeConfigurationResultUVE(MotionQueryPluginConfigurationCodeUVE::InvalidField, 0U,
                                          "Motion Query plugin configuration manifest is invalid.");
    }
    if (!IsBoundedRelativePathUVE(configuration.runtimePath) ||
        !IsBoundedRelativePathUVE(configuration.editorPath) ||
        !IsBoundedRelativePathUVE(configuration.sharedPath) ||
        !IsBoundedRelativePathUVE(configuration.testsPath)) {
        return MakeConfigurationResultUVE(MotionQueryPluginConfigurationCodeUVE::InvalidLayout, 0U,
                                          "Motion Query plugin configuration layout paths are invalid.");
    }
    return MakeConfigurationResultUVE(MotionQueryPluginConfigurationCodeUVE::Valid, 0U, "valid");
}

MotionQueryPluginConfigurationResultUVE ValidateMotionQueryPluginConfigurationParityUVE(
    const MotionQueryPluginConfigurationUVE& configuration,
    const MotionQueryPluginDescriptorUVE& descriptor) noexcept {
    const MotionQueryPluginConfigurationResultUVE validation =
        ValidateMotionQueryPluginConfigurationUVE(configuration);
    if (!validation.IsValidUVE()) {
        return validation;
    }
    if (configuration.manifest != descriptor.manifest) {
        return MakeConfigurationResultUVE(MotionQueryPluginConfigurationCodeUVE::ManifestMismatch, 0U,
                                          "Motion Query configuration manifest does not match native descriptor.");
    }
    if (configuration.runtimePath != "Source/Runtime" || configuration.editorPath != "Source/Editor" ||
        configuration.sharedPath != "Source/Shared" || configuration.testsPath != "Tests") {
        return MakeConfigurationResultUVE(MotionQueryPluginConfigurationCodeUVE::InvalidLayout, 0U,
                                          "Motion Query configuration layout does not match native plugin organization.");
    }
    return MakeConfigurationResultUVE(MotionQueryPluginConfigurationCodeUVE::Valid, 0U, "valid");
}

NativePluginRegistryResultUVE RegisterMotionQueryPluginUVE(NativePluginRegistryUVE& registry) {
    return registry.RegisterManifestUVE(MakeMotionQueryPluginDescriptorUVE().manifest);
}

NativePluginRegistryResultUVE RegisterMotionQueryPluginUVE(
    NativePluginRegistryUVE& registry, const NativePluginCapabilityPolicyUVE& policy) {
    return registry.RegisterManifestUVE(MakeMotionQueryPluginDescriptorUVE().manifest, policy);
}

} // namespace UVE::Plugins
