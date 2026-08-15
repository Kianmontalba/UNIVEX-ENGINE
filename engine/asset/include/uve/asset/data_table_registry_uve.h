#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "uve/asset/data_table_uve.h"

namespace UVE::Asset {

class DataTableRegistryUVE final {
public:
    static constexpr std::size_t kMaximumTablesUVE = 128U;

    DataTableRegistryUVE() = default;
    DataTableRegistryUVE(const DataTableRegistryUVE&) = delete;
    DataTableRegistryUVE& operator=(const DataTableRegistryUVE&) = delete;
    DataTableRegistryUVE(DataTableRegistryUVE&&) noexcept = default;
    DataTableRegistryUVE& operator=(DataTableRegistryUVE&&) noexcept = default;
    ~DataTableRegistryUVE() = default;

    [[nodiscard]] bool RegisterUVE(DataTableUVE table);
    [[nodiscard]] bool RemoveUVE(std::string_view name);
    [[nodiscard]] bool ClearUVE() noexcept;
    [[nodiscard]] bool TryGetSnapshotUVE(std::string_view name, DataTableSnapshotUVE& snapshot) const;
    [[nodiscard]] DataTableCatalogSnapshotUVE GetCatalogSnapshotUVE() const;
    [[nodiscard]] bool ContainsUVE(std::string_view name) const;
    [[nodiscard]] std::size_t SizeUVE() const noexcept;

private:
    void IncrementGenerationUVE() noexcept;
    [[nodiscard]] static bool IsValidTableSnapshotUVE(const DataTableSnapshotUVE& snapshot) noexcept;

    std::vector<DataTableUVE> m_tables;
    std::uint64_t m_generation = 1U;
};

} // namespace UVE::Asset
