// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstddef>
#include <memory>
#include <typeindex>
#include <vector>

#include "uve/events/i_event_system_uve.h"
#include "uve/memory/i_allocator_uve.h"
#include "uve/scene/i_entity_manager_uve.h"

namespace UVE::Scene {

/// EntityManagerUVE is the concrete, engine-standard implementation of IEntityManagerUVE: an
/// archetype ECS with chunk-based component storage (see ArchetypeUVE/ChunkUVE in
/// engine/scene/src/ — private implementation details hidden entirely behind this PIMPL, the
/// same "no internals in the public header" idiom ConfigManagerUVE established; here it's
/// motivated by avoiding incomplete-type risk in a public header rather than hiding a
/// third-party dependency, but the mechanism is identical).
/// Thread-safety: not thread-safe — every method (including ForEachUVE, inherited from
/// IEntityManagerUVE) must be called only from the main/scene thread, matching EngineCoreUVE's
/// own single-threaded contract.
class EntityManagerUVE final : public IEntityManagerUVE {
public:
    /// `allocator` backs every chunk's component storage (see ChunkUVE); `eventSystem` receives
    /// EntityCreatedEventUVE/EntityDestroyedEventUVE publications. Both references must outlive
    /// this EntityManagerUVE.
    EntityManagerUVE(Memory::IAllocatorUVE& allocator, Events::IEventSystemUVE& eventSystem);
    ~EntityManagerUVE() override;

    EntityManagerUVE(const EntityManagerUVE&) = delete;
    EntityManagerUVE& operator=(const EntityManagerUVE&) = delete;

    [[nodiscard]] EntityUVE CreateEntityUVE() override;
    void DestroyEntityUVE(EntityUVE entity) override;
    [[nodiscard]] bool IsAliveUVE(EntityUVE entity) const noexcept override;
    [[nodiscard]] std::size_t GetEntityCountUVE() const noexcept override;
    [[nodiscard]] std::vector<std::type_index> GetComponentTypesUVE(EntityUVE entity) const override;

protected:
    [[nodiscard]] void* AddComponentErased(EntityUVE entity, std::type_index componentType,
                                            const ComponentTypeInfoUVE& typeInfo) override;
    void RemoveComponentErased(EntityUVE entity, std::type_index componentType) override;
    [[nodiscard]] bool HasComponentErased(EntityUVE entity, std::type_index componentType) const override;
    [[nodiscard]] void* GetComponentErased(EntityUVE entity, std::type_index componentType) override;
    void ForEachErased(const std::vector<std::type_index>& componentTypes,
                        const std::function<void(EntityUVE, const std::vector<void*>&)>& callback) override;

private:
    struct ImplUVE;
    std::unique_ptr<ImplUVE> m_impl;
};

} // namespace UVE::Scene
