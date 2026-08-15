#include "uve/asset/data_table_registry_uve.h"

#include <algorithm>
#include <limits>
#include <utility>

namespace UVE::Asset {

bool DataTableRegistryUVE::RegisterUVE(DataTableUVE table) {
    const DataTableSnapshotUVE candidate = table.GetSnapshotUVE();
    if (!IsValidTableSnapshotUVE(candidate) || ContainsUVE(candidate.name) ||
        m_tables.size() >= kMaximumTablesUVE) {
        return false;
    }
    m_tables.push_back(std::move(table));
    IncrementGenerationUVE();
    return true;
}

bool DataTableRegistryUVE::RemoveUVE(const std::string_view name) {
    const auto iterator = std::find_if(m_tables.begin(), m_tables.end(), [name](const DataTableUVE& table) {
        return table.GetSnapshotUVE().name == name;
    });
    if (iterator == m_tables.end()) {
        return false;
    }
    m_tables.erase(iterator);
    IncrementGenerationUVE();
    return true;
}

bool DataTableRegistryUVE::ClearUVE() noexcept {
    if (m_tables.empty()) {
        return false;
    }
    m_tables.clear();
    IncrementGenerationUVE();
    return true;
}

bool DataTableRegistryUVE::TryGetSnapshotUVE(const std::string_view name, DataTableSnapshotUVE& snapshot) const {
    const auto iterator = std::find_if(m_tables.begin(), m_tables.end(), [name](const DataTableUVE& table) {
        return table.GetSnapshotUVE().name == name;
    });
    if (iterator == m_tables.end()) {
        return false;
    }
    snapshot = iterator->GetSnapshotUVE();
    return true;
}

DataTableCatalogSnapshotUVE DataTableRegistryUVE::GetCatalogSnapshotUVE() const {
    DataTableCatalogUVE catalog;
    for (const DataTableUVE& table : m_tables) {
        const DataTableSnapshotUVE snapshot = table.GetSnapshotUVE();
        static_cast<void>(catalog.UpsertUVE(snapshot));
    }
    DataTableCatalogSnapshotUVE snapshot = catalog.GetSnapshotUVE();
    snapshot.generation = m_generation;
    return snapshot;
}

bool DataTableRegistryUVE::ContainsUVE(const std::string_view name) const {
    return std::any_of(m_tables.begin(), m_tables.end(), [name](const DataTableUVE& table) {
        return table.GetSnapshotUVE().name == name;
    });
}

std::size_t DataTableRegistryUVE::SizeUVE() const noexcept {
    return m_tables.size();
}

void DataTableRegistryUVE::IncrementGenerationUVE() noexcept {
    if (m_generation < std::numeric_limits<std::uint64_t>::max()) {
        ++m_generation;
    }
}

bool DataTableRegistryUVE::IsValidTableSnapshotUVE(const DataTableSnapshotUVE& snapshot) noexcept {
    return !snapshot.name.empty() && snapshot.diagnostics.empty();
}

} // namespace UVE::Asset
