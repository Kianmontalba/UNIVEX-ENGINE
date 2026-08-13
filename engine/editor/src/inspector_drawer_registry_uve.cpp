// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#include "uve/editor/inspector_drawer_registry_uve.h"

#include <algorithm>
#include <utility>

namespace UVE::Editor {

bool InspectorDrawerRegistryUVE::RegisterDrawerUVE(InspectorDrawerEntryUVE entry) {
    if (entry.id.empty() || !entry.isEligible || !entry.draw || HasDrawerUVE(entry.id)) {
        return false;
    }
    m_entries.push_back(std::move(entry));
    return true;
}

void InspectorDrawerRegistryUVE::DrawEligibleUVE(const Scene::EntityUVE entity) const {
    const std::size_t drawerCount = m_entries.size();
    for (std::size_t index = 0U; index < drawerCount; ++index) {
        const InspectorDrawerEntryUVE& entry = m_entries[index];
        if (entry.isEligible(entity)) {
            entry.draw(entity);
        }
    }
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
