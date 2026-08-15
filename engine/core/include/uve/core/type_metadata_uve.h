// Copyright (c) 2026 UniVex Studios. All Rights Reserved.

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace UVE::Core {

enum class TypeMetadataKindUVE : std::uint8_t {
    Component = 0,
    Resource,
    VisualScriptNode,
    InspectorTarget,
    Other,
};

struct TypeMetadataPropertyUVE final {
    std::string name;
    std::string displayName;
    std::string typeId;
    bool editable = false;

    [[nodiscard]] bool operator==(const TypeMetadataPropertyUVE&) const = default;
};

struct TypeMetadataMethodUVE final {
    std::string name;
    std::string displayName;
    std::uint32_t flags = 0U;

    [[nodiscard]] bool operator==(const TypeMetadataMethodUVE&) const = default;
};

struct TypeMetadataEntryUVE final {
    TypeMetadataKindUVE kind = TypeMetadataKindUVE::Other;
    std::string typeId;
    std::string displayName;
    std::uint32_t version = 1U;
    std::vector<TypeMetadataPropertyUVE> properties;
    std::vector<TypeMetadataMethodUVE> methods;

    [[nodiscard]] bool operator==(const TypeMetadataEntryUVE&) const = default;
};

struct TypeMetadataSnapshotUVE final {
    std::uint64_t generation = 0U;
    bool entriesTruncated = false;
    std::vector<TypeMetadataEntryUVE> entries;

    [[nodiscard]] bool operator==(const TypeMetadataSnapshotUVE&) const = default;
};

enum class TypeMetadataRegistrationCodeUVE : std::uint8_t {
    Registered = 0,
    InvalidEntry,
    DuplicateType,
    CapacityExceeded,
};

struct TypeMetadataRegistrationResultUVE final {
    TypeMetadataRegistrationCodeUVE code = TypeMetadataRegistrationCodeUVE::InvalidEntry;
    std::string message;

    [[nodiscard]] bool IsRegisteredUVE() const noexcept {
        return code == TypeMetadataRegistrationCodeUVE::Registered;
    }
};

class TypeMetadataRegistryUVE final {
public:
    static constexpr std::size_t kMaximumTypesUVE = 256U;
    static constexpr std::size_t kMaximumMembersPerTypeUVE = 128U;
    static constexpr std::size_t kMaximumIdentifierBytesUVE = 128U;
    static constexpr std::size_t kMaximumDisplayNameBytesUVE = 256U;

    TypeMetadataRegistryUVE() = default;
    TypeMetadataRegistryUVE(const TypeMetadataRegistryUVE&) = delete;
    TypeMetadataRegistryUVE& operator=(const TypeMetadataRegistryUVE&) = delete;

    [[nodiscard]] TypeMetadataRegistrationResultUVE RegisterTypeUVE(TypeMetadataEntryUVE entry);
    [[nodiscard]] const TypeMetadataEntryUVE* FindTypeUVE(std::string_view typeId) const noexcept;
    [[nodiscard]] TypeMetadataSnapshotUVE GetSnapshotUVE() const;
    [[nodiscard]] std::size_t GetTypeCountUVE() const noexcept;
    [[nodiscard]] std::uint64_t GetGenerationUVE() const noexcept;

private:
    std::vector<TypeMetadataEntryUVE> m_entries;
    std::uint64_t m_generation = 0U;
};

} // namespace UVE::Core
