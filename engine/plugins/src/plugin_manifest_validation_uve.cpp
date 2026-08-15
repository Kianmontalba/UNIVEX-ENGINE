// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/plugin_manifest_validation_uve.h"

#include <algorithm>
#include <cctype>
#include <unordered_set>

namespace UVE::Plugins {
namespace {

[[nodiscard]] bool IsValidIdentifierUVE(const std::string& value) noexcept {
    if (value.empty() || value.size() > kNativePluginManifestMaximumIdentifierBytesUVE) {
        return false;
    }
    return std::all_of(value.cbegin(), value.cend(), [](const char character) {
        const unsigned char byte = static_cast<unsigned char>(character);
        return std::isalnum(byte) != 0 || character == '_' || character == '-' || character == '.';
    });
}

} // namespace

NativePluginManifestValidationResultUVE ValidateNativePluginManifestUVE(
    const NativePluginManifestUVE& manifest) {
    NativePluginManifestValidationResultUVE result;
    if (!IsValidIdentifierUVE(manifest.pluginId)) {
        result.diagnostics.push_back({NativePluginManifestValidationCodeUVE::InvalidPluginId, 0U,
                                      "Plugin ID must be a bounded identifier using letters, digits, '_', '-', or '.'."});
        return result;
    }
    if (manifest.displayName.empty()) {
        result.diagnostics.push_back({NativePluginManifestValidationCodeUVE::EmptyDisplayName, 0U,
                                      "Plugin display name must not be empty."});
        return result;
    }
    if (manifest.displayName.size() > kNativePluginManifestMaximumDisplayNameBytesUVE) {
        result.diagnostics.push_back({NativePluginManifestValidationCodeUVE::DisplayNameTooLong, 0U,
                                      "Plugin display name exceeds the bounded manifest limit."});
        return result;
    }
    if (manifest.requiredEngineProtocol != kNativePluginProtocolVersionUVE) {
        result.diagnostics.push_back({NativePluginManifestValidationCodeUVE::UnsupportedProtocol, 0U,
                                      "Plugin manifest requires an unsupported engine protocol."});
        return result;
    }
    if (manifest.capabilityIds.size() > NativePluginRegistryUVE::kMaximumCapabilitiesPerPluginUVE) {
        result.diagnostics.push_back({NativePluginManifestValidationCodeUVE::TooManyCapabilities, 0U,
                                      "Plugin manifest exceeds the bounded capability limit."});
        return result;
    }

    std::unordered_set<std::string> capabilities;
    for (std::size_t index = 0U; index < manifest.capabilityIds.size(); ++index) {
        const std::string& capability = manifest.capabilityIds[index];
        if (!IsValidIdentifierUVE(capability)) {
            result.diagnostics.push_back({NativePluginManifestValidationCodeUVE::InvalidCapabilityId, index,
                                          "Plugin capability must be a bounded identifier."});
            return result;
        }
        if (!capabilities.insert(capability).second) {
            result.diagnostics.push_back({NativePluginManifestValidationCodeUVE::DuplicateCapabilityId, index,
                                          "Plugin capabilities must be unique."});
            return result;
        }
    }
    return result;
}

} // namespace UVE::Plugins
