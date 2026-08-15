// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
#include "uve/plugins/plugin_registry_uve.h"

#include <algorithm>
#include <cctype>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace UVE::Plugins {

NativePluginRegistryResultUVE NativePluginRegistryUVE::RegisterManifestUVE(NativePluginManifestUVE manifest) {
    if (!IsValidIdentifierUVE(manifest.pluginId) || manifest.displayName.empty() ||
        manifest.requiredEngineProtocol != kNativePluginProtocolVersionUVE ||
        m_entries.contains(manifest.pluginId) || m_entries.size() >= kMaximumPluginsUVE ||
        manifest.capabilityIds.size() > kMaximumCapabilitiesPerPluginUVE) {
        return {NativePluginRegistryCodeUVE::Rejected, "Plugin manifest was rejected by the native protocol boundary."};
    }
    std::unordered_set<std::string> capabilities;
    for (const std::string& capability : manifest.capabilityIds) {
        if (!IsValidIdentifierUVE(capability) || !capabilities.insert(capability).second) {
            return {NativePluginRegistryCodeUVE::Rejected, "Plugin capabilities must be unique bounded identifiers."};
        }
    }
    const std::string pluginId = manifest.pluginId;
    m_entries.emplace(pluginId, EntryUVE{std::move(manifest), 0U, false});
    return {NativePluginRegistryCodeUVE::Accepted, "Plugin manifest registered."};
}

std::optional<NativePluginRegistrationScopeUVE> NativePluginRegistryUVE::OpenScopeUVE(
    const std::string_view pluginId) {
    const auto iterator = m_entries.find(std::string(pluginId));
    if (iterator == m_entries.end() || iterator->second.scopeOpen) {
        return std::nullopt;
    }
    if (m_nextGeneration == 0U) {
        return std::nullopt;
    }
    iterator->second.generation = m_nextGeneration++;
    iterator->second.scopeOpen = true;
    return NativePluginRegistrationScopeUVE{iterator->second.manifest.pluginId, iterator->second.generation};
}

NativePluginRegistryResultUVE NativePluginRegistryUVE::CloseScopeUVE(
    const NativePluginRegistrationScopeUVE& scope) {
    if (!scope.IsValidUVE()) {
        return {NativePluginRegistryCodeUVE::InvalidScope, "The plugin registration scope is invalid."};
    }
    const auto iterator = m_entries.find(scope.pluginId);
    if (iterator == m_entries.end()) {
        return {NativePluginRegistryCodeUVE::NotFound, "The plugin manifest is not registered."};
    }
    if (!iterator->second.scopeOpen || iterator->second.generation != scope.generation) {
        return {NativePluginRegistryCodeUVE::InvalidScope, "The plugin registration scope is stale or already closed."};
    }
    iterator->second.scopeOpen = false;
    iterator->second.generation = 0U;
    return {NativePluginRegistryCodeUVE::Accepted, "Plugin registration scope closed."};
}

const NativePluginManifestUVE* NativePluginRegistryUVE::FindManifestUVE(
    const std::string_view pluginId) const noexcept {
    const auto iterator = m_entries.find(std::string(pluginId));
    return iterator == m_entries.end() ? nullptr : &iterator->second.manifest;
}

bool NativePluginRegistryUVE::IsScopeOpenUVE(const std::string_view pluginId) const noexcept {
    const auto iterator = m_entries.find(std::string(pluginId));
    return iterator != m_entries.end() && iterator->second.scopeOpen;
}

std::size_t NativePluginRegistryUVE::GetManifestCountUVE() const noexcept {
    return m_entries.size();
}

std::size_t NativePluginRegistryUVE::GetOpenScopeCountUVE() const noexcept {
    return static_cast<std::size_t>(std::count_if(m_entries.cbegin(), m_entries.cend(), [](const auto& entry) {
        return entry.second.scopeOpen;
    }));
}

bool NativePluginRegistryUVE::IsValidIdentifierUVE(const std::string& value) noexcept {
    if (value.empty() || value.size() > 128U) {
        return false;
    }
    return std::all_of(value.cbegin(), value.cend(), [](const char character) {
        const unsigned char byte = static_cast<unsigned char>(character);
        return std::isalnum(byte) != 0 || character == '_' || character == '-' || character == '.';
    });
}

} // namespace UVE::Plugins
