// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/editor/inspector_drawer_registry_uve.h"

#include <algorithm>
#include <cctype>
#include <utility>

namespace UVE::Editor {

namespace {

[[nodiscard]] bool ContainsCaseInsensitiveUVE(const std::string_view text,
                                               const std::string_view query) noexcept {
    if (query.empty()) {
        return true;
    }
    const auto equalsCaseInsensitive = [](const char lhs, const char rhs) noexcept {
        return std::tolower(static_cast<unsigned char>(lhs)) ==
               std::tolower(static_cast<unsigned char>(rhs));
    };
    return std::search(text.begin(), text.end(), query.begin(), query.end(), equalsCaseInsensitive) != text.end();
}

} // namespace

bool InspectorDrawerRegistryUVE::RegisterDrawerUVE(InspectorDrawerEntryUVE entry) {
    if (entry.id.empty() || !entry.isEligible || !entry.draw || HasDrawerUVE(entry.id)) {
        return false;
    }
    m_entries.push_back(std::move(entry));
    return true;
}

void InspectorDrawerRegistryUVE::DrawEligibleUVE(const Scene::EntityUVE entity) const {
    DrawEligibleMatchingUVE(entity, {});
}

void InspectorDrawerRegistryUVE::DrawEligibleMatchingUVE(const Scene::EntityUVE entity,
                                                           const std::string_view filter) const {
    const std::size_t drawerCount = m_entries.size();
    for (std::size_t index = 0U; index < drawerCount; ++index) {
        const InspectorDrawerEntryUVE& entry = m_entries[index];
        if (entry.isEligible(entity) && ContainsCaseInsensitiveUVE(entry.id, filter)) {
            entry.draw(entity);
        }
    }
}

std::vector<std::string> InspectorDrawerRegistryUVE::GetEligibleDrawerIdsUVE(const Scene::EntityUVE entity) const {
    std::vector<std::string> identifiers;
    identifiers.reserve(m_entries.size());
    for (const InspectorDrawerEntryUVE& entry : m_entries) {
        if (entry.isEligible(entity)) {
            identifiers.push_back(entry.id);
        }
    }
    return identifiers;
}

std::size_t InspectorDrawerRegistryUVE::GetDrawerCountUVE() const noexcept {
    return m_entries.size();
}

bool InspectorDrawerRegistryUVE::HasDrawerUVE(const std::string_view id) const noexcept {
    return std::any_of(m_entries.cbegin(), m_entries.cend(), [id](const InspectorDrawerEntryUVE& entry) {
        return entry.id == id;
    });
}

} // namespace UVE::Editor
