// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include "uve/plugins/plugin_registry_uve.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace UVE::Plugins {

inline constexpr std::size_t kNativePluginManifestMaximumIdentifierBytesUVE = 128U;
inline constexpr std::size_t kNativePluginManifestMaximumDisplayNameBytesUVE = 256U;

enum class NativePluginManifestValidationCodeUVE : std::uint8_t {
    InvalidPluginId = 0,
    EmptyDisplayName,
    DisplayNameTooLong,
    UnsupportedProtocol,
    TooManyCapabilities,
    InvalidCapabilityId,
    DuplicateCapabilityId,
};

struct NativePluginManifestDiagnosticUVE final {
    NativePluginManifestValidationCodeUVE code = NativePluginManifestValidationCodeUVE::InvalidPluginId;
    std::size_t capabilityIndex = 0U;
    std::string message;
};

struct NativePluginManifestValidationResultUVE final {
    std::vector<NativePluginManifestDiagnosticUVE> diagnostics;

    [[nodiscard]] bool IsValidUVE() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] NativePluginManifestValidationResultUVE ValidateNativePluginManifestUVE(
    const NativePluginManifestUVE& manifest);

} // namespace UVE::Plugins
