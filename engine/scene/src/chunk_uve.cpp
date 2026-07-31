//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#include "chunk_uve.h"

#include <cstddef>

#include "uve/debug/assert_uve.h"

namespace UVE::Scene::Detail {

namespace {
[[nodiscard]] void* OffsetUVE(void* buffer, std::size_t elementSize, std::size_t row) {
    return static_cast<std::byte*>(buffer) + (elementSize * row);
}
} // namespace

ChunkUVE::ChunkUVE(Memory::IAllocatorUVE& allocator, const std::vector<std::type_index>& componentTypes,
                    const std::unordered_map<std::type_index, ComponentTypeInfoUVE>& typeInfos)
    : m_allocator(allocator) {
    m_entitiesInChunk.resize(kChunkCapacityUVE, kInvalidEntityUVE);
    for (const std::type_index& type : componentTypes) {
        const ComponentTypeInfoUVE& info = typeInfos.at(type);
        void* const buffer =
            m_allocator.AllocateUVE(info.size * kChunkCapacityUVE, info.alignment, __FILE__, __LINE__);
        m_columns.emplace(type, ColumnUVE{info, buffer});
    }
}

ChunkUVE::~ChunkUVE() {
    for (std::size_t row = 0; row < m_count; ++row) {
        DestroyAllColumnsUVE(row);
    }
    for (auto& [type, column] : m_columns) {
        m_allocator.DeallocateUVE(column.buffer);
    }
}

bool ChunkUVE::IsFullUVE() const noexcept {
    return m_count >= kChunkCapacityUVE;
}

std::size_t ChunkUVE::GetCountUVE() const noexcept {
    return m_count;
}

bool ChunkUVE::HasColumnUVE(std::type_index componentType) const {
    return m_columns.find(componentType) != m_columns.end();
}

const ChunkUVE::ColumnUVE& ChunkUVE::GetColumnUVE(std::type_index componentType) const {
    const auto columnIt = m_columns.find(componentType);
    UVE_ASSERT(columnIt != m_columns.end());
    return columnIt->second;
}

std::size_t ChunkUVE::ReserveRowUVE(EntityUVE entity) {
    UVE_ASSERT(!IsFullUVE());
    const std::size_t row = m_count;
    m_entitiesInChunk[row] = entity;
    ++m_count;
    return row;
}

void ChunkUVE::ConstructDefaultUVE(std::type_index componentType, std::size_t row) {
    const ColumnUVE& column = GetColumnUVE(componentType);
    column.typeInfo.constructDefault(OffsetUVE(column.buffer, column.typeInfo.size, row));
}

void ChunkUVE::DestroyComponentUVE(std::type_index componentType, std::size_t row) {
    const ColumnUVE& column = GetColumnUVE(componentType);
    column.typeInfo.destroy(OffsetUVE(column.buffer, column.typeInfo.size, row));
}

void ChunkUVE::DestroyAllColumnsUVE(std::size_t row) {
    for (auto& [type, column] : m_columns) {
        column.typeInfo.destroy(OffsetUVE(column.buffer, column.typeInfo.size, row));
    }
}

EntityUVE ChunkUVE::VacateRowUVE(std::size_t row) {
    UVE_ASSERT(row < m_count);
    const std::size_t lastRow = m_count - 1;

    EntityUVE movedEntity = kInvalidEntityUVE;
    if (row != lastRow) {
        for (auto& [type, column] : m_columns) {
            void* const destination = OffsetUVE(column.buffer, column.typeInfo.size, row);
            void* const source = OffsetUVE(column.buffer, column.typeInfo.size, lastRow);
            column.typeInfo.moveConstructAndDestroySource(destination, source);
        }
        movedEntity = m_entitiesInChunk[lastRow];
        m_entitiesInChunk[row] = movedEntity;
    }
    m_entitiesInChunk[lastRow] = kInvalidEntityUVE;
    --m_count;
    return movedEntity;
}

void* ChunkUVE::GetComponentPointerUVE(std::type_index componentType, std::size_t row) const {
    const ColumnUVE& column = GetColumnUVE(componentType);
    return OffsetUVE(column.buffer, column.typeInfo.size, row);
}

EntityUVE ChunkUVE::GetEntityAtRowUVE(std::size_t row) const {
    UVE_ASSERT(row < m_count);
    return m_entitiesInChunk[row];
}

void ChunkUVE::MoveComponentIntoUVE(std::type_index componentType, std::size_t sourceRow, ChunkUVE& destination,
                                     std::size_t destinationRow) const {
    void* const sourceSlot = GetComponentPointerUVE(componentType, sourceRow);
    void* const destinationSlot = destination.GetComponentPointerUVE(componentType, destinationRow);
    const ColumnUVE& column = GetColumnUVE(componentType);
    column.typeInfo.moveConstructAndDestroySource(destinationSlot, sourceSlot);
}

} // namespace UVE::Scene::Detail
