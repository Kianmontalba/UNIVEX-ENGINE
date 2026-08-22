// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#include "uve/scene/entity_manager_uve.h"

#include <cstdint>
#include <unordered_map>
#include <utility>

#include "archetype_signature_uve.h"
#include "archetype_uve.h"
#include "chunk_uve.h"
#include "uve/debug/assert_uve.h"
#include "uve/scene/entity_lifecycle_events_uve.h"

namespace UVE::Scene {

namespace {

/// Where one alive entity currently lives: which archetype, which of its chunks, which row.
struct EntityRecordUVE {
    std::uint32_t generation = 0;
    bool alive = false;
    Detail::ArchetypeUVE* archetype = nullptr;
    std::size_t chunkIndex = 0;
    std::size_t row = 0;
};

} // namespace

struct EntityManagerUVE::ImplUVE {
    ImplUVE(Memory::IAllocatorUVE& allocatorIn, Events::IEventSystemUVE& eventSystemIn)
        : allocator(allocatorIn), eventSystem(eventSystemIn) {}

    Memory::IAllocatorUVE& allocator;
    Events::IEventSystemUVE& eventSystem;
    std::unordered_map<std::type_index, ComponentTypeInfoUVE> typeInfos;
    std::vector<EntityRecordUVE> records;
    std::vector<std::uint32_t> freeIndices;
    std::unordered_map<Detail::ArchetypeSignatureUVE, std::unique_ptr<Detail::ArchetypeUVE>> archetypes;

