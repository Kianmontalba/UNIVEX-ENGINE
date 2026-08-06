// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "archetype_signature_uve.h"

#include <algorithm>
#include <utility>

namespace UVE::Scene::Detail {

ArchetypeSignatureUVE::ArchetypeSignatureUVE(std::vector<std::type_index> componentTypes)
    : m_sortedTypes(std::move(componentTypes)) {
    std::sort(m_sortedTypes.begin(), m_sortedTypes.end());
}

bool ArchetypeSignatureUVE::ContainsUVE(std::type_index type) const {
    return std::find(m_sortedTypes.begin(), m_sortedTypes.end(), type) != m_sortedTypes.end();
}

bool ArchetypeSignatureUVE::IsSupersetOfUVE(const ArchetypeSignatureUVE& other) const {
    for (const std::type_index& type : other.m_sortedTypes) {
        if (!ContainsUVE(type)) {
            return false;
        }
    }
    return true;
}

ArchetypeSignatureUVE ArchetypeSignatureUVE::WithUVE(std::type_index type) const {
    std::vector<std::type_index> types = m_sortedTypes;
    types.push_back(type);
    return ArchetypeSignatureUVE(std::move(types));
}

ArchetypeSignatureUVE ArchetypeSignatureUVE::WithoutUVE(std::type_index type) const {
    std::vector<std::type_index> types;
    types.reserve(m_sortedTypes.size());
    for (const std::type_index& existingType : m_sortedTypes) {
        if (existingType != type) {
            types.push_back(existingType);
        }
    }
    return ArchetypeSignatureUVE(std::move(types));
}

const std::vector<std::type_index>& ArchetypeSignatureUVE::GetTypesUVE() const noexcept {
    return m_sortedTypes;
}

} // namespace UVE::Scene::Detail
