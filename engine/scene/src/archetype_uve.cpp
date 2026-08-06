// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "archetype_uve.h"

#include <utility>

#include "uve/debug/assert_uve.h"

namespace UVE::Scene::Detail {

ArchetypeUVE::ArchetypeUVE(ArchetypeSignatureUVE signature, Memory::IAllocatorUVE& allocator,
                            const std::unordered_map<std::type_index, ComponentTypeInfoUVE>& typeInfos)
    : m_signature(std::move(signature)), m_allocator(allocator), m_typeInfos(typeInfos) {}

const ArchetypeSignatureUVE& ArchetypeUVE::GetSignatureUVE() const noexcept {
    return m_signature;
}

std::size_t ArchetypeUVE::GetEntityCountUVE() const noexcept {
    std::size_t total = 0;
    for (const std::unique_ptr<ChunkUVE>& chunk : m_chunks) {
        total += chunk->GetCountUVE();
    }
    return total;
}

ArchetypeUVE::LocationUVE ArchetypeUVE::ReserveEntityUVE(EntityUVE entity) {
    if (m_chunks.empty() || m_chunks.back()->IsFullUVE()) {
        m_chunks.push_back(
            std::make_unique<ChunkUVE>(m_allocator, m_signature.GetTypesUVE(), m_typeInfos));
    }
    const std::size_t chunkIndex = m_chunks.size() - 1;
    const std::size_t row = m_chunks[chunkIndex]->ReserveRowUVE(entity);
    return LocationUVE{chunkIndex, row};
}

EntityUVE ArchetypeUVE::DestroyEntityAtUVE(LocationUVE location) {
    UVE_ASSERT(location.chunkIndex < m_chunks.size());
    ChunkUVE& chunk = *m_chunks[location.chunkIndex];
    chunk.DestroyAllColumnsUVE(location.row);
    return chunk.VacateRowUVE(location.row);
}

EntityUVE ArchetypeUVE::VacateWithoutDestroyUVE(LocationUVE location) {
    UVE_ASSERT(location.chunkIndex < m_chunks.size());
    return m_chunks[location.chunkIndex]->VacateRowUVE(location.row);
}

ChunkUVE& ArchetypeUVE::GetChunkUVE(std::size_t chunkIndex) {
    UVE_ASSERT(chunkIndex < m_chunks.size());
    return *m_chunks[chunkIndex];
}

const ChunkUVE& ArchetypeUVE::GetChunkUVE(std::size_t chunkIndex) const {
    UVE_ASSERT(chunkIndex < m_chunks.size());
    return *m_chunks[chunkIndex];
}

} // namespace UVE::Scene::Detail
