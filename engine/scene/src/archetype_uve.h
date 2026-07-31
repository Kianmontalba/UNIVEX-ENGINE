//------------------------------------------------------------------------------
// UniVex Engine (UVE) — Proprietary Game Engine
// Copyright (c) 2026 UniVex Studios. All Rights Reserved.
// Unauthorized copying, modification, distribution, or use of this code
// in whole or in part is strictly prohibited without express written
// permission from UniVex Studios.
// Violators will be prosecuted to the fullest extent of the law.
//------------------------------------------------------------------------------

#pragma once

#include <cstddef>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "archetype_signature_uve.h"
#include "chunk_uve.h"
#include "uve/memory/i_allocator_uve.h"
#include "uve/scene/component_type_info_uve.h"
#include "uve/scene/entity_uve.h"

namespace UVE::Scene::Detail {

/// ArchetypeUVE owns every ChunkUVE storing entities with exactly its ArchetypeSignatureUVE. It
/// orchestrates row reservation (creating a new chunk once the last one is full) and the
/// destroy/vacate contract ChunkUVE documents. Private implementation detail — never appears in
/// any public header under engine/scene/include/.
/// Thread-safety: not thread-safe — owned by IEntityManagerUVE, which documents the same
/// main/scene-thread-only contract.
class ArchetypeUVE final {
public:
    /// A row's location within this archetype's chunk storage.
    struct LocationUVE {
        std::size_t chunkIndex = 0;
        std::size_t row = 0;
    };

    ArchetypeUVE(ArchetypeSignatureUVE signature, Memory::IAllocatorUVE& allocator,
                 const std::unordered_map<std::type_index, ComponentTypeInfoUVE>& typeInfos);

    [[nodiscard]] const ArchetypeSignatureUVE& GetSignatureUVE() const noexcept;
    [[nodiscard]] std::size_t GetEntityCountUVE() const noexcept;

    /// Reserves a row for `entity` in the last non-full chunk, creating a new chunk if none
    /// exists or the last one is full. Every column's memory at the returned location is raw
    /// and uninitialized — the caller must construct every column (see ChunkUVE's contract)
    /// before the row is valid.
    [[nodiscard]] LocationUVE ReserveEntityUVE(EntityUVE entity);

    /// Destroys every column's data at `location`, then compacts (swap-remove). Returns the
    /// entity that moved into `location` (kInvalidEntityUVE if none) — the caller must patch
    /// that entity's record to point at `location`.
    [[nodiscard]] EntityUVE DestroyEntityAtUVE(LocationUVE location);

    /// Compacts (swap-remove) at `location` WITHOUT destroying anything first — used by
    /// archetype-migration callers that have already moved/destroyed every column themselves.
    /// Same return-value contract as DestroyEntityAtUVE().
    [[nodiscard]] EntityUVE VacateWithoutDestroyUVE(LocationUVE location);

    [[nodiscard]] ChunkUVE& GetChunkUVE(std::size_t chunkIndex);
    [[nodiscard]] const ChunkUVE& GetChunkUVE(std::size_t chunkIndex) const;

    /// Invokes `visitor(EntityUVE, ChunkUVE&, std::size_t row)` once per occupied row across
    /// every chunk.
    template <typename TVisitor>
    void ForEachEntityUVE(TVisitor&& visitor) {
        for (const std::unique_ptr<ChunkUVE>& chunk : m_chunks) {
            const std::size_t count = chunk->GetCountUVE();
            for (std::size_t row = 0; row < count; ++row) {
                visitor(chunk->GetEntityAtRowUVE(row), *chunk, row);
            }
        }
    }

private:
    ArchetypeSignatureUVE m_signature;
    Memory::IAllocatorUVE& m_allocator;
    const std::unordered_map<std::type_index, ComponentTypeInfoUVE>& m_typeInfos;
    std::vector<std::unique_ptr<ChunkUVE>> m_chunks;
};

} // namespace UVE::Scene::Detail
