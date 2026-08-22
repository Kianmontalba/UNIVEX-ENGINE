// Copyright (c) 2026 UniVex Studios. All Rights Reserved.


#pragma once

#include <cstddef>
#include <functional>
#include <typeindex>
#include <utility>
#include <vector>

#include "uve/debug/assert_uve.h"
#include "uve/scene/component_type_info_uve.h"
#include "uve/scene/entity_uve.h"

namespace UVE::Scene {

namespace Detail {

/// Expands the type-erased `pointers` vector back into the `TComponents...` parameter pack, in
/// order, before invoking the caller's strongly-typed callback. A standard
/// std::index_sequence-based unpack — not exotic, just the mechanical way to turn a runtime
/// vector<void*> back into a compile-time-known pack of typed references.
template <typename... TComponents, typename TFunc, std::size_t... Is>
void InvokeForEachCallbackUVE(TFunc& callback, EntityUVE entity, const std::vector<void*>& pointers,
                               std::index_sequence<Is...>) {
    callback(entity, *static_cast<TComponents*>(pointers[Is])...);
}

} // namespace Detail

/// IEntityManagerUVE is the archetype ECS interface: entity lifecycle (create/destroy),
/// component attachment, and multi-component queries. Mirrors IEventSystemUVE's exact shape
/// (engine/events/include/uve/events/i_event_system_uve.h) — public, non-virtual member
/// templates for compile-time type safety at call sites, routing through protected pure-virtual
/// type-erased primitives a concrete implementation provides. Component types are identified by
/// std::type_index(typeid(T)), the same mechanism IEventSystemUVE already uses, rather than a
/// new global type-id registry (see docs/CODING_STANDARDS.md).
/// Thread-safety: not thread-safe — every method (including ForEachUVE) must be called only
/// from the main/scene thread, matching EngineCoreUVE's own single-threaded contract. A future
/// increment could add parallel ForEachUVE iteration via IThreadPoolUVE over already-stable
/// archetypes without changing this interface.
class IEntityManagerUVE {
public:
    virtual ~IEntityManagerUVE() = default;

    /// Creates a new, component-less entity and publishes EntityCreatedEventUVE.
    [[nodiscard]] virtual EntityUVE CreateEntityUVE() = 0;

    /// Destroys `entity`, destroying every component it owns and publishing EntityDestroyedEventUVE.
    /// Debug builds assert that the entity is alive; release builds return without mutation for a
    /// stale or invalid handle.
    virtual void DestroyEntityUVE(EntityUVE entity) = 0;

    /// True iff `entity`'s generation matches the current occupant of its index slot.
    [[nodiscard]] virtual bool IsAliveUVE(EntityUVE entity) const noexcept = 0;

    /// Number of currently-alive entities.
    [[nodiscard]] virtual std::size_t GetEntityCountUVE() const noexcept = 0;

    /// Returns the std::type_index of every component `entity` currently has, in unspecified
    /// order. Debug builds assert that the entity is alive; release builds return an empty vector
    /// for a stale or invalid handle. The generic, runtime-enumerable counterpart to
    /// HasComponentUVE<T>()/GetComponentUVE<T>() — used by SceneSerializerUVE to walk an
    /// entity's full component set without knowing the types in advance.
    [[nodiscard]] virtual std::vector<std::type_index> GetComponentTypesUVE(EntityUVE entity) const = 0;

    /// Attaches a `T` (constructed from `args`) to `entity`, migrating it to the archetype
    /// {entity's current components} ∪ {T}. Asserts `entity` is alive and does not already have
    /// a `T`. Returns a reference to the newly-constructed component (valid until the next
    /// structural change involving `entity` or any entity sharing its archetype/chunk).
    template <typename T, typename... TArgs>
    T& AddComponentUVE(EntityUVE entity, TArgs&&... args) {
        void* const slot = AddComponentErased(entity, std::type_index(typeid(T)), MakeComponentTypeInfoUVE<T>());
        return *(::new (slot) T(std::forward<TArgs>(args)...));
    }

