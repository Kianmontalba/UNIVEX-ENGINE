// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstddef>
#include <typeindex>
#include <vector>

namespace UVE::Scene::Detail {

/// The sorted set of component types making up one archetype. Private implementation detail —
/// never appears in any public header under engine/scene/include/. Two signatures with the same
/// types (regardless of insertion order) compare equal and hash equal, so they key
/// EntityManagerUVE's archetype-lookup map correctly.
class ArchetypeSignatureUVE final {
public:
    ArchetypeSignatureUVE() = default;
    explicit ArchetypeSignatureUVE(std::vector<std::type_index> componentTypes);

    [[nodiscard]] bool ContainsUVE(std::type_index type) const;
    [[nodiscard]] bool IsSupersetOfUVE(const ArchetypeSignatureUVE& other) const;
    [[nodiscard]] ArchetypeSignatureUVE WithUVE(std::type_index type) const;
    [[nodiscard]] ArchetypeSignatureUVE WithoutUVE(std::type_index type) const;
    [[nodiscard]] const std::vector<std::type_index>& GetTypesUVE() const noexcept;

    [[nodiscard]] bool operator==(const ArchetypeSignatureUVE& other) const noexcept {
        return m_sortedTypes == other.m_sortedTypes;
    }

private:
    std::vector<std::type_index> m_sortedTypes;
};

} // namespace UVE::Scene::Detail

template <>
struct std::hash<UVE::Scene::Detail::ArchetypeSignatureUVE> {
    [[nodiscard]] std::size_t operator()(const UVE::Scene::Detail::ArchetypeSignatureUVE& signature) const noexcept {
        std::size_t combined = 0;
        for (const std::type_index& type : signature.GetTypesUVE()) {
            combined ^= type.hash_code() + 0x9e3779b9U + (combined << 6) + (combined >> 2);
        }
        return combined;
    }
};
