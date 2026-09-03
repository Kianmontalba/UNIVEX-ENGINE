// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstddef>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "uve/memory/i_allocator_uve.h"
#include "uve/scene/component_type_info_uve.h"
#include "uve/scene/entity_uve.h"

namespace UVE::Scene::Detail {

/// Fixed capacity of every ChunkUVE — a simple, documented, testable policy (not the
/// byte-budget-driven variable capacity real DOTS chunks use; that's a documented future
/// optimization, not needed for correctness now).
inline constexpr std::size_t kChunkCapacityUVE = 512;

/// Sentinel row returned by ReserveRowUVE() if called on a full chunk (Release-build backstop —
/// no valid row is ever this value, since row indices are always < kChunkCapacityUVE).
inline constexpr std::size_t kInvalidRowUVE = static_cast<std::size_t>(-1);

/// ChunkUVE is one fixed-capacity (kChunkCapacityUVE-entity) block of an ArchetypeUVE's row
/// storage. Each component type in the owning archetype's signature gets its own separately-
/// allocated array of `size * kChunkCapacityUVE` bytes (simpler than interleaving multiple
/// component arrays into one single allocation with computed offsets/padding — a documented
/// simplification). All buffers are allocated via IAllocatorUVE::AllocateUVE()/
/// DeallocateUVE() — never raw `new`/`delete` — but per-row construction/destruction inside
/// those buffers uses the type-erased ComponentTypeInfoUVE function pointers (the type-erased
/// analog of ConstructUVE/DestroyUVE — see docs/CODING_STANDARDS.md's allocator-boundary note).
///
/// Row lifecycle contract (enforced by ArchetypeUVE, the only caller): a row reserved via
/// ReserveRowUVE() has RAW, UNINITIALIZED memory for every column until the caller has
/// constructed (via ConstructDefaultUVE(), MoveComponentIntoUVE(), or the public template layer
/// placement-constructing directly into GetComponentPointerUVE()'s result) every column at that
/// row — only then is the row considered valid. DestroyAllColumnsUVE()+VacateRowUVE() is the
/// pair used to fully destroy an entity; VacateRowUVE() alone (no preceding destroy) is used
/// during archetype migration, where every column was already individually moved-out or
/// destroyed by the caller.
/// Thread-safety: not thread-safe — owned entirely by its ArchetypeUVE, which is owned by
/// IEntityManagerUVE, which documents the same main/scene-thread-only contract.
class ChunkUVE final {
public:
    ChunkUVE(Memory::IAllocatorUVE& allocator, const std::vector<std::type_index>& componentTypes,
             const std::unordered_map<std::type_index, ComponentTypeInfoUVE>& typeInfos);
    ~ChunkUVE();

    ChunkUVE(const ChunkUVE&) = delete;
    ChunkUVE& operator=(const ChunkUVE&) = delete;
    ChunkUVE(ChunkUVE&&) = delete;
    ChunkUVE& operator=(ChunkUVE&&) = delete;

    [[nodiscard]] bool IsFullUVE() const noexcept;
    [[nodiscard]] std::size_t GetCountUVE() const noexcept;
    [[nodiscard]] bool HasColumnUVE(std::type_index componentType) const;

    /// Reserves the next free row for `entity`. Every column's memory at that row is raw and
    /// uninitialized until the caller constructs it (see class doc comment). UVE_ASSERTs the
    /// chunk is not already full; also returns kInvalidRowUVE and logs an error in Release on a
    /// full chunk, rather than writing past m_entitiesInChunk's capacity.
    [[nodiscard]] std::size_t ReserveRowUVE(EntityUVE entity);

    /// Default-constructs `componentType`'s data at `row`.
    void ConstructDefaultUVE(std::type_index componentType, std::size_t row);

    /// Destroys `componentType`'s already-constructed data at `row`.
    void DestroyComponentUVE(std::type_index componentType, std::size_t row);

    /// Destroys every column's already-constructed data at `row`.
    void DestroyAllColumnsUVE(std::size_t row);

    /// Compacts the chunk after `row`'s data has already been fully destroyed/moved-out by the
    /// caller: if `row` wasn't the last occupied row, move-constructs the last row's data into
    /// `row` and returns the entity that moved (kInvalidEntityUVE if no move was needed).
    /// UVE_ASSERTs `row` is in range; also returns kInvalidEntityUVE and logs an error in
    /// Release on an out-of-range row, rather than reading/writing past m_count.
    [[nodiscard]] EntityUVE VacateRowUVE(std::size_t row);

    [[nodiscard]] void* GetComponentPointerUVE(std::type_index componentType, std::size_t row) const;
    /// UVE_ASSERTs `row` is in range; also returns kInvalidEntityUVE and logs an error in
    /// Release on an out-of-range row.
    [[nodiscard]] EntityUVE GetEntityAtRowUVE(std::size_t row) const;

    /// Moves `componentType`'s data from this chunk's `sourceRow` into `destination`'s
    /// `destinationRow` (which must be raw/unconstructed for that column) — used by ArchetypeUVE
    /// during add/remove-component migrations.
    void MoveComponentIntoUVE(std::type_index componentType, std::size_t sourceRow, ChunkUVE& destination,
                               std::size_t destinationRow) const;

private:
    struct ColumnUVE {
        ComponentTypeInfoUVE typeInfo;
        void* buffer = nullptr;
    };

    [[nodiscard]] const ColumnUVE& GetColumnUVE(std::type_index componentType) const;

    Memory::IAllocatorUVE& m_allocator;
    std::unordered_map<std::type_index, ColumnUVE> m_columns;
    std::vector<EntityUVE> m_entitiesInChunk;
    std::size_t m_count = 0;
};

} // namespace UVE::Scene::Detail