    [[nodiscard]] Detail::ArchetypeUVE& FindOrCreateArchetypeUVE(const Detail::ArchetypeSignatureUVE& signature) {
        const auto archetypeIt = archetypes.find(signature);
        if (archetypeIt != archetypes.end()) {
            return *archetypeIt->second;
        }
        auto archetype = std::make_unique<Detail::ArchetypeUVE>(signature, allocator, typeInfos);
        Detail::ArchetypeUVE& archetypeRef = *archetype;
        archetypes.emplace(signature, std::move(archetype));
        return archetypeRef;
    }
};

EntityManagerUVE::EntityManagerUVE(Memory::IAllocatorUVE& allocator, Events::IEventSystemUVE& eventSystem)
    : m_impl(std::make_unique<ImplUVE>(allocator, eventSystem)) {}

EntityManagerUVE::~EntityManagerUVE() = default;

EntityUVE EntityManagerUVE::CreateEntityUVE() {
    std::uint32_t index;
    if (!m_impl->freeIndices.empty()) {
        index = m_impl->freeIndices.back();
        m_impl->freeIndices.pop_back();
    } else {
        index = static_cast<std::uint32_t>(m_impl->records.size());
        m_impl->records.emplace_back();
    }

    EntityRecordUVE& record = m_impl->records[index];
    record.alive = true;
    const EntityUVE entity{index, record.generation};

    Detail::ArchetypeUVE& emptyArchetype = m_impl->FindOrCreateArchetypeUVE(Detail::ArchetypeSignatureUVE{});
    const Detail::ArchetypeUVE::LocationUVE location = emptyArchetype.ReserveEntityUVE(entity);
    record.archetype = &emptyArchetype;
    record.chunkIndex = location.chunkIndex;
    record.row = location.row;

    m_impl->eventSystem.Publish(EntityCreatedEventUVE{entity});
    return entity;
}

void EntityManagerUVE::DestroyEntityUVE(EntityUVE entity) {
    if (!IsAliveUVE(entity)) {
        UVE_ASSERT(IsAliveUVE(entity));
        return;
    }
    EntityRecordUVE& record = m_impl->records[entity.index];

    m_impl->eventSystem.Publish(EntityDestroyedEventUVE{entity});

    const Detail::ArchetypeUVE::LocationUVE location{record.chunkIndex, record.row};
    const EntityUVE movedEntity = record.archetype->DestroyEntityAtUVE(location);
    if (movedEntity != kInvalidEntityUVE) {
        m_impl->records[movedEntity.index].row = record.row;
    }

    record.alive = false;
    record.archetype = nullptr;
    ++record.generation;
    m_impl->freeIndices.push_back(entity.index);
}

bool EntityManagerUVE::IsAliveUVE(EntityUVE entity) const noexcept {
    if (entity.index >= m_impl->records.size()) {
        return false;
    }
    const EntityRecordUVE& record = m_impl->records[entity.index];
    return record.alive && record.generation == entity.generation;
}

std::size_t EntityManagerUVE::GetEntityCountUVE() const noexcept {
    std::size_t count = 0;
    for (const EntityRecordUVE& record : m_impl->records) {
        if (record.alive) {
            ++count;
        }
    }
    return count;
}

std::vector<std::type_index> EntityManagerUVE::GetComponentTypesUVE(EntityUVE entity) const {
    if (!IsAliveUVE(entity)) {
        UVE_ASSERT(IsAliveUVE(entity));
        return {};
    }
    return m_impl->records[entity.index].archetype->GetSignatureUVE().GetTypesUVE();
}

void* EntityManagerUVE::AddComponentErased(EntityUVE entity, std::type_index componentType,
                                            const ComponentTypeInfoUVE& typeInfo) {
    UVE_ASSERT(IsAliveUVE(entity));
    EntityRecordUVE& record = m_impl->records[entity.index];
    Detail::ArchetypeUVE& oldArchetype = *record.archetype;
    UVE_ASSERT(!oldArchetype.GetSignatureUVE().ContainsUVE(componentType));

    m_impl->typeInfos.emplace(componentType, typeInfo);

    const Detail::ArchetypeSignatureUVE newSignature = oldArchetype.GetSignatureUVE().WithUVE(componentType);
    Detail::ArchetypeUVE& newArchetype = m_impl->FindOrCreateArchetypeUVE(newSignature);

    const Detail::ArchetypeUVE::LocationUVE oldLocation{record.chunkIndex, record.row};
    const Detail::ArchetypeUVE::LocationUVE newLocation = newArchetype.ReserveEntityUVE(entity);
    Detail::ChunkUVE& oldChunk = oldArchetype.GetChunkUVE(oldLocation.chunkIndex);
    Detail::ChunkUVE& newChunk = newArchetype.GetChunkUVE(newLocation.chunkIndex);

    for (const std::type_index& type : oldArchetype.GetSignatureUVE().GetTypesUVE()) {
        oldChunk.MoveComponentIntoUVE(type, oldLocation.row, newChunk, newLocation.row);
    }

    const EntityUVE movedEntity = oldArchetype.VacateWithoutDestroyUVE(oldLocation);
    if (movedEntity != kInvalidEntityUVE) {
        m_impl->records[movedEntity.index].row = oldLocation.row;
    }

    record.archetype = &newArchetype;
    record.chunkIndex = newLocation.chunkIndex;
    record.row = newLocation.row;

    return newChunk.GetComponentPointerUVE(componentType, newLocation.row);
}

void EntityManagerUVE::RemoveComponentErased(EntityUVE entity, std::type_index componentType) {
    UVE_ASSERT(IsAliveUVE(entity));
    EntityRecordUVE& record = m_impl->records[entity.index];
    Detail::ArchetypeUVE& oldArchetype = *record.archetype;
    UVE_ASSERT(oldArchetype.GetSignatureUVE().ContainsUVE(componentType));

    const Detail::ArchetypeSignatureUVE newSignature = oldArchetype.GetSignatureUVE().WithoutUVE(componentType);
    Detail::ArchetypeUVE& newArchetype = m_impl->FindOrCreateArchetypeUVE(newSignature);

    const Detail::ArchetypeUVE::LocationUVE oldLocation{record.chunkIndex, record.row};
    const Detail::ArchetypeUVE::LocationUVE newLocation = newArchetype.ReserveEntityUVE(entity);
    Detail::ChunkUVE& oldChunk = oldArchetype.GetChunkUVE(oldLocation.chunkIndex);
    Detail::ChunkUVE& newChunk = newArchetype.GetChunkUVE(newLocation.chunkIndex);

    for (const std::type_index& type : newSignature.GetTypesUVE()) {
        oldChunk.MoveComponentIntoUVE(type, oldLocation.row, newChunk, newLocation.row);
    }
    oldChunk.DestroyComponentUVE(componentType, oldLocation.row);

    const EntityUVE movedEntity = oldArchetype.VacateWithoutDestroyUVE(oldLocation);
    if (movedEntity != kInvalidEntityUVE) {
        m_impl->records[movedEntity.index].row = oldLocation.row;
    }

    record.archetype = &newArchetype;
    record.chunkIndex = newLocation.chunkIndex;
    record.row = newLocation.row;
}

bool EntityManagerUVE::HasComponentErased(EntityUVE entity, std::type_index componentType) const {
    if (!IsAliveUVE(entity)) {
        UVE_ASSERT(IsAliveUVE(entity));
        return false;
    }
    return m_impl->records[entity.index].archetype->GetSignatureUVE().ContainsUVE(componentType);
}

void* EntityManagerUVE::GetComponentErased(EntityUVE entity, std::type_index componentType) {
    UVE_ASSERT(IsAliveUVE(entity));
    const EntityRecordUVE& record = m_impl->records[entity.index];
    UVE_ASSERT(record.archetype->GetSignatureUVE().ContainsUVE(componentType));
    return record.archetype->GetChunkUVE(record.chunkIndex).GetComponentPointerUVE(componentType, record.row);
}

void EntityManagerUVE::ForEachErased(
    const std::vector<std::type_index>& componentTypes,
    const std::function<void(EntityUVE, const std::vector<void*>&)>& callback) {
    const Detail::ArchetypeSignatureUVE requested(componentTypes);
    for (auto& [signature, archetype] : m_impl->archetypes) {
        if (!archetype->GetSignatureUVE().IsSupersetOfUVE(requested)) {
            continue;
        }
        archetype->ForEachEntityUVE([&componentTypes, &callback](EntityUVE entity, Detail::ChunkUVE& chunk,
                                                                    std::size_t row) {
            std::vector<void*> pointers;
            pointers.reserve(componentTypes.size());
            for (const std::type_index& type : componentTypes) {
                pointers.push_back(chunk.GetComponentPointerUVE(type, row));
            }
            callback(entity, pointers);
        });
    }
}

} // namespace UVE::Scene
