// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace UVE::Plugins {

inline constexpr std::uint32_t kNativePluginProtocolVersionUVE = 1U;

enum class NativePluginRegistryCodeUVE : std::uint8_t {
    Accepted = 0,
    Rejected,
    NotFound,
    Busy,
    InvalidScope,
};

struct NativePluginVersionUVE final {
    std::uint16_t major = 0U;
    std::uint16_t minor = 0U;
    std::uint16_t patch = 0U;

    [[nodiscard]] constexpr bool operator==(const NativePluginVersionUVE&) const noexcept = default;
};

struct NativePluginManifestUVE final {
    std::string pluginId;
    std::string displayName;
    NativePluginVersionUVE version;
    std::uint32_t requiredEngineProtocol = kNativePluginProtocolVersionUVE;
    std::vector<std::string> capabilityIds;

    [[nodiscard]] bool operator==(const NativePluginManifestUVE&) const = default;
};

struct NativePluginRegistrationScopeUVE final {
    std::string pluginId;
    std::uint64_t generation = 0U;

    [[nodiscard]] bool IsValidUVE() const noexcept {
        return !pluginId.empty() && generation != 0U;
    }
};

struct NativePluginRegistryResultUVE final {
    NativePluginRegistryCodeUVE code = NativePluginRegistryCodeUVE::Rejected;
    std::string message;

    [[nodiscard]] bool IsAcceptedUVE() const noexcept {
        return code == NativePluginRegistryCodeUVE::Accepted;
    }
};

class NativePluginRegistryUVE final {
public:
    static constexpr std::size_t kMaximumPluginsUVE = 64U;
    static constexpr std::size_t kMaximumCapabilitiesPerPluginUVE = 32U;

    NativePluginRegistryUVE() = default;
    NativePluginRegistryUVE(const NativePluginRegistryUVE&) = delete;
    NativePluginRegistryUVE& operator=(const NativePluginRegistryUVE&) = delete;

    [[nodiscard]] NativePluginRegistryResultUVE RegisterManifestUVE(NativePluginManifestUVE manifest);
    [[nodiscard]] std::optional<NativePluginRegistrationScopeUVE> OpenScopeUVE(std::string_view pluginId);
    [[nodiscard]] NativePluginRegistryResultUVE CloseScopeUVE(const NativePluginRegistrationScopeUVE& scope);
    [[nodiscard]] const NativePluginManifestUVE* FindManifestUVE(std::string_view pluginId) const noexcept;
    [[nodiscard]] bool IsScopeOpenUVE(std::string_view pluginId) const noexcept;
    [[nodiscard]] std::size_t GetManifestCountUVE() const noexcept;
    [[nodiscard]] std::size_t GetOpenScopeCountUVE() const noexcept;

private:
    struct EntryUVE final {
        NativePluginManifestUVE manifest;
        std::uint64_t generation = 0U;
        bool scopeOpen = false;
    };

    [[nodiscard]] static bool IsValidIdentifierUVE(const std::string& value) noexcept;

    std::unordered_map<std::string, EntryUVE> m_entries;
    std::uint64_t m_nextGeneration = 1U;
};

} // namespace UVE::Plugins