    /// Removes `T` from `entity`, migrating it to the new archetype (entity's current
    /// components minus `T`). Asserts `entity` is alive and currently has a `T`.
    template <typename T>
    void RemoveComponentUVE(EntityUVE entity) {
        RemoveComponentErased(entity, std::type_index(typeid(T)));
    }

    /// True iff a live `entity` currently has a `T`. Debug builds assert that the entity is alive;
    /// release builds return false for a stale or invalid handle.
    template <typename T>
    [[nodiscard]] bool HasComponentUVE(EntityUVE entity) const {
        return HasComponentErased(entity, std::type_index(typeid(T)));
    }

    /// Returns a reference to `entity`'s `T`. Asserts `entity` is alive and currently has one.
    template <typename T>
    [[nodiscard]] T& GetComponentUVE(EntityUVE entity) {
        return *static_cast<T*>(GetComponentErased(entity, std::type_index(typeid(T))));
    }

    /// Const overload of GetComponentUVE<T>().
    template <typename T>
    [[nodiscard]] const T& GetComponentUVE(EntityUVE entity) const {
        return *static_cast<const T*>(
            const_cast<IEntityManagerUVE*>(this)->GetComponentErased(entity, std::type_index(typeid(T))));
    }

    /// Returns a type-erased pointer to `entity`'s component of type `componentType`. Debug builds
    /// assert that the entity is alive and currently has the component; release builds return
    /// nullptr for a stale entity or absent component. The runtime-typed counterpart to
    /// GetComponentUVE<T>(), used together with GetComponentTypesUVE() by SceneSerializerUVE to
    /// read every component of an entity whose types are only known at runtime.
    [[nodiscard]] void* GetComponentPointerUVE(EntityUVE entity, std::type_index componentType) {
        if (!IsAliveUVE(entity)) {
            UVE_ASSERT(IsAliveUVE(entity));
            return nullptr;
        }
        if (!HasComponentErased(entity, componentType)) {
            UVE_ASSERT(HasComponentErased(entity, componentType));
            return nullptr;
        }
        return GetComponentErased(entity, componentType);
    }

    /// Const overload of GetComponentPointerUVE().
    [[nodiscard]] const void* GetComponentPointerUVE(EntityUVE entity, std::type_index componentType) const {
        return const_cast<IEntityManagerUVE*>(this)->GetComponentPointerUVE(entity, componentType);
    }

    /// Invokes `callback(EntityUVE, TComponents&...)` once for every entity whose current
    /// archetype contains every one of `TComponents...` (it may contain others too). Iteration
    /// order is unspecified beyond "every matching entity exactly once."
    template <typename... TComponents, typename TFunc>
    void ForEachUVE(TFunc&& callback) {
        const std::vector<std::type_index> types{std::type_index(typeid(TComponents))...};
        ForEachErased(types, [&callback](EntityUVE entity, const std::vector<void*>& pointers) {
            Detail::InvokeForEachCallbackUVE<TComponents...>(callback, entity, pointers,
                                                               std::index_sequence_for<TComponents...>{});
        });
    }

protected:
    [[nodiscard]] virtual void* AddComponentErased(EntityUVE entity, std::type_index componentType,
                                                    const ComponentTypeInfoUVE& typeInfo) = 0;
    virtual void RemoveComponentErased(EntityUVE entity, std::type_index componentType) = 0;
    [[nodiscard]] virtual bool HasComponentErased(EntityUVE entity, std::type_index componentType) const = 0;
    [[nodiscard]] virtual void* GetComponentErased(EntityUVE entity, std::type_index componentType) = 0;

    /// `callback` receives, for every matching entity, its EntityUVE and a vector of raw
    /// component pointers in the same order as `componentTypes`.
    virtual void ForEachErased(
        const std::vector<std::type_index>& componentTypes,
        const std::function<void(EntityUVE, const std::vector<void*>&)>& callback) = 0;
};

} // namespace UVE::Scene
