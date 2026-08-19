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
inline constexpr std::size_t kNativePluginCapabilityPolicyMaximumEntriesUVE = 64U;

struct NativePluginCapabilityPolicyUVE final {
    bool allowAllCapabilities = true;
    std::vector<std::string> allowedCapabilityIds;
};

enum class NativePluginAbiNegotiationCodeUVE : std::uint8_t {
    Compatible = 0,
    InvalidEngineProtocol,
    UnsupportedPluginProtocol,
};

struct NativePluginAbiNegotiationResultUVE final {
    NativePluginAbiNegotiationCodeUVE code = NativePluginAbiNegotiationCodeUVE::InvalidEngineProtocol;
    std::uint32_t engineProtocol = 0U;
    std::uint32_t pluginProtocol = 0U;
    std::string message;

    [[nodiscard]] bool IsCompatibleUVE() const noexcept {
        return code == NativePluginAbiNegotiationCodeUVE::Compatible;
    }
};

enum class NativePluginManifestValidationCodeUVE : std::uint8_t {
    InvalidPluginId = 0,
    EmptyDisplayName,
    DisplayNameTooLong,
    UnsupportedProtocol,
    TooManyCapabilities,
    InvalidCapabilityId,
    DuplicateCapabilityId,
    CapabilityPolicyTooLarge,
    DuplicatePolicyCapabilityId,
    CapabilityNotAllowed,
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

[[nodiscard]] NativePluginAbiNegotiationResultUVE NegotiateNativePluginAbiUVE(
    const NativePluginManifestUVE& manifest,
    std::uint32_t engineProtocol = kNativePluginProtocolVersionUVE);

[[nodiscard]] NativePluginManifestValidationResultUVE ValidateNativePluginManifestUVE(
    const NativePluginManifestUVE& manifest);

[[nodiscard]] NativePluginManifestValidationResultUVE ValidateNativePluginManifestUVE(
    const NativePluginManifestUVE& manifest,
    const NativePluginCapabilityPolicyUVE& policy);

} // namespace UVE::Plugins
